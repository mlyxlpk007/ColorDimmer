#pragma once
#include <cstdint>

// 动画效果基类
class AnimEffect {
public:
    virtual ~AnimEffect() = default;
    
    // 生成动画数据的虚函数
    virtual void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) = 0;
    
    // 获取动画名称
    virtual const char* getName() const = 0;
    // 获取帧数
    virtual int getFrameCount() const = 0;
};

// 呼吸灯效果
class BreathEffect : public AnimEffect {
public:
    BreathEffect(uint8_t r = 255, uint8_t g = 100, uint8_t b = 50, int frames = 50) : r_(r), g_(g), b_(b), frames_(frames) {}
    
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "Breath"; }
    int getFrameCount() const override { return frames_; }
    
private:
    uint8_t r_, g_, b_;  // 基础颜色
    int frames_;
};

// 彩虹效果
class RainbowEffect : public AnimEffect {
public:
    RainbowEffect(int frames = 60) : frames_(frames) {}
    
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "Rainbow"; }
    int getFrameCount() const override { return frames_; }
private:
    int frames_;
};

// 闪烁效果
class BlinkEffect : public AnimEffect {
public:
    BlinkEffect(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int frames = 20) : r_(r), g_(g), b_(b), frames_(frames) {}
    
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "Blink"; }
    int getFrameCount() const override { return frames_; }
    
private:
    uint8_t r_, g_, b_;  // 颜色
    int frames_;
};

// 渐变效果
class GradientEffect : public AnimEffect {
public:
    GradientEffect(uint8_t r1 = 255, uint8_t g1 = 0, uint8_t b1 = 0,
                   uint8_t r2 = 0, uint8_t g2 = 0, uint8_t b2 = 255, int frames = 40)
        : r1_(r1), g1_(g1), b1_(b1), r2_(r2), g2_(g2), b2_(b2), frames_(frames) {}
    
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "Gradient"; }
    int getFrameCount() const override { return frames_; }
    
private:
    uint8_t r1_, g1_, b1_;  // 起始颜色
    uint8_t r2_, g2_, b2_;  // 结束颜色
    int frames_;
}; 

// 常亮白色照明效果
class WhiteStaticEffect : public AnimEffect {
public:
    WhiteStaticEffect(uint8_t w = 255) : w_(w) {}
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "WhiteStatic"; }
    int getFrameCount() const override { return 1; }
private:
    uint8_t w_;
};

// 色温效果
class ColorTempEffect : public AnimEffect {
public:
    ColorTempEffect(uint8_t tempIndex = 1) : tempIndex_(tempIndex) {}
    void generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) override;
    const char* getName() const override { return "ColorTemp"; }
    int getFrameCount() const override { return 1; }
    void setColorTemp(uint8_t tempIndex) { tempIndex_ = tempIndex; }
    uint8_t getColorTemp() const { return tempIndex_; }
private:
    uint8_t tempIndex_;
}; 