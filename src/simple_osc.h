#ifndef SIMPLE_OSC_H
#define SIMPLE_OSC_H

#include "module_manager.hpp"
#include "src_config.h"
#include <math.h>

const int8_t wave_table[7][32] = {
    // WAVE_P125
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7},

    // WAVE_P025
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7},

    // WAVE_P050
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},

    // WAVE_P075
    {-8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},

    // WAVE_TRIG
    // {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8},
    {0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8, -8, -7, -6, -5, -4, -3, -2, -1},

    // WAVE_SINE
    {0, 1, 2, 4, 5, 6, 6, 7, 7, 7, 6, 6, 5, 4, 2, 1, -1, -2, -3, -5, -6, -7, -7, -8, -8, -8, -7, -7, -6, -5, -3, -2},

    // WAVE_SAWT
    {7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8}
};

class SimpleOsc: public Module_t {
public:
    SimpleOsc() { module_info = {"simple osc", "libchara-dev", "A simple wavetable oscillator module.", false, false}; }
    float wave_t_c = 32.0f / SMP_RATE;

    int16_t out = 0;
    int16_t freq = 0;
    int16_t gate = 0;
    int wave = 4;

    float wave_time = 0;

    void start() {
        registerPort(&freq, PORT_AIN, "FREQ IN", "frequency input");
        registerPort(&gate, PORT_DIN, "GATE", "gate");
        registerPort(&out, PORT_AOUT, "OUTPUT", "signal output");
        registerParam(&wave, PARAM_INT, "Wave type", "wavetable");
        printf("SimpleOsc Start\n");
    }
    void stop() {
        printf("SimpleOsc Start\n");
    }
    void process() {
        if (gate) {
            wave_time += wave_t_c * freq;
            if (wave_time >= 32) {
                wave_time -= 32;
            }
            out = wave_table[wave][(int)roundf(wave_time)] * 2048;
        } else {
            out = 0;
        }
    }
    void customSettingPage() {

    }
    void customViewPage() {

    }
};

#endif