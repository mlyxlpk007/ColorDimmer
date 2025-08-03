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
    
    Serial.printf("BreathEffect generated: %d frames, base color R=%d G=%d B=%d\n", 
                  frameCount, r_, g_, b_);
}
void set_duv(int duv)
{
    _duv=duv;
}   
// 彩虹效果实现
void RainbowEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;
    
    for (int f = 0; f < frameCount; ++f) {
        float hue = (float)f / frameCount * 360.0f; // 0-360度色相
        
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                
                // HSV转RGB
                float h = hue + (x + y * W) * 10.0f; // 每个像素稍微偏移
                h = fmod(h, 360.0f);
                
                float s = 1.0f, v = 0.8f;
                float c = v * s;
                float x_hsv = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
                float m = v - c;
                
                float r, g, b;
                if (h < 60) {
                    r = c; g = x_hsv; b = 0;
                } else if (h < 120) {
                    r = x_hsv; g = c; b = 0;
                } else if (h < 180) {
                    r = 0; g = c; b = x_hsv;
                } else if (h < 240) {
                    r = 0; g = x_hsv; b = c;
                } else if (h < 300) {
                    r = x_hsv; g = 0; b = c;
                } else {
                    r = c; g = 0; b = x_hsv;
                }                
                animFrames[idx + 0] = (uint8_t)((r + m) * 255);
                animFrames[idx + 1] = (uint8_t)((g + m) * 255);
                animFrames[idx + 2] = (uint8_t)((b + m) * 255);
            }
        }
    }
    
    Serial.printf("RainbowEffect generated: %d frames\n", frameCount);
}

// 闪烁效果实现
void BlinkEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;
    
    for (int f = 0; f < frameCount; ++f) {
        // 闪烁：一半帧亮，一半帧暗
        bool isOn = (f < frameCount / 2);        
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                if (isOn) {
                    animFrames[idx + 0] = r_;  // R
                    animFrames[idx + 1] = g_;  // G
                    animFrames[idx + 2] = b_;  // B
                } else {
                    animFrames[idx + 0] = 0;   // R
                    animFrames[idx + 1] = 0;   // G
                    animFrames[idx + 2] = 0;   // B
                }
            }
        }
    }
    
    Serial.printf("BlinkEffect generated: %d frames, color R=%d G=%d B=%d\n", 
                  frameCount, r_, g_, b_);
}

// 渐变效果实现
void GradientEffect::generateAnimation(uint8_t* animFrames, int frameCount, int frameSize) {
    const int W = 6, H = 6;
    
    for (int f = 0; f < frameCount; ++f) {
        float progress = (float)f / (frameCount - 1); // 0-1的进度
        
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = f * frameSize + (y * W + x) * 3;
                
                // 线性插值
                animFrames[idx + 0] = (uint8_t)(r1_ + (r2_ - r1_) * progress);  // R
                animFrames[idx + 1] = (uint8_t)(g1_ + (g2_ - g1_) * progress);  // G
                animFrames[idx + 2] = (uint8_t)(b1_ + (b2_ - b1_) * progress);  // B
            }
        }
    }
    
    Serial.printf("GradientEffect generated: %d frames, from R=%d G=%d B=%d to R=%d G=%d B=%d\n", 
                  frameCount, r1_, g1_, b1_, r2_, g2_, b2_);
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
    Serial.printf("ColorTempEffect generated: %d frames, temp=%d, duv=%d, R=%d G=%d B=%d\n", 
                  frameCount, tempIndex_, duvIndex_, r, g, b);
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