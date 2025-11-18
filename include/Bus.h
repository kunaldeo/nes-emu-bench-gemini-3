#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "Cartridge.h"
#include "PPU.h"
#include "APU.h"

class CPU; // Forward declaration

class Bus {
public:
    Bus();
    ~Bus();

    // Devices on the bus
    std::shared_ptr<CPU> cpu;
    std::shared_ptr<PPU> ppu;
    std::shared_ptr<APU> apu;
    std::shared_ptr<Cartridge> cart;
    
    // 2KB System RAM
    std::array<uint8_t, 2048> cpuRam;

    // Read/Write
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);
    void insertCartridge(const std::shared_ptr<Cartridge>& cartridge);
    void reset();
    void clock();
    
    // Controller State
    uint8_t controller[2]; 

private:
    uint32_t nSystemClockCounter = 0;
    uint8_t controller_state[2];
};
