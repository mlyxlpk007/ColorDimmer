#ifndef SID_RMT_SENDER_INCLUDE
#define SID_RMT_SENDER_INCLUDE
#include <stdint.h>

#define SID_MAX_CHIPS 36 // 可根据实际需求调整
static const uint8_t LED_MATRIX_PATTERN[SID_MAX_CHIPS] = {
    0x0C, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0B, 0x11, 0x0D, 0x07,
    0x08, 0x09, 0x0E, 0x0F, 0x0A, 0x10, 0x17, 0x1D, 0x23, 0x22, 0x21, 0x20,
    0x1F, 0x1E, 0x18, 0x12, 0x16, 0x1C, 0x1B, 0x1A, 0x15, 0x14, 0x19, 0x13
};
void dimmer_blank();
// 亮度调节功能 (0-100)
extern uint8_t global_brightness;
void sid_rmt_init(void);
void send_data(const uint8_t* buf, int len, uint16_t gain);
void set_brightness(uint8_t brightness);
uint8_t get_brightness(void);
void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b);
void getColorTempRGBWithDuv(uint8_t tempIndex, uint8_t duvIndex, uint8_t* r, uint8_t* g, uint8_t* b);
#endif