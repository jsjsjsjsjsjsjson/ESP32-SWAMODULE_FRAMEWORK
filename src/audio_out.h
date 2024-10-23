#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "driver/i2s_std.h"
#include "module_manager.hpp"
#include "src_config.h"
#include "FreeRTOS.h"

const int BUFF_SIZE = 2048;

class i2s_audio_out: public Module_t {
public:

    i2s_audio_out() { module_info = {"ESP32 I2S Audio Out", "libchara-dev", "Audio signal output using ESP32's I2S", false, false}; }

    size_t writed;

    int16_t buffer[BUFF_SIZE];
    int16_t data = 0;
    int16_t bufferPoint = 0;

    i2s_chan_handle_t tx_handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SMP_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_42,
            .ws = GPIO_NUM_40,
            .dout = GPIO_NUM_41,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    void start() {
        registerPort(&data, PORT_AIN, "AUDIO OUTPUT", "audio output");
        i2s_new_channel(&chan_cfg, &tx_handle, NULL);
        i2s_channel_init_std_mode(tx_handle, &std_cfg);
        i2s_channel_enable(tx_handle);
        printf("i2s start!\n");
    }
    void stop() {
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        printf("i2s is disable\n");
    }
    void process() {
        buffer[bufferPoint] = data;
        // printf("buffer[%d] = %d;\n", bufferPoint, buffer[bufferPoint]);
        bufferPoint++;
        if (bufferPoint > BUFF_SIZE) {
            i2s_channel_write(tx_handle, buffer, sizeof(buffer), &writed, portMAX_DELAY);
            // printf("I2S: writed->%dBytes\n", writed);
            bufferPoint = 0;
        }
    }
    void customSettingPage() {

    }
    void customViewPage() {

    }
};

#endif