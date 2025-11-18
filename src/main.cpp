#include <iostream>
#include <SDL.h>
#include <memory>
#include "Bus.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow("NES Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        256 * 3, 240 * 3, SDL_WINDOW_SHOWN);
        
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 256, 240);

    bool quit = false;
    bool debug = false;
    SDL_Event event;
    int frame_count = 0;

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

        // Emulation Step
        for (int i = 0; i < 89342; i++) {
             nes.clock();
        }
        frame_count++;
        
        if (debug) {
             printf("PC: %04X, A: %02X, X: %02X, Y: %02X, Status: %02X\n", 
                    nes.cpu->pc, nes.cpu->a, nes.cpu->x, nes.cpu->y, nes.cpu->status);
        }

        // Draw
        SDL_UpdateTexture(texture, NULL, nes.ppu->GetScreen(), 256 * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        uint32_t frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
