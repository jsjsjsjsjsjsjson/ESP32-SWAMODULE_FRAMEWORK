#ifndef NOTE_INPUT_H
#define NOTE_INPUT_H

#include "module_manager.hpp"
#include <freertos/queue.h>
#include "key_event.h"

const float midi2freq_float[128] = {
    8.1757989156f,
    8.6619572180f,
    9.1770239974f,
    9.7227182413f,
    10.3008611535f,
    10.9133822323f,
    11.5623257097f,
    12.2498573744f,
    12.9782717994f,
    13.7500000000f,
    14.5676175474f,
    15.4338531643f,
    16.3515978313f,
    17.3239144361f,
    18.3540479948f,
    19.4454364826f,
    20.6017223071f,
    21.8267644646f,
    23.1246514195f,
    24.4997147489f,
    25.9565435987f,
    27.5000000000f,
    29.1352350949f,
    30.8677063285f,
    32.7031956626f,
    34.6478288721f,
    36.7080959897f,
    38.8908729653f,
    41.2034446141f,
    43.6535289291f,
    46.2493028390f,
    48.9994294977f,
    51.9130871975f,
    55.0000000000f,
    58.2704701898f,
    61.7354126570f,
    65.4063913251f,
    69.2956577442f,
    73.4161919794f,
    77.7817459305f,
    82.4068892282f,
    87.3070578583f,
    92.4986056779f,
    97.9988589954f,
    103.8261743950f,
    110.0000000000f,
    116.5409403795f,
    123.4708253140f,
    130.8127826503f,
    138.5913154884f,
    146.8323839587f,
    155.5634918610f,
    164.8137784564f,
    174.6141157165f,
    184.9972113558f,
    195.9977179909f,
    207.6523487900f,
    220.0000000000f,
    233.0818807590f,
    246.9416506281f,
    261.6255653006f,
    277.1826309769f,
    293.6647679174f,
    311.1269837221f,
    329.6275569129f,
    349.2282314330f,
    369.9944227116f,
    391.9954359817f,
    415.3046975799f,
    440.0000000000f,
    466.1637615181f,
    493.8833012561f,
    523.2511306012f,
    554.3652619537f,
    587.3295358348f,
    622.2539674442f,
    659.2551138257f,
    698.4564628660f,
    739.9888454233f,
    783.9908719635f,
    830.6093951599f,
    880.0000000000f,
    932.3275230362f,
    987.7666025122f,
    1046.5022612024f,
    1108.7305239075f,
    1174.6590716696f,
    1244.5079348883f,
    1318.5102276515f,
    1396.9129257320f,
    1479.9776908465f,
    1567.9817439270f,
    1661.2187903198f,
    1760.0000000000f,
    1864.6550460724f,
    1975.5332050245f,
    2093.0045224048f,
    2217.4610478150f,
    2349.3181433393f,
    2489.0158697766f,
    2637.0204553030f,
    2793.8258514640f,
    2959.9553816931f,
    3135.9634878540f,
    3322.4375806396f,
    3520.0000000000f,
    3729.3100921447f,
    3951.0664100490f,
    4186.0090448096f,
    4434.9220956300f,
    4698.6362866785f,
    4978.0317395533f,
    5274.0409106059f,
    5587.6517029281f,
    5919.9107633862f,
    6271.9269757080f,
    6644.8751612791f,
    7040.0000000000f,
    7458.6201842894f,
    7902.1328200980f,
    8372.0180896192f,
    8869.8441912599f,
    9397.2725733570f,
    9956.0634791066f,
    10548.0818212118f,
    11175.3034058561f,
    11839.8215267723f,
    12543.8539514160f
};

const int16_t midi2freq_int[128] = {
    8,
    9,
    9,
    10,
    10,
    11,
    12,
    12,
    13,
    14,
    15,
    15,
    16,
    17,
    18,
    19,
    21,
    22,
    23,
    24,
    26,
    28,
    29,
    31,
    33,
    35,
    37,
    39,
    41,
    44,
    46,
    49,
    52,
    55,
    58,
    62,
    65,
    69,
    73,
    78,
    82,
    87,
    92,
    98,
    104,
    110,
    117,
    123,
    131,
    139,
    147,
    156,
    165,
    175,
    185,
    196,
    208,
    220,
    233,
    247,
    262,
    277,
    294,
    311,
    330,
    349,
    370,
    392,
    415,
    440,
    466,
    494,
    523,
    554,
    587,
    622,
    659,
    698,
    740,
    784,
    831,
    880,
    932,
    988,
    1047,
    1109,
    1175,
    1245,
    1319,
    1397,
    1480,
    1568,
    1661,
    1760,
    1865,
    1976,
    2093,
    2217,
    2349,
    2489,
    2637,
    2794,
    2960,
    3136,
    3322,
    3520,
    3729,
    3951,
    4186,
    4435,
    4699,
    4978,
    5274,
    5588,
    5920,
    6272,
    6645,
    7040,
    7459,
    7902,
    8372,
    8870,
    9397,
    9956,
    10548,
    11175,
    11840,
    12544
};

class noteEventModule: public Module_t {
public:

    noteEventModule() { module_info = {"Note Event", "libchara-dev", "A special module that outputs note event, frequency, and on/off status. (FreeRTOS only)", false, false}; }

    int16_t note = 0;
    int16_t freq = 0;   
    int16_t status = false;
    uint16_t timer = 0;
    key_event_t noteEvent;

    void start() {
        registerPort(&note, PORT_DOUT, "NOTE", "note event output");
        registerPort(&freq, PORT_AOUT, "FREQ", "frequency output");
        registerPort(&status, PORT_DOUT, "STATUS", "Note status (bool)");
    }
    void stop() {
        printf("Note Event Stop\n");
    }
    void process() {
        if (!(timer & 15)) {
            if (readNoteEvent == pdTRUE) {
                if (noteEvent.status == KEY_ATTACK) {
                    status = true;
                } else if (noteEvent.status == KEY_RELEASE) {
                    status = false;
                }
                note = noteEvent.num;
                freq = midi2freq_int[note];
            }
            
        }
        timer++;
    }
    void customSettingPage() {

    }
    void customViewPage() {

    }
};

#endif