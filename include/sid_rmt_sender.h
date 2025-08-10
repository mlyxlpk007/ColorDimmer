#ifndef SID_RMT_SENDER_INCLUDE
#define SID_RMT_SENDER_INCLUDE
#include <stdint.h>
#include <Arduino.h>
#include <driver/rmt.h>

// 串口打印控制开关
#define ENABLE_SERIAL_PRINT 1  // 设置为0可以关闭所有串口打印

// 串口打印接口函数
void debug_print(const char* str);
void debug_print(int value);
void debug_print(unsigned int value);
void debug_print(long value);
void debug_print(unsigned long value);
void debug_printf(const char* format, ...);
void debug_println(const char* str);
void debug_println(int value);
void debug_println(unsigned int value);
void debug_println(long value);
void debug_println(unsigned long value);

#define SID_MAX_CHIPS 36 // 可根据实际需求调整
static const uint8_t LED_MATRIX_PATTERN[SID_MAX_CHIPS] = {
    0x0C, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0B, 0x11, 0x0D, 0x07,
    0x08, 0x09, 0x0E, 0x0F, 0x0A, 0x10, 0x17, 0x1D, 0x23, 0x22, 0x21, 0x20,
    0x1F, 0x1E, 0x18, 0x12, 0x16, 0x1C, 0x1B, 0x1A, 0x15, 0x14, 0x19, 0x13
};
// 亮度调节功能 (0-100)
extern uint8_t global_brightness;
void sid_rmt_init(void);
void send_data(const uint8_t* buf, int len, uint16_t gain);
void set_brightness(uint8_t brightness);
uint8_t get_brightness(void);
// 帧同步：每次调用推进一步并返回当前应用亮度（±1 或按特殊淡出策略）
uint8_t get_brightness_frame(void);
// 启动一个帧同步亮度淡入/淡出到目标值（用于 dimmer_blank 内部调用）
void start_brightness_fade(uint8_t target, uint8_t frames);
void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b);
void getColorTempRGBWithDuv(uint8_t tempIndex, uint8_t duvIndex, uint8_t* r, uint8_t* g, uint8_t* b);
#endif