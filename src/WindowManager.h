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
    SemaphoreHandle_t bufferMutex;
    Window(uint16_t w, uint16_t h, int16_t x = 0, int16_t y = 0, WindowType type = NORMAL_WINDOW, bool hasBorder = false, DitheringType dithering = NO_DITHERING) 
        : Adafruit_GFX(w, h), posX(x), posY(y), windowType(type), bufferSize(w * h), hasBorder(hasBorder), ditheringType(dithering) {
        drawBuffer = new uint8_t[bufferSize]();     // 绘图缓冲区（灰度 0–255）
        displayBuffer = new uint8_t[bufferSize]();  // 显示缓冲区（灰度 0–255）
        keyEventQueue = xQueueCreate(QUEUE_LENGTH, sizeof(key_event_t));
        bufferMutex = xSemaphoreCreateMutex();
    }

    ~Window() {
        delete[] drawBuffer;
        delete[] displayBuffer;
        vQueueDelete(keyEventQueue);
        vSemaphoreDelete(bufferMutex);
    }

    // 提交显示
    void display() {
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE) {
            std::memcpy(displayBuffer, drawBuffer, bufferSize);
            xSemaphoreGive(bufferMutex);
        }
    }

    // 获取显示缓冲区
    uint8_t* getDisplayBuffer() const {
        return displayBuffer;
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
        return idleEvent;
    }

    // 将按键事件推送到窗口的事件队列
    void pushKeyEvent(const key_event_t& event) {
        xQueueSend(keyEventQueue, &event, portMAX_DELAY);
    }

    // drawPixel 方法
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
    QueueHandle_t keyEventQueue; // 按键事件队列
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
    Window* registerWindow(uint16_t w, uint16_t h, int16_t initialX = -1, int16_t initialY = -1, WindowType type = NORMAL_WINDOW, bool hasBorder = false, DitheringType dithering = NO_DITHERING) {
        int16_t x = initialX;
        int16_t y = initialY;

        if (x == -1 || y == -1) {
            if (!windows.empty()) {
                Window* lastWindow = windows.back();
                x = lastWindow->getPosX() + 1;
                y = lastWindow->getPosY() + 1;
                if (x + w > SCREEN_WIDTH) x = 0;
                if (y + h > SCREEN_HEIGHT) y = 0;
            } else {
                x = (x == -1) ? 0 : x;
                y = (y == -1) ? 0 : y;
            }
        }

        // 创建窗口
        Window* window = new Window(w, h, x, y, type, hasBorder, dithering);
        int insertPos = getWindowInsertionIndex(type);
        windows.insert(windows.begin() + insertPos, window);
        
        startAnimation(window, true);
        
        updateCoveredAreas();
        return window;
    }

    // 注销并销毁指定的窗口
    void unregisterWindow(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) {
            startAnimation(window, false);
            unregisterPendingWindows.push_back(window);
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

#define SHOW_FPS

    // 显示所有窗口
    void display_all() {
        updateAnimations();

        for (auto& row : covered) {
            std::fill(row.begin(), row.end(), false);
        }

        for (auto window : windows) {
            if (window->getWindowType() == MODAL_WINDOW || window->getWindowType() == POPUP_WINDOW) {
                continue;
            }
            renderWindowContent(window);
        }

        bool hasModalOrPopup = std::any_of(windows.begin(), windows.end(), [](Window* w) {
            return w->getWindowType() == MODAL_WINDOW || w->getWindowType() == POPUP_WINDOW;
        });

        if (hasModalOrPopup) {
            drawCheckerboardMask();
        }

        for (auto window : windows) {
            if (window->getWindowType() == MODAL_WINDOW || window->getWindowType() == POPUP_WINDOW) {
                renderWindowContent(window);
            }
        }

        if (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) {
            if (showFps) {
                static unsigned long lastFrameTime = 0;
                unsigned long currentTime = millis();
                unsigned long frameTime = currentTime - lastFrameTime;
                lastFrameTime = currentTime;

                display->setTextSize(1);
                display->setTextColor(SSD1306_WHITE);
                display->setCursor(0, 0);
                display->printf("FPS: %lu", frameTime > 0 ? 1000 / frameTime : 0);
            }
            display->display();
            xSemaphoreGive(displayMutex);
        }
    }

    int getForegroundWindowIndex() const {
        for (int i = windows.size() - 1; i >= 0; --i) {
            if (windows[i]->getWindowType() == MODAL_WINDOW || windows[i]->getWindowType() == POPUP_WINDOW) {
                return i;
            }
        }
        return windows.size();
    }

    void setShowFps(bool status) {
        showFps = status;
    }

    bool getShowFps() {
        return showFps;
    }

private:
    Adafruit_SSD1306* display;
    std::vector<Window*> windows;
    DitheringType ditheringType;
    std::vector<std::vector<bool>> covered;
    std::vector<Window*> unregisterPendingWindows;
    SemaphoreHandle_t displayMutex = xSemaphoreCreateMutex();
    bool showFps = false;

    struct AnimationState {
        Window* window;
        bool registering;
        float progress;
        bool active;
    };

    std::vector<AnimationState> animations;

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
        return windows.size();
    }

    void drawWindowBorder(int16_t x, int16_t y, uint16_t w, uint16_t h) {
        display->drawRect(x - 1, y - 1, w + 2, h + 2, SSD1306_WHITE);
    }

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
        if (xSemaphoreTake(window->bufferMutex, portMAX_DELAY) == pdTRUE) {
            uint8_t* buffer = window->getDisplayBuffer();
            int16_t posX = window->getPosX();
            int16_t posY = window->getPosY();
            uint16_t width = window->getWidth();
            uint16_t height = window->getHeight();

            DitheringType dithering = window->getDitheringType();
            bool orderedDithering = (dithering == ORDERED_DITHERING);

            int16_t renderOffsetY = getAnimationOffset(window);
            int16_t renderPosY = posY + renderOffsetY;

            if (window->hasBorderEnabled()) {
                drawWindowBorder(posX, renderPosY, width, height);
            }

            for (int16_t y = 0; y < height; y++) {
                for (int16_t x = 0; x < width; x++) {
                    int16_t screenX = posX + x;
                    int16_t screenY = renderPosY + y;

                    if (screenX >= SCREEN_WIDTH || screenY >= SCREEN_HEIGHT || screenY < 0 || covered[screenX][screenY]) {
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
            xSemaphoreGive(window->bufferMutex);
        }
    }

    void startAnimation(Window* window, bool registering) {
        // 查找是否已有该窗口的动画
        auto it = std::find_if(animations.begin(), animations.end(), [window](const AnimationState& anim) {
            return anim.window == window;
        });

        if (it != animations.end()) {
            // 如果已有动画且是不同方向的动画，则进行方向切换
            if (it->registering != registering) {
                it->registering = registering;
                it->progress = 1.0f - it->progress;  // 反向进度
            }
            it->active = true;
        } else {
            // 如果没有现有动画，则新建一个
            AnimationState animation = {window, registering, 0.0f, true};
            animations.push_back(animation);
        }
    }

    // 更新动画状态
    void updateAnimations() {
        for (auto& animation : animations) {
            if (!animation.active) {
                continue;
            }

            animation.progress += 0.042f;
            if (animation.progress >= 1.0f) {
                animation.progress = 1.0f;
                animation.active = false;
            }
        }

        // 删除注销动画结束的窗口
        for (auto it = unregisterPendingWindows.begin(); it != unregisterPendingWindows.end(); ) {
            auto pendingWindow = *it;
            auto animIt = std::find_if(animations.begin(), animations.end(), [pendingWindow](const AnimationState& anim) {
                return anim.window == pendingWindow && !anim.active;
            });
            if (animIt != animations.end()) {
                windows.erase(std::remove(windows.begin(), windows.end(), pendingWindow), windows.end());
                delete pendingWindow;
                it = unregisterPendingWindows.erase(it);
            } else {
                ++it;
            }
        }

        // 移除不活跃的动画
        animations.erase(std::remove_if(animations.begin(), animations.end(), [](const AnimationState& anim) {
            return !anim.active;
        }), animations.end());
    }

    // 指数减速
    int16_t getAnimationOffset(Window* window) {
        for (const auto& animation : animations) {
            if (animation.window == window && animation.active) {
                float easing;
                if (animation.registering) {
                    easing = 1 - powf(1 - animation.progress, 6);
                } else {
                    easing = powf(animation.progress, 5);
                }
                int16_t startY = animation.registering ? SCREEN_HEIGHT : 0;
                int16_t endY = animation.registering ? 0 : SCREEN_HEIGHT;
                return static_cast<int16_t>(startY + (endY - startY) * easing);
            }
        }
        return 0;
    }
};

/*
// 随机绘制函数图像
void drawFunction(Window* window, int functionType) {
    window->fillScreen(0);

    for (int x = 0; x < window->getWidth(); x++) {
        float scaledX = static_cast<float>(x) / window->getWidth() * 4 * M_PI;
        int y = 0;

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

        if (y >= 0 && y < window->getHeight()) {
            window->drawPixel(x, y, SSD1306_WHITE);
        }
    }

    window->display();
}

void runDemoTask(void* pvParameters) {
    printf("%ld\n", esp_get_free_heap_size());

    Window* dynamicWindow = window_manager.registerWindow(30, 20, 10, 10, FLOATING_WINDOW, true, NO_DITHERING);
    dynamicWindow->fillScreen(1);
    dynamicWindow->setTextColor(0);
    dynamicWindow->printf("hello");
    dynamicWindow->display();
    printf("%ld\n", esp_get_free_heap_size());
    vTaskDelay(256);
    for (uint8_t i = 0; i < 8; i++) {
        window_manager.unregisterWindow(dynamicWindow);
        printf("%ld\n", esp_get_free_heap_size());
        vTaskDelay(256);
        dynamicWindow = window_manager.registerWindow(random(30, 40), random(15, 24), 1, 1, FLOATING_WINDOW, true, NO_DITHERING);
        printf("%ld\n", esp_get_free_heap_size());
        dynamicWindow->fillScreen(i & 1);
        dynamicWindow->setTextColor(!(i & 1));
        dynamicWindow->printf("hello");
        dynamicWindow->display();
        vTaskDelay(256);
    }
    vTaskDelay(256);

    Window* grayscaleTestWindow = window_manager.registerWindow(50, 30, 0, SCREEN_HEIGHT - 30, FLOATING_WINDOW, true, ORDERED_DITHERING); 
    vTaskDelay(128);

    Window* sineWaveWindow = window_manager.registerWindow(80, 20, 10, 10, NORMAL_WINDOW, true, NO_DITHERING);

    window_manager.bringToFront(grayscaleTestWindow);
    vTaskDelay(128);

    int16_t dx = 2, dy = 1, x = 0, y = 0;
    uint16_t w = 30, h = 20;
    uint8_t brightness = 0;
    int8_t delta = 5;
    float angle = 0.0;
    int16_t backgroundOffset = 0;
    int currentFunction = 0;

    srand((unsigned int)time(NULL));

    Window* backgroundWindow = window_manager.registerWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, FIXED_BOTTOM_WINDOW, false, NO_DITHERING);
    window_manager.setShowFps(true);
    vTaskDelay(128);

    while (true) {
        backgroundWindow->fillScreen(0);
        for (int16_t i = 0; i < SCREEN_WIDTH; i += 16) {
            backgroundWindow->drawFastVLine((i + backgroundOffset) % SCREEN_WIDTH, 0, 64, SSD1306_WHITE);
        }
        backgroundOffset = (backgroundOffset + 1) % SCREEN_WIDTH;
        backgroundWindow->display();

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
            currentFunction = rand() & 3;
            drawFunction(dynamicWindow, currentFunction);

            int randomLayer = rand() % window_manager.getForegroundWindowIndex();
            window_manager.sendToBack(dynamicWindow);
            for (int i = 0; i < randomLayer; i++) {
                window_manager.moveUp(dynamicWindow);
            }
        } else {
            dynamicWindow->setPosition(x, y);
            dynamicWindow->setSize(w, h);
            dynamicWindow->display();
        }

        brightness += delta;
        if (brightness >= 255 || brightness <= 0) delta = -delta;

        uint16_t newWidth = 30 + 10 * sin(angle);
        uint16_t newHeight = 20 + 5 * sin(angle * 2);
        grayscaleTestWindow->setSize(newWidth, newHeight);

        grayscaleTestWindow->setPosition(0, SCREEN_HEIGHT - newHeight);

        grayscaleTestWindow->fillScreen(brightness);
        grayscaleTestWindow->display();

        angle += 0.1;
        if (angle > 2 * M_PI) angle -= 2 * M_PI;

        int16_t sineY = 20 + 10 * sin(angle);
        sineWaveWindow->setPosition((int16_t)(SCREEN_WIDTH / 2 - 40), sineY);
        sineWaveWindow->fillScreen(0);
        sineWaveWindow->setCursor(5, 5);
        sineWaveWindow->setTextSize(1);
        sineWaveWindow->setTextColor(SSD1306_WHITE);
        for (int i = 0; i < 12; i++) {
            char randomChar = 'A' + (rand() % 26);
            sineWaveWindow->printf("%c", randomChar);
        }
        sineWaveWindow->display();

        vTaskDelay(24);
    }
}
*/