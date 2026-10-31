# Salvoq

Salvoq is a zero-dependency, ultra-lightweight native C audio visualizer for Windows. Operating via low-latency WASAPI loopback, it executes real-time system-wide or per-application audio stream capture—including concurrent dual side-by-side feeds—leveraging a custom-built 2048-point radix-2 Cooley-Tukey FFT pipeline to render high-fidelity, discrete frequency spectra directly within Windows Terminal.

## Build

Use MSYS2 MINGW64:

```sh
pacman -S mingw-w64-x86_64-gcc make
make
./build/salvoq.exe --test-tone
```

Other targets:

```sh
make debug      # UBSan debug build
make test       # build and run self-tests
make dist-win   # static release executable in dist/salvoq.exe
make clean
```

## How to run

1. Download `salvoq.exe` from GitHub Releases.
2. Optional: verify it with `CertUtil -hashfile salvoq.exe SHA256`.
3. Place it in a folder.
4. If SmartScreen appears, choose **More info** → **Run anyway**.
5. Open Windows Terminal.
6. Run `.\salvoq.exe`, or `.\salvoq.exe --test-tone`.
7. Play audio; the bars respond to the default speakers automatically.
8. For one app: `.\salvoq.exe --app spotify.exe`. For two: `.\salvoq.exe --app spotify.exe --app discord.exe`.
9. Press `q` to quit.

## Command line

Run `salvoq --help` for every option. Examples:

```sh
salvoq --theme neon --tint-strength 80
salvoq --gradient "#001F3F,#0074D9,#7FDBFF" --mirror on
salvoq --bars 32 --bar-gap on --charset ascii
salvoq --db-range -100,-10 --tilt 3 --decay 0.92
salvoq --fps 120 --idle-fps 10 --test-tone
salvoq --app spotify.exe --app discord.exe --split stacked
```

`--app` requires the target program to already be running and Windows 10 version 2004 or later. `SALVOQ_DEVICE` selects a playback device by partial, case-insensitive name; an unmatched name exits with code 2. It is ignored with `--test-tone` and `--app`.

## Keys

- `q` or `Q`: quit
- `Ctrl-C`: quit

## Exit codes

- `0`: normal exit
- `1`: usage error or failed self-test
- `2`: capture or console failure

## Troubleshooting

- **No bars:** check the default playback device, play audio, or try `--test-tone`.
- **Terminal too small:** use a terminal at least 20x6.
- **SmartScreen:** the executable is unsigned.
- **`--app` fails:** ensure the process is running and Windows is version 2004 or newer.

## License

MIT, see LICENSE.
