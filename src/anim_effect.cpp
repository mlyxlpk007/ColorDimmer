#include "anim_effect.hpp"
#include "sid_rmt_sender.h"
#include <math.h>
#include <Arduino.h>
int _duv=3;
// 呼吸灯效果实现
void BreathEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;
    for (int f = 0; f < frameCount; ++f) {
        // 亮度正弦变化，范围30~255
        float t = (float)f / frameCount;
        float breath = (sin(t * 2 * 3.14159f - 3.14159f/2) + 1) / 2; // 0~1
        float intensity = 30 + breath * 225; // 30~255        
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                animFrames[idx + 0] = (uint8_t)(r_ * intensity / 255.0f);  // R
                animFrames[idx + 1] = (uint8_t)(g_ * intensity / 255.0f);  // G
                animFrames[idx + 2] = (uint8_t)(b_ * intensity / 255.0f);  // B
            }
        }
    }
    debug_printf("BreathEffect generated: %d frames, base color R=%d G=%d B=%d\n", 
                  frameCount, r_, g_, b_);
}
void set_duv(int duv)
{
    _duv=duv;
} 
void WhiteStaticEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;
    for (int f = 0; f < frameCount; ++f) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                animFrames[idx + 0] = w_; // R
                animFrames[idx + 1] = w_; // G
                animFrames[idx + 2] = w_; // B
            }
        }
    }
    Serial.printf("WhiteStaticEffect generated: %d frames, W=%d\n", frameCount, w_);
}

void ColorTempEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;    
    // 获取色温对应的RGB值（带DUV）
    uint8_t r, g, b;   
    getColorTempRGBWithDuv(tempIndex_, duvIndex_, &r, &g, &b);
    for (int f = 0; f < frameCount; ++f) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                animFrames[idx + 0] = r; // R
                animFrames[idx + 1] = g; // G
                animFrames[idx + 2] = b; // B
            }
        }
    }
    // Serial.printf("ColorTempEffect generated: %d frames, temp=%d, duv=%d, R=%d G=%d B=%d\n", 
    //               frameCount, tempIndex_, duvIndex_, r, g, b);
}

// ImageDataEffect实现
void ImageDataEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    if (!imageData_) {
        Serial.println("ImageDataEffect: No image data provided!");
        return;
    }
    
    // 从图像数据中读取帧数和延迟 (新格式：16位大端帧数 + 8位延迟)
    uint16_t dataFrameCount = (imageData_[0] << 8) | imageData_[1];  // 第1-2字节：帧数(大端)
    uint8_t frameDelay = imageData_[2];                              // 第3字节：帧延迟
    
    Serial.printf("ImageDataEffect: Loading %d frames, delay=%dms\n", 
                  dataFrameCount, frameDelay);
    
    // 检查数据是否足够
    if (dataFrameCount == 0 || dataFrameCount > frameCount) {
        Serial.printf("ImageDataEffect: Invalid frame count %d (max: %d)\n", dataFrameCount, frameCount);
        return;
    }
    
    // 复制RGB数据（跳过前3个字节的头部）
    const uint8_t* rgbData = imageData_ + 3;
    int copySize = dataFrameCount * frameSize;
    
    if (copySize > frameCount * frameSize) {
        copySize = frameCount * frameSize;
    }
    
    memcpy(animFrames, rgbData, copySize);
    
    Serial.printf("ImageDataEffect: Copied %d bytes of RGB data\n", copySize);
} 


 