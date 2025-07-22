#pragma once
#include <stdint.h>

#define SID_MAX_CHIPS 36 // 可根据实际需求调整

void sid_rmt_init(void);
void send_data(const uint8_t* buf, int len, uint16_t gain);
