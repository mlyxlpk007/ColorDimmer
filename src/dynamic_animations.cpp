#include "dynamic_animations.h"
#include <Arduino.h>
#include <cmath>

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

// ==================== CandleFlameAnimation 实现 ====================

CandleFlameAnimation::CandleFlameAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount)
    : DynamicAnimation("CandleFlame", frameCount, 50), r_(r), g_(g), b_(b), 
      flameIntensity_(0.8f), windEffect_(0.3f) {
}

void CandleFlameAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    // 清空缓冲区
    memset(buffer, 0, 108);
    
    // 计算时间因子
    float time = frameIndex * 0.2f;  // 控制晃动速度
    
    // 烛火中心位置（6x6矩阵的中心）
    int centerX = 3, centerY = 3;
    
    // 生成烛火形状和晃动效果
    for (int led = 0; led < 36; led++) {
        int row = led / 6;
        int col = led % 6;
        
        // 计算到中心的距离
        float dx = (col - centerX) * 0.5f;
        float dy = (row - centerY) * 0.5f;
        float distance = sqrt(dx*dx + dy*dy);
        
        // 基础烛火形状（椭圆形）
        float flameShape = 1.0f - (distance * distance) / 4.0f;
        if (flameShape < 0) flameShape = 0;
        
        // 添加晃动效果（水平方向）
        float windOffset = sin(time + distance * 0.5f) * windEffect_;
        float adjustedX = dx + windOffset;
        float adjustedDistance = sqrt(adjustedX*adjustedX + dy*dy);
        float windShape = 1.0f - (adjustedDistance * adjustedDistance) / 4.0f;
        if (windShape < 0) windShape = 0;
        
        // 组合形状
        float finalShape = (flameShape + windShape) * 0.5f;
        
        // 添加闪烁效果
        float flicker = 0.8f + 0.2f * sin(time * 3.0f + distance * 2.0f);
        
        // 计算最终强度
        float intensity = finalShape * flicker * flameIntensity_;
        if (intensity > 1.0f) intensity = 1.0f;
        if (intensity < 0.0f) intensity = 0.0f;
        
        // 应用颜色和强度
        int baseIndex = led * 3;
        buffer[baseIndex + 0] = (uint8_t)(r_ * intensity);  // R
        buffer[baseIndex + 1] = (uint8_t)(g_ * intensity);  // G
        buffer[baseIndex + 2] = (uint8_t)(b_ * intensity);  // B
    }
}

void CandleFlameAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
}

void CandleFlameAnimation::setFlameIntensity(float intensity) {
    if (intensity >= 0.0f && intensity <= 1.0f) {
        flameIntensity_ = intensity;
    }
}

void CandleFlameAnimation::setWindEffect(float wind) {
    if (wind >= 0.0f && wind <= 1.0f) {
        windEffect_ = wind;
    }
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

// ==================== 烛火数据生成函数 ====================

// 生成60帧烛火晃动数据
void generateCandleFlameData(uint8_t* buffer, int frameCount, uint8_t r, uint8_t g, uint8_t b) {
    if (frameCount < 1) return;
    
    // 烛火参数
    float flameIntensity = 0.8f;    // 火焰强度
    float windEffect = 0.3f;        // 风效果强度
    float flickerIntensity = 0.2f;  // 闪烁强度
    
    for (int frame = 0; frame < frameCount; frame++) {
        // 计算时间因子
        float time = frame * 0.2f;  // 控制晃动速度
        
        // 烛火中心位置（6x6矩阵的中心）
        int centerX = 3, centerY = 3;
        
        // 生成烛火形状和晃动效果
        for (int led = 0; led < 36; led++) {
            int row = led / 6;
            int col = led % 6;
            
            // 计算到中心的距离
            float dx = (col - centerX) * 0.5f;
            float dy = (row - centerY) * 0.5f;
            float distance = sqrt(dx*dx + dy*dy);
            
            // 基础烛火形状（椭圆形）
            float flameShape = 1.0f - (distance * distance) / 4.0f;
            if (flameShape < 0) flameShape = 0;
            
            // 添加晃动效果（水平方向）
            float windOffset = sin(time + distance * 0.5f) * windEffect;
            float adjustedX = dx + windOffset;
            float adjustedDistance = sqrt(adjustedX*adjustedX + dy*dy);
            float windShape = 1.0f - (adjustedDistance * adjustedDistance) / 4.0f;
            if (windShape < 0) windShape = 0;
            
            // 组合形状
            float finalShape = (flameShape + windShape) * 0.5f;
            
            // 添加闪烁效果
            float flicker = 0.8f + flickerIntensity * sin(time * 3.0f + distance * 2.0f);
            
            // 计算最终强度
            float intensity = finalShape * flicker * flameIntensity;
            if (intensity > 1.0f) intensity = 1.0f;
            if (intensity < 0.0f) intensity = 0.0f;
            
            // 应用颜色和强度
            int baseIndex = frame * 108 + led * 3;
            buffer[baseIndex + 0] = (uint8_t)(r * intensity);  // R
            buffer[baseIndex + 1] = (uint8_t)(g * intensity);  // G
            buffer[baseIndex + 2] = (uint8_t)(b * intensity);  // B
        }
    }
}

// 生成优化的烛火数据（更真实的火焰效果）
void generateOptimizedCandleFlameData(uint8_t* buffer, int frameCount, uint8_t r, uint8_t g, uint8_t b) {
    if (frameCount < 1) return;
    
    // 火焰参数
    float baseIntensity = 0.9f;     // 基础强度
    float windStrength = 0.4f;      // 风力强度
    float flickerRate = 0.15f;      // 闪烁频率
    float flameHeight = 0.8f;       // 火焰高度因子
    
    for (int frame = 0; frame < frameCount; frame++) {
        float time = frame * 0.15f;  // 时间因子
        
        for (int led = 0; led < 36; led++) {
            int row = led / 6;
            int col = led % 6;
            
            // 计算相对位置
            float x = (col - 2.5f) / 2.5f;  // -1.0 到 1.0
            float y = (row - 2.5f) / 2.5f;  // -1.0 到 1.0
            
            // 基础火焰形状（锥形）
            float flameBase = 1.0f - (x * x) / 1.5f;
            if (flameBase < 0) flameBase = 0;
            
            // 高度衰减
            float heightFactor = 1.0f - (y + 1.0f) * flameHeight;
            if (heightFactor < 0) heightFactor = 0;
            
            // 风力效果（水平晃动）
            float windOffset = sin(time + y * 2.0f) * windStrength * (1.0f - y * 0.5f);
            float adjustedX = x + windOffset;
            float windShape = 1.0f - (adjustedX * adjustedX) / 1.5f;
            if (windShape < 0) windShape = 0;
            
            // 组合形状
            float shape = (flameBase + windShape) * 0.5f * heightFactor;
            
            // 闪烁效果
            float flicker = 0.85f + flickerRate * sin(time * 4.0f + x * 3.0f + y * 2.0f);
            
            // 最终强度
            float intensity = shape * flicker * baseIntensity;
            if (intensity > 1.0f) intensity = 1.0f;
            if (intensity < 0.0f) intensity = 0.0f;
            
            // 写入数据
            int baseIndex = frame * 108 + led * 3;
            buffer[baseIndex + 0] = (uint8_t)(r * intensity);  // R
            buffer[baseIndex + 1] = (uint8_t)(g * intensity);  // G
            buffer[baseIndex + 2] = (uint8_t)(b * intensity);  // B
        }
    }
} 