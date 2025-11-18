#pragma once
#include <cstdint>
#include <functional>

class APU {
public:
    APU();
    ~APU();

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void clock();
    void reset();

    double GetOutputSample();

private:
    uint32_t frame_clock_counter = 0;
    uint32_t clock_counter = 0;

    // Length Counter Lookup Table
    uint8_t length_table[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

    struct Sequencer {
        uint32_t sequence = 0;
        uint16_t timer = 0;
        uint16_t reload = 0;
        uint8_t output = 0;
        
        uint8_t clock(bool enable, std::function<void(uint32_t&)> func) {
            if (enable) {
                timer--;
                if (timer == 0xFFFF) {
                    timer = reload;
                    func(sequence);
                    output = sequence & 1;
                }
            }
            return output;
        }
    };

    struct LengthCounter {
        uint8_t counter = 0;
        bool halt = false;
        void clock(bool enable, bool halt) {
            if (!enable) counter = 0;
            else if (counter > 0 && !halt) counter--;
        }
    };
    
    struct Envelope {
        bool start = false;
        bool disable = false;
        uint16_t divider_count = 0;
        uint16_t volume = 0;
        uint16_t output = 0;
        uint16_t decay_count = 0;
        
        void clock(bool loop) {
            if (!start) {
                if (divider_count == 0) {
                    divider_count = volume;
                    if (decay_count == 0) {
                        if (loop) decay_count = 15;
                    } else {
                        decay_count--;
                    }
                } else {
                    divider_count--;
                }
            } else {
                start = false;
                decay_count = 15;
                divider_count = volume;
            }
            
            if (disable) output = volume;
            else output = decay_count;
        }
    };

    struct Pulse {
        bool enabled = false;
        Pulse() {
             // Default duty sequences
             // 12.5%, 25%, 50%, 75%
        }
        
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint8_t duty_mode = 0;
        uint8_t duty_value = 0;
        
        LengthCounter length_counter;
        Envelope envelope;
        
        // Sweep
        bool sweep_enable = false;
        bool sweep_down = false;
        uint8_t sweep_period = 0;
        uint8_t sweep_shift = 0;
        uint8_t sweep_timer = 0;
        bool sweep_reload = false;
        uint16_t target_period = 0;
        bool sweep_mute = false;

        void clock_sweep(int channel) { // 0 for Pulse1 (ones complement), 1 for Pulse2 (twos complement)
             uint16_t change = timer_period >> sweep_shift;
             if (sweep_down) {
                 target_period = timer_period - change - (channel ? 0 : 1); // Pulse 1 adds 1 for negate
             } else {
                 target_period = timer_period + change;
             }
             
             if (target_period > 0x7FF || timer_period < 8) {
                 sweep_mute = true;
             } else {
                 sweep_mute = false;
             }
             
             if (sweep_timer == 0) {
                 if (sweep_enable && !sweep_mute) {
                     timer_period = target_period;
                 }
                 sweep_timer = sweep_period;
             } else {
                 sweep_timer--;
             }
             
             if (sweep_reload) {
                 sweep_timer = sweep_period;
                 sweep_reload = false;
             }
        }
        
        uint8_t output = 0;
    } pulse1, pulse2;
    
    struct Triangle {
        bool enabled = false;
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint8_t sequence = 0;
        LengthCounter length_counter;
        
        uint8_t linear_counter_reload = 0;
        uint8_t linear_counter = 0;
        bool linear_counter_reload_flag = false;
        bool control_flag = false;
        
        uint8_t output = 0;
    } triangle;
    
    struct Noise {
        bool enabled = false;
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint16_t shift_register = 1;
        bool mode = false; // 0 = 32767 steps, 1 = 93 steps
        
        LengthCounter length_counter;
        Envelope envelope;
        
        uint8_t output = 0;
    } noise;

    // Global Control
    // 0: 4-step sequence (mode 0)
    // 1: 5-step sequence (mode 1)
    uint8_t frame_counter_mode = 0;
    bool irq_inhibit = false;
    
    // Audio Output
    // Pulse 1, Pulse 2, Triangle, Noise, DMC
};
