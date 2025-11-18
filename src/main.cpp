#include <iostream>
#include <iomanip>
#include <SDL.h>
#include <memory>
#include <vector>
#include "Bus.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "Cartridge.h"

// Audio settings
const int SAMPLE_RATE = 44100;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / 60;

int main(int argc, char* argv[]) {
    Bus nes;
    std::shared_ptr<Cartridge> cart = std::make_shared<Cartridge>("mario.nes");

    if (!cart->ImageValid()) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    nes.insertCartridge(cart);
    nes.reset();

    // SDL Setup
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow("NES Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        256 * 3, 240 * 3, SDL_WINDOW_SHOWN);
        
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 256, 240);
        
    // Audio Setup
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL; // Use queueing

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_PauseAudioDevice(audioDevice, 0);

    bool quit = false;
    bool debug = false;
    SDL_Event event;
    
    std::vector<float> audioBuffer;
    audioBuffer.reserve(SAMPLES_PER_FRAME);
    
    // Timing for Audio Sync (replaces simple Delay)
    // CPU runs at 1.789773 MHz. Audio at 44100 Hz.
    // Samples per CPU cycle = 44100 / 1789773 = ~0.0246
    // Or CPU cycles per sample = ~40.58
    double sample_accumulator = 0;
    double cpu_cycles_per_sample = 1789773.0 / 44100.0;

    while (!quit) {
        uint32_t frameStart = SDL_GetTicks();
        
        // Handle Input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
                if (event.key.keysym.sym == SDLK_d) debug = !debug;
            }
        }
        
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        nes.controller[0] = 0x00;
        nes.controller[0] |= state[SDL_SCANCODE_X] ? 0x80 : 0x00; // A
        nes.controller[0] |= state[SDL_SCANCODE_Z] ? 0x40 : 0x00; // B
        nes.controller[0] |= state[SDL_SCANCODE_A] ? 0x20 : 0x00; // Select
        nes.controller[0] |= state[SDL_SCANCODE_S] ? 0x10 : 0x00; // Start
        nes.controller[0] |= state[SDL_SCANCODE_UP] ? 0x08 : 0x00;
        nes.controller[0] |= state[SDL_SCANCODE_DOWN] ? 0x04 : 0x00;
        nes.controller[0] |= state[SDL_SCANCODE_LEFT] ? 0x02 : 0x00;
        nes.controller[0] |= state[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00;

        if (debug) {
             std::cout << "PC: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << nes.cpu->pc
                       << ", A: " << std::setw(2) << (int)nes.cpu->a
                       << ", X: " << std::setw(2) << (int)nes.cpu->x
                       << ", Y: " << std::setw(2) << (int)nes.cpu->y
                       << ", Status: " << std::setw(2) << (int)nes.cpu->status
                       << std::dec << std::endl;
        }

        // Emulation Step
        for (int i = 0; i < 89342; i++) { // PPU Cycles per frame
             nes.clock();
             
             // Audio Sampling
             // APU now runs at CPU clock (1.79MHz).
             // We want 44.1kHz.
             // CPU cycles per sample = 1789773 / 44100 = ~40.58
             
             // Bus::clock runs at PPU speed (3x CPU).
             // So we increment accumulator by 1.0 for every PPU cycle?
             // No, APU state updates every 3 PPU cycles.
             // If we sample every PPU cycle, we oversample.
             // We should sample based on PPU time.
             
             sample_accumulator += 1.0; // 1 PPU cycle
             // PPU Clock / Sample Rate = 5369318 / 44100 = 121.75
             
             if (sample_accumulator >= 121.75) {
                 sample_accumulator -= 121.75;
                 float sample = (float)nes.apu->GetOutputSample();
                 audioBuffer.push_back(sample);
             }
        }
        
        // Queue Audio
        if (SDL_GetQueuedAudioSize(audioDevice) < 4096 * 4) { // Don't buffer too much to avoid latency
             SDL_QueueAudio(audioDevice, audioBuffer.data(), audioBuffer.size() * sizeof(float));
        }
        audioBuffer.clear();

        // Draw
        SDL_UpdateTexture(texture, NULL, nes.ppu->GetScreen(), 256 * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // Sync using Audio Queue instead of Delay if possible, or hybrid
        // Basic Delay Sync
        uint32_t frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }

    SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
