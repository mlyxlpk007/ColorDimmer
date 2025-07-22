#include "sid_rmt_sender.h"
#include "driver/rmt.h"
#include "esp_log.h"

#define RMT_TX_CHANNEL    RMT_CHANNEL_0
#define RMT_TX_GPIO       25
#define RMT_CLK_DIV       4   // 80MHz/4=20MHz, 1tick=0.05us

#define T0H_TICKS  6
#define T0L_TICKS  18
#define T1H_TICKS  18
#define T1L_TICKS  6
#define RESET_TICKS 4200

void sid_rmt_init(void)
{
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_TX_CHANNEL,
        .gpio_num = RMT_TX_GPIO,
        .clk_div = RMT_CLK_DIV,
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);
}

static void build_sid_rmt_items(const uint32_t* data, int chip_count, uint16_t gain, rmt_item32_t* items, int* item_count)
{
    int idx = 0;
    // Reset (帧头)
    items[idx].level0 = 0;
    items[idx].duration0 = RESET_TICKS;
    items[idx].level1 = 0;
    items[idx].duration1 = 0;
    idx++;

    // 芯片数据
    for (int i = 0; i < chip_count; ++i) {
        uint32_t d = data[i];
        for (int bit = 31; bit >= 0; --bit) {
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
    items[idx].duration1 = 0;
    idx++;

    *item_count = idx;
}

void send_data(const uint8_t* buf, int len, uint16_t gain)
{
    int chip_count = len / 4;
    uint32_t chip_data[SID_MAX_CHIPS];
    for (int i = 0; i < chip_count; ++i) {
        chip_data[i] = ((uint32_t)buf[i*4+0] << 24) |
                       ((uint32_t)buf[i*4+1] << 16) |
                       ((uint32_t)buf[i*4+2] << 8)  |
                       ((uint32_t)buf[i*4+3]);
    }
    rmt_item32_t items[2 + 32*SID_MAX_CHIPS + 16];
    int item_count = 0;
    build_sid_rmt_items(chip_data, chip_count, gain, items, &item_count);
    rmt_write_items(RMT_TX_CHANNEL, items, item_count, true);
    rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);
} 