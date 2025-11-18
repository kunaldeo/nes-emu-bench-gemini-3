#include "Bus.h"
#include "CPU.h"
#include "PPU.h"

Bus::Bus() {
    // Clear RAM
    for (auto& i : cpuRam) i = 0x00;
    
    // Connect devices
    cpu = std::make_shared<CPU>();
    ppu = std::make_shared<PPU>();
    apu = std::make_shared<APU>();
    
    cpu->ConnectBus(this);
}

Bus::~Bus() {
}

void Bus::write(uint16_t addr, uint8_t data) {
    if (cart->cpuWrite(addr, data)) {
        // The cartridge handled the write
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        // System RAM Address mirroring
        cpuRam[addr & 0x07FF] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        ppu->cpuWrite(addr & 0x0007, data);
    }
    else if (addr >= 0x4000 && addr <= 0x4017) {
        // APU Registers (excluding 4014 DMA and 4016/4017 controller which overlap)
        if (addr == 0x4014) {
             // DMA
            uint8_t dma_page = data;
            uint16_t dma_addr = (uint16_t)dma_page << 8;
            
            ppu->setOAMAddress(0x00);
            for (uint16_t i = 0; i < 256; i++) {
                ppu->writeOAMData(read(dma_addr + i));
            }
        } 
        else if (addr == 0x4016 || addr == 0x4017) {
             // Controller & APU Frame Counter (4017)
             // 4017 is BOTH Controller 2 Write AND APU Frame Counter
             if (addr == 0x4016) controller_state[0] = controller[0];
             if (addr == 0x4017) {
                 controller_state[1] = controller[1]; // Usually unused
                 apu->cpuWrite(addr, data); // Frame Counter
             }
        }
        else {
             apu->cpuWrite(addr, data);
        }
    }
    // Controller I/O ...
}

uint8_t Bus::read(uint16_t addr, bool bReadOnly) {
    uint8_t data = 0x00;

    if (cart->cpuRead(addr, data)) {
        // Cartridge handled the read
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF) {
        // System RAM Address mirroring
        data = cpuRam[addr & 0x07FF];
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        data = ppu->cpuRead(addr & 0x0007, bReadOnly);
    }
    else if (addr >= 0x4000 && addr <= 0x4015) {
        data = apu->cpuRead(addr);
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        data = (controller_state[addr & 0x0001] & 0x80) > 0;
        controller_state[addr & 0x0001] <<= 1;
    }

    return data;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cartridge) {
    this->cart = cartridge;
    ppu->ConnectCartridge(cartridge);
}

void Bus::reset() {
    cpu->reset();
    nSystemClockCounter = 0;
}

void Bus::clock() {
    ppu->clock();
    
    if (nSystemClockCounter % 3 == 0) {
        cpu->clock();
        apu->clock();
    }
    
    if (ppu->nmi) {
        ppu->nmi = false;
        cpu->nmi();
    }
    
    nSystemClockCounter++;
}