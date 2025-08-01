#ifndef SID_RMT_SENDER_INCLUDE
#define SID_RMT_SENDER_INCLUDE
#include <stdint.h>

#define SID_MAX_CHIPS 36 // 可根据实际需求调整

// 亮度调节功能 (0-100)
extern uint8_t global_brightness;
void sid_rmt_init(void);
void send_data(const uint8_t* buf, int len, uint16_t gain);
void set_brightness(uint8_t brightness);
uint8_t get_brightness(void);
void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b);
void getColorTempRGBWithDuv(uint8_t tempIndex, uint8_t duvIndex, uint8_t* r, uint8_t* g, uint8_t* b);
#endif