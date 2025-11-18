Gemini 3 has been released, and I couldn't wait to run my own benchmark on it. I lovingly call it nes-emu-bench. The task of this benchmark is for a coding agent to implement a NES emulator (my favorite Nintendo Gaming Console) which is cycle-accurate-ish and has accurate-ish sound when playing Super Mario Bros. The goal is for a normal human being to find it fun to play. The previous champion of this benchmark was Claude 4.5 with claude-code, which took 4 hours and multiple prompts. Gemini 3 with gemini-cli broke the record in just ~50 minutes, producing a much better emulator with little guidance. See the results at: https://github.com/kunaldeo/nes-emu-bench-gemini-3

### More on the benchmark:
The benchmark includes NES Hardware Specs (https://d1.amobbs.com/bbs_upload782111/files_28/ourdev_551332.pdf) for the coding agent to refer. A Super Mario ROM is provided for the coding agent to check its work against.
The prompt includes instructions that the coding agent should build debugging tools such as taking screenshots from the running emulator and printing CPU/PPU register states. This allows the coding agent to run autonomously as it works towards building the emulator and saves me from providing screenshots and other debug data manually.
Some aspects are still difficult for the coding agent to gauge, such as playback speed and sound accuracy, so these were provided manually.

<img width="805" height="448" alt="Gemini 3 gemini-cli" src="https://github.com/user-attachments/assets/97a6cc71-5835-49a8-b279-4655e0320251" />


# NES Emulator

A C++ NES Emulator using SDL2. This project implements the core components of the Nintendo Entertainment System, including the CPU (6502), PPU (2C02), and APU (Audio), providing a playable experience for NROM (Mapper 0) games like *Super Mario Bros*.

<img width="798" height="778" alt="Screenshot 2025-11-19 at 1 47 41 AM" src="https://github.com/user-attachments/assets/d4323412-f01d-44ea-bce5-8f24f96257f3" />


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
