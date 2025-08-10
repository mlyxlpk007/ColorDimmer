# 红外接收头数字键打印功能总结

## 功能概述
已成功为RGB调光器的红外接收功能添加了完整的数字键打印功能，包括按键识别、功能说明和数值显示。

## 已添加的数字键映射

### 数字键 0-9
- **Key 0**: 开关灯光
  - 关屏时显示: "IR: Key 0 - Light OFF"
  - 开屏时显示: "IR: Key 0 - Light ON (Color Temp Mode, 100%)" 或 "IR: Key 0 - Light ON (Default Mode, 50%)"

- **Key 1**: 色温模式
  - 显示: "IR: Key 1 - Color temp mode, temp=X, DUV index Y, brightness=Z%"

- **Key 2**: 色温减/DUV减
  - 色温模式: "IR: Key 2 - Color temp decreased to X"
  - DUV模式: "IR: Key 2 - DUV decreased to X"

- **Key 3**: 色温加/DUV加
  - 色温模式: "IR: Key 3 - Color temp increased to X"
  - DUV模式: "IR: Key 3 - DUV increased to X"

- **Key 4**: 亮度增加
  - 显示: "IR: Key 4 - Brightness increased"

- **Key 5**: 亮度减少
  - 显示: "IR: Key 5 - Brightness decreased"

- **Key 6**: 切换色温/DUV调节模式
  - DUV模式: "IR: Key 6 - Switched to DUV adjustment mode"
  - 色温模式: "IR: Key 6 - Switched to color temperature adjustment mode"

- **Key 7**: 色温模式1600K
  - 显示: "IR: Key 7 - Color temp mode 1600K, DUV index X (transition)"

- **Key 8**: 色温模式4000K
  - 显示: "IR: Key 8 - Color temp mode 4000K, DUV index X (transition)"

- **Key 9**: 色温模式2700K
  - 显示: "IR: Key 9 - Color temp mode 2700K, DUV index X (transition)"

## 其他功能键
- **OK键**: "IR: OK Key - Color temp mode 6500K, DUV index X (transition)"
- **左键**: "IR: Left Key - Color temp mode 2700K, DUV index X (transition)"
- **菜单键**: "IR: Menu Key - Cycle to animation X: EffectName"
- **功能切换键**: "IR: Function Key - Switched to DUV/Color temperature adjustment mode"

## 增强的调试信息

### 红外码接收显示
- 原始接收: "IR Received: 0xXXXXXXXX (Decimal: XXXXXXXXX)"
- 处理开始: "Processing IR Code: 0xXXXXXXXX (Decimal: XXXXXXXXX)"
- 命令检测: "=== IR Command Detected ==="
- 命令处理完成: "=== IR Command Processed ==="

## 技术特点
1. **完整的数字键映射**: 覆盖了0-9所有数字键
2. **详细的调试信息**: 每个按键都有明确的功能说明
3. **数值显示**: 显示当前的色温、DUV、亮度等参数值
4. **状态跟踪**: 清晰显示当前的操作模式和状态变化
5. **十六进制和十进制**: 同时显示红外码的两种格式

## 使用方法
1. 通过串口监视器查看红外接收的调试信息
2. 按下遥控器的数字键，观察对应的功能说明
3. 长按按键时，会显示连续调节的信息
4. 所有按键操作都会在串口输出中记录

## 文件修改
- 主要修改: `src/main.cpp`
- 修改函数: `handleIRCode()`, `readIR()`, `loop()`
- 添加内容: 数字键case语句、调试打印信息、状态跟踪 