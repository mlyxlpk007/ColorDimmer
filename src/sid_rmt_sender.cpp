#include "sid_rmt_sender.h"
extern "C" {
#include "driver/rmt.h"
#include "soc/rmt_struct.h"
#include "soc/rmt_reg.h"
}
#include "esp_log.h"
#include <Arduino.h>
extern bool lightPower;
// 全局亮度变量
uint8_t global_brightness = 100; // 默认100%亮度
// 色温模式变量
extern uint8_t currentColorTemp;
extern bool colorTempMode; // 是否处于色温模式

// 色温索引到RGB值的转换函数
void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b) {
    // 色温范围：2700K-6500K，索引1-61
    // 线性插值计算RGB值
    float ratio = (float)(tempIndex - 1) / 60.0f; // 0.0 - 1.0
    
    if (tempIndex <= 20) {
        // 暖色温 (2700K-3000K) - 橙黄色
        *r = 255;
        *g = 147 + (uint8_t)(ratio * 20); // 147-167
        *b = 41 + (uint8_t)(ratio * 20);  // 41-61
    } else if (tempIndex <= 40) {
        // 中性色温 (4000K-5000K) - 暖白色
        *r = 255;
        *g = 167 + (uint8_t)((ratio - 0.33f) * 88); // 167-255
        *b = 61 + (uint8_t)((ratio - 0.33f) * 163); // 61-224
    } else {
        // 冷色温 (6000K-6500K) - 冷白色
        *r = 201 + (uint8_t)((ratio - 0.67f) * 54); // 201-255
        *g = 226 + (uint8_t)((ratio - 0.67f) * 29); // 226-255
        *b = 255;
    }
    
    Serial.printf("Color temp %d: R=%d G=%d B=%d\n", tempIndex, *r, *g, *b);
}
#define RMT_TX_CHANNEL    RMT_CHANNEL_1
#define RMT_TX_GPIO       GPIO_NUM_25
#define RMT_CLK_DIV 1
#define T0H_TICKS  23
#define T0L_TICKS  73
#define T1H_TICKS  72
#define T1L_TICKS  24
#define RESET_TICKS 16320

void sid_rmt_init(void)
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_TX_CHANNEL;
    config.gpio_num = RMT_TX_GPIO;
    config.clk_div = RMT_CLK_DIV;
    config.mem_block_num = 1; // 必须为1
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

    esp_err_t err;
    err = rmt_config(&config);
    Serial.printf("rmt_config: %d\n", err);
    err = rmt_driver_install(config.channel, 0, 0);
    Serial.printf("rmt_driver_install: %d\n", err);
}

static void build_sid_rmt_items(const uint32_t* data, int chip_count, uint16_t gain, rmt_item32_t* items, int* item_count)
{
    int idx = 0;
    // Reset (帧头)
    items[idx].level0 = 0;
    items[idx].duration0 = RESET_TICKS;
    items[idx].level1 = 0;
    items[idx].duration1 = 300;
    idx++;

    // 芯片数据
    for (int i = 0; i < chip_count; ++i) {
        uint32_t d = data[i];
        for (int bit = 23; bit >= 0; --bit) {
            if (d & (1U << bit)) {
                items[idx].level0 = 1;
                items[idx].duration0 = T1H_TICKS;
                items[idx].level1 = 0;
                items[idx].duration1 = T1L_TICKS;
            } else {
                items[idx].level0 = 1;
                items[idx].duration0 = T0H_TICKS;
                items[idx].level1 = 0;
                items[idx].duration1 = T0L_TICKS;
            }
            idx++;
        }
    }
    // 增益
    for (int bit = 15; bit >= 0; --bit) {
        if (gain & (1U << bit)) {
            items[idx].level0 = 1;
            items[idx].duration0 = T1H_TICKS;
            items[idx].level1 = 0;
            items[idx].duration1 = T1L_TICKS;
        } else {
            items[idx].level0 = 1;
            items[idx].duration0 = T0H_TICKS;
            items[idx].level1 = 0;
            items[idx].duration1 = T0L_TICKS;
        }
        idx++;
    }
    // Reset (帧尾)
    items[idx].level0 = 0;
    items[idx].duration0 = RESET_TICKS;
    items[idx].level1 = 0;
    items[idx].duration1 = 100;
    idx++;
    *item_count = idx;
}

void send_data(const uint8_t* buf, int len, uint16_t gain)
{
    //Serial.println("send_data called");
   // gain=0XFFFF;
    int chip_count = len / 3;
    if (chip_count > SID_MAX_CHIPS) {
   //     Serial.println("chip_count overflow");
        return;
    }
    uint32_t chip_data[SID_MAX_CHIPS];
    if(!lightPower)
    {
        for (int i = 0; i < chip_count; ++i) {
            chip_data[i] = 0x000000;
        }
    } else {
        // 统一处理：应用亮度调节到RGB数据
        for (int i = 0; i < chip_count; ++i) {
            // 应用亮度调节到RGB值
            uint8_t r = (uint8_t)((buf[i*3+0] * global_brightness) / 100);
            uint8_t g = (uint8_t)((buf[i*3+1] * global_brightness) / 100);
            uint8_t b = (uint8_t)((buf[i*3+2] * global_brightness) / 100);
            
            chip_data[i] = ((uint32_t)r << 16) |
                           ((uint32_t)g << 8)  |
                           ((uint32_t)b);
        }
    }
    rmt_item32_t items[2 + 24*SID_MAX_CHIPS + 16];
    int item_count = 0;
    build_sid_rmt_items(chip_data, chip_count, gain, items, &item_count);
    rmt_write_items(RMT_TX_CHANNEL, items, item_count, true);
    rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);
}

// 设置亮度 (0-100)
void set_brightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    global_brightness = brightness;
    Serial.printf("Brightness set to: %d%%\n", global_brightness);
}

// 获取当前亮度
uint8_t get_brightness(void) {
    return global_brightness;
} 