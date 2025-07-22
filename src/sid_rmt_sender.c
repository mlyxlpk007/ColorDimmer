#include "sid_rmt_sender.h"

#define T0H_NS  300   // 0.3us
#define T0L_NS  900   // 0.9us
#define T1H_NS  900   // 0.9us
#define T1L_NS  300   // 0.3us
#define RESET_US 210

static inline uint32_t ns_to_ticks(uint32_t ns) {
    return (uint32_t)(ns / 12.5); // APB 80MHz
}

SIDDriver::SIDDriver(gpio_num_t gpio, rmt_channel_t channel)
    : _gpio(gpio), _channel(channel) {}

void SIDDriver::begin() {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(_gpio, _channel);
    config.clk_div = 1;  // 80MHz / 1 = 12.5ns ticks
    rmt_config(&config);
    rmt_driver_install(_channel, 0, 0);
}

void SIDDriver::build_rmt_items(const uint8_t* buf, size_t len, rmt_item32_t* items) {
    size_t idx = 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t byte = buf[i];
        for (int bit = 7; bit >= 0; --bit) {
            bool is1 = (byte >> bit) & 0x01;
            items[idx].level0 = 1;
            items[idx].duration0 = ns_to_ticks(is1 ? T1H_NS : T0H_NS);
            items[idx].level1 = 0;
            items[idx].duration1 = ns_to_ticks(is1 ? T1L_NS : T0L_NS);
            ++idx;
        }
    }
    // Reset
    items[idx].level0 = 0;
    items[idx].duration0 = (uint32_t)(RESET_US * 80);  // 80MHz ticks
    items[idx].level1 = 0;
    items[idx].duration1 = 0;
}

void SIDDriver::send_data(const uint8_t* buf, size_t len) {
    const size_t item_num = len * 8 + 1;
    rmt_item32_t* items = (rmt_item32_t*)malloc(item_num * sizeof(rmt_item32_t));
    if (!items) return;

    build_rmt_items(buf, len, items);
    rmt_write_items(_channel, items, item_num, true);
    rmt_wait_tx_done(_channel, portMAX_DELAY);
    free(items);
}
