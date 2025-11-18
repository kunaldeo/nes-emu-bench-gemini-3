#pragma once
#include <cstdint>
#include <memory>
#include <array>

class Cartridge;

class PPU {
public:
    PPU();
    ~PPU();

    // Communications with Main Bus
    uint8_t cpuRead(uint16_t addr, bool rdonly = false);
    void cpuWrite(uint16_t addr, uint8_t data);

    // Communications with PPU Bus
    uint8_t ppuRead(uint16_t addr, bool rdonly = false);
    void ppuWrite(uint16_t addr, uint8_t data);

    // Interface
    void ConnectCartridge(const std::shared_ptr<Cartridge>& cart);
    void clock();
    uint32_t* GetScreen();

    // Public OAM Access for DMA
    void setOAMAddress(uint8_t addr);
    void writeOAMData(uint8_t data);
    
    // Status Flags
    bool nmi = false;

private:
    std::shared_ptr<Cartridge> cart;

    // Visuals
    uint32_t sprScreen[256 * 240];
    std::array<uint32_t, 64> palScreen;

    // Memory
    uint8_t tblName[2][1024]; // VRAM (2kB)
    uint8_t tblPalette[32];

    // OAM
    uint8_t oam[256];
    uint8_t oam_addr = 0x00;

    struct sOAMEntry {
        uint8_t y;
        uint8_t id;
        uint8_t attribute;
        uint8_t x;
    } OAM[64];

    // Internal Sprite Rendering State
    struct sSpriteScanline {
        uint8_t y;
        uint8_t id;
        uint8_t attribute;
        uint8_t x;
        uint8_t tile_id;
        uint8_t pattern_lo;
        uint8_t pattern_hi;
    } spriteScanline[8];
    
    uint8_t sprite_count = 0;

    // Registers
    int16_t scanline = 0;
    int16_t cycle = 0;

    union {
        struct {
            uint8_t unused : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_zero_hit : 1;
            uint8_t vertical_blank : 1;
        };
        uint8_t reg;
    } status;

    union {
        struct {
            uint8_t nametable_x : 1;
            uint8_t nametable_y : 1;
            uint8_t increment_mode : 1;
            uint8_t pattern_sprite : 1;
            uint8_t pattern_background : 1;
            uint8_t sprite_size : 1;
            uint8_t slave_mode : 1;
            uint8_t enable_nmi : 1;
        };
        uint8_t reg;
    } control;

    union {
        struct {
            uint8_t grayscale : 1;
            uint8_t render_background_left : 1;
            uint8_t render_sprites_left : 1;
            uint8_t render_background : 1;
            uint8_t render_sprites : 1;
            uint8_t emphasize_red : 1;
            uint8_t emphasize_green : 1;
            uint8_t emphasize_blue : 1;
        };
        uint8_t reg;
    } mask;

    uint8_t address_latch = 0x00;
    uint8_t ppu_data_buffer = 0x00;
    
    // Loopy's VRAM Address Registers
    union loopy_register {
        struct {
            uint16_t coarse_x : 5;
            uint16_t coarse_y : 5;
            uint16_t nametable_x : 1;
            uint16_t nametable_y : 1;
            uint16_t fine_y : 3;
            uint16_t unused : 1;
        };
        uint16_t reg = 0x0000;
    };

    loopy_register vram_addr; // v
    loopy_register tram_addr; // t
    uint8_t fine_x = 0x00;    // x
    uint8_t write_toggle = 0x00; // w

    // Background Shifters
    uint16_t bg_shifter_pattern_lo = 0x0000;
    uint16_t bg_shifter_pattern_hi = 0x0000;
    uint16_t bg_shifter_attrib_lo = 0x0000;
    uint16_t bg_shifter_attrib_hi = 0x0000;

    // Background Latches (Next Tile Data)
    uint8_t bg_next_tile_id = 0x00;
    uint8_t bg_next_tile_attrib = 0x00;
    uint8_t bg_next_tile_lsb = 0x00;
    uint8_t bg_next_tile_msb = 0x00;
};
