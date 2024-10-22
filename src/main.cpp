#include <Arduino.h>
#include <stdint.h>
#include "src_config.h"
#include "SerialTerminal.h"
#include "Adafruit_SSD1306.h"
#include "connect_manager.hpp"
#include "audio_out.h"

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

/*
void loadModCmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("%s <Module name>\n", argv[0]);return;}
    modules[0] = manager.createModule(argv[1]);
}

void unloadModCmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("%s <Module name>\n", argv[0]);return;}
    manager.releaseModule(argv[1]);
}

void printModInfoCmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("%s <Module name>\n", argv[0]);return;}
    manager.printModuleInfo(argv[1]);
}
*/

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
    /*
    terminal.addCommand("loadMod", loadModCmd);
    terminal.addCommand("unloadMod", unloadModCmd);
    terminal.addCommand("testProcess", testProcessCmd);
    terminal.addCommand("printModInfo", printModInfoCmd);
    */
    terminal.addCommand("printAllModInfo", printAllModInfoCmd);
    terminal.addCommand("get_free_heap", get_free_heap_cmd);
    for (;;) {
        terminal.update();
        vTaskDelay(1);
    }
}

void setup() {
    manager.module_manager.registerModule<TestModule>();
    manager.module_manager.registerModule<VolCtrl>();
    manager.module_manager.registerModule<i2s_audio_out>();

    manager.createModule("noise generator");
    manager.createModule("volume control");
    manager.createModule("ESP32 I2S Audio Out");

    manager.connect(0, 0, 1, 0);
    manager.connect(1, 0, 2, 0);

    for (uint8_t m = 0; m < manager.getSlotSize(); m++) {
        module_info_t info = manager.modules[m]->module_info;
        printf("Module #%d Information:\n", m);
        printf("Name: %s\n", info.name);
        printf("Author: %s\n", info.author);
        printf("Profile: %s\n", info.profile);
        printf(" Input Port:\n");
        for (uint8_t p = 0; p < manager.getInputPortCount(m); p++) {
            port_t& info = manager.getInputPort(m, p);
            printf("  Input #%d\n", p);
            printf("   Name: %s\n", info.name);
            printf("   Profile: %s\n", info.profile);
            printf("   data: %d\n", *info.data);
        }
        printf(" Output Port:\n");
        for (uint8_t p = 0; p < manager.getOutputPortCount(m); p++) {
            port_t& info = manager.getOutputPort(m, p);
            printf("  Output #%d\n", p);
            printf("   Name: %s\n", info.name);
            printf("   Profile: %s\n", info.profile);
            printf("   data: %d\n", *info.data);
        }
        printf("\n");
    }

    vTaskDelay(128);

    while (1) {
        manager.process_all();
        // vTaskDelay(1);
    }

    xTaskCreate(serialDebug, "terminal", 4096, NULL, 2, NULL);
}

void loop() {
    vTaskDelete(NULL);
}