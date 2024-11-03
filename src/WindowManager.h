#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <FreeRTOS.h>
#include <queue>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define QUEUE_LENGTH 10  // 每个窗口按键事件队列长度

// 4x4 Bayer矩阵用于有序抖动
const uint8_t bayerMatrix[4][4] = {
    {  0, 128,  32, 160 },
    { 192,  64, 224,  96 },
    {  48, 176,  16, 144 },
    { 240, 112, 208,  80 }
};

// 抖动类型
enum DitheringType {
    NO_DITHERING,        // 不启用抖动
    ORDERED_DITHERING,   // 有序抖动（Bayer矩阵）
    ERROR_DIFFUSION      // 误差扩散抖动（Floyd-Steinberg）
};

// 窗口类型枚举
enum WindowType {
    NORMAL_WINDOW,        // 普通窗口
    POPUP_WINDOW,         // 弹窗，始终保持在最前级
    MODAL_WINDOW,         // 模态窗口，禁用其他窗口的交互
    FLOATING_WINDOW,      // 浮动窗口，保持在前面，但不遮挡弹窗和模态窗口
    FIXED_BOTTOM_WINDOW   // 固定底部窗口，始终在最底层
};

// 按键事件状态枚举
typedef enum {
    KEY_IDLE,
    KEY_ATTACK,
    KEY_RELEASE
} key_status_t;

// 按键事件结构
typedef struct {
    uint8_t num;         // 按键编号
    key_status_t status; // 按键状态
} key_event_t;

// 窗口类
class Window : public Adafruit_GFX {
public:
    Window(uint16_t w, uint16_t h, int16_t x = 0, int16_t y = 0, WindowType type = NORMAL_WINDOW, bool hasBorder = false, DitheringType dithering = NO_DITHERING) 
        : Adafruit_GFX(w, h), posX(x), posY(y), windowType(type), bufferSize(w * h), hasBorder(hasBorder), ditheringType(dithering) {
        drawBuffer = new uint8_t[bufferSize]();     // 绘图缓冲区（灰度 0–255）
        displayBuffer = new uint8_t[bufferSize]();  // 显示缓冲区（灰度 0–255）
        keyEventQueue = xQueueCreate(QUEUE_LENGTH, sizeof(key_event_t));  // 创建独立的按键事件队列
    }

    ~Window() {
        delete[] drawBuffer;
        delete[] displayBuffer;
        vQueueDelete(keyEventQueue);  // 删除按键事件队列
    }

    // 提交显示，将绘图缓冲区的内容复制到显示缓冲区
    void display() {
        std::memcpy(displayBuffer, drawBuffer, bufferSize);
    }

    // 设置窗口的位置
    void setPosition(int16_t x, int16_t y) {
        posX = x;
        posY = y;
    }

    // 设置窗口大小
    void setSize(uint16_t w, uint16_t h) {
        if (w * h > bufferSize) {
            delete[] drawBuffer;
            delete[] displayBuffer;
            bufferSize = w * h;
            drawBuffer = new uint8_t[bufferSize]();
            displayBuffer = new uint8_t[bufferSize]();
        }
        _width = w;
        _height = h;
    }

    // 获取显示缓冲区
    uint8_t* getDisplayBuffer() const {
        return displayBuffer;
    }

    // 获取窗口宽度
    uint16_t getWidth() const {
        return _width;
    }

    // 获取窗口高度
    uint16_t getHeight() const {
        return _height;
    }

    // 获取窗口在屏幕上的X坐标
    int16_t getPosX() const {
        return posX;
    }

    // 获取窗口在屏幕上的Y坐标
    int16_t getPosY() const {
        return posY;
    }

    // 获取窗口类型
    WindowType getWindowType() const {
        return windowType;
    }

    // 获取当前的抖动类型
    DitheringType getDitheringType() const {
        return ditheringType;
    }

    // 设置窗口的抖动类型
    void setDitheringType(DitheringType type) {
        ditheringType = type;
    }

    // 是否启用边框
    bool hasBorderEnabled() const {
        return hasBorder;
    }

    // 获取按键事件
    key_event_t getKeyEvent() {
        key_event_t event;
        static const key_event_t idleEvent = {0, KEY_IDLE};
        if (xQueueReceive(keyEventQueue, &event, 0) == pdPASS) {
            return event;
        }
        return idleEvent;  // 返回空闲状态
    }

    // 将按键事件推送到窗口的事件队列
    void pushKeyEvent(const key_event_t& event) {
        xQueueSend(keyEventQueue, &event, portMAX_DELAY);
    }

    // 绘制像素方法，支持灰度值，写入绘图缓冲区
    inline void drawPixel(int16_t x, int16_t y, uint16_t color) override __attribute__((always_inline)) {
        if (x >= 0 && x < width() && y >= 0 && y < height()) {
            drawBuffer[x + y * width()] = color;
        }
    }

private:
    uint8_t* drawBuffer;       // 绘图缓冲区（灰度 0–255）
    uint8_t* displayBuffer;    // 显示缓冲区（灰度 0–255）
    size_t bufferSize;         // 缓冲区大小
    int16_t posX;              // 窗口的 X 坐标
    int16_t posY;              // 窗口的 Y 坐标
    WindowType windowType;     // 窗口类型
    bool hasBorder;            // 是否有边框
    DitheringType ditheringType; // 抖动类型
    QueueHandle_t keyEventQueue; // 独立的按键事件队列
};

// 窗口管理器类
class WindowManager {
public:
    WindowManager(Adafruit_SSD1306* display)
        : display(display), ditheringType(ORDERED_DITHERING) {
        covered.resize(SCREEN_WIDTH, std::vector<bool>(SCREEN_HEIGHT, false));
    }

    ~WindowManager() {
        for (auto window : windows) {
            delete window;
        }
    }

    // 创建并注册一个新的窗口
    Window* registerWindow(uint16_t w, uint16_t h, WindowType type = NORMAL_WINDOW, bool hasBorder = false, DitheringType dithering = NO_DITHERING) {
        int16_t x = 0;
        int16_t y = 0;

        if (!windows.empty()) {
            Window* lastWindow = windows.back();
            x = lastWindow->getPosX() + 1;
            y = lastWindow->getPosY() + 1;
            if (x + w > SCREEN_WIDTH) x = 0;
            if (y + h > SCREEN_HEIGHT) y = 0;
        }

        Window* window = new Window(w, h, x, y, type, hasBorder, dithering);
        int insertPos = getWindowInsertionIndex(type);
        windows.insert(windows.begin() + insertPos, window);
        updateCoveredAreas();
        return window;
    }

    // 注销并销毁指定的窗口
    void unregisterWindow(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) {
            windows.erase(it);
            delete window;
            updateCoveredAreas(); 
        }
    }

    void bringToFront(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) {
            windows.erase(it);
            windows.push_back(window);
            enforceFixedLayerOrder();
            updateCoveredAreas();
        }
    }

    void sendToBack(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) {
            windows.erase(it);
            windows.insert(windows.begin(), window);
            enforceFixedLayerOrder();
            updateCoveredAreas();
        }
    }

    void moveUp(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end() && it + 1 != windows.end() && window->getWindowType() != FIXED_BOTTOM_WINDOW) {
            std::iter_swap(it, it + 1);
            enforceFixedLayerOrder();
            updateCoveredAreas();
        }
    }

    void moveDown(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.begin() && window->getWindowType() != MODAL_WINDOW) {
            std::iter_swap(it, it - 1);
            enforceFixedLayerOrder();
            updateCoveredAreas();
        }
    }

    void pushKeyEvent(const key_event_t& event) {
        if (!windows.empty()) {
            windows.back()->pushKeyEvent(event);
        }
    }

    void display_all() {
        // 重置覆盖矩阵
        for (auto& row : covered) {
            std::fill(row.begin(), row.end(), false);
        }

        // 绘制普通窗口内容
        for (auto window : windows) {
            if (window->getWindowType() == MODAL_WINDOW || window->getWindowType() == POPUP_WINDOW) {
                continue;
            }
            renderWindowContent(window);
        }

        // 检查是否存在模态窗口或弹窗
        bool hasModalOrPopup = std::any_of(windows.begin(), windows.end(), [](Window* w) {
            return w->getWindowType() == MODAL_WINDOW || w->getWindowType() == POPUP_WINDOW;
        });

        // 如果存在模态窗口或弹窗，在空白区域上绘制棋盘格遮罩
        if (hasModalOrPopup) {
            drawCheckerboardMask();
        }

        // 绘制弹窗和模态窗口
        for (auto window : windows) {
            if (window->getWindowType() == MODAL_WINDOW || window->getWindowType() == POPUP_WINDOW) {
                renderWindowContent(window);
            }
        }

        display->display();
    }

    int getForegroundWindowIndex() const {
        for (int i = windows.size() - 1; i >= 0; --i) {
            if (windows[i]->getWindowType() == MODAL_WINDOW || windows[i]->getWindowType() == POPUP_WINDOW) {
                return i;
            }
        }
        return windows.size();
    }

private:
    Adafruit_SSD1306* display;
    std::vector<Window*> windows;
    DitheringType ditheringType;
    std::vector<std::vector<bool>> covered;

    void updateCoveredAreas() {
        for (auto& row : covered) {
            std::fill(row.begin(), row.end(), false);
        }
        for (const auto& window : windows) {
            int16_t x = window->getPosX();
            int16_t y = window->getPosY();
            uint16_t w = window->getWidth();
            uint16_t h = window->getHeight();
            for (int16_t i = x; i < x + w && i < SCREEN_WIDTH; i++) {
                for (int16_t j = y; j < y + h && j < SCREEN_HEIGHT; j++) {
                    covered[i][j] = true;
                }
            }
        }
    }

    void enforceFixedLayerOrder() {
        auto fixedBottomEnd = std::stable_partition(windows.begin(), windows.end(), [](Window* w) {
            return w->getWindowType() == FIXED_BOTTOM_WINDOW;
        });

        std::stable_partition(fixedBottomEnd, windows.end(), [](Window* w) {
            return w->getWindowType() != MODAL_WINDOW;
        });
    }

    int getWindowInsertionIndex(WindowType type) const {
        switch (type) {
            case FIXED_BOTTOM_WINDOW:
                return 0;
            case NORMAL_WINDOW:
            case FLOATING_WINDOW:
                return getForegroundWindowIndex();
            case MODAL_WINDOW:
            case POPUP_WINDOW:
                return windows.size();
        }
        return windows.size();  // 默认插入到最后
    }

    void drawWindowBorder(int16_t x, int16_t y, uint16_t w, uint16_t h) {
        display->drawRect(x - 1, y - 1, w + 2, h + 2, SSD1306_WHITE);
    }

    // 绘制棋盘格遮罩，只在非覆盖区域上绘制
    void drawCheckerboardMask() {
        for (int16_t i = 0; i < SCREEN_WIDTH; i++) {
            for (int16_t j = 0; j < SCREEN_HEIGHT; j++) {
                if (!covered[i][j] && ((i + j) & 1) == 0) { 
                    display->drawPixel(i, j, SSD1306_BLACK);
                }
            }
        }
    }

    void renderWindowContent(Window* window) {
        uint8_t* buffer = window->getDisplayBuffer();
        int16_t posX = window->getPosX();
        int16_t posY = window->getPosY();
        uint16_t width = window->getWidth();
        uint16_t height = window->getHeight();

        if (window->hasBorderEnabled()) {
            drawWindowBorder(posX, posY, width, height);
        }

        DitheringType dithering = window->getDitheringType();
        bool orderedDithering = (dithering == ORDERED_DITHERING);

        for (int16_t y = 0; y < height; y++) {
            for (int16_t x = 0; x < width; x++) {
                int16_t screenX = posX + x;
                int16_t screenY = posY + y;

                if (screenX >= SCREEN_WIDTH || screenY >= SCREEN_HEIGHT || covered[screenX][screenY]) {
                    continue;
                }

                uint8_t pixelValue = buffer[x + y * width];
                bool isWhite;

                if (dithering != NO_DITHERING) {  // 如果启用了抖动
                    if (orderedDithering) {  // 有序抖动
                        int ditherThreshold = bayerMatrix[y & 3][x & 3];  // y % 4 -> y & 3, x % 4 -> x & 3
                        isWhite = pixelValue > ditherThreshold;
                    } else if (dithering == ERROR_DIFFUSION) {  // 误差扩散抖动
                        isWhite = pixelValue > 127;
                        int error = pixelValue - (isWhite ? 255 : 0);

                        if (x + 1 < width)                buffer[(x + 1) + y * width] += (error * 7) >> 4;
                        if (y + 1 < height) {
                            if (x > 0)                   buffer[(x - 1) + (y + 1) * width] += (error * 3) >> 4;
                                                        buffer[x + (y + 1) * width] += (error * 5) >> 4;
                            if (x + 1 < width)           buffer[(x + 1) + (y + 1) * width] += error >> 4;
                        }
                    }
                } else {  // 不启用抖动
                    isWhite = pixelValue;
                }

                display->drawPixel(screenX, screenY, isWhite ? SSD1306_WHITE : SSD1306_BLACK);
            }
        }
    }
};
