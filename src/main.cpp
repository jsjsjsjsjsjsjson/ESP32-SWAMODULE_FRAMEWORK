#include <Arduino.h>
#include <stdint.h>
#include "src_config.h"
#include "SerialTerminal.h"
#include "Adafruit_SSD1306.h"
#include "connect_manager.hpp"
#include "note_input.h"
#include "audio_out.h"
#include "key_event.h"
#include "simple_osc.h"

#include "Adafruit_MPR121.h"

Adafruit_MPR121 touchPad0;
Adafruit_MPR121 touchPad1;

ConnectionManager manager;

class TestModule: public Module_t {
public:
    TestModule() { module_info = {"noise generator", "libchara-dev", "A simple noise generator", false, false}; }
    int16_t out;

    uint16_t lfsr = 0xACE1u;
    unsigned period = 0;

    int16_t generate_noise() {
        unsigned lsb = lfsr & 1;
        lfsr >>= 1;
        if (lsb) {
            lfsr ^= 0xB400u;
        }
        return lfsr;
    }

    void start() {
        registerPort(&out, PORT_AOUT, "OUTPUT", "noise generator output");
        printf("NoiseStart\n");
    }
    void stop() {
        printf("NoiseStop\n");
    }
    void process() {
        out = generate_noise();
    }
    void customSettingPage() {

    }
    void customViewPage() {

    }
    ~TestModule() {
        printf("Releasing resources for %s\n", module_info.name);
    }
};

class VolCtrl: public Module_t {
public:
    VolCtrl() { module_info = {"volume control", "libchara-dev", "A simple volume control", false, false}; }
    int16_t out;
    int16_t in;
    void start() {
        registerPort(&out, PORT_AOUT, "OUTPUT", "volume control output");
        registerPort(&in, PORT_AIN, "INPUT", "volume control input");
        printf("VolCtrl Start\n");
    }
    void stop() {
        printf("VolCtrl Start\n");
    }
    void process() {
        out = in * 0.02;
    }
    void customSettingPage() {

    }
    void customViewPage() {

    }
};

void restartCmd(int argc, const char* argv[]) {
    printf("Rebooting...\n");
    ESP.restart();
}

void atkNoteCmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("%s <note>\n", argv[0]);return;}
    key_event_t noteEvent;
    noteEvent.num = strtol(argv[1], NULL, 0);
    noteEvent.status = KEY_ATTACK;
    if (xQueueSend(xNoteQueue, &noteEvent, portMAX_DELAY) == pdPASS) {
        printf("NOTE %d ATTACK\n", noteEvent.num);
    }
}

void rlsNoteCmd(int argc, const char* argv[]) {
    key_event_t noteEvent = {255, KEY_RELEASE};
    if (xQueueSend(xNoteQueue, &noteEvent, portMAX_DELAY) == pdPASS) {
        printf("NOTE RELEASE\n", noteEvent.num);
    }
}

void printSlotInfoCmd(int argc, const char* argv[]) {
    manager.printModuleInfo();
}

void printAllModInfoCmd(int argc, const char* argv[]) {
    manager.module_manager.printAllRegisteredModules();
}

void get_free_heap_cmd(int argc, const char* argv[]) {
    printf("Free heap size: %ld\n", esp_get_free_heap_size());
}

/*
void testProcessCmd(int argc, const char* argv[]) {
    printf("Test Process:\n");
    if (modules[0] == nullptr) {
        printf("FAILED\n");
    } else {
        modules[0]->process();
    }
}
*/

void serialDebug(void *arg) {
    SerialTerminal terminal;
    vTaskDelay(16);
    terminal.begin(115200, "ESP32MODULE");
    terminal.addCommand("reboot", restartCmd);
    terminal.addCommand("atkNote", atkNoteCmd);
    terminal.addCommand("rlsNote", rlsNoteCmd);
    terminal.addCommand("printSlotInfo", printSlotInfoCmd);
    terminal.addCommand("printAllModInfo", printAllModInfoCmd);
    terminal.addCommand("get_free_heap", get_free_heap_cmd);
    for (;;) {
        terminal.update();
        vTaskDelay(1);
    }
}

void soundEng(void *arg) {
    for (;;) {
        manager.process_all();
    }
}

void setup() {
    xNoteQueue = xQueueCreate(8, sizeof(key_event_t));
    manager.module_manager.registerModule<SimpleOsc>();
    manager.module_manager.registerModule<VolCtrl>();
    manager.module_manager.registerModule<i2s_audio_out>();
    manager.module_manager.registerModule<noteEventModule>();

    manager.createModule("simple osc");
    manager.createModule("volume control");
    manager.createModule("ESP32 I2S Audio Out");
    manager.createModule("Note Event");

    manager.connect(3, 1, 0, 0);
    manager.connect(3, 2, 0, 1);
    manager.connect(0, 0, 2, 0);

    manager.printModuleInfo();

    xTaskCreatePinnedToCore(serialDebug, "terminal", 4096, NULL, 2, NULL, 1);
    xTaskCreate(soundEng, "Sound Eng", 4096, NULL, 2, NULL);
}

void loop() {
    vTaskDelete(NULL);
}