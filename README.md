# Salvoq

**Salvoq** is a zero-dependency, ultra-lightweight terminal audio visualizer written entirely in pure C. It captures real-time system audio streams and uses a custom, hand-rolled Fast Fourier Transform (FFT) engine to turn your music into a fluid, animated dashboard of reactive frequency bars right inside your command-line interface [GitHub].

```text
    ██
    ██       ██                  ██
    ██   ██  ██                  ██   ██
██  ██   ██  ██  ██          ██  ██   ██  ██
██  ██   ██  ██  ██   ██     ██  ██   ██  ██   ██
██  ██   ██  ██  ██   ██     ██  ██   ██  ██   ██
═══════════════════════════════════════════════════
[ Salvoq ]  Bass  │  Mids  │  Treble  │  FPS: 60  
```

## ✨ Features

* **Zero Dependencies:** Built entirely using raw C standard libraries. No `FFTW`, no `ncurses`, and no bloated GUI engines.
* **Pure Math Implementation:** Computes discrete frequencies manually using an in-place Radix-2 Cooley-Tukey FFT algorithm with bit-reversal shuffling.
* **Twin-Thread Architecture:** Utilizes POSIX threads (`<pthread.h>`) to separate audio hardware capture loop pipelines from UI rendering calculations.
* **Smooth Rendering:** Uses double-buffered canvas structures and raw ANSI escape sequences to draw audio blocks fluidly at 60 FPS without layout flickering.
* **Hardware Native:** Interfaces directly with **ALSA/PipeWire** on Linux and **CoreAudio** frameworks on macOS [GitHub].

## 🚀 Quick Start

### Prerequisites
Ensure your platform has a standard C compiler (`gcc` or `clang`) and the necessary system development audio headers.

### Compilation
Clone the repository and build the binary natively using the optimized system pipeline configurations:

```bash
gcc -O3 -march=native main.c -o salvoq -lpthread -lm
```

### Execution
Run the compiled application binary straight from your favorite terminal workspace window emulator environment:

```bash
./salvoq
```

## ⚙️ Configuration
Salvoq scales to your terminal screen layouts automatically. It reads hardware sizes via system queries to stretch columns cleanly whether tiled in minimal setups or maximized full-screen.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.

