#pragma once
#include <cstdint>
#include <cstddef>

// 场景数据结构
struct SceneData {
    uint8_t frameCount;    // 帧数 (0-255)
    uint8_t frameDelayMs;  // 帧延迟毫秒
    uint8_t* rgbData;      // RGB数据指针
    size_t dataSize;       // 数据大小
};

// 场景管理器接口
class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    // 初始化
    bool init();
    
    // 场景管理
    bool loadScene(uint8_t sceneId, SceneData& sceneData);
    bool saveScene(uint8_t sceneId, const uint8_t* rgbData, uint8_t frameCount, uint8_t frameDelayMs);
    bool deleteScene(uint8_t sceneId);
    bool sceneExists(uint8_t sceneId);
    
    // 获取场景信息
    bool getSceneInfo(uint8_t sceneId, uint8_t& frameCount, uint8_t& frameDelayMs);
    
    // 列出所有场景
    void listScenes();
    
    // 释放场景数据
    void freeSceneData(SceneData& sceneData);
    
    // 存储空间管理
    size_t getFreeSpace();
    size_t getTotalSpace();
    
    // 批量操作
    bool createDefaultScenes();  // 创建默认场景
    bool clearAllScenes();       // 清除所有场景

private:
    class SceneManagerImpl;
    SceneManagerImpl* pImpl;  // 私有实现指针
}; 