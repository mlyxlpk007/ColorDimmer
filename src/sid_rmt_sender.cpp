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

// 色温RGB查表数据 (305行，每5行为一组DUV变体)
static const uint8_t COLOR_TEMP_TABLE[305][3] = {
    // 色温1的5种DUV变体
    {180, 16, 3},   // DUV=0.006
    {181, 13, 6},   // DUV=0.003
    {182, 10, 9},   // DUV=0
    {182, 7, 12},   // DUV=-0.003
    {183, 3, 14},   // DUV=-0.006
    // 色温2的5种DUV变体
    {172, 26, 3},   // DUV=0.006
    {173, 21, 7},   // DUV=0.003
    {173, 16, 11},  // DUV=0
    {175, 11, 15},  // DUV=-0.003
    {175, 6, 19},   // DUV=-0.006
    // 色温3的5种DUV变体
    {163, 35, 2},   // DUV=0.006
    {164, 30, 6},   // DUV=0.003
    {165, 24, 10},  // DUV=0
    {166, 19, 15},  // DUV=-0.003
    {168, 14, 19},  // DUV=-0.006
    // 色温4的5种DUV变体
    {155, 42, 3},   // DUV=0.006
    {156, 36, 7},   // DUV=0.003
    {158, 31, 12},  // DUV=0
    {159, 25, 16},  // DUV=-0.003
    {160, 20, 20},  // DUV=-0.006
    // 色温5的5种DUV变体
    {147, 50, 3},   // DUV=0.006
    {149, 44, 7},   // DUV=0.003
    {150, 38, 12},  // DUV=0
    {152, 32, 16},  // DUV=-0.003
    {153, 27, 20},  // DUV=-0.006
    // 色温6的5种DUV变体
    {140, 58, 3},   // DUV=0.006
    {141, 52, 7},   // DUV=0.003
    {143, 45, 12},  // DUV=0
    {145, 39, 16},  // DUV=-0.003
    {146, 33, 21},  // DUV=-0.006
    // 色温7的5种DUV变体
    {132, 65, 3},   // DUV=0.006
    {134, 58, 7},   // DUV=0.003
    {136, 52, 12},  // DUV=0
    {138, 46, 17},  // DUV=-0.003
    {139, 39, 21},  // DUV=-0.006
    // 色温8的5种DUV变体
    {125, 72, 3},   // DUV=0.006
    {127, 65, 8},   // DUV=0.003
    {129, 58, 13},  // DUV=0
    {131, 52, 17},  // DUV=-0.003
    {133, 45, 22},  // DUV=-0.006
    // 色温9的5种DUV变体
    {118, 80, 2},   // DUV=0.006
    {120, 73, 7},   // DUV=0.003
    {122, 66, 12},  // DUV=0
    {125, 59, 16},  // DUV=-0.003
    {127, 52, 21},  // DUV=-0.006
    // 色温10的5种DUV变体
    {111, 86, 2},   // DUV=0.006
    {114, 79, 7},   // DUV=0.003
    {116, 71, 12},  // DUV=0
    {119, 64, 17},  // DUV=-0.003
    {121, 57, 22},  // DUV=-0.006
    // 色温11的5种DUV变体
    {105, 92, 3},   // DUV=0.006
    {108, 84, 8},   // DUV=0.003
    {110, 77, 13},  // DUV=0
    {113, 69, 18},  // DUV=-0.003
    {116, 62, 23},  // DUV=-0.006
    // 色温12的5种DUV变体
    {98, 100, 2},   // DUV=0.006
    {101, 92, 7},   // DUV=0.003
    {104, 84, 12},  // DUV=0
    {107, 76, 17},  // DUV=-0.003
    {109, 69, 22},  // DUV=-0.006
    // 色温13的5种DUV变体
    {93, 102, 5},   // DUV=0.006
    {96, 94, 10},   // DUV=0.003
    {99, 86, 15},   // DUV=0
    {102, 78, 20},  // DUV=-0.003
    {105, 70, 25},  // DUV=-0.006
    // 色温14的5种DUV变体
    {89, 103, 8},   // DUV=0.006
    {92, 95, 13},   // DUV=0.003
    {95, 87, 18},   // DUV=0
    {98, 79, 23},   // DUV=-0.003
    {101, 72, 27},  // DUV=-0.006
    // 色温15的5种DUV变体
    {85, 105, 11},  // DUV=0.006
    {88, 97, 16},   // DUV=0.003
    {91, 89, 21},   // DUV=0
    {94, 80, 25},   // DUV=-0.003
    {97, 73, 30},   // DUV=-0.006
    // 色温16的5种DUV变体
    {81, 106, 14},  // DUV=0.006
    {84, 97, 19},   // DUV=0.003
    {87, 89, 23},   // DUV=0
    {91, 81, 28},   // DUV=-0.003
    {94, 73, 33},   // DUV=-0.006
    // 色温17的5种DUV变体
    {77, 107, 16},  // DUV=0.006
    {81, 98, 21},   // DUV=0.003
    {84, 90, 26},   // DUV=0
    {88, 82, 31},   // DUV=-0.003
    {91, 74, 36},   // DUV=-0.006
    // 色温18的5种DUV变体
    {74, 107, 19},  // DUV=0.006
    {77, 98, 24},   // DUV=0.003
    {81, 90, 29},   // DUV=0
    {85, 82, 34},   // DUV=-0.003
    {88, 74, 38},   // DUV=-0.006
    // 色温19的5种DUV变体
    {70, 107, 22},  // DUV=0.006
    {74, 99, 27},   // DUV=0.003
    {78, 90, 32},   // DUV=0
    {82, 82, 37},   // DUV=-0.003
    {85, 74, 41},   // DUV=-0.006
    // 色温20的5种DUV变体
    {67, 107, 26},  // DUV=0.006
    {71, 98, 30},   // DUV=0.003
    {75, 90, 35},   // DUV=0
    {79, 82, 39},   // DUV=-0.003
    {83, 73, 44},   // DUV=-0.006
    // 色温21的5种DUV变体
    {65, 107, 29},  // DUV=0.006
    {69, 98, 33},   // DUV=0.003
    {73, 90, 38},   // DUV=0
    {77, 81, 42},   // DUV=-0.003
    {80, 73, 47},   // DUV=-0.006
    // 色温22的5种DUV变体
    {62, 107, 32},  // DUV=0.006
    {66, 98, 36},   // DUV=0.003
    {70, 89, 41},   // DUV=0
    {74, 81, 45},   // DUV=-0.003
    {78, 73, 49},   // DUV=-0.006
    // 色温23的5种DUV变体
    {60, 106, 35},  // DUV=0.006
    {64, 97, 39},   // DUV=0.003
    {68, 88, 43},   // DUV=0
    {72, 80, 48},   // DUV=-0.003
    {76, 72, 52},   // DUV=-0.006
    // 色温24的5种DUV变体
    {57, 105, 38},  // DUV=0.006
    {62, 96, 42},   // DUV=0.003
    {66, 88, 46},   // DUV=0
    {70, 79, 50},   // DUV=-0.003
    {75, 71, 55},   // DUV=-0.006
    // 色温25的5种DUV变体
    {55, 104, 40},  // DUV=0.006
    {60, 95, 45},   // DUV=0.003
    {64, 87, 49},   // DUV=0
    {69, 78, 53},   // DUV=-0.003
    {73, 70, 57},   // DUV=-0.006
    // 色温26的5种DUV变体
    {53, 103, 43},  // DUV=0.006
    {58, 95, 48},   // DUV=0.003
    {63, 86, 52},   // DUV=0
    {67, 78, 56},   // DUV=-0.003
    {71, 69, 60},   // DUV=-0.006
    // 色温27的5种DUV变体
    {52, 102, 46},  // DUV=0.006
    {56, 93, 50},   // DUV=0.003
    {61, 85, 54},   // DUV=0
    {65, 76, 58},   // DUV=-0.003
    {70, 68, 62},   // DUV=-0.006
    // 色温28的5种DUV变体
    {50, 101, 49},  // DUV=0.006
    {55, 92, 53},   // DUV=0.003
    {59, 84, 57},   // DUV=0
    {64, 75, 61},   // DUV=-0.003
    {68, 67, 64},   // DUV=-0.006
    // 色温29的5种DUV变体
    {48, 100, 52},  // DUV=0.006
    {53, 91, 56},   // DUV=0.003
    {58, 83, 59},   // DUV=0
    {63, 74, 63},   // DUV=-0.003
    {67, 66, 67},   // DUV=-0.006
    // 色温30的5种DUV变体
    {47, 99, 54},   // DUV=0.006
    {52, 90, 58},   // DUV=0.003
    {57, 81, 62},   // DUV=0
    {61, 73, 66},   // DUV=-0.003
    {66, 65, 69},   // DUV=-0.006
    // 色温31的5种DUV变体
    {46, 97, 57},   // DUV=0.006
    {51, 89, 61},   // DUV=0.003
    {56, 80, 64},   // DUV=0
    {60, 72, 68},   // DUV=-0.003
    {65, 64, 71},   // DUV=-0.006
    // 色温32的5种DUV变体
    {44, 96, 60},   // DUV=0.006
    {49, 87, 63},   // DUV=0.003
    {54, 79, 67},   // DUV=0
    {59, 71, 70},   // DUV=-0.003
    {64, 63, 74},   // DUV=-0.006
    // 色温33的5种DUV变体
    {43, 95, 62},   // DUV=0.006
    {48, 86, 66},   // DUV=0.003
    {53, 78, 69},   // DUV=0
    {58, 70, 72},   // DUV=-0.003
    {63, 62, 76},   // DUV=-0.006
    // 色温34的5种DUV变体
    {42, 93, 65},   // DUV=0.006
    {47, 85, 68},   // DUV=0.003
    {52, 76, 71},   // DUV=0
    {57, 68, 75},   // DUV=-0.003
    {62, 60, 78},   // DUV=-0.006
    // 色温35的5种DUV变体
    {41, 92, 67},   // DUV=0.006
    {46, 83, 70},   // DUV=0.003
    {52, 75, 74},   // DUV=0
    {56, 67, 77},   // DUV=-0.003
    {61, 59, 80},   // DUV=-0.006
    // 色温36的5种DUV变体
    {40, 91, 69},   // DUV=0.006
    {45, 82, 73},   // DUV=0.003
    {51, 74, 76},   // DUV=0
    {56, 66, 79},   // DUV=-0.003
    {60, 58, 82},   // DUV=-0.006
    // 色温37的5种DUV变体
    {39, 89, 72},   // DUV=0.006
    {45, 80, 75},   // DUV=0.003
    {50, 72, 78},   // DUV=0
    {55, 64, 81},   // DUV=-0.003
    {60, 57, 84},   // DUV=-0.006
    // 色温38的5种DUV变体
    {39, 87, 74},   // DUV=0.006
    {44, 79, 77},   // DUV=0.003
    {49, 71, 80},   // DUV=0
    {54, 63, 83},   // DUV=-0.003
    {59, 55, 86},   // DUV=-0.006
    // 色温39的5种DUV变体
    {38, 86, 76},   // DUV=0.006
    {43, 78, 79},   // DUV=0.003
    {49, 70, 82},   // DUV=0
    {54, 62, 85},   // DUV=-0.003
    {59, 54, 87},   // DUV=-0.006
    // 色温40的5种DUV变体
    {37, 85, 78},   // DUV=0.006
    {43, 76, 81},   // DUV=0.003
    {48, 68, 84},   // DUV=0
    {53, 60, 87},   // DUV=-0.003
    {58, 53, 89},   // DUV=-0.006
    // 色温41的5种DUV变体
    {36, 83, 80},   // DUV=0.006
    {42, 75, 83},   // DUV=0.003
    {47, 67, 86},   // DUV=0
    {52, 59, 88},   // DUV=-0.003
    {57, 52, 91},   // DUV=-0.006
    // 色温42的5种DUV变体
    {36, 82, 82},   // DUV=0.006
    {41, 74, 85},   // DUV=0.003
    {47, 66, 88},   // DUV=0
    {52, 58, 90},   // DUV=-0.003
    {57, 50, 93},   // DUV=-0.006
    // 色温43的5种DUV变体
    {35, 80, 84},   // DUV=0.006
    {41, 72, 87},   // DUV=0.003
    {46, 64, 90},   // DUV=0
    {51, 57, 92},   // DUV=-0.003
    {57, 49, 94},   // DUV=-0.006
    // 色温44的5种DUV变体
    {35, 79, 86},   // DUV=0.006
    {40, 71, 89},   // DUV=0.003
    {46, 63, 91},   // DUV=0
    {51, 55, 94},   // DUV=-0.003
    {56, 48, 96},   // DUV=-0.006
    // 色温45的5种DUV变体
    {34, 77, 88},   // DUV=0.006
    {40, 69, 91},   // DUV=0.003
    {45, 62, 93},   // DUV=0
    {51, 54, 95},   // DUV=-0.003
    {56, 47, 98},   // DUV=-0.006
    // 色温46的5种DUV变体
    {34, 76, 90},   // DUV=0.006
    {40, 68, 92},   // DUV=0.003
    {45, 60, 95},   // DUV=0
    {50, 53, 97},   // DUV=-0.003
    {55, 46, 99},   // DUV=-0.006
    // 色温47的5种DUV变体
    {34, 75, 92},   // DUV=0.006
    {39, 67, 94},   // DUV=0.003
    {45, 59, 96},   // DUV=0
    {50, 52, 98},   // DUV=-0.003
    {55, 44, 101},  // DUV=-0.006
    // 色温48的5种DUV变体
    {33, 73, 94},   // DUV=0.006
    {39, 66, 96},   // DUV=0.003
    {44, 58, 98},   // DUV=0
    {50, 50, 100},  // DUV=-0.003
    {55, 43, 102},  // DUV=-0.006
    // 色温49的5种DUV变体
    {33, 72, 95},   // DUV=0.006
    {39, 64, 97},   // DUV=0.003
    {44, 57, 99},   // DUV=0
    {49, 49, 102},  // DUV=-0.003
    {54, 42, 103},  // DUV=-0.006
    // 色温50的5种DUV变体
    {32, 71, 97},   // DUV=0.006
    {38, 63, 99},   // DUV=0.003
    {44, 55, 101},  // DUV=0
    {49, 48, 103},  // DUV=-0.003
    {54, 41, 105},  // DUV=-0.006
    // 色温51的5种DUV变体
    {31, 64, 104},  // DUV=0.006
    {37, 57, 106},  // DUV=0.003
    {43, 50, 108},  // DUV=0
    {48, 43, 110},  // DUV=-0.003
    {53, 36, 111},  // DUV=-0.006
    // 色温52的5种DUV变体
    {30, 59, 111},  // DUV=0.006
    {36, 51, 113},  // DUV=0.003
    {42, 44, 114},  // DUV=0
    {47, 38, 116},  // DUV=-0.003
    {52, 31, 117},  // DUV=-0.006
    // 色温53的5种DUV变体
    {30, 53, 117},  // DUV=0.006
    {35, 46, 118},  // DUV=0.003
    {41, 39, 120},  // DUV=0
    {47, 33, 121},  // DUV=-0.003
    {52, 26, 122},  // DUV=-0.006
    // 色温54的5种DUV变体
    {29, 48, 122},  // DUV=0.006
    {35, 42, 123},  // DUV=0.003
    {41, 35, 124},  // DUV=0
    {46, 28, 125},  // DUV=-0.003
    {52, 22, 126},  // DUV=-0.006
    // 色温55的5种DUV变体
    {29, 44, 127},  // DUV=0.006
    {35, 37, 128},  // DUV=0.003
    {41, 31, 129},  // DUV=0
    {46, 25, 130},  // DUV=-0.003
    {51, 18, 130},  // DUV=-0.006
    // 色温56的5种DUV变体
    {29, 40, 131},  // DUV=0.006
    {35, 33, 132},  // DUV=0.003
    {41, 27, 132},  // DUV=0
    {46, 21, 134},  // DUV=-0.003
    {51, 15, 134},  // DUV=-0.006
    // 色温57的5种DUV变体
    {29, 36, 135},  // DUV=0.006
    {35, 30, 135},  // DUV=0.003
    {40, 24, 136},  // DUV=0
    {46, 18, 136},  // DUV=-0.003
    {51, 12, 137},  // DUV=-0.006
    // 色温58的5种DUV变体
    {29, 33, 138},  // DUV=0.006
    {35, 27, 139},  // DUV=0.003
    {41, 21, 139},  // DUV=0
    {46, 15, 139},  // DUV=-0.003
    {51, 9, 140},   // DUV=-0.006
    // 色温59的5种DUV变体
    {29, 30, 141},  // DUV=0.006
    {35, 24, 142},  // DUV=0.003
    {41, 18, 142},  // DUV=0
    {46, 12, 142},  // DUV=-0.003
    {51, 6, 142},   // DUV=-0.006
    // 色温60的5种DUV变体
    {30, 27, 144},  // DUV=0.006
    {35, 21, 144},  // DUV=0.003
    {41, 15, 144},  // DUV=0
    {46, 9, 145},   // DUV=-0.003
    {51, 4, 145},   // DUV=-0.006
    // 色温61的5种DUV变体
    {29, 24, 146},  // DUV=0.006
    {35, 18, 146},  // DUV=0.003
    {41, 13, 146},  // DUV=0
    {46, 7, 147},   // DUV=-0.003
    {51, 2, 147}    // DUV=-0.006
};

// DUV偏移值表
static const float DUV_OFFSETS[5] = {0.006f, 0.003f, 0.0f, -0.003f, -0.006f};

// 色温索引到RGB值的转换函数 (查表版本)
void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b) {
    // 参数检查
    if (tempIndex < 1 || tempIndex > 61 || !r || !g || !b) {
        *r = *g = *b = 0;
        return;
    }
    
    // 默认使用标准DUV (DUV=0，即第3个变体)
    uint8_t duvIndex = 2; // DUV=0对应的索引
    
    // 计算表格索引：每个色温有5个DUV变体
    uint16_t tableIndex = (tempIndex - 1) * 5 + duvIndex;
    
    // 查表获取RGB值
    *r = COLOR_TEMP_TABLE[tableIndex][0];
    *g = COLOR_TEMP_TABLE[tableIndex][1];
    *b = COLOR_TEMP_TABLE[tableIndex][2];
    
    Serial.printf("Color temp %d (DUV=%.3f): R=%d G=%d B=%d\n", 
                 tempIndex, DUV_OFFSETS[duvIndex], *r, *g, *b);
}

// 带DUV参数的色温RGB转换函数
void getColorTempRGBWithDuv(uint8_t tempIndex, uint8_t duvIndex, uint8_t* r, uint8_t* g, uint8_t* b) {
    // 参数检查
    if (tempIndex < 1 || tempIndex > 61 || duvIndex >= 5 || !r || !g || !b) {
        *r = *g = *b = 0;
        return;
    }
    
    // 计算表格索引：每个色温有5个DUV变体
    uint16_t tableIndex = (tempIndex - 1) * 5 + duvIndex;
    
    // 查表获取RGB值
    *r = COLOR_TEMP_TABLE[tableIndex][0];
    *g = COLOR_TEMP_TABLE[tableIndex][1];
    *b = COLOR_TEMP_TABLE[tableIndex][2];
    
    Serial.printf("Color temp %d (DUV=%.3f): R=%d G=%d B=%d\n", 
                 tempIndex, DUV_OFFSETS[duvIndex], *r, *g, *b);
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