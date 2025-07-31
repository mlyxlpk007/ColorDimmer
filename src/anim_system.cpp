#include "anim_system.hpp"
#include "sid_rmt_sender.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// 全局实例指针，用于任务回调
static AnimSystem* g_animSystem = nullptr;

AnimSystem::AnimSystem()
    : bufferA_(nullptr), bufferB_(nullptr), updateBuffer_(nullptr), sendBuffer_(nullptr), bufferSwapped_(false),
      animFrames_(nullptr), animFrameCount_(0), currentFrame_(0), animationRunning_(false), currentEffect_(nullptr),
      animMutex_(nullptr), eventGroup_(nullptr), updateTaskHandle_(nullptr), sendTaskHandle_(nullptr)
{
}

AnimSystem::~AnimSystem() {
    stop();
    if (animMutex_) {
        vSemaphoreDelete(animMutex_);
    }
    if (eventGroup_) {
        vEventGroupDelete(eventGroup_);
    }
    if (animFrames_) {
        free(animFrames_);
    }
    if (bufferA_) {
        free(bufferA_);
    }
    if (bufferB_) {
        free(bufferB_);
    }
}

void AnimSystem::init() {
    // 创建互斥锁和事件组
    animMutex_ = xSemaphoreCreateMutex();
    eventGroup_ = xEventGroupCreate();
    
    // 设置全局实例指针
    g_animSystem = this;
    
    // 生成默认测试动画
    generateTestAnimation();
    
    Serial.println("AnimSystem initialized");
}

void AnimSystem::setEffect(AnimEffect* effect) {
    if (!effect) return;
    currentEffect_ = effect;
    int newFrameCount = effect->getFrameCount();
    int newFrameBytes = newFrameCount * FRAME_SIZE;
    // 释放旧内存
    if (animFrames_) free(animFrames_);
    if (bufferA_) free(bufferA_);
    if (bufferB_) free(bufferB_);
    // 分配新内存
    animFrames_ = (uint8_t*)malloc(newFrameBytes);
    bufferA_ = (uint8_t*)malloc(FRAME_SIZE);
    bufferB_ = (uint8_t*)malloc(FRAME_SIZE);
    updateBuffer_ = bufferA_;
    sendBuffer_ = bufferB_;
    animFrameCount_ = newFrameCount;
    currentFrame_ = 0;
    // 生成新动画
    currentEffect_->generateAnimation(animFrames_, animFrameCount_, FRAME_SIZE);
    Serial.printf("Effect set to: %s, frames=%d\n", currentEffect_->getName(), animFrameCount_);
}

void AnimSystem::start() {
    if (!animationRunning_ && currentEffect_) {
        animationRunning_ = true;
        currentFrame_ = 0;
        xEventGroupClearBits(eventGroup_, SEND_COMPLETE_BIT);
        // 初始化第一帧数据到两个buffer
        memcpy(bufferA_, &animFrames_[0], FRAME_SIZE);
        memcpy(bufferB_, &animFrames_[0], FRAME_SIZE);
        updateBuffer_ = bufferA_;
        sendBuffer_ = bufferB_;
        // 创建任务
        xTaskCreate(updateTaskEntry, "AnimUpdate", 4096, this, 3, &updateTaskHandle_);
        xTaskCreate(sendTaskEntry, "AnimSend", 8192, this, 3, &sendTaskHandle_);
        Serial.println("Animation started");
    }
}

void AnimSystem::stop() {
    if (animationRunning_) {
        animationRunning_ = false;
        
        // 删除任务
        if (updateTaskHandle_) {
            vTaskDelete(updateTaskHandle_);
            updateTaskHandle_ = nullptr;
        }
        if (sendTaskHandle_) {
            vTaskDelete(sendTaskHandle_);
            sendTaskHandle_ = nullptr;
        }
        
        Serial.println("Animation stopped");
    }
}

void AnimSystem::setAnimationData(const uint8_t* data, int frameCount) {
    if (frameCount > ANIM_FRAMES) {
        frameCount = ANIM_FRAMES;
    }
    memcpy(animFrames_, data, frameCount * FRAME_SIZE);
}

void AnimSystem::generateTestAnimation() {
    if (!currentEffect_) {
        static BreathEffect defaultEffect(255, 100, 50, 50);
        setEffect(&defaultEffect);
    } else {
        setEffect(currentEffect_);
    }
}

bool AnimSystem::isRunning() const {
    return animationRunning_;
}



void AnimSystem::updateTaskEntry(void* parameter) {
    if (g_animSystem) {
        g_animSystem->updateTask();
    }
}

void AnimSystem::sendTaskEntry(void* parameter) {
    if (g_animSystem) {
        g_animSystem->sendTask();
    }
}

void AnimSystem::updateTask() {
    Serial.println("Animation Update Task Started");
    while (1) {
        if (animationRunning_) {
            EventBits_t bits = xEventGroupWaitBits(
                eventGroup_,
                SEND_COMPLETE_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );
            if (bits & SEND_COMPLETE_BIT) {
                xSemaphoreTake(animMutex_, portMAX_DELAY);
                memcpy(updateBuffer_, &animFrames_[currentFrame_ * FRAME_SIZE], FRAME_SIZE);
                currentFrame_ = (currentFrame_ + 1) % animFrameCount_;
                xSemaphoreGive(animMutex_);
            }
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}
uint8_t* temp;
void AnimSystem::sendTask() {
    Serial.println("Animation Send Task Started");
    while (1) {
        if (animationRunning_) {
            // 发送当前发送buffer的数据
        //    Serial.println("send start-------");    
            send_data(sendBuffer_, FRAME_SIZE, 0xFFFF);
        //    Serial.println("send end-------------------------------");          
            
            // 发送完成后，在互斥锁保护下交换buffer
            xSemaphoreTake(animMutex_, portMAX_DELAY);
            temp = updateBuffer_;
            updateBuffer_ = sendBuffer_;
            sendBuffer_ = temp;
            xSemaphoreGive(animMutex_);            
            // 发送完成信号给更新任务
            xEventGroupSetBits(eventGroup_, SEND_COMPLETE_BIT);
            
            // 800Kbps芯片，可以更快的刷新率
            vTaskDelay(20 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
} 