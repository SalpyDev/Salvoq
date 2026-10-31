
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "capture.h"
#include "platform.h"
#include "common.h"

static void push_silence(capture_source *s, uint64_t t0, uint64_t *pushed) {
    uint64_t due = ((plat_now_ns() - t0) * SALVOQ_SAMPLE_RATE / 1000000000ULL) - *pushed;
    if (due > SALVOQ_SAMPLE_RATE) { t0 = plat_now_ns(); *pushed = 0; due = 0; }
    float zeros[512];
    memset(zeros, 0, sizeof zeros);
    while (due) {
        size_t n = due > 512 ? 512 : (size_t)due;
        rb_write(&s->ring, zeros, n);
        *pushed += n;
        due -= n;
    }
}

static int wasapi_loop(capture_source *s, IAudioClient *client, IAudioCaptureClient *cap, HANDLE event) {
    uint64_t t0 = plat_now_ns(), pushed = 0;
    while (atomic_load(&s->running)) {
        DWORD w = WaitForSingleObject(event, 100);
        if (w == WAIT_OBJECT_0) {
            UINT32 packet;
            while (SUCCEEDED(IAudioCaptureClient_GetNextPacketSize(cap, &packet)) && packet) {
                BYTE *data;
                DWORD flags;
                if (FAILED(IAudioCaptureClient_GetBuffer(cap, &data, &packet, &flags, NULL, NULL))) return -1;
                float mono[4096];
                UINT32 frames = packet;
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) memset(mono, 0, frames * sizeof(float));
                else {
                    float *stereo = (float *)data;
                    for (UINT32 i = 0; i < frames; i++)
                        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
                }
                rb_write(&s->ring, mono, frames);
                pushed += frames;
                IAudioCaptureClient_ReleaseBuffer(cap, packet);
            }
        } else if (w != WAIT_TIMEOUT) return -1;
        push_silence(s, t0, &pushed);
    }
    return 0;
}

static DWORD WINAPI wasapi_thread(void *arg) {
    capture_source *s = arg;
    HRESULT hr;
    IMMDeviceEnumerator *enumr = NULL;
    IMMDevice *dev = NULL;
    IAudioClient *client = NULL;
    IAudioCaptureClient *cap = NULL;
    HANDLE event = NULL;
    const char *want = getenv("SALVOQ_DEVICE");
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&enumr);
    if (FAILED(hr)) goto fail;
    if (!want || !*want) {
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumr, eRender, eConsole, &dev);
    } else {
        IMMDeviceCollection *col = NULL;
        UINT count = 0;
        if (FAILED(IMMDeviceEnumerator_EnumAudioEndpoints(enumr, eRender, DEVICE_STATE_ACTIVE, &col))) goto fail;
        IMMDeviceCollection_GetCount(col, &count);
        for (UINT i = 0; i < count; i++) {
            IMMDevice *d = NULL;
            IPropertyStore *store = NULL;
            PROPVARIANT v;
            if (FAILED(IMMDeviceCollection_Item(col, i, &d))) continue;
            if (SUCCEEDED(IMMDevice_OpenPropertyStore(d, STGM_READ, &store)) &&
                SUCCEEDED(IPropertyStore_GetValue(store, &PKEY_Device_FriendlyName, &v))) {
                if (v.pwszVal && StrStrIW(v.pwszVal, (const WCHAR *)want)) { dev = d; break; }
            }
            IMMDevice_Release(d);
        }
        if (!dev) {
            fprintf(stderr, "salvoq: no playback device matches \"%s\"\n", want);
            exit(2);
        }
    }
    if (FAILED(hr) || !dev) goto fail;
    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&client);
    if (FAILED(hr)) goto fail;
    {
        WAVEFORMATEXTENSIBLE fmt;
        memset(&fmt, 0, sizeof fmt);
        fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        fmt.Format.nChannels = 2;
        fmt.Format.nSamplesPerSec = SALVOQ_SAMPLE_RATE;
        fmt.Format.wBitsPerSample = 32;
        fmt.Format.nBlockAlign = 8;
        fmt.Format.nAvgBytesPerSec = SALVOQ_SAMPLE_RATE * 8;
        fmt.Format.cbSize = 22;
        fmt.Samples.wValidBitsPerSample = 32;
        fmt.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        fmt.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        hr = IAudioClient_Initialize(client, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
            0, 0, (WAVEFORMATEX *)&fmt, NULL);
    }
    if (FAILED(hr)) { fprintf(stderr, "salvoq: WASAPI Initialize failed: 0x%08lX\n", (unsigned long)hr); exit(2); }
    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    IAudioClient_SetEventHandle(client, event);
    hr = IAudioClient_GetService(client, &IID_IAudioCaptureClient, (void **)&cap);
    if (FAILED(hr)) goto fail;
    IAudioClient_Start(client);
    if (wasapi_loop(s, client, cap, event) != 0) { fprintf(stderr, "salvoq: capture failed\n"); exit(2); }
    IAudioClient_Stop(client);
    if (cap) IAudioCaptureClient_Release(cap);
    if (client) IAudioClient_Release(client);
    if (dev) IMMDevice_Release(dev);
    if (enumr) IMMDeviceEnumerator_Release(enumr);
    if (event) CloseHandle(event);
    CoUninitialize();
    return 0;
fail:
    fprintf(stderr, "salvoq: WASAPI capture failed: 0x%08lX (try --test-tone)\n", (unsigned long)hr);
    exit(2);
}

int capture_start_wasapi(capture_source *s, const char *device) {
    (void)device;
    atomic_store(&s->running, 1);
    plat_thread t;
    if (plat_thread_create(&t, wasapi_thread, s) != 0) return -1;
    return 0;
}
