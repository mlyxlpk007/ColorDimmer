#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "anim_effect.hpp"

// 动画系统配置
#define FRAME_SIZE (6 * 6 * 3)  // 6x6点，每点3字节RGB
#define ANIM_FRAMES 100         // 动画帧数，增加到100帧

// 事件组位定义
#define SEND_COMPLETE_BIT BIT0
#define UPDATE_READY_BIT  BIT1

class AnimSystem {
public:
    AnimSystem();
    ~AnimSystem();
    
    // 初始化和控制
    void init();
    void start();
    void stop();
    
    // 设置动画效果（多态）
    void setEffect(AnimEffect* effect);
    
    // 更新当前效果（不重启动画）
    void updateCurrentEffect();
    
    // 设置动画数据
    void setAnimationData(const uint8_t* data, int frameCount);
    
    // 生成测试动画（默认呼吸灯）
    void generateTestAnimation();
    
    // 运行状态查询
    bool isRunning() const;

    // 配置：静态变化是否使用过渡、过渡帧数
    void setStaticTransitionEnabled(bool enabled) { staticTransitionEnabled_ = enabled; }
    void setStaticTransitionFrames(int frames) { if (frames > 0) staticTransitionFrames_ = frames; }
    void setGammaBlendEnabled(bool enabled) { gammaBlendEnabled_ = enabled; }

    // 业务：色温调整（可选择是否过渡）
    void updateColorTemp(uint8_t tempIndex, uint8_t duvIndex, bool useTransition);

    // 业务：亮度设置（当与当前亮度差值>10时，内部20帧平滑过渡）
    void setBrightnessSmooth(uint8_t targetBrightness);

private:
    // 双buffer
    uint8_t* bufferA_ = nullptr;
    uint8_t* bufferB_ = nullptr;
    uint8_t* updateBuffer_ = nullptr;
    uint8_t* sendBuffer_ = nullptr;
    bool bufferSwapped_ = false;
    
    // 动画数据
    uint8_t* animFrames_ = nullptr;
    int animFrameCount_ = 0;
    int currentFrame_ = 0;
    bool animationRunning_ = false;
    
    // 动画效果
    AnimEffect* currentEffect_ = nullptr;
    int frameDelayMs_ = 20;
    // 静态动画切换的过渡状态
    bool transitionActive_ = false;
    AnimEffect* transitionTarget_ = nullptr;
    bool staticTransitionEnabled_ = true;
    int staticTransitionFrames_ = 60;
    bool hasPendingColorTemp_ = false;
    uint8_t pendingTempIndex_ = 0;
    uint8_t pendingDuvIndex_ = 0;
    bool gammaBlendEnabled_ = true;
    // 亮度冻结控制（避免色温过渡时亮度同步变化导致跳变）
    bool brightnessFreezeActive_ = false;
    uint8_t lastUsedBrightness_ = 100;
    uint8_t pendingBrightness_ = 0;
    
    // 亮度控制
    uint8_t brightness_ = 100;
    
    // 同步和任务
    SemaphoreHandle_t animMutex_;
    EventGroupHandle_t eventGroup_;  // 事件组用于任务同步
    TaskHandle_t updateTaskHandle_;
    TaskHandle_t sendTaskHandle_;
    
    // 任务函数
    static void updateTaskEntry(void* parameter);
    static void sendTaskEntry(void* parameter);
    void updateTask();
    void sendTask();
}; 