#include "PPU.h"
#include "Cartridge.h"
#include <cstring>

PPU::PPU() {
    // Fixed NES Palette
    palScreen[0x00] = 0xFF7C7C7C; palScreen[0x01] = 0xFF0000FC; palScreen[0x02] = 0xFF0000BC; palScreen[0x03] = 0xFF4428BC;
    palScreen[0x04] = 0xFF940084; palScreen[0x05] = 0xFFA80020; palScreen[0x06] = 0xFFA81000; palScreen[0x07] = 0xFF881400;
    palScreen[0x08] = 0xFF503000; palScreen[0x09] = 0xFF007800; palScreen[0x0A] = 0xFF006800; palScreen[0x0B] = 0xFF005800;
    palScreen[0x0C] = 0xFF004058; palScreen[0x0D] = 0xFF000000; palScreen[0x0E] = 0xFF000000; palScreen[0x0F] = 0xFF000000;

    palScreen[0x10] = 0xFFBCBCBC; palScreen[0x11] = 0xFF0078F8; palScreen[0x12] = 0xFF0058F8; palScreen[0x13] = 0xFF6844FC;
    palScreen[0x14] = 0xFFD800CC; palScreen[0x15] = 0xFFE40058; palScreen[0x16] = 0xFFF83800; palScreen[0x17] = 0xFFE45C10;
    palScreen[0x18] = 0xFFAC7C00; palScreen[0x19] = 0xFF00B800; palScreen[0x1A] = 0xFF00A800; palScreen[0x1B] = 0xFF00A844;
    palScreen[0x1C] = 0xFF008888; palScreen[0x1D] = 0xFF000000; palScreen[0x1E] = 0xFF000000; palScreen[0x1F] = 0xFF000000;

    palScreen[0x20] = 0xFFF8F8F8; palScreen[0x21] = 0xFF3CBCFC; palScreen[0x22] = 0xFF6888FC; palScreen[0x23] = 0xFF9878F8;
    palScreen[0x24] = 0xFFF878F8; palScreen[0x25] = 0xFFF85898; palScreen[0x26] = 0xFFF87858; palScreen[0x27] = 0xFFFCA044;
    palScreen[0x28] = 0xFFF8B800; palScreen[0x29] = 0xFFB8F818; palScreen[0x2A] = 0xFF58D854; palScreen[0x2B] = 0xFF58F898;
    palScreen[0x2C] = 0xFF00E8D8; palScreen[0x2D] = 0xFF787878; palScreen[0x2E] = 0xFF000000; palScreen[0x2F] = 0xFF000000;

    palScreen[0x30] = 0xFFFCFCFC; palScreen[0x31] = 0xFFA4E4FC; palScreen[0x32] = 0xFFB8B8F8; palScreen[0x33] = 0xFFD8B8F8;
    palScreen[0x34] = 0xFFF8B8F8; palScreen[0x35] = 0xFFF8A4C0; palScreen[0x36] = 0xFFF0D0B0; palScreen[0x37] = 0xFFFCE0A8;
    palScreen[0x38] = 0xFFF8D878; palScreen[0x39] = 0xFFD8F878; palScreen[0x3A] = 0xFFB8F8B8; palScreen[0x3B] = 0xFFB8F8D8;
    palScreen[0x3C] = 0xFF00FCFC; palScreen[0x3D] = 0xFFF8D8F8; palScreen[0x3E] = 0xFF000000; palScreen[0x3F] = 0xFF000000;

    memset(sprScreen, 0, sizeof(sprScreen));
    memset(tblName, 0, sizeof(tblName));
    memset(tblPalette, 0, sizeof(tblPalette));
    memset(oam, 0, sizeof(oam));
    memset(spriteScanline, 0, sizeof(spriteScanline));
    
    vram_addr.reg = 0;
    tram_addr.reg = 0;
    fine_x = 0;
    write_toggle = 0;
    
    bg_shifter_pattern_lo = 0;
    bg_shifter_pattern_hi = 0;
    bg_shifter_attrib_lo = 0;
    bg_shifter_attrib_hi = 0;
}

PPU::~PPU() {
}

void PPU::ConnectCartridge(const std::shared_ptr<Cartridge>& cart) {
    this->cart = cart;
}

uint32_t* PPU::GetScreen() {
    return sprScreen;
}

void PPU::setOAMAddress(uint8_t addr) {
    oam_addr = addr;
}

void PPU::writeOAMData(uint8_t data) {
    oam[oam_addr] = data;
    oam_addr++;
}

uint8_t PPU::cpuRead(uint16_t addr, bool rdonly) {
    uint8_t data = 0x00;
    switch (addr) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
            status.vertical_blank = 0;
            write_toggle = 0;
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            data = oam[oam_addr];
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: // PPU Data
            data = ppu_data_buffer;
            ppu_data_buffer = this->ppuRead(vram_addr.reg);
            
            if (vram_addr.reg >= 0x3F00) data = ppu_data_buffer;
            
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }
    return data;
}

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0x0000: // Control
            control.reg = data;
            tram_addr.nametable_x = control.nametable_x;
            tram_addr.nametable_y = control.nametable_y;
            break;
        case 0x0001: // Mask
            mask.reg = data;
            break;
        case 0x0003: // OAM Address
            oam_addr = data;
            break;
        case 0x0004: // OAM Data
            writeOAMData(data);
            break;
        case 0x0005: // Scroll
            if (write_toggle == 0) {
                fine_x = data & 0x07;
                tram_addr.coarse_x = data >> 3;
                write_toggle = 1;
            } else {
                tram_addr.fine_y = data & 0x07;
                tram_addr.coarse_y = data >> 3;
                write_toggle = 0;
            }
            break;
        case 0x0006: // PPU Address
            if (write_toggle == 0) {
                tram_addr.reg = (tram_addr.reg & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
                write_toggle = 1;
            } else {
                tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
                vram_addr = tram_addr;
                write_toggle = 0;
            }
            break;
        case 0x0007: // PPU Data
            this->ppuWrite(vram_addr.reg, data);
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }
}

uint8_t PPU::ppuRead(uint16_t addr, bool rdonly) {
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cart->ppuRead(addr, data)) {
        // Cartridge
    } else if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Pattern Table
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        // Nametables with mirroring
        addr &= 0x0FFF;
        if (cart->Mirror() == Cartridge::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF) data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF) data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF) data = tblName[1][addr & 0x03FF];
        } else if (cart->Mirror() == Cartridge::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF) data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF) data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF) data = tblName[1][addr & 0x03FF];
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        data = tblPalette[addr];
    }
    return data;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;
    if (cart->ppuWrite(addr, data)) {
        // Cartridge
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        if (cart->Mirror() == Cartridge::VERTICAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF) tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF) tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF) tblName[1][addr & 0x03FF] = data;
        } else if (cart->Mirror() == Cartridge::HORIZONTAL) {
            if (addr >= 0x0000 && addr <= 0x03FF) tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF) tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF) tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF) tblName[1][addr & 0x03FF] = data;
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        tblPalette[addr] = data;
    }
}

void PPU::clock() {
    // Helper: Load Shifters
    auto LoadBackgroundShifters = [&]() {
        bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
        bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
        bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0x01) ? 0xFF : 0x00);
        bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0x02) ? 0xFF : 0x00);
    };

    // Helper: Update Shifters
    auto UpdateShifters = [&]() {
        if (mask.render_background) {
            bg_shifter_pattern_lo <<= 1;
            bg_shifter_pattern_hi <<= 1;
            bg_shifter_attrib_lo <<= 1;
            bg_shifter_attrib_hi <<= 1;
        }
    };
    
    // Helper: Increment Scroll X
    auto IncrementScrollX = [&]() {
        if (mask.render_background || mask.render_sprites) {
            if (vram_addr.coarse_x == 31) {
                vram_addr.coarse_x = 0;
                vram_addr.nametable_x = ~vram_addr.nametable_x;
            } else {
                vram_addr.coarse_x++;
            }
        }
    };
    
    // Helper: Increment Scroll Y
    auto IncrementScrollY = [&]() {
        if (mask.render_background || mask.render_sprites) {
            if (vram_addr.fine_y < 7) {
                vram_addr.fine_y++;
            } else {
                vram_addr.fine_y = 0;
                if (vram_addr.coarse_y == 29) {
                    vram_addr.coarse_y = 0;
                    vram_addr.nametable_y = ~vram_addr.nametable_y;
                } else if (vram_addr.coarse_y == 31) {
                    vram_addr.coarse_y = 0;
                } else {
                    vram_addr.coarse_y++;
                }
            }
        }
    };
    
    auto TransferAddressX = [&]() {
        if (mask.render_background || mask.render_sprites) {
            vram_addr.nametable_x = tram_addr.nametable_x;
            vram_addr.coarse_x = tram_addr.coarse_x;
        }
    };

    auto TransferAddressY = [&]() {
        if (mask.render_background || mask.render_sprites) {
            vram_addr.fine_y = tram_addr.fine_y;
            vram_addr.nametable_y = tram_addr.nametable_y;
            vram_addr.coarse_y = tram_addr.coarse_y;
        }
    };

    if (scanline >= -1 && scanline < 240) {
        if (scanline == 0 && cycle == 0) cycle = 1;
        
        if (scanline == -1 && cycle == 1) {
             status.vertical_blank = 0;
             status.sprite_overflow = 0;
             status.sprite_zero_hit = 0;
             for(int i=0; i<8; i++) {
                 spriteScanline[i].y = 0xFF;
             }
             sprite_count = 0;
        }

        if ((cycle >= 2 && cycle <= 257) || (cycle >= 321 && cycle <= 338)) {
            UpdateShifters();
            
            if (mask.render_background || mask.render_sprites) { // Fetch cycles
                 switch ((cycle - 1) % 8) {
                     case 0:
                         LoadBackgroundShifters();
                         bg_next_tile_id = this->ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
                         break;
                     case 2:
                         bg_next_tile_attrib = this->ppuRead(0x23C0 | (vram_addr.nametable_y << 11) 
                                                             | (vram_addr.nametable_x << 10) 
                                                             | ((vram_addr.coarse_y >> 2) << 3) 
                                                             | (vram_addr.coarse_x >> 2));
                         if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
                         if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
                         bg_next_tile_attrib &= 0x03;
                         break;
                     case 4:
                         bg_next_tile_lsb = this->ppuRead((control.pattern_background << 12) 
                                                          + ((uint16_t)bg_next_tile_id << 4) 
                                                          + (vram_addr.fine_y) + 0);
                         break;
                     case 6:
                         bg_next_tile_msb = this->ppuRead((control.pattern_background << 12) 
                                                          + ((uint16_t)bg_next_tile_id << 4) 
                                                          + (vram_addr.fine_y) + 8);
                         break;
                     case 7:
                         IncrementScrollX();
                         break;
                 }
            }
        }
        
        if (cycle == 256) IncrementScrollY();
        if (cycle == 257) {
            LoadBackgroundShifters();
            TransferAddressX();
        }
        if (cycle == 338 || cycle == 340) bg_next_tile_id = this->ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
        if (scanline == -1 && cycle >= 280 && cycle < 305) TransferAddressY();

        // Sprite Evaluation
        if (cycle == 257 && scanline >= 0) {
            memset(spriteScanline, 0xFF, sizeof(spriteScanline));
            sprite_count = 0;
            uint8_t nOAMEntry = 0;
            for (uint8_t i = 0; i < 64; i++) {
                uint8_t y = oam[i*4 + 0];
                int16_t diff = (int16_t)scanline - (int16_t)y;
                uint8_t size = control.sprite_size ? 16 : 8;
                if (diff >= 0 && diff < size && nOAMEntry < 8) {
                    spriteScanline[nOAMEntry].y = y;
                    spriteScanline[nOAMEntry].tile_id = oam[i*4 + 1];
                    spriteScanline[nOAMEntry].attribute = oam[i*4 + 2];
                    spriteScanline[nOAMEntry].x = oam[i*4 + 3];
                    spriteScanline[nOAMEntry].id = i;
                    nOAMEntry++;
                }
            }
            sprite_count = nOAMEntry;
        }
    }

    // Rendering (Pixel Output)
    if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
        uint8_t bg_pixel = 0;
        uint8_t bg_palette = 0;
        
        if (mask.render_background) {
            if (mask.render_background_left || (cycle - 1) >= 8) {
                uint16_t bit_mux = 0x8000 >> fine_x;
                uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
                uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
                bg_pixel = (p1 << 1) | p0;
                
                uint8_t pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
                uint8_t pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
                bg_palette = (pal1 << 1) | pal0;
            }
        }
        
        uint8_t spr_pixel = 0;
        uint8_t spr_palette = 0;
        uint8_t spr_priority = 0;
        bool spr_zero = false;
        
        if (mask.render_sprites) {
            if (mask.render_sprites_left || (cycle - 1) >= 8) {
                for (uint8_t i = 0; i < sprite_count; i++) {
                    int x = cycle - 1;
                    if (x >= spriteScanline[i].x && x < spriteScanline[i].x + 8) {
                        uint8_t size = control.sprite_size ? 16 : 8;
                        uint8_t row = scanline - spriteScanline[i].y - 1; // -1 for delay
                        uint8_t attr = spriteScanline[i].attribute;
                        if (attr & 0x80) row = size - 1 - row;
                        
                        uint8_t tile_idx = spriteScanline[i].tile_id;
                        uint16_t pat_addr = 0;
                        if (size == 8) pat_addr = (control.pattern_sprite ? 0x1000 : 0x0000) + (tile_idx * 16) + row;
                        else pat_addr = ((tile_idx & 0x01) ? 0x1000 : 0x0000) + ((tile_idx & 0xFE) * 16) + row + ((row >= 8) ? 8 : 0); // row >=8 ? 16-8 : 0 simplifies
                        // Wait, 8x16: top tile is index & FE. bottom is index | 01. 
                        // 16 bytes per tile.
                        if (size == 16) {
                             if (row < 8) pat_addr = ((tile_idx & 0x01) ? 0x1000 : 0x0000) + ((tile_idx & 0xFE) * 16) + row;
                             else pat_addr = ((tile_idx & 0x01) ? 0x1000 : 0x0000) + ((tile_idx & 0xFE) * 16) + 16 + (row - 8);
                        }
                        
                        uint8_t p_lo = this->ppuRead(pat_addr);
                        uint8_t p_hi = this->ppuRead(pat_addr + 8);
                        uint8_t fx = x - spriteScanline[i].x;
                        if (attr & 0x40) fx = 7 - fx;
                        
                        uint8_t p_val = ((p_lo >> (7 - fx)) & 0x01) | (((p_hi >> (7 - fx)) & 0x01) << 1);
                        
                        if (p_val != 0) {
                            spr_pixel = p_val;
                            spr_palette = (attr & 0x03) + 4;
                            spr_priority = (attr & 0x20) == 0;
                            if (spriteScanline[i].id == 0) spr_zero = true;
                            break;
                        }
                    }
                }
            }
        }
        
        uint32_t final_color = palScreen[this->ppuRead(0x3F00)];
        
        if (bg_pixel == 0 && spr_pixel == 0) {
            final_color = palScreen[this->ppuRead(0x3F00)];
        } else if (bg_pixel != 0 && spr_pixel == 0) {
            final_color = palScreen[this->ppuRead(0x3F00 + (bg_palette << 2) + bg_pixel)];
        } else if (bg_pixel == 0 && spr_pixel != 0) {
            final_color = palScreen[this->ppuRead(0x3F00 + (spr_palette << 2) + spr_pixel)];
        } else {
            if (spr_zero && mask.render_background && mask.render_sprites) {
                if (!mask.render_background_left || !mask.render_sprites_left) {
                    if (cycle - 1 >= 8) status.sprite_zero_hit = 1;
                } else {
                    if (cycle - 1 != 255) status.sprite_zero_hit = 1;
                }
            }
            
            if (spr_priority) {
                final_color = palScreen[this->ppuRead(0x3F00 + (spr_palette << 2) + spr_pixel)];
            } else {
                final_color = palScreen[this->ppuRead(0x3F00 + (bg_palette << 2) + bg_pixel)];
            }
        }
        
        sprScreen[scanline * 256 + (cycle - 1)] = final_color;
    }

    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
        }
    }
    if (scanline == 241 && cycle == 1) {
        status.vertical_blank = 1;
        if (control.enable_nmi) nmi = true;
    }
}
