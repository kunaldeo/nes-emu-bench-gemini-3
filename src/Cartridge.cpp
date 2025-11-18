#include "Cartridge.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& sFileName) {
    struct sHeader {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;

    bImageValid = false;

    std::ifstream ifs;
    ifs.open(sFileName, std::ifstream::binary);
    if (ifs.is_open()) {
        // Read file header
        ifs.read((char*)&header, sizeof(sHeader));

        if (header.name[0] == 'N' && header.name[1] == 'E' && header.name[2] == 'S' && header.name[3] == 0x1A) {
            
            // Read Mapper ID
            nMapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
            
            // Mirroring
            if (header.mapper1 & 0x01) mirror = VERTICAL;
            else mirror = HORIZONTAL;

            // Determine File Format 
            uint8_t nFileType = 1;
            if (header.mapper2 & 0x0C) nFileType = 2; // NES 2.0

            if (nFileType == 1) {
                nPRGBanks = header.prg_rom_chunks;
                vPRGMemory.resize(nPRGBanks * 16384);
                ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

                nCHRBanks = header.chr_rom_chunks;
                if (nCHRBanks == 0) {
                    // Create CHR RAM
                    vCHRMemory.resize(8192);
                } else {
                    vCHRMemory.resize(nCHRBanks * 8192);
                    ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
                }
                
                bImageValid = true;
                std::cout << "ROM Loaded: " << sFileName << std::endl;
                std::cout << "PRG Banks: " << (int)nPRGBanks << " CHR Banks: " << (int)nCHRBanks << " Mapper: " << (int)nMapperID << std::endl;
            }
        }
        ifs.close();
    }
}

Cartridge::~Cartridge() {
}

bool Cartridge::ImageValid() {
    return bImageValid;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t &data) {
    // Mapper 0 Logic
    if (nMapperID == 0) {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            if (nPRGBanks > 1) {
                // 32K ROM
                data = vPRGMemory[addr & 0x7FFF];
            } else {
                // 16K ROM (Mirrored)
                data = vPRGMemory[addr & 0x3FFF];
            }
            return true;
        }
    }
    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    // Mapper 0 Logic
    if (nMapperID == 0) {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            // ROM is read-only
            return true;
        }
    }
    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t &data) {
    // Mapper 0 Logic
    if (nMapperID == 0) {
        if (addr >= 0x0000 && addr <= 0x1FFF) {
            data = vCHRMemory[addr];
            return true;
        }
    }
    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    // Mapper 0 Logic
    if (nMapperID == 0) {
        if (addr >= 0x0000 && addr <= 0x1FFF) {
            if (nCHRBanks == 0) {
                // If CHR RAM
                vCHRMemory[addr] = data;
                return true;
            }
        }
    }
    return false;
}

Cartridge::MIRROR Cartridge::Mirror() {
    return mirror;
}
