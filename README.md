


https://github.com/user-attachments/assets/37856765-a51d-4a3d-8d4d-d0c2c01feaae




# Salvoq

Salvoq is a zero-dependency, ultra-lightweight native C audio visualizer for Windows. Operating via low-latency WASAPI loopback, it executes real-time system-wide or per-application audio stream capture—including concurrent dual side-by-side feeds—leveraging a custom-built 2048-point radix-2 Cooley-Tukey FFT pipeline to render high-fidelity, discrete frequency spectra directly within Windows Terminal.

Salvoq is Windows-only and works best in Windows Terminal. It captures the default playback mix automatically; no virtual cable or setup is required.

## Build

Install MSYS2, open the MINGW64 shell, and install the toolchain:

```sh
pacman -S mingw-w64-x86_64-gcc make
```

Run the targets from the repository root:

```sh
make              # optimized build/salvoq.exe
make debug        # UBSan debug build/salvoq-debug.exe
make test         # build, then run --selftest
make dist-win     # static dist/salvoq.exe
make clean        # remove build/ and dist/
```

The release executable is statically linked and needs no MSYS2 DLLs. It is unsigned, so Windows SmartScreen may require More info, then Run anyway.

## How to run

1. Download [`salvoq.exe`](https://github.com/SalpyDev/Salvoq/releases/download/v1.0.0-rc11/salvoq.exe) from GitHub Releases and optionally download `SHA256SUMS.txt`.
2. Verify it with `CertUtil -hashfile salvoq.exe SHA256` or `sha256sum salvoq.exe`.
3. Open PowerShell or Windows Terminal and change to the folder containing the executable, for example:

	```powershell
	cd "$HOME\Downloads"
	.\salvoq.exe
	```

	PowerShell does not search the current directory when resolving commands, so `salvoq` by itself fails unless the executable's folder has been added to `PATH`. Use `.\salvoq.exe` from that folder, or use the full path:

	```powershell
	& "C:\Path\To\salvoq.exe" --theme neon --tint-strength 80
	```

4. Play audio. The bars follow the default playback device.
5. Use `.\salvoq.exe --test-tone` when no audio hardware is available.
6. Press `q` or `Ctrl-C` to quit.

The reference classic presentation uses 26 grouped bars, a black background, one-cell gaps, a green-to-yellow-to-red vertical gradient, colored lower-band tinting, centered band labels, and no peak-marker glyphs.

## Command line

Run `salvoq --help` for the same list printed by the executable. Options may appear in any order. `--help`, `--version`, and `--selftest` act immediately and ignore later options. Repeated options use their last value, except `--app`, which may be given at most twice, and `--band-color`, where each band keeps its last value.

### Sources and timing

| Option | Values and default | Description |
| --- | --- | --- |
| `--test-tone` | off | Use the diagnostic synthetic source instead of WASAPI. `SALVOQ_DEVICE` is ignored. |
| `--fps N` | `1..120`, default `60` | Active target frame rate. |
| `--idle-fps N` | `1..120`, default `15` | Frame rate after one second with every bar at zero. |
| `--selftest` | | Run FFT and Hann-window tests, then exit 0 on pass or 1 on failure. |
| `--version` | | Print the Makefile version and exit 0. |
| `--help` | | Print usage, options, and `SALVOQ_DEVICE`, then exit 0. |

### Themes and customization

`--theme` accepts `classic`, `neon`, `ocean`, `ember`, `mono`, `synthwave`, or `luna`; default is `classic`.

| Option | Values and default | Description |
| --- | --- | --- |
| `--theme NAME` | seven names, default `classic` | Select the complete palette. |
| `--gradient "#hex,#hex,..."` | 2 to 6 colors | Replace the theme gradient from bottom to top. |
| `--tint on\|off` | default on; `luna` off | Enable or disable band tinting. |
| `--tint-strength N` | `0..100`, default `60` | Tint blend percentage. |
| `--tint-height N` | `0..100`, default `25` | Percentage of each bar receiving tint. |
| `--band-color BAND=#hex` | `bass`, `mids`, `treble` | Override one band color; repeatable. |
| `--peak-color #hex` | default `#FFFFFF` | Accepted compatibility option. Peak-marker glyphs are disabled in the reference UI. |
| `--color-depth MODE` | `auto`, `truecolor`, `256`, `none`; default `auto` | Select terminal color output. |
| `--bg #hex` | terminal default | Set the background color of every cell. |
| `--mirror on\|off` | default off | Grow bars around the center row. |
| `--bar-gap MODE` | `auto`, `on`, `off`; default `auto` | Control the one-cell gaps. |
| `--charset MODE` | `blocks`, `ascii`; default `blocks` | Use block glyphs or `#` for filled cells. |

The classic palette is green `#00E676`, yellow `#FFEA00`, red `#FF1744`, with bass magenta `#D500F9`, mids cyan `#00E5FF`, and treble blue `#2979FF`. The other theme colors are defined in the renderer and can be overridden with the options above. `luna` is the black-and-white theme.

Examples:

```sh
.\salvoq.exe --theme neon --tint-strength 80
.\salvoq.exe --gradient "#001F3F,#0074D9,#7FDBFF" --mirror on
.\salvoq.exe --band-color bass=#FF00FF --band-color treble=#00FFFF
.\salvoq.exe --bg '#000000' --color-depth truecolor --charset ascii
.\salvoq.exe --bars 32 --bar-gap on --charset ascii
```

### Dynamics, frequency, and layout

| Option | Values and default | Description |
| --- | --- | --- |
| `--peak-hold S` | `0..10`, default `1.0` | Peak-state hold time. Marker drawing is disabled in the reference UI. |
| `--peak-fall U` | `0..5`, default `0.6`; `0` disables peak state | Peak-state fall speed. |
| `--decay F` | `0.50..0.99`, default `0.85` | Per-analysis-frame release factor. |
| `--attack MODE` | `instant`, `smooth`; default `instant` | Instant or half-step attack. |
| `--db-range FLOOR,CEIL` | default `-80,0` | FLOOR `-120..-20`; CEIL `-20..0`; CEIL must exceed FLOOR. |
| `--tilt DB_PER_OCT` | `-6..6`, default `0` | High-frequency tilt before dB mapping. |
| `--freq-range LO,HI` | default `20,20000` | LO `20..20000`; HI must exceed LO and be at most 20000. |
| `--bars N` | `16`, `32`, `64`, or `128`; default `64` | FFT analysis bars. The classic reference groups them into 26 display columns. |
| `--style MODE` | `classic`; `polished` prints a compatibility message | Classic is the only style. |

Examples:

```sh
salvoq --db-range -100,-10 --tilt 3 --decay 0.92
salvoq --attack smooth --peak-hold 0 --peak-fall 0
salvoq --freq-range 40,16000 --bars 32
salvoq --fps 120 --idle-fps 10 --test-tone
```

### Per-application capture

`--app NAME` captures the named running executable and its child-process tree instead of the default device mix. `.exe` is added if omitted. The target must already be running, and this feature needs Windows 10 version 2004 (build 19041) or later. `SALVOQ_DEVICE` is ignored with `--app`.

```sh
salvoq --app spotify.exe
salvoq --app spotify.exe --app discord.exe --split side
salvoq --app spotify.exe --app discord.exe --split stacked
```

`--split` accepts `side` or `stacked` and defaults to `side`. It is ignored with one app. A missing app value, third app, duplicate app, app combined with `--test-tone`, or invalid split mode exits 1. An unknown running app exits 2.

## Device selection

`SALVOQ_DEVICE` is a case-insensitive substring of an active playback device friendly name:

```powershell
$env:SALVOQ_DEVICE = "Speakers"
.\salvoq.exe
```

If no active endpoint matches, Salvoq prints the error and exits 2. The variable is ignored with `--test-tone` and `--app`. Exclusive-mode applications are not captured, and a default-device change takes effect after restarting Salvoq.

## Parsing and exit codes

Missing values, out-of-range values, malformed colors, invalid names, invalid modes, and invalid range ordering print usage to stderr and exit 1. `SALVOQ_TRACE=1` prints per-frame values in debug builds only.

| Code | Meaning |
| --- | --- |
| 0 | Normal `q`, `Q`, `Ctrl-C`, or close-event exit. |
| 1 | Usage error, bad option/value, or failed self-test. |
| 2 | Capture failure, unmatched playback device, process-loopback failure, or console failure. |

## Troubleshooting

- No bars: confirm the default playback device and try `--test-tone`.
- Terminal too small: use at least 20x6.
- Full-screen transition: allow the terminal one frame to settle; the renderer preserves the audio history while the console is resized.
- `--app` failure: start the process first and check that Windows is version 2004 or newer.
- SmartScreen: the release is unsigned; choose More info, then Run anyway.

## License

MIT, see LICENSE.
