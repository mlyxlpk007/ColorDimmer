#include "anim_system.hpp"
#include "sid_rmt_sender.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>







// 全局实例指针，用于任务回调
static AnimSystem* g_animSystem = nullptr;

// 导出到发送模块的亮度冻结标志与冻结值
bool g_anim_brightnessFreezeActive = false;
uint8_t g_anim_frozenBrightness = 100;

static inline float toLinear(float c) {
    // 近似sRGB->Linear，简化：pow(c, 2.2)
    return powf(c, 2.2f);
}
static inline float toSRGB(float c) {
    // 线性->sRGB
    return powf(c, 1.0f / 2.2f);
}

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
    
    // 检查创建是否成功
    if (!animMutex_) {
        debug_println("ERROR: Failed to create animMutex_");
    }
    if (!eventGroup_) {
        debug_println("ERROR: Failed to create eventGroup_");
    }
    
    // 设置全局实例指针
    g_animSystem = this;    
    // 生成默认测试动画
    generateTestAnimation();    
    debug_printf("AnimSystem initialized - mutex: %p, eventGroup: %p\n", animMutex_, eventGroup_);
}

void AnimSystem::updateColorTemp(uint8_t tempIndex, uint8_t duvIndex, bool useTransition) {
    // 仅在当前效果是色温（静态）时进行过渡（无RTTI版）
    if (!(currentEffect_ && currentEffect_->isColorTemp())) {
        debug_println("updateColorTemp: current effect is not ColorTempEffect.");
        return;
    }
    ColorTempEffect* colorEff = (ColorTempEffect*)currentEffect_;

    if (!(useTransition && staticTransitionEnabled_)) {
        // 直接切换
        xSemaphoreTake(animMutex_, portMAX_DELAY);
        colorEff->setColorTemp(tempIndex);
        colorEff->setDuvIndex(duvIndex);
        colorEff->generateAnimation(animFrames_, 1, FRAME_SIZE);
        animFrameCount_ = 1;
        currentFrame_ = 0;
        xSemaphoreGive(animMutex_);

        // 刷新两个buffer
        if (eventGroup_) {
            xEventGroupClearBits(eventGroup_, SEND_COMPLETE_BIT | UPDATE_READY_BIT);
        }
        memcpy(bufferA_, &animFrames_[0], FRAME_SIZE);
        memcpy(bufferB_, &animFrames_[0], FRAME_SIZE);
        if (eventGroup_) {
            xEventGroupSetBits(eventGroup_, UPDATE_READY_BIT);
        }
        // Serial.printf("ColorTemp set directly: temp=%u duv=%u\n", tempIndex, duvIndex);
        return;
    }

    // 冻结当前亮度，避免过渡期间亮度变化导致可见跳变
    lastUsedBrightness_ = get_brightness();
    brightnessFreezeActive_ = true;
    g_anim_brightnessFreezeActive = true;
    g_anim_frozenBrightness = lastUsedBrightness_;

    // 过渡：从“当前显示颜色”到“目标颜色”，做gamma校正的插值
    uint8_t curR8=0, curG8=0, curB8=0;
    // 从当前sendBuffer_拿到正在显示的颜色（任一像素；本模式单色）
    xSemaphoreTake(animMutex_, portMAX_DELAY);
    if (sendBuffer_) {
        curR8 = sendBuffer_[0];
        curG8 = sendBuffer_[1];
        curB8 = sendBuffer_[2];
    }
    xSemaphoreGive(animMutex_);

    // 目标颜色（不应用亮度，这里仅生成原始RGB，亮度在send_data里统一应用）
    uint8_t endR8, endG8, endB8;
    getColorTempRGBWithDuv(tempIndex, duvIndex, &endR8, &endG8, &endB8);

    // 转为0..1
    float curR = curR8 / 255.0f, curG = curG8 / 255.0f, curB = curB8 / 255.0f;
    float endR = endR8 / 255.0f, endG = endG8 / 255.0f, endB = endB8 / 255.0f;

    // 转到线性空间
    if (gammaBlendEnabled_) {
        curR = toLinear(curR); curG = toLinear(curG); curB = toLinear(curB);
        endR = toLinear(endR); endG = toLinear(endG); endB = toLinear(endB);
    }

    int frames = staticTransitionFrames_;
    xSemaphoreTake(animMutex_, portMAX_DELAY);
    if (animFrames_) free(animFrames_);
    animFrames_ = (uint8_t*)malloc(frames * FRAME_SIZE);
    animFrameCount_ = frames;
    currentFrame_ = 0;
    frameDelayMs_ = 20; // 固定过渡速度

    for (int f = 0; f < frames; ++f) {
        float t = (frames > 1) ? ((float)f / (float)(frames - 1)) : 1.0f;
        float rLin = curR + (endR - curR) * t;
        float gLin = curG + (endG - curG) * t;
        float bLin = curB + (endB - curB) * t;
        if (gammaBlendEnabled_) {
            rLin = toSRGB(rLin);
            gLin = toSRGB(gLin);
            bLin = toSRGB(bLin);
        }
        uint8_t r = (uint8_t)lroundf(rLin * 255.0f);
        uint8_t g = (uint8_t)lroundf(gLin * 255.0f);
        uint8_t b = (uint8_t)lroundf(bLin * 255.0f);
        uint8_t* dst = &animFrames_[f * FRAME_SIZE];
        for (int i = 0; i < FRAME_SIZE; i += 3) {
            dst[i + 0] = r;
            dst[i + 1] = g;
            dst[i + 2] = b;
        }
    }

    // 标记过渡中，并在过渡结束后应用目标色温
    transitionActive_ = true;
    hasPendingColorTemp_ = true;
    pendingTempIndex_ = tempIndex;
    pendingDuvIndex_ = duvIndex;

    // 启动过渡：刷新buffer，触发更新
    memcpy(bufferA_, &animFrames_[0], FRAME_SIZE);
    memcpy(bufferB_, &animFrames_[0], FRAME_SIZE);
    xSemaphoreGive(animMutex_);

    if (eventGroup_) {
        xEventGroupClearBits(eventGroup_, SEND_COMPLETE_BIT | UPDATE_READY_BIT);
        xEventGroupSetBits(eventGroup_, UPDATE_READY_BIT);
    }

    debug_printf("ColorTemp transitioning (gamma %s, brightness freeze %u%%): cur=(%u,%u,%u) -> target=(%u,%u,%u), frames=%d\n",
                  gammaBlendEnabled_?"on":"off", lastUsedBrightness_, curR8, curG8, curB8, endR8, endG8, endB8, frames);
}

void AnimSystem::setEffect(AnimEffect* effect) {
    if (!effect) return;

    bool wantTransition = false;
    if (animationRunning_ && currentEffect_ && currentEffect_->getFrameCount() == 1 && effect->getFrameCount() == 1) {
        // 运行中且静态->静态，走20帧过渡
        wantTransition = true;
        transitionTarget_ = effect;
    }

    // 释放旧内存
    if (animFrames_) free(animFrames_);
    if (bufferA_) free(bufferA_);
    if (bufferB_) free(bufferB_);

    // 分配新内存（静态过渡需要20帧容量，否则按目标帧数）
    int allocateFrames = wantTransition ? staticTransitionFrames_ : effect->getFrameCount();
    int allocateBytes = allocateFrames * FRAME_SIZE;
    animFrames_ = (uint8_t*)malloc(allocateBytes);
    bufferA_ = (uint8_t*)malloc(FRAME_SIZE);
    bufferB_ = (uint8_t*)malloc(FRAME_SIZE);
    updateBuffer_ = bufferA_;
    sendBuffer_ = bufferB_;
    animFrameCount_ = allocateFrames;
    currentFrame_ = 0;

    if (wantTransition) {
        // 不立刻切换currentEffect_，等待过渡完成后再切
        frameDelayMs_ = 20; // 过渡速度（帧间隔可保留20ms）
        debug_printf("Effect set (pending transition): %s -> %s\n", currentEffect_->getName(), transitionTarget_->getName());
    } else {
        currentEffect_ = effect;
        frameDelayMs_ = currentEffect_->getFrameDelay();
        // 生成新动画
        currentEffect_->generateAnimation(animFrames_, animFrameCount_, FRAME_SIZE);

        // 直接播放真实数据，不做首尾平滑处理
        debug_printf("Effect set to: %s, frames=%d, delay=%dms\n", currentEffect_->getName(), animFrameCount_, frameDelayMs_);
    }
}

void AnimSystem::updateCurrentEffect() {
    if (!currentEffect_ || !animationRunning_) return;
    
    // 等待发送任务完成当前发送
    if (eventGroup_) {
        xEventGroupWaitBits(eventGroup_, SEND_COMPLETE_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(100));
    }
    
    // 在互斥锁保护下重新生成动画数据
    xSemaphoreTake(animMutex_, portMAX_DELAY);

    // 如果当前效果和目标效果都为静态（帧数=1），构建20帧跨淡入过渡
    bool doTransition = (currentEffect_ && currentEffect_->getFrameCount() == 1 && transitionTarget_ && transitionTarget_->getFrameCount() == 1);
    if (doTransition) {
        // 准备起止帧
        uint8_t start[FRAME_SIZE];
        uint8_t end[FRAME_SIZE];
        // 当前静态帧存到 start
        memcpy(start, &animFrames_[0], FRAME_SIZE);
        // 让目标效果生成其静态帧到临时缓冲
        uint8_t tmpFrames[FRAME_SIZE];
        transitionTarget_->generateAnimation(tmpFrames, 1, FRAME_SIZE);
        memcpy(end, tmpFrames, FRAME_SIZE);
        // 生成20帧过渡到 animFrames_
        const int transitionFrames = staticTransitionFrames_;
        animFrameCount_ = transitionFrames;
        for (int f = 0; f < transitionFrames; ++f) {
            float t = (float)(f + 1) / (float)transitionFrames; // 0..1
            uint8_t* dst = &animFrames_[f * FRAME_SIZE];
            for (int i = 0; i < FRAME_SIZE; ++i) {
                int a = start[i];
                int b = end[i];
                dst[i] = (uint8_t)(a + (int)((b - a) * t));
            }
        }
        currentFrame_ = 0;
        frameDelayMs_ = 20; // 过渡帧间隔 20ms（保留）
        transitionActive_ = true;
    } else {
        // 非静态过渡，或无目标：正常重建当前效果动画
        currentEffect_->generateAnimation(animFrames_, animFrameCount_, FRAME_SIZE);
        // 直接播放真实数据，不做首尾平滑处理
        frameDelayMs_ = currentEffect_->getFrameDelay();
        currentFrame_ = 0;  // 重置到第一帧
    }

    // 更新两个buffer为新的第一帧数据
    memcpy(bufferA_, &animFrames_[0], FRAME_SIZE);
    memcpy(bufferB_, &animFrames_[0], FRAME_SIZE);
    xSemaphoreGive(animMutex_);
    
    // 重新置位事件：先清，再置更新就绪
    if (eventGroup_) {
        xEventGroupClearBits(eventGroup_, SEND_COMPLETE_BIT | UPDATE_READY_BIT);
        xEventGroupSetBits(eventGroup_, UPDATE_READY_BIT);
    }
    // Serial.printf("Current effect updated: %s\n", currentEffect_->getName());
}

void AnimSystem::start() {
    if (!animationRunning_ && currentEffect_ && eventGroup_) {
        animationRunning_ = true;
        currentFrame_ = 0;
        // 初始化第一帧数据到两个buffer
        memcpy(bufferA_, &animFrames_[0], FRAME_SIZE);
        memcpy(bufferB_, &animFrames_[0], FRAME_SIZE);
        updateBuffer_ = bufferA_;
        sendBuffer_ = bufferB_;
        // 事件初始化：清空并宣告有可发送帧
        xEventGroupClearBits(eventGroup_, SEND_COMPLETE_BIT | UPDATE_READY_BIT);
        xEventGroupSetBits(eventGroup_, UPDATE_READY_BIT);
        // 创建任务
        xTaskCreate(updateTaskEntry, "AnimUpdate", 4096, this, 3, &updateTaskHandle_);
        xTaskCreate(sendTaskEntry, "AnimSend", 8192, this, 3, &sendTaskHandle_);
        debug_println("Animation started");
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
        
        debug_println("Animation stopped");
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

void AnimSystem::setBrightnessSmooth(uint8_t targetBrightness) {
    if (targetBrightness > 100) targetBrightness = 100;
    uint8_t current = get_brightness();
    int delta = (int)targetBrightness - (int)current;

    if (brightnessFreezeActive_) {
        // 过渡中色温冻结亮度：记下待应用亮度，结束后一次性生效
        pendingBrightness_ = targetBrightness;
        debug_printf("Brightness request deferred due to color temp transition: %u -> %u\n", current, targetBrightness);
        return;
    }

    if (abs(delta) <= 10) {
        set_brightness(targetBrightness);
        return;
    }

    // 进行20帧平滑过渡（跟随sendTask节奏）
    const int frames = 20;
    for (int f = 0; f < frames; ++f) {
        float t = (frames > 1) ? ((float)f / (float)(frames - 1)) : 1.0f;
        int br = (int)lroundf(current + delta * t);
        if (br < 0) br = 0; if (br > 100) br = 100;
        set_brightness((uint8_t)br);
        // 等待一帧发送间隔
        vTaskDelay(pdMS_TO_TICKS(frameDelayMs_));
    }
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
    debug_println("Animation Update Task Started");
    while (1) {
        if (animationRunning_) {
            // 等待一帧发送完成，再准备下一帧
            EventBits_t bits = xEventGroupWaitBits(
                eventGroup_,
                SEND_COMPLETE_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );
            if (bits & SEND_COMPLETE_BIT) {
                xSemaphoreTake(animMutex_, portMAX_DELAY);

                // 写入当前帧到更新buffer
                memcpy(updateBuffer_, &animFrames_[currentFrame_ * FRAME_SIZE], FRAME_SIZE);
                // 记录刚写入的帧索引
                int justCopiedIndex = currentFrame_;
                // 计算下一帧索引
                currentFrame_ = (currentFrame_ + 1) % animFrameCount_;

                // 如果刚写入的是最后一帧，则也把该帧写到sendBuffer_，确保交换前后两个buffer一致
                if (justCopiedIndex == (animFrameCount_ - 1)) {
                    memcpy(sendBuffer_, &animFrames_[justCopiedIndex * FRAME_SIZE], FRAME_SIZE);
                }

                xSemaphoreGive(animMutex_);
                // 通知发送任务：更新就绪
                xEventGroupSetBits(eventGroup_, UPDATE_READY_BIT);

                // 如果处于静态过渡并且已经写完最后一帧，切换到目标静态效果
                if (transitionActive_ && justCopiedIndex == (animFrameCount_ - 1)) {
                    if (hasPendingColorTemp_) {
                        // 处于色温过渡：应用目标色温并回到单帧静态（无RTTI）
                        if (currentEffect_ && currentEffect_->isColorTemp()) {
                            ColorTempEffect* colorEff = (ColorTempEffect*)currentEffect_;
                            colorEff->setColorTemp(pendingTempIndex_);
                            colorEff->setDuvIndex(pendingDuvIndex_);
                            colorEff->generateAnimation(animFrames_, 1, FRAME_SIZE);
                            animFrameCount_ = 1;
                            currentFrame_ = 0;
                        }
                        hasPendingColorTemp_ = false;
                    } else if (transitionTarget_) {
                        // 普通静态->静态效果过渡完成：切换效果
                        currentEffect_ = transitionTarget_;
                        currentEffect_->generateAnimation(animFrames_, 1, FRAME_SIZE);
                        animFrameCount_ = 1;
                        currentFrame_ = 0;
                        transitionTarget_ = nullptr;
                    }
                    frameDelayMs_ = currentEffect_ ? currentEffect_->getFrameDelay() : 20;
                    transitionActive_ = false;

                    // 过渡结束，解除亮度冻结（若期间外部调整过亮度，可在此恢复）
                    brightnessFreezeActive_ = false;
                    g_anim_brightnessFreezeActive = false;
                    g_anim_frozenBrightness = get_brightness();
                    if (pendingBrightness_ != 0) {
                        set_brightness(pendingBrightness_);
                        pendingBrightness_ = 0;
                    }
                }
            }
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}
uint8_t* temp;
void AnimSystem::sendTask() {
    debug_println("Animation Send Task Started");
    while (1) {
        if (animationRunning_) {
            // 等待更新任务准备好一帧
            EventBits_t bits = xEventGroupWaitBits(
                eventGroup_,
                UPDATE_READY_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );
            if (bits & UPDATE_READY_BIT) {
                // 交换buffer，使新更新的buffer成为发送buffer
                xSemaphoreTake(animMutex_, portMAX_DELAY);
                temp = updateBuffer_;
                updateBuffer_ = sendBuffer_;
                sendBuffer_ = temp;
                xSemaphoreGive(animMutex_);

                // 发送当前发送buffer的数据
                send_data(sendBuffer_, FRAME_SIZE, 0xFFFF);

                // 通知更新任务可以准备下一帧
                xEventGroupSetBits(eventGroup_, SEND_COMPLETE_BIT);

                // 使用效果的帧延时
                vTaskDelay(pdMS_TO_TICKS(frameDelayMs_));
            }
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
} 


 