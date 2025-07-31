#include "scene_manager.h"
#include <Arduino.h>

// 场景数据生成器
class SceneGenerator {
public:
    static void generateBreathingScene(SceneManager& sceneManager, uint8_t sceneId, 
                                      uint8_t r, uint8_t g, uint8_t b, uint8_t frameCount = 30) {
        uint8_t* rgbData = (uint8_t*)malloc(108 * frameCount);
        if (!rgbData) return;
        
        for (int frame = 0; frame < frameCount; frame++) {
            float brightness = (sin(frame * 2 * PI / frameCount) + 1) / 2.0f;
            uint8_t intensity = (uint8_t)(brightness * 255);
            
            for (int led = 0; led < 36; led++) {
                int baseIndex = frame * 108 + led * 3;
                rgbData[baseIndex + 0] = (uint8_t)(r * intensity / 255.0f);
                rgbData[baseIndex + 1] = (uint8_t)(g * intensity / 255.0f);
                rgbData[baseIndex + 2] = (uint8_t)(b * intensity / 255.0f);
            }
        }
        
        sceneManager.saveScene(sceneId, rgbData, frameCount, 50);
        free(rgbData);
    }
    
    static void generateRainbowScene(SceneManager& sceneManager, uint8_t sceneId, uint8_t frameCount = 60) {
        uint8_t* rgbData = (uint8_t*)malloc(108 * frameCount);
        if (!rgbData) return;
        
        for (int frame = 0; frame < frameCount; frame++) {
            float hue = (float)frame / frameCount * 360.0f;
            
            for (int led = 0; led < 36; led++) {
                int baseIndex = frame * 108 + led * 3;
                
                // HSV转RGB
                float h = hue + (led * 10.0f);
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
                
                rgbData[baseIndex + 0] = (uint8_t)((r + m) * 255);
                rgbData[baseIndex + 1] = (uint8_t)((g + m) * 255);
                rgbData[baseIndex + 2] = (uint8_t)((b + m) * 255);
            }
        }
        
        sceneManager.saveScene(sceneId, rgbData, frameCount, 30);
        free(rgbData);
    }
    
    static void generateBlinkScene(SceneManager& sceneManager, uint8_t sceneId, 
                                  uint8_t r, uint8_t g, uint8_t b, uint8_t frameCount = 20) {
        uint8_t* rgbData = (uint8_t*)malloc(108 * frameCount);
        if (!rgbData) return;
        
        for (int frame = 0; frame < frameCount; frame++) {
            bool isOn = (frame < frameCount / 2);
            
            for (int led = 0; led < 36; led++) {
                int baseIndex = frame * 108 + led * 3;
                if (isOn) {
                    rgbData[baseIndex + 0] = r;
                    rgbData[baseIndex + 1] = g;
                    rgbData[baseIndex + 2] = b;
                } else {
                    rgbData[baseIndex + 0] = 0;
                    rgbData[baseIndex + 1] = 0;
                    rgbData[baseIndex + 2] = 0;
                }
            }
        }
        
        sceneManager.saveScene(sceneId, rgbData, frameCount, 100);
        free(rgbData);
    }
    
    static void generateGradientScene(SceneManager& sceneManager, uint8_t sceneId,
                                     uint8_t r1, uint8_t g1, uint8_t b1,
                                     uint8_t r2, uint8_t g2, uint8_t b2, uint8_t frameCount = 40) {
        uint8_t* rgbData = (uint8_t*)malloc(108 * frameCount);
        if (!rgbData) return;
        
        for (int frame = 0; frame < frameCount; frame++) {
            float progress = (float)frame / (frameCount - 1);
            
            for (int led = 0; led < 36; led++) {
                int baseIndex = frame * 108 + led * 3;
                rgbData[baseIndex + 0] = (uint8_t)(r1 + (r2 - r1) * progress);
                rgbData[baseIndex + 1] = (uint8_t)(g1 + (g2 - g1) * progress);
                rgbData[baseIndex + 2] = (uint8_t)(b1 + (b2 - b1) * progress);
            }
        }
        
        sceneManager.saveScene(sceneId, rgbData, frameCount, 40);
        free(rgbData);
    }
    
    static void generateStaticScene(SceneManager& sceneManager, uint8_t sceneId,
                                   uint8_t r, uint8_t g, uint8_t b) {
        uint8_t* rgbData = (uint8_t*)malloc(108);
        if (!rgbData) return;
        
        for (int led = 0; led < 36; led++) {
            int baseIndex = led * 3;
            rgbData[baseIndex + 0] = r;
            rgbData[baseIndex + 1] = g;
            rgbData[baseIndex + 2] = b;
        }
        
        sceneManager.saveScene(sceneId, rgbData, 1, 0);
        free(rgbData);
    }
    
    // 从头文件数据创建场景
    static void createSceneFromData(SceneManager& sceneManager, uint8_t sceneId,
                                   const uint8_t* headerData, uint8_t frameCount, uint8_t frameDelayMs) {
        size_t dataSize = 108 * frameCount;
        uint8_t* rgbData = (uint8_t*)malloc(dataSize);
        if (!rgbData) return;
        
        memcpy(rgbData, headerData, dataSize);
        sceneManager.saveScene(sceneId, rgbData, frameCount, frameDelayMs);
        free(rgbData);
    }
    
    // 生成所有默认场景
    static void generateAllDefaultScenes(SceneManager& sceneManager) {
        Serial.println("生成默认场景...");
        
        // 场景0: 红色呼吸灯
        generateBreathingScene(sceneManager, 0, 255, 0, 0, 30);
        
        // 场景1: 绿色呼吸灯
        generateBreathingScene(sceneManager, 1, 0, 255, 0, 30);
        
        // 场景2: 蓝色呼吸灯
        generateBreathingScene(sceneManager, 2, 0, 0, 255, 30);
        
        // 场景3: 彩虹效果
        generateRainbowScene(sceneManager, 3, 60);
        
        // 场景4: 白色闪烁
        generateBlinkScene(sceneManager, 4, 255, 255, 255, 20);
        
        // 场景5: 红色闪烁
        generateBlinkScene(sceneManager, 5, 255, 0, 0, 20);
        
        // 场景6: 红到蓝渐变
        generateGradientScene(sceneManager, 6, 255, 0, 0, 0, 0, 255, 40);
        
        // 场景7: 绿到黄渐变
        generateGradientScene(sceneManager, 7, 0, 255, 0, 255, 255, 0, 40);
        
        // 场景8: 静态白色
        generateStaticScene(sceneManager, 8, 255, 255, 255);
        
        // 场景9: 静态红色
        generateStaticScene(sceneManager, 9, 255, 0, 0);
        
        Serial.println("默认场景生成完成");
    }
};

// 全局场景管理器实例
SceneManager g_sceneManager;

// 初始化场景系统
bool initSceneSystem() {
    if (!g_sceneManager.init()) {
        Serial.println("场景系统初始化失败");
        return false;
    }
    
    Serial.println("场景系统初始化成功");
    
    // 如果没有场景，创建默认场景
    if (!g_sceneManager.sceneExists(0)) {
        SceneGenerator::generateAllDefaultScenes(g_sceneManager);
    }
    
    // 列出所有场景
    g_sceneManager.listScenes();
    
    return true;
}

// 加载场景到动画系统
bool loadSceneToAnimation(uint8_t sceneId) {
    SceneData sceneData;
    
    if (!g_sceneManager.loadScene(sceneId, sceneData)) {
        Serial.printf("加载场景 %d 失败\n", sceneId);
        return false;
    }
    
    Serial.printf("成功加载场景 %d: 帧数=%d, 延迟=%dms\n", 
                 sceneId, sceneData.frameCount, sceneData.frameDelayMs);
    
    // 这里可以将场景数据传递给动画系统
    // 注意：需要根据你的动画系统接口进行调整
    
    g_sceneManager.freeSceneData(sceneData);
    return true;
} 