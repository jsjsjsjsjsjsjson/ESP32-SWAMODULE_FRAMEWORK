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

// 随机绘制函数图像
void drawFunction(Window* window, int functionType) {
    window->fillScreen(0);  // 清空窗口内容

    for (int x = 0; x < window->getWidth(); x++) {
        float scaledX = static_cast<float>(x) / window->getWidth() * 4 * M_PI;  // 将 x 坐标缩放到 [0, 4π]
        int y = 0;

        // 选择函数类型并计算 y 坐标
        switch (functionType) {
            case 0:  // log2(x)
                y = window->getHeight() - 1 - static_cast<int>(log2(scaledX + 1) * 10);  // 加 1 避免 log2(0)
                break;
            case 1:  // sin(x)
                y = window->getHeight() / 2 - static_cast<int>(sin(scaledX) * (window->getHeight() / 2));
                break;
            case 2:  // cos(x)
                y = window->getHeight() / 2 - static_cast<int>(cos(scaledX) * (window->getHeight() / 2));
                break;
            case 3:  // tan(x)
                y = window->getHeight() / 2 - static_cast<int>(tan(scaledX) * (window->getHeight() / 4));
                break;
        }

        // 限制 y 坐标范围，避免绘图超出边界
        if (y >= 0 && y < window->getHeight()) {
            window->drawPixel(x, y, SSD1306_WHITE);
        }
    }

    window->display();
}

void runDemoTask(void* pvParameters) {
    Window* backgroundWindow = window_manager.registerWindow(SCREEN_WIDTH, SCREEN_HEIGHT, FIXED_BOTTOM_WINDOW, false, NO_DITHERING);

    // 创建浮动窗口（动态移动和缩放）
    Window* dynamicWindow = window_manager.registerWindow(30, 20, FLOATING_WINDOW, true, NO_DITHERING);
    dynamicWindow->setPosition(0, 0);

    // 创建位于最顶层的灰度测试窗口，放置在屏幕左下角
    Window* grayscaleTestWindow = window_manager.registerWindow(50, 30, FLOATING_WINDOW, true, ERROR_DIFFUSION); 
    grayscaleTestWindow->setPosition(0, SCREEN_HEIGHT - 30); // 初始位置在左下角

    // 创建沿正弦曲线移动的“Hello, World!”窗口
    Window* sineWaveWindow = window_manager.registerWindow(80, 20, NORMAL_WINDOW, true, NO_DITHERING);
    sineWaveWindow->setPosition(0, 10);

    window_manager.bringToFront(grayscaleTestWindow);

    int16_t dx = 2, dy = 1, x = 0, y = 0;
    uint16_t w = 30, h = 20;
    uint8_t brightness = 0;
    int8_t delta = 5;
    float angle = 0.0;
    int16_t backgroundOffset = 0;
    int currentFunction = 0;

    while (true) {
        // 1. 更新背景窗口内容，创建移动的图案效果
        backgroundWindow->fillScreen(0);  // 清空背景窗口
        for (int16_t i = 0; i < SCREEN_WIDTH; i += 16) {
            backgroundWindow->drawPixel((i + backgroundOffset) % SCREEN_WIDTH, SCREEN_HEIGHT / 2, SSD1306_WHITE);
        }
        backgroundOffset = (backgroundOffset + 1) % SCREEN_WIDTH;
        backgroundWindow->display();

        // 2. 更新浮动窗口位置和大小，确保不超出屏幕范围
        x += dx;
        y += dy;

        bool hitEdge = false;

        if (x + w > SCREEN_WIDTH || x < 0) {
            dx = -dx;
            x = std::max<int16_t>(0, std::min<int16_t>(x, SCREEN_WIDTH - w));
            hitEdge = true;
        }
        if (y + h > SCREEN_HEIGHT || y < 0) {
            dy = -dy;
            y = std::max<int16_t>(0, std::min<int16_t>(y, SCREEN_HEIGHT - h));
            hitEdge = true;
        }

        if (hitEdge) {
            // 每次碰到边缘，随机选择一个函数来绘制
            currentFunction = rand() & 3;  // 随机选择 0 到 3 之间的一个数
            drawFunction(dynamicWindow, currentFunction);

            // 每次碰到边缘，随机改变窗口层级
            int randomLayer = rand() % window_manager.getForegroundWindowIndex();
            window_manager.sendToBack(dynamicWindow); // 重置到最底层
            for (int i = 0; i < randomLayer; i++) {
                window_manager.moveUp(dynamicWindow);  // 移动到随机层级
            }
        } else {
            dynamicWindow->setPosition(x, y);
            dynamicWindow->setSize(w, h);
            dynamicWindow->display();
        }

        // 3. 更新灰度测试窗口的颜色和大小变化
        brightness += delta;
        if (brightness >= 255 || brightness <= 0) delta = -delta;
        
        // 计算灰度测试窗口大小的正弦变化
        uint16_t newWidth = 30 + 10 * sin(angle);
        uint16_t newHeight = 20 + 5 * sin(angle * 2);
        grayscaleTestWindow->setSize(newWidth, newHeight);
        
        // 更新位置，以保持在屏幕左下角
        grayscaleTestWindow->setPosition(0, SCREEN_HEIGHT - newHeight);
        
        // 填充灰度颜色
        grayscaleTestWindow->fillScreen(brightness);
        grayscaleTestWindow->display();

        angle += 0.1;
        if (angle > 2 * M_PI) angle -= 2 * M_PI;

        // 4. 更新正弦曲线窗口位置
        int16_t sineY = 20 + 10 * sin(angle);
        sineWaveWindow->setPosition((int16_t)(SCREEN_WIDTH / 2 - 40), sineY);
        sineWaveWindow->fillScreen(0);  // 清空窗口内容
        sineWaveWindow->setCursor(5, 5);
        sineWaveWindow->setTextSize(1);
        sineWaveWindow->setTextColor(SSD1306_WHITE);
        sineWaveWindow->print("Hello, World!");
        sineWaveWindow->display();

        // 延时更新
        vTaskDelay(30);
    }
}

void GUI(void *arg) {
    for (;;) {

    }
}

void setup() {
    // esp_restart();
    SPI.begin(17, -1, 16);
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
    xTaskCreate(runDemoTask, "GUI", 10240, NULL, 3, NULL);
    printf("GUI Created\nSetup return\n");
}

void loop() {
    vTaskDelete(NULL);
}