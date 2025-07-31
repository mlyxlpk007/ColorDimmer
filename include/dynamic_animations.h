#pragma once
#include "unified_animation.h"
#include <Arduino.h>

// 呼吸灯动画
class BreathingAnimation : public DynamicAnimation {
public:
    BreathingAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 30);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void getColor(uint8_t& r, uint8_t& g, uint8_t& b) const;
    
private:
    uint8_t r_, g_, b_;
};

// 彩虹动画
class RainbowAnimation : public DynamicAnimation {
public:
    RainbowAnimation(int frameCount = 60);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setSaturation(float saturation) { saturation_ = saturation; }
    void setValue(float value) { value_ = value; }
    
private:
    float saturation_;
    float value_;
    
    // HSV转RGB辅助函数
    void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b);
};

// 闪烁动画
class BlinkAnimation : public DynamicAnimation {
public:
    BlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 20);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setDutyCycle(float dutyCycle) { dutyCycle_ = dutyCycle; }
    
private:
    uint8_t r_, g_, b_;
    float dutyCycle_;  // 占空比 (0.0-1.0)
};

// 渐变动画
class GradientAnimation : public DynamicAnimation {
public:
    GradientAnimation(uint8_t r1, uint8_t g1, uint8_t b1, 
                      uint8_t r2, uint8_t g2, uint8_t b2, int frameCount = 40);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setStartColor(uint8_t r, uint8_t g, uint8_t b);
    void setEndColor(uint8_t r, uint8_t g, uint8_t b);
    
private:
    uint8_t r1_, g1_, b1_;  // 起始颜色
    uint8_t r2_, g2_, b2_;  // 结束颜色
};

// 波浪动画
class WaveAnimation : public DynamicAnimation {
public:
    WaveAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 60);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setWaveSpeed(float speed) { waveSpeed_ = speed; }
    void setWaveAmplitude(float amplitude) { waveAmplitude_ = amplitude; }
    
private:
    uint8_t r_, g_, b_;
    float waveSpeed_;
    float waveAmplitude_;
};

// 旋转动画
class RotateAnimation : public DynamicAnimation {
public:
    RotateAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 36);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setRotationSpeed(float speed) { rotationSpeed_ = speed; }
    
private:
    uint8_t r_, g_, b_;
    float rotationSpeed_;
};

// 脉冲动画
class PulseAnimation : public DynamicAnimation {
public:
    PulseAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 40);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPulseCount(int count) { pulseCount_ = count; }
    
private:
    uint8_t r_, g_, b_;
    int pulseCount_;
};

// 随机闪烁动画
class RandomBlinkAnimation : public DynamicAnimation {
public:
    RandomBlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 50);
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setDensity(float density) { density_ = density; }
    void reset() override;
    
private:
    uint8_t r_, g_, b_;
    float density_;  // 闪烁密度 (0.0-1.0)
    uint32_t randomSeed_;
}; 