#pragma once
#include <cstdint>
#include <cstddef>

// 统一的动画基类
class Animation {
public:
    enum Type {
        STATIC,      // 静态场景
        DYNAMIC,     // 动态动画（实时生成）
        PRESET       // 预设效果（从存储加载）
    };
    
    virtual ~Animation() = default;
    
    // 核心接口
    virtual void generateFrame(uint8_t* buffer, int frameIndex) = 0;
    virtual int getFrameCount() const = 0;
    virtual int getFrameDelay() const = 0;
    virtual Type getType() const = 0;
    virtual const char* getName() const = 0;
    
    // 存储相关
    virtual bool canBeSaved() const { return true; }
    virtual bool loadFromStorage(uint8_t sceneId) { return false; }
    virtual bool saveToStorage(uint8_t sceneId) { return false; }
    
    // 参数设置
    virtual void setParameter(const char* name, float value) {}
    virtual float getParameter(const char* name) const { return 0.0f; }
    
    // 状态管理
    virtual void reset() {}
    virtual bool isFinished() const { return false; }
};

// 静态场景（单帧）
class StaticAnimation : public Animation {
public:
    StaticAnimation(uint8_t r, uint8_t g, uint8_t b, const char* name = "Static");
    StaticAnimation(const uint8_t* rgbData, const char* name = "Static");
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    int getFrameCount() const override { return 1; }
    int getFrameDelay() const override { return 0; }
    Type getType() const override { return STATIC; }
    const char* getName() const override { return name_; }
    
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void getColor(uint8_t& r, uint8_t& g, uint8_t& b) const;
    
private:
    uint8_t rgbData_[108];  // 36个LED * 3字节
    const char* name_;
};

// 动态动画（实时生成）
class DynamicAnimation : public Animation {
public:
    DynamicAnimation(const char* name, int frameCount, int frameDelay);
    virtual ~DynamicAnimation() = default;
    
    int getFrameCount() const override { return frameCount_; }
    int getFrameDelay() const override { return frameDelay_; }
    Type getType() const override { return DYNAMIC; }
    const char* getName() const override { return name_; }
    
    void setFrameCount(int count) { frameCount_ = count; }
    void setFrameDelay(int delay) { frameDelay_ = delay; }
    
protected:
    int frameCount_;
    int frameDelay_;
    const char* name_;
};

// 预设动画（从存储加载）
class PresetAnimation : public Animation {
public:
    PresetAnimation(const char* name, uint8_t sceneId);
    ~PresetAnimation();
    
    void generateFrame(uint8_t* buffer, int frameIndex) override;
    int getFrameCount() const override { return frameCount_; }
    int getFrameDelay() const override { return frameDelay_; }
    Type getType() const override { return PRESET; }
    const char* getName() const override { return name_; }
    
    bool loadFromStorage(uint8_t sceneId) override;
    bool saveToStorage(uint8_t sceneId) override;
    bool isLoaded() const { return loaded_; }
    
private:
    uint8_t* cachedData_;  // 缓存的数据
    uint8_t frameCount_;
    uint8_t frameDelay_;
    const char* name_;
    uint8_t sceneId_;
    bool loaded_;
};

// 动画管理器
class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager();
    
    // 初始化
    bool init();
    
    // 创建动画
    Animation* createStaticAnimation(uint8_t r, uint8_t g, uint8_t b, const char* name = nullptr);
    Animation* createPresetAnimation(uint8_t sceneId, const char* name = nullptr);
    
    // 动态动画工厂
    Animation* createBreathingAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 30);
    Animation* createRainbowAnimation(int frameCount = 60);
    Animation* createBlinkAnimation(uint8_t r, uint8_t g, uint8_t b, int frameCount = 20);
    Animation* createGradientAnimation(uint8_t r1, uint8_t g1, uint8_t b1, 
                                      uint8_t r2, uint8_t g2, uint8_t b2, int frameCount = 40);
    
    // 存储管理
    bool saveAnimation(uint8_t sceneId, Animation* animation);
    Animation* loadAnimation(uint8_t sceneId);
    bool deleteAnimation(uint8_t sceneId);
    bool animationExists(uint8_t sceneId);
    
    // 播放控制
    void playAnimation(Animation* animation);
    void stopAnimation();
    Animation* getCurrentAnimation() const { return currentAnimation_; }
    
    // 信息查询
    void listAnimations();
    size_t getFreeSpace();
    size_t getTotalSpace();
    
    // 批量操作
    bool createDefaultAnimations();
    bool clearAllAnimations();
    
private:
    class AnimationManagerImpl;
    AnimationManagerImpl* pImpl_;
    Animation* currentAnimation_;
}; 