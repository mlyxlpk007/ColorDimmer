#include <esp32-hal-gpio.h>
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <driver/rmt.h>
#include <soc/rmt_reg.h>
#include "sid_rmt_sender.h"
#include "images.h"
#include "anim_system.hpp"
#include "anim_effect.hpp"
#include "gradient_rgb_pattern.h"

#define SID_MAX_CHIPS 36
#define IR_RECV_PIN 26
#define BUTTON_PIN 33
#define LED_PIN    0

// 串口通信协议定义
#define SERIAL_FRAME_HEADER 0x01
#define SERIAL_CMD_D7 0xd7      // 设置开关/亮度
#define SERIAL_CMD_DA 0xda      // 设置色温
#define SERIAL_CMD_DD 0xdd      // 设置动态场景
#define SERIAL_CMD_A2 0xa2      // 查询灯运行状态
#define SERIAL_MAX_DATA_LEN 64
#define SERIAL_BUFFER_SIZE 128

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-1234-1234-1234-abcdef123456"

// 串口通信状态机
enum SerialState {
  WAIT_HEADER,
  WAIT_CMD,
  WAIT_LENGTH,
  WAIT_DATA,
  WAIT_CHECKSUM
};

// 串口通信变量
SerialState serialState = WAIT_HEADER;
uint8_t serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialBufferIndex = 0;
uint8_t expectedLength = 0;
uint8_t receivedChecksum = 0;
uint8_t calculatedChecksum = 0;

// 串口超时机制
unsigned long lastSerialReceiveTime = 0;
const unsigned long SERIAL_TIMEOUT_MS = 1000; // 1秒超时

// 函数声明
void setLedMode(int mode);
void switchAnimationEffect();
void handleSerialCommand();
void sendSerialResponse(uint8_t cmd, uint8_t* data, uint8_t length);
void handleIRCode(uint32_t code);

enum LedMode {
  LED_OFF,
  LED_ON,
  LED_BLINK_SLOW,
  LED_BLINK_FAST,
  LED_BREATH
}; 

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
volatile LedMode currentMode = LED_OFF;  // 跨核共享，需 volatile

// 动画系统实例
AnimSystem animSystem;

// 动画效果实例
BreathEffect breathEffect(255, 100, 50);      // 橙红色呼吸灯
RainbowEffect rainbowEffect;                  // 彩虹效果
BlinkEffect blinkEffect(255, 255, 255);       // 白色闪烁
GradientEffect gradientEffect(255, 0, 0, 0, 0, 255); // 红到蓝渐变
WhiteStaticEffect whiteStaticEffect(255);         // 常亮白色照明
ColorTempEffect colorTempEffect(1);               // 色温效果
ImageDataEffect imageDataEffect(img1_data);       // 从images.h加载的动画数据
ImageDataEffect czcxEffect(czcx_data);            // 彩虹彩虹效果
ImageDataEffect jl3Effect(jl3_data);              // 极光3效果
ImageDataEffect lt2Effect(lt2_data);              // 流星2效果
ImageDataEffect lt3Effect(lt3_data);              // 流星3效果

// 当前动画效果索引
int currentAnimEffect = 0;
const int MAX_ANIM_EFFECTS = 10;  // 增加了5个ImageDataEffect

// 色温和场景控制
uint8_t currentColorTemp = 1;    // 当前色温索引 (1-61)
uint8_t currentScene = 0;        // 当前场景 (0-30)
bool lightPower = true;         // 灯开关状态
bool colorTempMode = false;      // 是否处于色温模式

// 声明外部变量，供sid_rmt_sender.cpp使用
extern uint8_t currentColorTemp;
extern bool colorTempMode;



// ----- 按键任务变量 -----
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

rmt_item32_t bit0 ,bit1 ,reset_code;

// 初始化函数
void initIR() {
  irrecv.enableIRIn();  // 启动红外接收
  Serial.println("IR receiver initialized.");
}

// 读取函数，返回接收到的值，如果没有返回0
uint32_t readIR() {
  if (irrecv.decode(&results)) {
    uint32_t code = results.value;
    Serial.printf("IR Received: 0x%X\n", code);
    irrecv.resume();  // 准备下一次接收
    return code;
  }
  return 0;  // 无数据
}

// 串口通信处理函数
void processSerialData(uint8_t data) {
  lastSerialReceiveTime = millis(); // 更新接收时间
  
  switch (serialState) {
    case WAIT_HEADER:
      if (data == SERIAL_FRAME_HEADER) {
        serialBuffer[0] = data;
        serialBufferIndex = 1;
        calculatedChecksum = data;
        serialState = WAIT_CMD;
      }
      break;
      
    case WAIT_CMD:
      if (data == SERIAL_CMD_D7 || data == SERIAL_CMD_DA || data == SERIAL_CMD_DD || data == SERIAL_CMD_A2) {
        serialBuffer[1] = data;
        serialBufferIndex = 2;
        calculatedChecksum += data;
        serialState = WAIT_LENGTH;
      } else {
        // 命令无效，重置状态机
        serialState = WAIT_HEADER;
      }
      break;
      
    case WAIT_LENGTH:
      if (data <= SERIAL_MAX_DATA_LEN) {
        expectedLength = data;
        serialBuffer[2] = data;
        serialBufferIndex = 3;
        calculatedChecksum += data;
        serialState = WAIT_DATA;
      } else {
        // 数据长度无效，重置状态机
        serialState = WAIT_HEADER;
      }
      break;
      
    case WAIT_DATA:
      serialBuffer[serialBufferIndex++] = data;
      calculatedChecksum += data;
      
      if (serialBufferIndex >= 3 + expectedLength) {
        serialState = WAIT_CHECKSUM;
      }
      break;
      
    case WAIT_CHECKSUM:
      receivedChecksum = data;
      
      // 验证校验和
      if (1/*receivedChecksum == calculatedChecksum*/) {
        // 校验成功，处理命令
        handleSerialCommand();
      } else {
        Serial.printf("Serial checksum error! received=0x%02X, calculated=0x%02X\n", 
                     receivedChecksum, calculatedChecksum);
      }
      
      // 重置状态机
      serialState = WAIT_HEADER;
      serialBufferIndex = 0;
      break;
  }
}

// 处理串口命令
void handleSerialCommand() {
  uint8_t cmd = serialBuffer[1];
  uint8_t length = serialBuffer[2];
  
  Serial.printf("Received command: 0x%02X, length: %d\n", cmd, length);
  
  switch (cmd) {
    case SERIAL_CMD_D7: {
      // 处理0xD7命令 - 设置开关/亮度
      Serial.println("Processing D7 command - Set power/brightness");
      if (length >= 1) {
        uint8_t value = serialBuffer[3];
        if (value == 0) {
          // 关闭灯
          lightPower = false;
          animSystem.stop();
          Serial.println("Light turned OFF");
        } else if (value >= 1 && value <= 100) {
          // 打开灯并设置亮度
          lightPower = true;
          set_brightness(value);
          // 如果动画系统还没启动，启动它
          if (!animSystem.isRunning()) {
            animSystem.start();
          }
          Serial.printf("Light turned ON, brightness: %d%%\n", value);
        }
        
        // 发送响应
        uint8_t response[] = {value};
        sendSerialResponse(SERIAL_CMD_D7, response, 1);
      }
      break;
    }
      
    case SERIAL_CMD_DA: {
      // 处理0xDA命令 - 设置色温
      Serial.println("Processing DA command - Set color temperature");
      if (length >= 1) {
        uint8_t colorTemp = serialBuffer[3];
        if (colorTemp >= 1 && colorTemp <= 61) {
          currentColorTemp = colorTemp;
          colorTempMode = true; // 进入色温模式
          lightPower = true;    // 确保灯是打开的          
          
          // 设置色温效果
          colorTempEffect.setColorTemp(colorTemp);
          colorTempEffect.setDuvIndex(3); // 默认DUV=0
          
          // 如果动画系统还没启动，先启动
          if (!animSystem.isRunning()) {
            animSystem.setEffect(&colorTempEffect);
            animSystem.start();
          } else {
            // 如果已经在运行，直接更新效果
            animSystem.updateCurrentEffect();
          }
          
          Serial.printf("Color temperature mode set to: %d, light ON\n", colorTemp);
        }        
        // 发送响应
        uint8_t response[] = {colorTemp};
        sendSerialResponse(SERIAL_CMD_DA, response, 1);
      }
      break;
    }
      
    case SERIAL_CMD_DD: {
      // 处理0xDD命令 - 设置动态场景
      Serial.println("Processing DD command - Set dynamic scene");
      if (length >= 1) {
        uint8_t scene = serialBuffer[3];
        if (scene <= 30) {
          currentScene = scene;
          currentAnimEffect = scene % MAX_ANIM_EFFECTS; // 映射到动画效果
          colorTempMode = false; // 退出色温模式
          
          // 根据场景设置不同的动画效果
          AnimEffect* effects[] = {&breathEffect, &rainbowEffect, &blinkEffect, &gradientEffect, &whiteStaticEffect, &imageDataEffect, &czcxEffect, &jl3Effect, &lt2Effect, &lt3Effect};
          if (lightPower) {
            // 如果动画系统还没启动，先启动
            if (!animSystem.isRunning()) {
              animSystem.setEffect(effects[currentAnimEffect]);
              animSystem.start();
            } else {
              // 如果已经在运行，直接更新效果
              animSystem.setEffect(effects[currentAnimEffect]);
              animSystem.updateCurrentEffect();
            }
          }
          
          Serial.printf("Scene set to: %d (effect: %d)\n", scene, currentAnimEffect);
        }
        
        // 发送响应
        uint8_t response[] = {scene};
        sendSerialResponse(SERIAL_CMD_DD, response, 1);
      }
      break;
    }
      
    case SERIAL_CMD_A2: {
      // 处理0xA2命令 - 查询灯运行状态
      Serial.println("Processing A2 command - Query light status");
      
      // 发送状态响应
      uint8_t status[] = {
        static_cast<uint8_t>(lightPower ? 0x01 : 0x00),  // 开关状态
        get_brightness(),                    // 当前亮度
        currentColorTemp,                    // 当前色温
        currentScene,                        // 当前场景
        static_cast<uint8_t>(colorTempMode ? 0xFF : currentAnimEffect)  // 当前模式/动画效果
      };
      sendSerialResponse(SERIAL_CMD_A2, status, 5);
      break;
    }
      
    default:
      Serial.println("Unknown command");
      break;
  }
  
  // 打印接收到的数据（调试用）
  Serial.print("Data: ");
  for (int i = 3; i < 3 + length; i++) {
    Serial.printf("0x%02X ", serialBuffer[i]);
  }
  Serial.println();
}

// 发送串口响应
void sendSerialResponse(uint8_t cmd, uint8_t* data, uint8_t length) {
  uint8_t checksum = 0x06 + cmd + length;  // 响应帧头为0x06
  
  // 发送帧头 (响应帧头为0x06)
  Serial.write(0x06);  
  // 发送命令
  Serial.write(cmd);  
  // 发送长度
  Serial.write(length);  
  // 发送数据
  if (data && length > 0) {
    for (int i = 0; i < length; i++) {
      Serial.write(data[i]);
      checksum += data[i];
    }
  }  
  // 发送校验和
  Serial.write(checksum);
}

// 测试串口协议的函数（可选）
void testSerialProtocol() {
  // 测试0xda命令：设置LED模式为呼吸灯
  uint8_t testData[] = {0x04}; // LED_BREATH模式
  uint8_t checksum = SERIAL_FRAME_HEADER + SERIAL_CMD_DA + 1;
  for (int i = 0; i < 1; i++) {
    checksum += testData[i];
  }  
  Serial.write(SERIAL_FRAME_HEADER);
  Serial.write(SERIAL_CMD_DA);
  Serial.write(1);
  Serial.write(testData[0]);
  Serial.write(checksum);
  
  Serial.println("Test command sent");
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("client connected");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("client disconnect");
    pServer->startAdvertising();  // 断开后自动重启广播
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.print("received data: ");
      for (int i = 0; i < value.length(); i++)
      {
        Serial.print(value[i]);
      }
      Serial.print("\n");
    }
  }
};

// ----- LED任务变量 -----
unsigned long lastLedUpdate = 0;
int ledState = LOW;
int brightness = 0;
int breathDirection = 1;

void startBreathEffect() {
  ledcAttachPin(LED_PIN, 0);  // 绑定通道
  ledcSetup(0, 5000, 8);      // 5kHz，8位分辨率
  // 后续ledcWrite() 在 loop 中改变亮度
}

void stopBreathEffect() {
  ledcDetachPin(LED_PIN);     // 取消 PWM
  pinMode(LED_PIN, OUTPUT);   // 恢复为数字输出
  digitalWrite(LED_PIN, LOW);
}

// 设置LED状态
void setLedMode(int mode) {
  currentMode = static_cast<LedMode>(mode);
  lastLedUpdate = millis();
  ledState = LOW;
  brightness = 0;
  breathDirection = 1;
 if(currentMode==4)
 {
    startBreathEffect();
 }else{
    stopBreathEffect();
 }
}

bool readButton() {
  static uint32_t lastDebounceTime = 0;
  static bool lastStableState = HIGH;  // 初始为抬起状态
  static bool lastReadState = HIGH;
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastReadState) {
    lastDebounceTime = millis();  // 状态改变，重置时间
  }
  if ((millis() - lastDebounceTime) > 150) {
    // 状态稳定超过防抖时间
    if (reading != lastStableState) {
      lastStableState = reading;
      if (lastStableState == LOW) {
        return true;  // 检测到一次有效按下
      }
    }
  }
  lastReadState = reading;
  return false;  // 没有新的按下
}

// 切换动画效果
void switchAnimationEffect() {
  currentAnimEffect = (currentAnimEffect + 1) % MAX_ANIM_EFFECTS;
  
  AnimEffect* effects[] = {&breathEffect, &rainbowEffect, &blinkEffect, &gradientEffect, &whiteStaticEffect, &imageDataEffect, &czcxEffect, &jl3Effect, &lt2Effect, &lt3Effect};
  const char* effectNames[] = {"Breath", "Rainbow", "Blink", "Gradient", "WhiteStatic", "ImageData", "CZCX", "JL3", "LT2", "LT3"};
  
  // 如果动画系统还没启动，先启动
  if (!animSystem.isRunning()) {
    animSystem.setEffect(effects[currentAnimEffect]);
    animSystem.start();
  } else {
    // 如果已经在运行，直接更新效果
    animSystem.setEffect(effects[currentAnimEffect]);
    animSystem.updateCurrentEffect();
  }
  
  Serial.printf("Switched to animation: %s\n", effectNames[currentAnimEffect]);
}
// 🔘 按键检测任务（绑定 core 0）
void TaskReadButton(void* pvParameters) {
  Serial.println("TaskReadButton");
  while (true) {
    if (readButton()) {
        // 切换LED状态
        currentMode = static_cast<LedMode>((currentMode + 1) % 5);
        setLedMode(currentMode);
        Serial.print("[BTN] Switched to LED mode: ");
        Serial.println(currentMode);        
        // 同时切换动画效果
        switchAnimationEffect();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 小延时避免占满CPU
  }
}

// 💡 LED显示任务（绑定 core 1）
void TaskLedControl(void* pvParameters) {
   Serial.println("TaskLedControl"); 
  while (true) {
    unsigned long now = millis();

    switch (currentMode) {
      case LED_OFF:
        digitalWrite(LED_PIN, LOW);
        break;

      case LED_ON:
        digitalWrite(LED_PIN, HIGH);
        break;
      case LED_BLINK_SLOW:
       digitalWrite(LED_PIN, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;

      case LED_BLINK_FAST:
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        break;

      case LED_BREATH:
        if (now - lastLedUpdate >= 20) {
          brightness += breathDirection * 5;
          if (brightness >= 255) {
            brightness = 255;
            breathDirection = -1;
          } else if (brightness <= 0) {
            brightness = 0;
            breathDirection = 1;
          }
          ledcWrite(0, brightness);
          lastLedUpdate = now;
        }
        break;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);  // 避免任务跑满
  }
}

// 📡 串口通信任务（绑定 core 0）
void TaskSerialComm(void* pvParameters) {
  Serial.println("TaskSerialComm started");  
  while (true) {
    if (Serial.available()) {
      uint8_t data = Serial.read();
      processSerialData(data);
    }    
    // 检查超时
    if (serialState != WAIT_HEADER && 
        (millis() - lastSerialReceiveTime) > SERIAL_TIMEOUT_MS) {
      Serial.println("Serial timeout, resetting state machine");
      serialState = WAIT_HEADER;
      serialBufferIndex = 0;
    }    
    vTaskDelay(1 / portTICK_PERIOD_MS);  // 1ms延时，避免占满CPU
  }
}

int colorTemp=1;
void setup() {
  Serial.begin(115200); 
  sid_rmt_init();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  ledcAttachPin(LED_PIN, 0);     // 使用 PWM 通道0
  ledcSetup(0, 5000, 8);         // 5kHz, 8-bit
  delay(1000);
  dimmer_blank();
  
  // 初始化动画系统
  animSystem.init();
  
  // 初始化默认状态
  currentColorTemp = 1;
  colorTempMode = false;  // 默认不是色温模式
  lightPower = true;
  
  // 设置默认动画效果（呼吸灯）
  animSystem.setEffect(&breathEffect);
  animSystem.start();
 initIR();
  // 创建按键检测任务（core 0）
  xTaskCreatePinnedToCore(
    TaskReadButton,    // 任务函数
    "ButtonTask",      // 名称
    2048,              // 堆栈大小
    NULL,              // 参数
    1,                 // 优先级
    NULL,              // 任务句柄
    0                  // 跑在 core 0
  );
  // 创建LED控制任务（core 1）
  xTaskCreatePinnedToCore(
    TaskLedControl,
    "LedTask",
    2048,
    NULL,
    1,
    NULL,
    1                  // 跑在 core 1
  );
  BLEDevice::init("RGB Dimmer");  // 设备名
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello from ESP32");
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE Ready,wait connect"); 
  Serial.end();  // 停止默认串口调试输出
  Serial.begin(9600);//重新初始化为通信用途
  Serial.println("Serial communication initialized at 9600 baud");  
  // 创建串口通信任务（core 0）
  xTaskCreatePinnedToCore(
    TaskSerialComm,    // 任务函数
    "SerialTask",      // 名称
    4096,              // 堆栈大小（串口处理需要更多栈空间）
    NULL,              // 参数
    2,                 // 优先级（比按键任务高）
    NULL,              // 任务句柄
    0                  // 跑在 core 0
  );
}

int current_lght=0;

// 处理红外遥控码
void handleIRCode(uint32_t code) {
  switch (code) {
         case 0xFFA25D:  // 0键 - 开关灯光
       lightPower = !lightPower;
       if (!lightPower) {
         // 关闭灯光
         dimmer_blank();
         Serial.println("IR: Light OFF");
       } else if (colorTempMode) {
         // 如果在色温模式下开启，重新发送当前色温数据
         uint8_t r, g, b;
         getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
         
         uint8_t staticFrame[108];
         for (int i = 0; i < 36; i++) {
           staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
           staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
           staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
         }
         send_data(staticFrame, 108, 0xFFFF);
         Serial.printf("IR: Light ON (Color temp mode, temp=%d)\n", colorTemp);
       }
       break;
      
         case 0xFFE21D:  // 1键 - 色温
       lightPower = true;
       colorTemp=2;
       currentColorTemp = colorTemp;
       colorTempMode = true; // 进入色温模式
       lightPower = true;    // 确保灯是打开的          
       
       // 停止动画系统，直接发送静态色温数据
       if (animSystem.isRunning()) {
         animSystem.stop();
       }
       
       // 直接发送色温数据
       uint8_t r, g, b;
       getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
       
       // 创建静态帧数据（应用亮度调节）
       uint8_t staticFrame[108]; // 6x6x3 = 108字节
       for (int i = 0; i < 36; i++) { // 36个LED
         // 应用亮度调节
         staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
         staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
         staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
       }
       
       // 直接发送数据
       send_data(staticFrame, 108, 0xFFFF);
       Serial.printf("IR: Color temp mode, temp=%d, brightness=%d%%, R=%d G=%d B=%d\n", colorTemp, global_brightness, r, g, b);
       break;
      
         case 0xFFE01F:  // 色温减
     {
       lightPower = true;
       if(colorTemp > 1)
       {
         colorTemp--;
       }
       currentColorTemp = colorTemp;
       colorTempMode = true; // 进入色温模式
       lightPower = true;    // 确保灯是打开的          
       
       // 停止动画系统，直接发送静态色温数据
       if (animSystem.isRunning()) {
         animSystem.stop();
       }
       
       // 直接发送色温数据
       uint8_t r, g, b;
       getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
       
       // 创建静态帧数据（应用亮度调节）
       uint8_t staticFrame[108]; // 6x6x3 = 108字节
       for (int i = 0; i < 36; i++) { // 36个LED
         // 应用亮度调节
         staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
         staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
         staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
       }
       
       // 直接发送数据
       send_data(staticFrame, 108, 0xFFFF);
       Serial.printf("IR: Color temp decreased to %d, brightness=%d%%, R=%d G=%d B=%d\n", colorTemp, global_brightness, r, g, b);
     }
           case 0xFF906F:  // 色温加
     {
       lightPower = true;
       if(colorTemp < 61)
       {
         colorTemp++;
       }
       currentColorTemp = colorTemp;
       colorTempMode = true; // 进入色温模式
       lightPower = true;    // 确保灯是打开的          
       
       // 停止动画系统，直接发送静态色温数据
       if (animSystem.isRunning()) {
         animSystem.stop();
       }
       
       // 直接发送色温数据
       uint8_t r, g, b;
       getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
       
       // 创建静态帧数据（应用亮度调节）
       uint8_t staticFrame[108]; // 6x6x3 = 108字节
       for (int i = 0; i < 36; i++) { // 36个LED
         // 应用亮度调节
         staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
         staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
         staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
       }
       
       // 直接发送数据
       send_data(staticFrame, 108, 0xFFFF);
       Serial.printf("IR: Color temp increased to %d, brightness=%d%%, R=%d G=%d B=%d\n", colorTemp, global_brightness, r, g, b);
     }
         
         case 0xFF02FD:  // 上键 - 增加亮度
       if (global_brightness < 100) {
         global_brightness += 10;
         if (global_brightness > 100) global_brightness = 100;
         Serial.printf("IR: Brightness increased to %d%%\n", global_brightness);
         
         // 如果在色温模式下，重新发送当前色温数据
         if (colorTempMode && lightPower) {
           uint8_t r, g, b;
           getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
           
           uint8_t staticFrame[108];
           for (int i = 0; i < 36; i++) {
             staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
             staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
             staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
           }
           send_data(staticFrame, 108, 0xFFFF);
         }
       }
       break;
      
             case 0xFF9867:  // 下键 - 减少亮度
         if (global_brightness > 10) {
           global_brightness -= 10;
           if (global_brightness < 10) global_brightness = 10;
           Serial.printf("IR: Brightness decreased to %d%%\n", global_brightness);
           
           // 如果在色温模式下，重新发送当前色温数据
           if (colorTempMode && lightPower) {
             uint8_t r, g, b;
             getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
             
             uint8_t staticFrame[108];
             for (int i = 0; i < 36; i++) {
               staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
               staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
               staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
             }
             send_data(staticFrame, 108, 0xFFFF);
           }
         }
         break;   
              case 0xFF42BD:  // 7键 - 色温模式：1600K，亮度100%，DUV+6
         {
           lightPower = true;
           colorTemp = 1;  // 1600K对应色温索引1
           currentColorTemp = colorTemp;
           colorTempMode = true; // 进入色温模式
           lightPower = true;    // 确保灯是打开的
           
           // 设置亮度为100%
           global_brightness = 100;
           
           // 停止动画系统，直接发送静态色温数据
           if (animSystem.isRunning()) {
             animSystem.stop();
           }
           
           // 直接发送色温数据（DUV+6对应索引1）
           uint8_t r, g, b;
           getColorTempRGBWithDuv(colorTemp, 1, &r, &g, &b);
           
           // 创建静态帧数据（应用亮度调节）
           uint8_t staticFrame[108]; // 6x6x3 = 108字节
           for (int i = 0; i < 36; i++) { // 36个LED
             // 应用亮度调节
             staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
             staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
             staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
           }
           
           // 直接发送数据
           send_data(staticFrame, 108, 0xFFFF);
           Serial.printf("IR: Color temp mode 1600K, brightness=100%%, DUV+6, R=%d G=%d B=%d\n", r, g, b);
         }
         break;
      
    case 0xFF4AB5:  // 8键 - 色温模式：4000K，亮度100%，DUV:0
      {
        lightPower = true;
        colorTemp = 20;  // 4000K对应色温索引20
        currentColorTemp = colorTemp;
        colorTempMode = true; // 进入色温模式
        lightPower = true;    // 确保灯是打开的
        
        // 设置亮度为100%
        global_brightness = 100;
        
        // 停止动画系统，直接发送静态色温数据
        if (animSystem.isRunning()) {
          animSystem.stop();
        }
        
        // 直接发送色温数据（DUV:0对应索引3）
        uint8_t r, g, b;
        getColorTempRGBWithDuv(colorTemp, 3, &r, &g, &b);
        
        // 创建静态帧数据（应用亮度调节）
        uint8_t staticFrame[108]; // 6x6x3 = 108字节
        for (int i = 0; i < 36; i++) { // 36个LED
          // 应用亮度调节
          staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
          staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
          staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
        }
        
        // 直接发送数据
        send_data(staticFrame, 108, 0xFFFF);
        Serial.printf("IR: Color temp mode 4000K, brightness=100%%, DUV:0, R=%d G=%d B=%d\n", r, g, b);
      }
      break;
      
    case 0xFFA857:  // 9键 - 色温模式：2700K，亮度50%，DUV:-3
      {
        lightPower = true;
        colorTemp = 10;  // 2700K对应色温索引10
        currentColorTemp = colorTemp;
        colorTempMode = true; // 进入色温模式
        lightPower = true;    // 确保灯是打开的
        
        // 设置亮度为50%
        global_brightness = 50;
        
        // 停止动画系统，直接发送静态色温数据
        if (animSystem.isRunning()) {
          animSystem.stop();
        }
        
        // 直接发送色温数据（DUV:-3对应索引4）
        uint8_t r, g, b;
        getColorTempRGBWithDuv(colorTemp, 4, &r, &g, &b);
        
        // 创建静态帧数据（应用亮度调节）
        uint8_t staticFrame[108]; // 6x6x3 = 108字节
        for (int i = 0; i < 36; i++) { // 36个LED
          // 应用亮度调节
          staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
          staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
          staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
        }
        
        // 直接发送数据
        send_data(staticFrame, 108, 0xFFFF);
        Serial.printf("IR: Color temp mode 2700K, brightness=50%%, DUV:-3, R=%d G=%d B=%d\n", r, g, b);
      }
      break;
      
    case 0xFF10EF:  // OK键 - 色温模式：6500K，亮度50%，DUV:-3
      {
        lightPower = true;
        colorTemp = 30;  // 6500K对应色温索引30
        currentColorTemp = colorTemp;
        colorTempMode = true; // 进入色温模式
        lightPower = true;    // 确保灯是打开的
        
        // 设置亮度为50%
        global_brightness = 50;
        
        // 停止动画系统，直接发送静态色温数据
        if (animSystem.isRunning()) {
          animSystem.stop();
        }
        
        // 直接发送色温数据（DUV:-3对应索引4）
        uint8_t r, g, b;
        getColorTempRGBWithDuv(colorTemp, 4, &r, &g, &b);
        
        // 创建静态帧数据（应用亮度调节）
        uint8_t staticFrame[108]; // 6x6x3 = 108字节
        for (int i = 0; i < 36; i++) { // 36个LED
          // 应用亮度调节
          staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
          staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
          staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
        }
        
        // 直接发送数据
        send_data(staticFrame, 108, 0xFFFF);
        Serial.printf("IR: Color temp mode 6500K, brightness=50%%, DUV:-3, R=%d G=%d B=%d\n", r, g, b);
      }
      break;
      
    case 0xFF38C7:  // 左键 - 色温模式：2700K，亮度50%，DUV:-3
      {
        lightPower = true;
        colorTemp = 10;  // 2700K对应色温索引10
        currentColorTemp = colorTemp;
        colorTempMode = true; // 进入色温模式
        lightPower = true;    // 确保灯是打开的
        
        // 设置亮度为50%
        global_brightness = 50;
        
        // 停止动画系统，直接发送静态色温数据
        if (animSystem.isRunning()) {
          animSystem.stop();
        }
        
        // 直接发送色温数据（DUV:-3对应索引4）
        uint8_t r, g, b;
        getColorTempRGBWithDuv(colorTemp, 4, &r, &g, &b);
        
        // 创建静态帧数据（应用亮度调节）
        uint8_t staticFrame[108]; // 6x6x3 = 108字节
        for (int i = 0; i < 36; i++) { // 36个LED
          // 应用亮度调节
          staticFrame[i*3 + 0] = (uint8_t)((r * global_brightness) / 100);
          staticFrame[i*3 + 1] = (uint8_t)((g * global_brightness) / 100);
          staticFrame[i*3 + 2] = (uint8_t)((b * global_brightness) / 100);
        }
        
        // 直接发送数据
        send_data(staticFrame, 108, 0xFFFF);
        Serial.printf("IR: Color temp mode 2700K, brightness=50%%, DUV:-3, R=%d G=%d B=%d\n", r, g, b);
      }
      break;
      
    case 0xFF5AA5:  // 循环切换动画效果
      // 退出色温模式
      colorTempMode = false;
      
      // 切换到下一个动画效果
      currentAnimEffect = (currentAnimEffect + 1) % MAX_ANIM_EFFECTS;
      
      if (lightPower) {
        // 根据当前动画效果索引选择对应的效果
        AnimEffect* effects[] = {&breathEffect, &rainbowEffect, &blinkEffect, &gradientEffect, &whiteStaticEffect, &imageDataEffect, &czcxEffect, &jl3Effect, &lt2Effect, &lt3Effect};
        const char* effectNames[] = {"Breath", "Rainbow", "Blink", "Gradient", "WhiteStatic", "ImageData", "CZCX", "JL3", "LT2", "LT3"};
        
        if (!animSystem.isRunning()) {
          animSystem.setEffect(effects[currentAnimEffect]);
          animSystem.start();
        } else {
          animSystem.setEffect(effects[currentAnimEffect]);
          animSystem.updateCurrentEffect();
        }
        
        Serial.printf("IR: Cycle to animation %d: %s\n", currentAnimEffect, effectNames[currentAnimEffect]);
      }
      break;

  
    default:
      Serial.printf("IR: Unknown code 0x%X\n", code);
      break;
  }
}

void loop() {
    // 主循环 - 处理红外接收
    uint32_t irCode = readIR();
    if (irCode != 0) {
        Serial.printf("IR: Unknown code 0x%X\n", irCode);
       handleIRCode(irCode);
    }
    
    delay(50);  // 50ms延时，平衡响应速度和CPU占用
}


