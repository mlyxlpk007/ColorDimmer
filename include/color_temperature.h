#pragma once
#include <cstdint>

// 色温系统配置
#define COLOR_TEMP_STANDARD_COUNT 61    // 标准色温数量 (2700K-6500K)
#define COLOR_TEMP_DUV_VARIANTS 5       // 每个色温的DUV变体数量
#define COLOR_TEMP_TOTAL_COUNT (COLOR_TEMP_STANDARD_COUNT * COLOR_TEMP_DUV_VARIANTS)  // 总色温数量

// DUV微调方向
enum DuvDirection {
    DUV_MINUS_2 = 0,  // 最偏绿
    DUV_MINUS_1 = 1,  // 偏绿
    DUV_ZERO = 2,     // 标准 (DUV=0)
    DUV_PLUS_1 = 3,   // 偏紫
    DUV_PLUS_2 = 4    // 最偏紫
};

// 色温结构体
struct ColorTemperature {
    uint16_t kelvin;      // 色温值 (K)
    float duv;           // DUV偏移值
    uint8_t r, g, b;     // RGB值
    const char* name;    // 色温名称
};

// 色温管理器
class ColorTemperatureManager {
public:
    ColorTemperatureManager();
    ~ColorTemperatureManager();
    
    // 初始化色温系统
    bool init();
    
    // 根据索引获取色温 (0-304)
    bool getColorTempByIndex(uint16_t index, ColorTemperature& colorTemp);
    
    // 根据色温和DUV获取RGB值
    bool getColorTempRGB(uint16_t kelvin, DuvDirection duv, uint8_t* r, uint8_t* g, uint8_t* b);
    
    // 根据索引获取RGB值 (兼容旧接口)
    void getColorTempRGB(uint8_t tempIndex, uint8_t* r, uint8_t* g, uint8_t* b);
    
    // 获取色温信息
    uint16_t getKelvinByIndex(uint16_t index);
    DuvDirection getDuvByIndex(uint16_t index);
    const char* getNameByIndex(uint16_t index);
    
    // 色温范围查询
    uint16_t getMinKelvin() const { return 2700; }
    uint16_t getMaxKelvin() const { return 6500; }
    
    // 获取标准色温数量
    uint16_t getStandardCount() const { return COLOR_TEMP_STANDARD_COUNT; }
    
    // 获取总色温数量
    uint16_t getTotalCount() const { return COLOR_TEMP_TOTAL_COUNT; }
    
    // 色温转换工具
    static void kelvinToRGB(uint16_t kelvin, uint8_t* r, uint8_t* g, uint8_t* b);
    static void applyDuvAdjustment(uint8_t* r, uint8_t* g, uint8_t* b, float duv);
    static uint16_t indexToKelvin(uint16_t index);
    static DuvDirection indexToDuv(uint16_t index);
    static uint16_t kelvinDuvToIndex(uint16_t kelvin, DuvDirection duv);
    
    // 预设色温
    static const uint16_t PRESET_WARM_WHITE = 2700;      // 暖白
    static const uint16_t PRESET_SOFT_WHITE = 3000;      // 软白
    static const uint16_t PRESET_COOL_WHITE = 4000;      // 冷白
    static const uint16_t PRESET_DAYLIGHT = 5000;        // 日光
    static const uint16_t PRESET_COOL_DAYLIGHT = 6500;   // 冷日光

private:
    // 标准色温表 (61个标准色温)
    static const uint16_t STANDARD_KELVINS[COLOR_TEMP_STANDARD_COUNT];
    
    // DUV偏移值表
    static const float DUV_OFFSETS[COLOR_TEMP_DUV_VARIANTS];
    
    // 色温名称表
    static const char* TEMP_NAMES[COLOR_TEMP_STANDARD_COUNT];
    
    // 计算色温到RGB的转换
    static void calculateKelvinRGB(uint16_t kelvin, uint8_t* r, uint8_t* g, uint8_t* b);
    
    // 应用DUV调整
    static void applyDuvShift(uint8_t* r, uint8_t* g, uint8_t* b, float duv, uint16_t kelvin);
}; 