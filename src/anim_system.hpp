#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "anim_effect.hpp"

// 动画系统配置
#define FRAME_SIZE (6 * 6 * 3)  // 6x6点，每点3字节RGB
#define ANIM_FRAMES 50          // 动画帧数

// 事件组位定义
#define SEND_COMPLETE_BIT BIT0

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
    
    // 设置动画数据
    void setAnimationData(const uint8_t* data, int frameCount);
    
    // 生成测试动画（默认呼吸灯）
    void generateTestAnimation();
    
    // 运行状态查询
    bool isRunning() const;

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