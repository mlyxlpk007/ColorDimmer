#include "dynamic_animations.h"
#include <Arduino.h>
#include <cmath>

// ==================== BreathingAnimation 实现 ====================

BreathingAnimation::BreathingAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("Breathing", frameCount, 50), r_(r), g_(g), b_(b) {
}

void BreathingAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    // 使用正弦函数创建呼吸效果
    float brightness = (sin(frameIndex * 2 * PI / frameCount_) + 1) / 2.0f;
    uint8_t intensity = (uint8_t)(brightness * 255);
    
    for (int led = 0; led < 36; led++) {
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = (uint8_t)(r_ * intensity / 255.0f);
        buffer[baseIndex + 1] = (uint8_t)(g_ * intensity / 255.0f);
        buffer[baseIndex + 2] = (uint8_t)(b_ * intensity / 255.0f);
    }
}

void BreathingAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

void BreathingAnimation::getColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = r_;
    g = g_;
    b = b_;
}

// ==================== RainbowAnimation 实现 ====================

RainbowAnimation::RainbowAnimation(int frameCount)
    : DynamicAnimation("Rainbow", frameCount, 30), saturation_(1.0f), value_(0.8f) {
}

void RainbowAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    float hue = (float)frameIndex / frameCount_ * 360.0f;
    
    for (int led = 0; led < 36; led++) {
        // 为每个LED添加不同的色相偏移
        float ledHue = hue + (led * 10.0f);
        ledHue = fmod(ledHue, 360.0f);
        
        uint8_t r, g, b;
        hsvToRgb(ledHue, saturation_, value_, r, g, b);
        
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = r;
        buffer[baseIndex + 1] = g;
        buffer[baseIndex + 2] = b;
    }
}

void RainbowAnimation::hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float rf, gf, bf;
    if (h < 60) {
        rf = c; gf = x; bf = 0;
    } else if (h < 120) {
        rf = x; gf = c; bf = 0;
    } else if (h < 180) {
        rf = 0; gf = c; bf = x;
    } else if (h < 240) {
        rf = 0; gf = x; bf = c;
    } else if (h < 300) {
        rf = x; gf = 0; bf = c;
    } else {
        rf = c; gf = 0; bf = x;
    }
    
    r = (uint8_t)((rf + m) * 255);
    g = (uint8_t)((gf + m) * 255);
    b = (uint8_t)((bf + m) * 255);
}

// ==================== BlinkAnimation 实现 ====================

BlinkAnimation::BlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("Blink", frameCount, 100), r_(r), g_(g), b_(b), dutyCycle_(0.5f) {
}

void BlinkAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    bool isOn = (frameIndex < frameCount_ * dutyCycle_);
    
    for (int led = 0; led < 36; led++) {
        int baseIndex = led * 3;
        if (isOn) {
            buffer[baseIndex + 0] = r_;
            buffer[baseIndex + 1] = g_;
            buffer[baseIndex + 2] = b_;
        } else {
            buffer[baseIndex + 0] = 0;
            buffer[baseIndex + 1] = 0;
            buffer[baseIndex + 2] = 0;
        }
    }
}

void BlinkAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

// ==================== GradientAnimation 实现 ====================

GradientAnimation::GradientAnimation(uint8_t r1, uint8_t g1, uint8_t b1, 
                                     uint8_t r2, uint8_t g2, uint8_t b2, int frameCount)
    : DynamicAnimation("Gradient", frameCount, 40), 
      r1_(r1), g1_(g1), b1_(b1), r2_(r2), g2_(g2), b2_(b2) {
}

void GradientAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    float progress = (float)frameIndex / (frameCount_ - 1);
    
    for (int led = 0; led < 36; led++) {
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = (uint8_t)(r1_ + (r2_ - r1_) * progress);
        buffer[baseIndex + 1] = (uint8_t)(g1_ + (g2_ - g1_) * progress);
        buffer[baseIndex + 2] = (uint8_t)(b1_ + (b2_ - b1_) * progress);
    }
}

void GradientAnimation::setStartColor(uint8_t r, uint8_t g, uint8_t b) {
    r1_ = r;
    g1_ = g;
    b1_ = b;
}

void GradientAnimation::setEndColor(uint8_t r, uint8_t g, uint8_t b) {
    r2_ = r;
    g2_ = g;
    b2_ = b;
}

// ==================== WaveAnimation 实现 ====================

WaveAnimation::WaveAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("Wave", frameCount, 50), r_(r), g_(g), b_(b), 
      waveSpeed_(1.0f), waveAmplitude_(0.5f) {
}

void WaveAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    for (int led = 0; led < 36; led++) {
        // 计算LED在6x6矩阵中的位置
        int row = led / 6;
        int col = led % 6;
        
        // 创建波浪效果
        float wave = sin((row + col + frameIndex * waveSpeed_) * 0.5f) * waveAmplitude_ + 0.5f;
        uint8_t intensity = (uint8_t)(wave * 255);
        
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = (uint8_t)(r_ * intensity / 255.0f);
        buffer[baseIndex + 1] = (uint8_t)(g_ * intensity / 255.0f);
        buffer[baseIndex + 2] = (uint8_t)(b_ * intensity / 255.0f);
    }
}

void WaveAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

// ==================== RotateAnimation 实现 ====================

RotateAnimation::RotateAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("Rotate", frameCount, 100), r_(r), g_(g), b_(b), rotationSpeed_(1.0f) {
}

void RotateAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    // 清空缓冲区
    memset(buffer, 0, 108);
    
    // 计算旋转角度
    float angle = frameIndex * rotationSpeed_ * 10.0f;
    int centerX = 3, centerY = 3;  // 6x6矩阵的中心
    
    // 在中心位置放置一个LED
    int ledIndex = centerY * 6 + centerX;
    int baseIndex = ledIndex * 3;
    buffer[baseIndex + 0] = r_;
    buffer[baseIndex + 1] = g_;
    buffer[baseIndex + 2] = b_;
    
    // 在旋转位置放置另一个LED
    float rad = angle * PI / 180.0f;
    int x = centerX + (int)(2 * cos(rad));
    int y = centerY + (int)(2 * sin(rad));
    
    if (x >= 0 && x < 6 && y >= 0 && y < 6) {
        ledIndex = y * 6 + x;
        baseIndex = ledIndex * 3;
        buffer[baseIndex + 0] = r_;
        buffer[baseIndex + 1] = g_;
        buffer[baseIndex + 2] = b_;
    }
}

void RotateAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

// ==================== PulseAnimation 实现 ====================

PulseAnimation::PulseAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("Pulse", frameCount, 80), r_(r), g_(g), b_(b), pulseCount_(3) {
}

void PulseAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    // 创建脉冲效果
    float pulse = sin(frameIndex * pulseCount_ * 2 * PI / frameCount_);
    uint8_t intensity = (uint8_t)((pulse + 1) * 127.5f);
    
    for (int led = 0; led < 36; led++) {
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = (uint8_t)(r_ * intensity / 255.0f);
        buffer[baseIndex + 1] = (uint8_t)(g_ * intensity / 255.0f);
        buffer[baseIndex + 2] = (uint8_t)(b_ * intensity / 255.0f);
    }
}

void PulseAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

// ==================== RandomBlinkAnimation 实现 ====================

RandomBlinkAnimation::RandomBlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("RandomBlink", frameCount, 60), r_(r), g_(g), b_(b), 
      density_(0.3f), randomSeed_(0) {
    reset();
}

void RandomBlinkAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    // 清空缓冲区
    memset(buffer, 0, 108);
    
    // 为每个LED生成随机状态
    for (int led = 0; led < 36; led++) {
        // 使用简单的伪随机数生成器
        randomSeed_ = (randomSeed_ * 1103515245 + 12345) & 0x7fffffff;
        float randomValue = (float)(randomSeed_ & 0xFFFF) / 65535.0f;
        
        if (randomValue < density_) {
            int baseIndex = led * 3;
            buffer[baseIndex + 0] = r_;
            buffer[baseIndex + 1] = g_;
            buffer[baseIndex + 2] = b_;
        }
    }
}

void RandomBlinkAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

void RandomBlinkAnimation::reset() {
    randomSeed_ = millis();  // 使用当前时间作为随机种子
} 