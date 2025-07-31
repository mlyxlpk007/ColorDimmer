#include "unified_animation.h"
#include "dynamic_animations.h"
#include <SPIFFS.h>
#include <Arduino.h>
#include "esp_log.h"
#include <cstring>

static const char* TAG = "UnifiedAnimation";

// ==================== StaticAnimation 实现 ====================

StaticAnimation::StaticAnimation(uint8_t r, uint8_t g, uint8_t b, const char* name)
    : name_(name) {
    setColor(r, g, b);
}

StaticAnimation::StaticAnimation(const uint8_t* rgbData, const char* name)
    : name_(name) {
    if (rgbData) {
        memcpy(rgbData_, rgbData, 108);
    } else {
        memset(rgbData_, 0, 108);
    }
}

void StaticAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    memcpy(buffer, rgbData_, 108);
}

void StaticAnimation::setColor(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < 36; i++) {
        rgbData_[i * 3 + 0] = r;
        rgbData_[i * 3 + 1] = g;
        rgbData_[i * 3 + 2] = b;
    }
}

void StaticAnimation::getColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = rgbData_[0];
    g = rgbData_[1];
    b = rgbData_[2];
}

// ==================== DynamicAnimation 实现 ====================

DynamicAnimation::DynamicAnimation(const char* name, int frameCount, int frameDelay)
    : frameCount_(frameCount), frameDelay_(frameDelay), name_(name) {
}

// ==================== PresetAnimation 实现 ====================

PresetAnimation::PresetAnimation(const char* name, uint8_t sceneId)
    : cachedData_(nullptr), frameCount_(0), frameDelay_(0), 
      name_(name), sceneId_(sceneId), loaded_(false) {
    loadFromStorage(sceneId);
}

PresetAnimation::~PresetAnimation() {
    if (cachedData_) {
        free(cachedData_);
        cachedData_ = nullptr;
    }
}

void PresetAnimation::generateFrame(uint8_t* buffer, int frameIndex) {
    if (!loaded_ || !cachedData_ || frameIndex >= frameCount_) {
        memset(buffer, 0, 108);
        return;
    }
    
    memcpy(buffer, cachedData_ + frameIndex * 108, 108);
}

bool PresetAnimation::loadFromStorage(uint8_t sceneId) {
    char filename[32];
    snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
    
    if (!SPIFFS.exists(filename)) {
        ESP_LOGE(TAG, "场景文件不存在: %s", filename);
        return false;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        ESP_LOGE(TAG, "无法打开场景文件: %s", filename);
        return false;
    }
    
    // 读取文件头
    uint8_t header[2];
    if (file.read(header, 2) != 2) {
        ESP_LOGE(TAG, "读取文件头失败");
        file.close();
        return false;
    }
    
    frameCount_ = header[0];
    frameDelay_ = header[1];
    
    if (frameCount_ == 0 || frameCount_ > 255) {
        ESP_LOGE(TAG, "无效的帧数: %d", frameCount_);
        file.close();
        return false;
    }
    
    // 分配内存
    size_t dataSize = 108 * frameCount_;
    if (cachedData_) {
        free(cachedData_);
    }
    cachedData_ = (uint8_t*)malloc(dataSize);
    if (!cachedData_) {
        ESP_LOGE(TAG, "内存分配失败");
        file.close();
        return false;
    }
    
    // 读取RGB数据
    if (file.read(cachedData_, dataSize) != dataSize) {
        ESP_LOGE(TAG, "读取RGB数据失败");
        free(cachedData_);
        cachedData_ = nullptr;
        file.close();
        return false;
    }
    
    file.close();
    loaded_ = true;
    sceneId_ = sceneId;
    
    ESP_LOGI(TAG, "场景加载成功: %s, 帧数: %d, 延迟: %dms", filename, frameCount_, frameDelay_);
    return true;
}

bool PresetAnimation::saveToStorage(uint8_t sceneId) {
    if (!cachedData_ || !loaded_) {
        ESP_LOGE(TAG, "没有数据可保存");
        return false;
    }
    
    char filename[32];
    snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
    
    // 计算文件大小
    size_t fileSize = 2 + (108 * frameCount_);
    
    // 检查可用空间
    size_t freeSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    if (freeSpace < fileSize) {
        ESP_LOGE(TAG, "存储空间不足");
        return false;
    }
    
    // 创建文件
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        ESP_LOGE(TAG, "无法创建场景文件: %s", filename);
        return false;
    }
    
    // 写入文件头
    uint8_t header[2] = {frameCount_, frameDelay_};
    if (file.write(header, 2) != 2) {
        ESP_LOGE(TAG, "写入文件头失败");
        file.close();
        return false;
    }
    
    // 写入RGB数据
    size_t dataSize = 108 * frameCount_;
    if (file.write(cachedData_, dataSize) != dataSize) {
        ESP_LOGE(TAG, "写入RGB数据失败");
        file.close();
        return false;
    }
    
    file.close();
    sceneId_ = sceneId;
    
    ESP_LOGI(TAG, "场景保存成功: %s, 帧数: %d, 延迟: %dms", filename, frameCount_, frameDelay_);
    return true;
}

// ==================== AnimationManager 实现 ====================

class AnimationManager::AnimationManagerImpl {
public:
    static const size_t MAX_ANIMATIONS = 30;
    
    AnimationManagerImpl() : initialized_(false) {}
    
    ~AnimationManagerImpl() {
        if (initialized_ && SPIFFS.begin()) {
            SPIFFS.end();
        }
    }
    
    bool init() {
        if (!SPIFFS.begin(true)) {
            ESP_LOGE(TAG, "SPIFFS初始化失败");
            return false;
        }
        
        initialized_ = true;
        ESP_LOGI(TAG, "动画管理器初始化成功");
        ESP_LOGI(TAG, "总空间: %d bytes", getTotalSpace());
        ESP_LOGI(TAG, "可用空间: %d bytes", getFreeSpace());
        
        return true;
    }
    
    bool saveAnimation(uint8_t sceneId, Animation* animation) {
        if (!initialized_ || !animation) return false;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        
        // 计算文件大小
        int frameCount = animation->getFrameCount();
        int frameDelay = animation->getFrameDelay();
        size_t fileSize = 2 + (108 * frameCount);
        
        // 检查可用空间
        if (getFreeSpace() < fileSize) {
            ESP_LOGE(TAG, "存储空间不足");
            return false;
        }
        
        // 创建文件
        File file = SPIFFS.open(filename, "w");
        if (!file) {
            ESP_LOGE(TAG, "无法创建场景文件: %s", filename);
            return false;
        }
        
        // 写入文件头
        uint8_t header[2] = {(uint8_t)frameCount, (uint8_t)frameDelay};
        if (file.write(header, 2) != 2) {
            ESP_LOGE(TAG, "写入文件头失败");
            file.close();
            return false;
        }
        
        // 写入RGB数据
        uint8_t frameBuffer[108];
        for (int i = 0; i < frameCount; i++) {
            animation->generateFrame(frameBuffer, i);
            if (file.write(frameBuffer, 108) != 108) {
                ESP_LOGE(TAG, "写入帧数据失败");
                file.close();
                return false;
            }
        }
        
        file.close();
        
        ESP_LOGI(TAG, "动画保存成功: %s, 帧数: %d, 延迟: %dms", filename, frameCount, frameDelay);
        return true;
    }
    
    bool animationExists(uint8_t sceneId) {
        if (!initialized_) return false;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        return SPIFFS.exists(filename);
    }
    
    bool deleteAnimation(uint8_t sceneId) {
        if (!initialized_) return false;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        
        if (!SPIFFS.exists(filename)) {
            ESP_LOGE(TAG, "场景文件不存在: %s", filename);
            return false;
        }
        
        if (SPIFFS.remove(filename)) {
            ESP_LOGI(TAG, "场景删除成功: %s", filename);
            return true;
        } else {
            ESP_LOGE(TAG, "场景删除失败: %s", filename);
            return false;
        }
    }
    
    void listAnimations() {
        if (!initialized_) return;
        
        ESP_LOGI(TAG, "动画列表:");
        for (uint8_t i = 0; i < MAX_ANIMATIONS; i++) {
            if (animationExists(i)) {
                char filename[32];
                snprintf(filename, sizeof(filename), "/scene_%02d.dat", i);
                File file = SPIFFS.open(filename, "r");
                if (file && file.size() >= 2) {
                    uint8_t header[2];
                    file.read(header, 2);
                    file.close();
                    ESP_LOGI(TAG, "  动画 %d: 帧数=%d, 延迟=%dms", i, header[0], header[1]);
                }
            }
        }
    }
    
    size_t getFreeSpace() {
        return initialized_ ? (SPIFFS.totalBytes() - SPIFFS.usedBytes()) : 0;
    }
    
    size_t getTotalSpace() {
        return initialized_ ? SPIFFS.totalBytes() : 0;
    }
    
    bool createDefaultAnimations() {
        if (!initialized_) return false;
        
        ESP_LOGI(TAG, "创建默认动画...");
        
        // 创建一些示例动画
        BreathingAnimation breathAnim(255, 0, 0, 30);
        saveAnimation(0, &breathAnim);
        
        BreathingAnimation breathAnim2(0, 255, 0, 30);
        saveAnimation(1, &breathAnim2);
        
        RainbowAnimation rainbowAnim(60);
        saveAnimation(2, &rainbowAnim);
        
        BlinkAnimation blinkAnim(255, 255, 255, 20);
        saveAnimation(3, &blinkAnim);
        
        GradientAnimation gradientAnim(255, 0, 0, 0, 0, 255, 40);
        saveAnimation(4, &gradientAnim);
        
        ESP_LOGI(TAG, "默认动画创建完成");
        return true;
    }
    
    bool clearAllAnimations() {
        if (!initialized_) return false;
        
        ESP_LOGI(TAG, "清除所有动画...");
        
        for (uint8_t i = 0; i < MAX_ANIMATIONS; i++) {
            if (animationExists(i)) {
                deleteAnimation(i);
            }
        }
        
        return true;
    }
    
private:
    bool initialized_;
};

// AnimationManager 实现
AnimationManager::AnimationManager() : pImpl_(new AnimationManagerImpl()), currentAnimation_(nullptr) {}

AnimationManager::~AnimationManager() {
    delete pImpl_;
}

bool AnimationManager::init() {
    return pImpl_->init();
}

Animation* AnimationManager::createStaticAnimation(uint8_t r, uint8_t g, uint8_t b, const char* name) {
    return new StaticAnimation(r, g, b, name);
}

Animation* AnimationManager::createPresetAnimation(uint8_t sceneId, const char* name) {
    char defaultName[32];
    if (!name) {
        snprintf(defaultName, sizeof(defaultName), "Preset_%d", sceneId);
        name = defaultName;
    }
    return new PresetAnimation(name, sceneId);
}

Animation* AnimationManager::createBreathingAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount) {
    return new BreathingAnimation(r, g, b, frameCount);
}

Animation* AnimationManager::createRainbowAnimation(int frameCount) {
    return new RainbowAnimation(frameCount);
}

Animation* AnimationManager::createBlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount) {
    return new BlinkAnimation(r, g, b, frameCount);
}

Animation* AnimationManager::createGradientAnimation(uint8_t r1, uint8_t g1, uint8_t b1, 
                                                    uint8_t r2, uint8_t g2, uint8_t b2, int frameCount) {
    return new GradientAnimation(r1, g1, b1, r2, g2, b2, frameCount);
}

bool AnimationManager::saveAnimation(uint8_t sceneId, Animation* animation) {
    return pImpl_->saveAnimation(sceneId, animation);
}

Animation* AnimationManager::loadAnimation(uint8_t sceneId) {
    return createPresetAnimation(sceneId);
}

bool AnimationManager::deleteAnimation(uint8_t sceneId) {
    return pImpl_->deleteAnimation(sceneId);
}

bool AnimationManager::animationExists(uint8_t sceneId) {
    return pImpl_->animationExists(sceneId);
}

void AnimationManager::playAnimation(Animation* animation) {
    if (currentAnimation_ != animation) {
        currentAnimation_ = animation;
        ESP_LOGI(TAG, "播放动画: %s", animation ? animation->getName() : "None");
    }
}

void AnimationManager::stopAnimation() {
    currentAnimation_ = nullptr;
    ESP_LOGI(TAG, "停止动画播放");
}

void AnimationManager::listAnimations() {
    pImpl_->listAnimations();
}

size_t AnimationManager::getFreeSpace() {
    return pImpl_->getFreeSpace();
}

size_t AnimationManager::getTotalSpace() {
    return pImpl_->getTotalSpace();
}

bool AnimationManager::createDefaultAnimations() {
    return pImpl_->createDefaultAnimations();
}

bool AnimationManager::clearAllAnimations() {
    return pImpl_->clearAllAnimations();
} 