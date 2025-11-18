#include "APU.h"
#include <cstring>
#include <cmath>

APU::APU() {
}

APU::~APU() {
}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0x4000: // Pulse 1 Duty/Volume/Envelope
            pulse1.duty_mode = (data & 0xC0) >> 6;
            pulse1.length_counter.halt = (data & 0x20); // Halt / Envelope Loop
            pulse1.envelope.disable = (data & 0x10); // Constant Volume / Envelope Disable
            pulse1.envelope.volume = (data & 0x0F);
            break;
        case 0x4001: // Pulse 1 Sweep
            pulse1.sweep_enable = (data & 0x80);
            pulse1.sweep_period = (data & 0x70) >> 4;
            pulse1.sweep_down = (data & 0x08);
            pulse1.sweep_shift = (data & 0x07);
            pulse1.sweep_reload = true;
            break;
        case 0x4002: // Pulse 1 Timer Low
            pulse1.timer_period = (pulse1.timer_period & 0xFF00) | data;
            break;
        case 0x4003: // Pulse 1 Timer High / Length
            pulse1.timer_period = (pulse1.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
            if (pulse1.enabled) 
                pulse1.length_counter.counter = length_table[(data & 0xF8) >> 3];
            pulse1.envelope.start = true;
            pulse1.duty_value = 0;
            break;

        case 0x4004: // Pulse 2 Duty/Volume/Envelope
            pulse2.duty_mode = (data & 0xC0) >> 6;
            pulse2.length_counter.halt = (data & 0x20);
            pulse2.envelope.disable = (data & 0x10);
            pulse2.envelope.volume = (data & 0x0F);
            break;
        case 0x4005: // Pulse 2 Sweep
            pulse2.sweep_enable = (data & 0x80);
            pulse2.sweep_period = (data & 0x70) >> 4;
            pulse2.sweep_down = (data & 0x08);
            pulse2.sweep_shift = (data & 0x07);
            pulse2.sweep_reload = true;
            break;
        case 0x4006: // Pulse 2 Timer Low
            pulse2.timer_period = (pulse2.timer_period & 0xFF00) | data;
            break;
        case 0x4007: // Pulse 2 Timer High / Length
            pulse2.timer_period = (pulse2.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
            if (pulse2.enabled) 
                pulse2.length_counter.counter = length_table[(data & 0xF8) >> 3];
            pulse2.envelope.start = true;
            pulse2.duty_value = 0;
            break;
        
        case 0x4008: // Triangle Linear Counter
            triangle.control_flag = (data & 0x80); // Control / Length Halt
            triangle.linear_counter_reload = (data & 0x7F);
            break;
        case 0x400A: // Triangle Timer Low
            triangle.timer_period = (triangle.timer_period & 0xFF00) | data;
            break;
        case 0x400B: // Triangle Timer High / Length
            triangle.timer_period = (triangle.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
             if (triangle.enabled) 
                triangle.length_counter.counter = length_table[(data & 0xF8) >> 3];
            triangle.linear_counter_reload_flag = true;
            break;

        case 0x400C: // Noise Envelope
            noise.length_counter.halt = (data & 0x20);
            noise.envelope.disable = (data & 0x10);
            noise.envelope.volume = (data & 0x0F);
            break;
        case 0x400E: // Noise Loop / Period
            noise.mode = (data & 0x80);
            {
                uint16_t periods[] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
                noise.timer_period = periods[data & 0x0F];
            }
            break;
        case 0x400F: // Noise Length
            if (noise.enabled) 
                noise.length_counter.counter = length_table[(data & 0xF8) >> 3];
            noise.envelope.start = true;
            break;

        case 0x4015: // Status
            pulse1.enabled = (data & 0x01);
            pulse2.enabled = (data & 0x02);
            triangle.enabled = (data & 0x04);
            noise.enabled = (data & 0x08);
            
            if (!pulse1.enabled) pulse1.length_counter.counter = 0;
            if (!pulse2.enabled) pulse2.length_counter.counter = 0;
            if (!triangle.enabled) triangle.length_counter.counter = 0;
            if (!noise.enabled) noise.length_counter.counter = 0;
            break;
            
        case 0x4017: // Frame Counter
            frame_counter_mode = (data & 0x80);
            irq_inhibit = (data & 0x40);
            frame_clock_counter = 0;
            
            if (frame_counter_mode) { // 5-step immediately clocks
                 pulse1.length_counter.clock(pulse1.enabled, pulse1.length_counter.halt);
                 pulse2.length_counter.clock(pulse2.enabled, pulse2.length_counter.halt);
                 triangle.length_counter.clock(triangle.enabled, triangle.control_flag);
                 noise.length_counter.clock(noise.enabled, noise.length_counter.halt);
                 
                 pulse1.clock_sweep(0);
                 pulse2.clock_sweep(1);
                 
                 pulse1.envelope.clock(pulse1.length_counter.halt);
                 pulse2.envelope.clock(pulse2.length_counter.halt);
                 triangle.linear_counter = (triangle.control_flag) ? triangle.linear_counter_reload : triangle.linear_counter; // Simpler
                 noise.envelope.clock(noise.length_counter.halt);
            }
            break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    if (addr == 0x4015) {
        if (pulse1.length_counter.counter > 0) data |= 0x01;
        if (pulse2.length_counter.counter > 0) data |= 0x02;
        if (triangle.length_counter.counter > 0) data |= 0x04;
        if (noise.length_counter.counter > 0) data |= 0x08;
    }
    return data;
}

void APU::clock() {
    // Frame Counter (Approximate 240Hz / 4 or 5 steps)
    // Running at CPU clock speed (approx 1.789773 MHz)
    // Frame counter steps every 7457 cycles
    
    frame_clock_counter++;
    
    // Quarter Frame (Envelopes, Linear Counter) approx 240Hz
    // Half Frame (Length Counters, Sweep) approx 120Hz
    
    // Simplified Step 4 mode:
    // Step 1: 7457 (Quarter)
    // Step 2: 14913 (Quarter + Half)
    // Step 3: 22371 (Quarter)
    // Step 4: 29829 (Quarter + Half + IRQ)
    // Step 5: 29830 (Reset)
    
    bool quarter_frame = false;
    bool half_frame = false;
    
    if (frame_clock_counter == 7457) {
        quarter_frame = true;
    } else if (frame_clock_counter == 14913) {
        quarter_frame = true;
        half_frame = true;
    } else if (frame_clock_counter == 22371) {
        quarter_frame = true;
    } else if (frame_clock_counter == 29829) {
        quarter_frame = true;
        half_frame = true;
        if (!frame_counter_mode) frame_clock_counter = 0;
    } else if (frame_clock_counter == 37281) { // Mode 1 step 5
         if (frame_counter_mode) frame_clock_counter = 0;
    }
    
    if (quarter_frame) {
        // Envelopes & Linear Counter
        pulse1.envelope.clock(pulse1.length_counter.halt);
        pulse2.envelope.clock(pulse2.length_counter.halt);
        noise.envelope.clock(noise.length_counter.halt);
        
        // Triangle Linear Counter
        if (triangle.linear_counter_reload_flag) {
            triangle.linear_counter = triangle.linear_counter_reload;
        } else if (triangle.linear_counter > 0) {
            triangle.linear_counter--;
        }
        if (!triangle.control_flag) {
            triangle.linear_counter_reload_flag = false;
        }
    }
    
    if (half_frame) {
        // Length Counters & Sweep
        pulse1.length_counter.clock(pulse1.enabled, pulse1.length_counter.halt);
        pulse2.length_counter.clock(pulse2.enabled, pulse2.length_counter.halt);
        triangle.length_counter.clock(triangle.enabled, triangle.control_flag);
        noise.length_counter.clock(noise.enabled, noise.length_counter.halt);
        
        pulse1.clock_sweep(0);
        pulse2.clock_sweep(1);
    }
    
    // Pulse 1 Clock
    // Pulse Timer clocks every 2 CPU cycles
    if (clock_counter % 2 == 0) {
        if (pulse1.timer > 0) {
            pulse1.timer--;
        } else {
            pulse1.timer = pulse1.timer_period;
            pulse1.duty_value++;
            pulse1.duty_value &= 0x07;
        }
        
        if (pulse2.timer > 0) {
            pulse2.timer--;
        } else {
            pulse2.timer = pulse2.timer_period;
            pulse2.duty_value++;
            pulse2.duty_value &= 0x07;
        }
        
        if (noise.timer > 0) {
            noise.timer--;
        } else {
            noise.timer = noise.timer_period;
            uint8_t feedback = (noise.shift_register & 0x01) ^ ((noise.shift_register >> (noise.mode ? 6 : 1)) & 0x01);
            noise.shift_register >>= 1;
            noise.shift_register |= (feedback << 14);
        }
    }
    
    // Triangle Clock (CPU Speed)
    if (triangle.timer > 0) {
        triangle.timer--;
    } else {
        triangle.timer = triangle.timer_period;
        if (triangle.linear_counter > 0 && triangle.length_counter.counter > 0) {
            triangle.sequence++;
            triangle.sequence &= 0x1F;
        }
    }

    clock_counter++;
}

void APU::reset() {
    frame_clock_counter = 0;
    clock_counter = 0;
    // Reset other states...
}

double APU::GetOutputSample() {
    double pulse_out = 0;
    
    // Pulse 1 Output
    if (pulse1.enabled && pulse1.timer_period > 8 && !pulse1.sweep_mute && pulse1.length_counter.counter > 0) {
        // Duty Cycle Lookup
        // 0: 0 1 0 0 0 0 0 0 (12.5%)
        // 1: 0 1 1 0 0 0 0 0 (25%)
        // 2: 0 1 1 1 1 0 0 0 (50%)
        // 3: 1 0 0 1 1 1 1 1 (25% negated) -> actually 75%
        uint8_t seq = 0;
        switch (pulse1.duty_mode) {
            case 0: seq = 0b01000000; break; // 12.5
            case 1: seq = 0b01100000; break; // 25
            case 2: seq = 0b01111000; break; // 50
            case 3: seq = 0b10011111; break; // 75
        }
        if ((seq >> (7 - pulse1.duty_value)) & 0x01) {
            pulse_out += pulse1.envelope.output;
        }
    }
    
    // Pulse 2 Output
    if (pulse2.enabled && pulse2.timer_period > 8 && !pulse2.sweep_mute && pulse2.length_counter.counter > 0) {
        uint8_t seq = 0;
        switch (pulse2.duty_mode) {
            case 0: seq = 0b01000000; break;
            case 1: seq = 0b01100000; break;
            case 2: seq = 0b01111000; break;
            case 3: seq = 0b10011111; break;
        }
        if ((seq >> (7 - pulse2.duty_value)) & 0x01) {
            pulse_out += pulse2.envelope.output;
        }
    }
    
    // Better approximation from NesDev wiki:
    double output = 0;
    if (pulse_out > 0) {
        output = 95.52 / (8128.0 / pulse_out + 100);
    }
    
    double tri_out = 0;
    double noise_out = 0;
    double dmc_out = 0; // DMC not implemented yet
    
    if (triangle.enabled && triangle.linear_counter > 0 && triangle.length_counter.counter > 0 && triangle.timer_period > 2) {
         uint8_t seq_val = triangle.sequence;
         if (seq_val > 15) seq_val = 31 - seq_val;
         tri_out = seq_val; // Use raw DAC value 0-15
    }
    
    if (noise.enabled && noise.length_counter.counter > 0 && !(noise.shift_register & 0x01)) {
         noise_out = noise.envelope.output; // Use raw DAC value 0-15
    }
    
    double tnd_out = 0;
    double tnd_denom = (tri_out / 8227.0) + (noise_out / 12241.0) + (dmc_out / 22638.0);
    
    if (tnd_denom > 0) {
        tnd_out = 159.79 / (1.0 / tnd_denom + 100);
    }
    
    return output + tnd_out;
}
