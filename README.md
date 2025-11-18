# NES Emulator

A C++ NES Emulator using SDL2. This project implements the core components of the Nintendo Entertainment System, including the CPU (6502), PPU (2C02), and APU (Audio), providing a playable experience for NROM (Mapper 0) games like *Super Mario Bros.*

## Features
- **CPU**: Cycle-accurate Ricoh 2A03 (MOS 6502 variant) emulation.
- **PPU**: Cycle-accurate rendering pipeline with support for background scrolling (Loopy), sprites, and correct timing.
- **APU**: Implementation of Pulse 1, Pulse 2, Triangle, and Noise channels for authentic audio.
- **Mappers**: Support for iNES Mapper 0 (NROM).

## Prerequisites
- **CMake** (3.10 or higher)
- **SDL2** development libraries (Simple DirectMedia Layer)
- **C++ Compiler** with C++17 support (GCC, Clang, MSVC)

## Building

1. Create a build directory within the project root:
   ```bash
   mkdir build
   cd build
   ```

2. Generate the build files using CMake:
   ```bash
   cmake ..
   ```

3. Compile the project:
   ```bash
   make
   ```

## Usage

The emulator expects a ROM file named `mario.nes` to be present in the project root directory.

To start the emulator:

```bash
./build/nes_emu
```

### Controls

| Keyboard Key | NES Controller Input |
| :--- | :--- |
| **Arrow Keys** | D-Pad (Up, Down, Left, Right) |
| **X** | A Button |
| **Z** | B Button |
| **A** | Select |
| **S** | Start |
| **ESC** | Quit Emulator |

## Debugging Mode

The emulator features a built-in CPU register debugger useful for tracing execution flow.

- **Activate/Deactivate**: Press the **`D`** key during gameplay.
- **Output**: Debug logs are printed directly to the console (stdout).

**Debug Output Format:**
The log displays the state of the CPU registers at the beginning of each frame:

```text
PC: <Program Counter>, A: <Accumulator>, X: <X Reg>, Y: <Y Reg>, Status: <Flags>
```

**Example:**
```text
PC: 800A, A: 00, X: FF, Y: 00, Status: 26
PC: 8014, A: 01, X: FF, Y: 00, Status: 24
```

This allows you to monitor the internal state of the emulated CPU in real-time.
