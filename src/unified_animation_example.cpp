#include "unified_animation.h"
#include "dynamic_animations.h"
#include <Arduino.h>

// 全局动画管理器
AnimationManager g_animationManager;

// 演示统一动画系统
void demoUnifiedAnimationSystem() {
    Serial.println("=== 统一动画系统演示 ===");
    
    // 初始化动画管理器
    if (!g_animationManager.init()) {
        Serial.println("动画管理器初始化失败");
        return;
    }
    
    // 创建默认动画
    if (!g_animationManager.animationExists(0)) {
        Serial.println("创建默认动画...");
        g_animationManager.createDefaultAnimations();
    }
    
    // 列出所有动画
    g_animationManager.listAnimations();
    
    // 演示1: 静态动画
    Serial.println("\n--- 演示1: 静态动画 ---");
    Animation* staticAnim = g_animationManager.createStaticAnimation(255, 0, 0, "红色静态");
    g_animationManager.playAnimation(staticAnim);
    delay(2000);
    
    // 演示2: 动态动画 - 呼吸灯
    Serial.println("\n--- 演示2: 呼吸灯 ---");
    Animation* breathAnim = g_animationManager.createBreathingAnimation(0, 255, 0, 30);
    g_animationManager.playAnimation(breathAnim);
    delay(3000);
    
    // 演示3: 动态动画 - 彩虹
    Serial.println("\n--- 演示3: 彩虹效果 ---");
    Animation* rainbowAnim = g_animationManager.createRainbowAnimation(60);
    g_animationManager.playAnimation(rainbowAnim);
    delay(3000);
    
    // 演示4: 预设动画
    Serial.println("\n--- 演示4: 预设动画 ---");
    if (g_animationManager.animationExists(0)) {
        Animation* presetAnim = g_animationManager.loadAnimation(0);
        g_animationManager.playAnimation(presetAnim);
        delay(3000);
    }
    
    // 演示5: 波浪动画
    Serial.println("\n--- 演示5: 波浪效果 ---");
    WaveAnimation* waveAnim = new WaveAnimation(255, 0, 255, 60);
    waveAnim->setWaveSpeed(1.5f);
    waveAnim->setWaveAmplitude(0.7f);
    g_animationManager.playAnimation(waveAnim);
    delay(3000);
    
    // 演示6: 旋转动画
    Serial.println("\n--- 演示6: 旋转效果 ---");
    RotateAnimation* rotateAnim = new RotateAnimation(0, 255, 255, 36);
    rotateAnim->setRotationSpeed(2.0f);
    g_animationManager.playAnimation(rotateAnim);
    delay(3000);
    
    // 演示7: 随机闪烁
    Serial.println("\n--- 演示7: 随机闪烁 ---");
    RandomBlinkAnimation* randomAnim = new RandomBlinkAnimation(255, 255, 0, 50);
    randomAnim->setDensity(0.4f);
    g_animationManager.playAnimation(randomAnim);
    delay(3000);
    
    // 演示8: 保存自定义动画
    Serial.println("\n--- 演示8: 保存自定义动画 ---");
    PulseAnimation* pulseAnim = new PulseAnimation(255, 100, 50, 40);
    pulseAnim->setPulseCount(5);
    if (g_animationManager.saveAnimation(10, pulseAnim)) {
        Serial.println("自定义动画保存成功");
    }
    
    // 演示9: 加载保存的动画
    Serial.println("\n--- 演示9: 加载保存的动画 ---");
    Animation* loadedAnim = g_animationManager.loadAnimation(10);
    if (loadedAnim) {
        g_animationManager.playAnimation(loadedAnim);
        delay(3000);
    }
    
    // 停止动画
    g_animationManager.stopAnimation();
    Serial.println("动画演示完成");
    
    // 清理内存
    delete staticAnim;
    delete breathAnim;
    delete rainbowAnim;
    delete waveAnim;
    delete rotateAnim;
    delete randomAnim;
    delete pulseAnim;
    delete loadedAnim;
}

// 动画播放任务
void animationPlayTask(void* parameter) {
    Animation* currentAnim = nullptr;
    int currentFrame = 0;
    uint32_t lastFrameTime = 0;
    
    while (true) {
        Animation* anim = g_animationManager.getCurrentAnimation();
        
        if (anim && anim != currentAnim) {
            // 新动画开始
            currentAnim = anim;
            currentFrame = 0;
            lastFrameTime = millis();
            Serial.printf("开始播放动画: %s\n", anim->getName());
        }
        
        if (currentAnim) {
            uint32_t currentTime = millis();
            int frameDelay = currentAnim->getFrameDelay();
            
            if (currentTime - lastFrameTime >= frameDelay) {
                // 生成当前帧
                uint8_t frameBuffer[108];
                currentAnim->generateFrame(frameBuffer, currentFrame);
                
                // 发送到LED
                // 这里需要调用你的LED发送函数
                // send_data(frameBuffer, 108, 0xFFFF);
                
                // 更新帧计数
                currentFrame++;
                if (currentFrame >= currentAnim->getFrameCount()) {
                    currentFrame = 0;  // 循环播放
                }
                
                lastFrameTime = currentTime;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延迟
    }
}

// 初始化动画系统
bool initUnifiedAnimationSystem() {
    Serial.println("初始化统一动画系统...");
    
    if (!g_animationManager.init()) {
        Serial.println("动画管理器初始化失败");
        return false;
    }
    
    // 创建动画播放任务
    xTaskCreate(
        animationPlayTask,
        "AnimationPlay",
        4096,
        nullptr,
        2,
        nullptr
    );
    
    Serial.println("统一动画系统初始化完成");
    return true;
}

// 切换动画效果
void switchAnimationEffect(uint8_t effectType, uint8_t sceneId = 0) {
    Animation* newAnim = nullptr;
    
    switch (effectType) {
        case 0:  // 静态红色
            newAnim = g_animationManager.createStaticAnimation(255, 0, 0, "红色");
            break;
        case 1:  // 静态绿色
            newAnim = g_animationManager.createStaticAnimation(0, 255, 0, "绿色");
            break;
        case 2:  // 静态蓝色
            newAnim = g_animationManager.createStaticAnimation(0, 0, 255, "蓝色");
            break;
        case 3:  // 呼吸灯
            newAnim = g_animationManager.createBreathingAnimation(255, 100, 50, 30);
            break;
        case 4:  // 彩虹
            newAnim = g_animationManager.createRainbowAnimation(60);
            break;
        case 5:  // 闪烁
            newAnim = g_animationManager.createBlinkAnimation(255, 255, 255, 20);
            break;
        case 6:  // 渐变
            newAnim = g_animationManager.createGradientAnimation(255, 0, 0, 0, 0, 255, 40);
            break;
        case 7:  // 波浪
            newAnim = new WaveAnimation(255, 0, 255, 60);
            break;
        case 8:  // 旋转
            newAnim = new RotateAnimation(0, 255, 255, 36);
            break;
        case 9:  // 脉冲
            newAnim = new PulseAnimation(255, 200, 100, 40);
            break;
        case 10: // 随机闪烁
            newAnim = new RandomBlinkAnimation(255, 255, 0, 50);
            break;
        case 11: // 预设动画
            if (g_animationManager.animationExists(sceneId)) {
                newAnim = g_animationManager.loadAnimation(sceneId);
            }
            break;
        default:
            Serial.printf("未知的动画类型: %d\n", effectType);
            return;
    }
    
    if (newAnim) {
        g_animationManager.playAnimation(newAnim);
        Serial.printf("切换到动画: %s\n", newAnim->getName());
    }
}

// 保存当前动画
void saveCurrentAnimation(uint8_t sceneId) {
    Animation* currentAnim = g_animationManager.getCurrentAnimation();
    if (currentAnim && currentAnim->canBeSaved()) {
        if (g_animationManager.saveAnimation(sceneId, currentAnim)) {
            Serial.printf("动画保存到场景 %d 成功\n", sceneId);
        } else {
            Serial.printf("动画保存到场景 %d 失败\n", sceneId);
        }
    } else {
        Serial.println("当前动画无法保存");
    }
} 