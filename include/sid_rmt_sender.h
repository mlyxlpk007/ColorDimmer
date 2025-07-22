#pragma once

#include <Arduino.h>
#include "driver/rmt.h"

class SIDDriver {
public:
    SIDDriver(gpio_num_t gpio, rmt_channel_t channel = RMT_CHANNEL_0);
    void begin();
    void send_data(const uint8_t* buf, size_t len);

private:
    gpio_num_t _gpio;
    rmt_channel_t _channel;

    void build_rmt_items(const uint8_t* buf, size_t len, rmt_item32_t* items);
};
