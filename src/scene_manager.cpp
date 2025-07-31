#include "scene_manager.h"
#include <SPIFFS.h>
#include <Arduino.h>
#include "esp_log.h"
#include <cstring>

static const char* TAG = "SceneManager";

// 私有实现类
class SceneManager::SceneManagerImpl {
public:
    static const size_t FRAME_SIZE = 108;  // 6x6x3 = 108字节每帧
    static const size_t MAX_FRAMES = 255;  // 最大帧数
    static const size_t MAX_SCENES = 30;   // 最大场景数
    
    SceneManagerImpl() : initialized_(false) {}
    
    ~SceneManagerImpl() {
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
        ESP_LOGI(TAG, "SPIFFS初始化成功");
        ESP_LOGI(TAG, "总空间: %d bytes", getTotalSpace());
        ESP_LOGI(TAG, "可用空间: %d bytes", getFreeSpace());
        
        return true;
    }
    
    bool loadScene(uint8_t sceneId, SceneData& sceneData) {
        if (!initialized_) return false;
        
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
        
        uint8_t frameCount = header[0];
        uint8_t frameDelayMs = header[1];
        
        if (frameCount == 0 || frameCount > MAX_FRAMES) {
            ESP_LOGE(TAG, "无效的帧数: %d", frameCount);
            file.close();
            return false;
        }
        
        // 计算数据大小
        size_t dataSize = FRAME_SIZE * frameCount;
        
        // 分配内存
        sceneData.rgbData = (uint8_t*)malloc(dataSize);
        if (!sceneData.rgbData) {
            ESP_LOGE(TAG, "内存分配失败");
            file.close();
            return false;
        }
        
        // 读取RGB数据
        if (file.read(sceneData.rgbData, dataSize) != dataSize) {
            ESP_LOGE(TAG, "读取RGB数据失败");
            free(sceneData.rgbData);
            sceneData.rgbData = nullptr;
            file.close();
            return false;
        }
        
        file.close();
        
        // 设置场景数据
        sceneData.frameCount = frameCount;
        sceneData.frameDelayMs = frameDelayMs;
        sceneData.dataSize = dataSize;
        
        ESP_LOGI(TAG, "场景加载成功: %s, 帧数: %d, 延迟: %dms", filename, frameCount, frameDelayMs);
        return true;
    }
    
    bool saveScene(uint8_t sceneId, const uint8_t* rgbData, uint8_t frameCount, uint8_t frameDelayMs) {
        if (!initialized_) return false;
        
        if (frameCount == 0 || frameCount > MAX_FRAMES) {
            ESP_LOGE(TAG, "无效的帧数: %d", frameCount);
            return false;
        }
        
        if (!rgbData) {
            ESP_LOGE(TAG, "RGB数据为空");
            return false;
        }
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        
        // 计算文件大小
        size_t fileSize = 2 + (FRAME_SIZE * frameCount);
        
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
        uint8_t header[2] = {frameCount, frameDelayMs};
        if (file.write(header, 2) != 2) {
            ESP_LOGE(TAG, "写入文件头失败");
            file.close();
            return false;
        }
        
        // 写入RGB数据
        size_t dataSize = FRAME_SIZE * frameCount;
        if (file.write(rgbData, dataSize) != dataSize) {
            ESP_LOGE(TAG, "写入RGB数据失败");
            file.close();
            return false;
        }
        
        file.close();
        
        ESP_LOGI(TAG, "场景保存成功: %s, 帧数: %d, 延迟: %dms", filename, frameCount, frameDelayMs);
        return true;
    }
    
    bool deleteScene(uint8_t sceneId) {
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
    
    bool sceneExists(uint8_t sceneId) {
        if (!initialized_) return false;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        return SPIFFS.exists(filename);
    }
    
    bool getSceneInfo(uint8_t sceneId, uint8_t& frameCount, uint8_t& frameDelayMs) {
        if (!initialized_) return false;
        
        char filename[32];
        snprintf(filename, sizeof(filename), "/scene_%02d.dat", sceneId);
        
        if (!SPIFFS.exists(filename)) {
            return false;
        }
        
        File file = SPIFFS.open(filename, "r");
        if (!file) {
            return false;
        }
        
        if (file.size() < 2) {
            file.close();
            return false;
        }
        
        uint8_t header[2];
        if (file.read(header, 2) != 2) {
            file.close();
            return false;
        }
        
        file.close();
        
        frameCount = header[0];
        frameDelayMs = header[1];
        
        return (frameCount > 0 && frameCount <= MAX_FRAMES);
    }
    
    void listScenes() {
        if (!initialized_) return;
        
        ESP_LOGI(TAG, "场景列表:");
        for (uint8_t i = 0; i < MAX_SCENES; i++) {
            if (sceneExists(i)) {
                uint8_t frameCount, frameDelayMs;
                if (getSceneInfo(i, frameCount, frameDelayMs)) {
                    ESP_LOGI(TAG, "  场景 %d: 帧数=%d, 延迟=%dms", i, frameCount, frameDelayMs);
                }
            }
        }
    }
    
    void freeSceneData(SceneData& sceneData) {
        if (sceneData.rgbData) {
            free(sceneData.rgbData);
            sceneData.rgbData = nullptr;
        }
        sceneData.dataSize = 0;
        sceneData.frameCount = 0;
        sceneData.frameDelayMs = 0;
    }
    
    size_t getFreeSpace() {
        return initialized_ ? (SPIFFS.totalBytes() - SPIFFS.usedBytes()) : 0;
    }
    
    size_t getTotalSpace() {
        return initialized_ ? SPIFFS.totalBytes() : 0;
    }
    
    bool createDefaultScenes() {
        if (!initialized_) return false;
        
        ESP_LOGI(TAG, "创建默认场景...");
        
        // 创建一些示例场景
        for (uint8_t i = 0; i < 5; i++) {
            if (!sceneExists(i)) {
                // 生成简单的测试场景
                uint8_t* testData = (uint8_t*)malloc(FRAME_SIZE * 20);
                if (testData) {
                    // 生成简单的渐变场景
                    for (int frame = 0; frame < 20; frame++) {
                        uint8_t intensity = (frame * 255) / 20;
                        for (int led = 0; led < 36; led++) {
                            int baseIndex = frame * 108 + led * 3;
                            testData[baseIndex + 0] = intensity;     // R
                            testData[baseIndex + 1] = 255 - intensity; // G
                            testData[baseIndex + 2] = 128;           // B
                        }
                    }
                    
                    saveScene(i, testData, 20, 50 + i * 10);
                    free(testData);
                }
            }
        }
        
        return true;
    }
    
    bool clearAllScenes() {
        if (!initialized_) return false;
        
        ESP_LOGI(TAG, "清除所有场景...");
        
        for (uint8_t i = 0; i < MAX_SCENES; i++) {
            if (sceneExists(i)) {
                deleteScene(i);
            }
        }
        
        return true;
    }
    
private:
    bool initialized_;
};

// SceneManager 实现
SceneManager::SceneManager() : pImpl(new SceneManagerImpl()) {}

SceneManager::~SceneManager() {
    delete pImpl;
}

bool SceneManager::init() {
    return pImpl->init();
}

bool SceneManager::loadScene(uint8_t sceneId, SceneData& sceneData) {
    return pImpl->loadScene(sceneId, sceneData);
}

bool SceneManager::saveScene(uint8_t sceneId, const uint8_t* rgbData, uint8_t frameCount, uint8_t frameDelayMs) {
    return pImpl->saveScene(sceneId, rgbData, frameCount, frameDelayMs);
}

bool SceneManager::deleteScene(uint8_t sceneId) {
    return pImpl->deleteScene(sceneId);
}

bool SceneManager::sceneExists(uint8_t sceneId) {
    return pImpl->sceneExists(sceneId);
}

bool SceneManager::getSceneInfo(uint8_t sceneId, uint8_t& frameCount, uint8_t& frameDelayMs) {
    return pImpl->getSceneInfo(sceneId, frameCount, frameDelayMs);
}

void SceneManager::listScenes() {
    pImpl->listScenes();
}

void SceneManager::freeSceneData(SceneData& sceneData) {
    pImpl->freeSceneData(sceneData);
}

size_t SceneManager::getFreeSpace() {
    return pImpl->getFreeSpace();
}

size_t SceneManager::getTotalSpace() {
    return pImpl->getTotalSpace();
}

bool SceneManager::createDefaultScenes() {
    return pImpl->createDefaultScenes();
}

bool SceneManager::clearAllScenes() {
    return pImpl->clearAllScenes();
} 