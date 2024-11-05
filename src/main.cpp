#include <Arduino.h>
#include <stdint.h>
#include "src_config.h"
#include "SerialTerminal.h"
#include "Adafruit_SSD1306.h"
#include "connect_manager.hpp"
// #include "note_input.h"
#include "audio_out.h"
#include "simple_osc.h"

#include "WindowManager.h"

#include "font3x5.h"

#include "font4x5.h"

#include <Adafruit_Keypad.h>

#include "Adafruit_MPR121.h"

#include "Adafruit_SSD1322.h"

#define ROWS 4 // rows
#define COLS 3 // columns

typedef enum {
    KEY_L,
    KEY_OK,
    KEY_MENU,
    KEY_UP,
    KEY_S,
    KEY_NAVI,
    KEY_R,
    KEY_BACK,
    KEY_OCTD,
    KEY_DOWN,
    KEY_P,
    KEY_OCTU
} key_define_t;

const uint8_t key_map[ROWS][COLS] = {
  {KEY_L,KEY_OK,KEY_MENU},
  {KEY_UP,KEY_S,KEY_NAVI},
  {KEY_R,KEY_BACK,KEY_OCTD},
  {KEY_DOWN,KEY_P,KEY_OCTU}
};

const uint8_t rowPins[ROWS] = {47, 18, 45, 46};
const uint8_t colPins[COLS] = {38, 39, 48};

Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(key_map), (uint8_t*)rowPins, (uint8_t*)colPins, ROWS, COLS);

Adafruit_SSD1306 display(128, 64, &SPI, 7, 15, 6, 10000000);

Adafruit_MPR121 touchPad0;
Adafruit_MPR121 touchPad1;

WindowManager window_manager(&display);
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
*/
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
    /*
    terminal.addCommand("atkNote", atkNoteCmd);
    terminal.addCommand("rlsNote", rlsNoteCmd);
    */
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
        vTaskDelay(1);
    }
}

void refreshDisplay(void *arg) {
    for (;;) {
        window_manager.display_all();
        vTaskDelay(REFRESH_DELAY_MS);
    }
}

void GUI(void *arg) {
    window_manager.setShowFps(true);
    Window* backgroundWindow = window_manager.registerWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, FIXED_BOTTOM_WINDOW, false, NO_DITHERING);
    backgroundWindow->fillScreen(0);
    backgroundWindow->fillRect(0, 0, 128, 6, 1);
    backgroundWindow->display();
    vTaskDelay(1024);
    window_manager.unregisterWindow(backgroundWindow);
    for (;;) {
        vTaskDelay(32);
    }
}

void setup() {
    // esp_restart();
    SPI.begin(17, -1, 16);
    SPI.setFrequency(60000000);
    display.begin(SSD1306_SWITCHCAPVCC);
    // xNoteQueue = xQueueCreate(8, sizeof(key_event_t));
    manager.module_manager.registerModule<SimpleOsc>();
    manager.module_manager.registerModule<VolCtrl>();
    manager.module_manager.registerModule<i2s_audio_out>();
    // manager.module_manager.registerModule<noteEventModule>();

    // display.printf("INIT...\n");
    // display.display();
    printf("PRINT F\n");
    manager.printModuleInfo();
    printf("PRINT B\n");

    xTaskCreatePinnedToCore(serialDebug, "terminal", 4096, NULL, 3, NULL, 1);
    printf("Terminal Created\n");
    xTaskCreate(soundEng, "Sound Eng", 4096, NULL, 2, NULL);
    printf("Sound Eng Created\n");
    xTaskCreatePinnedToCore(refreshDisplay, "Display", 2048, NULL, 3, NULL, 1);
    xTaskCreate(GUI, "GUI", 10240, NULL, 3, NULL);
    printf("GUI Created\nSetup return\n");
}

void loop() {
    vTaskDelete(NULL);
}