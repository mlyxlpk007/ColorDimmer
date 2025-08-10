# TracePro SMD2727灯珠 + 单颗透镜光学模拟工程

## 📁 项目结构

```
E:\Works\TPlens\
├── README.md                    # 项目说明文件
├── Models\                      # 模型文件目录
│   ├── SMD2727_LED.tpf         # SMD2727灯珠模型
│   ├── Single_Lens.tpf         # 单颗透镜模型
│   └── Assembly.tpf            # 组装模型
├── Analysis\                    # 分析文件目录
│   ├── Intensity_Distribution.ray
│   ├── Color_Analysis.ray
│   └── Efficiency_Report.txt
├── Results\                     # 结果文件目录
│   ├── IES_Files\              # IES光强分布文件
│   ├── Images\                 # 分析图像
│   └── Data\                   # 数据文件
├── Documentation\               # 文档目录
│   ├── TRACEPRO_USAGE_GUIDE.md # 使用指南
│   └── tracepro_smd2727_lens_simulation.txt # 详细说明
└── Scripts\                     # 脚本目录
    ├── tracepro_smd2727_lens_script.txt     # TracePro脚本
    └── tracepro_automation.py               # Python自动化脚本
```

## 🎯 项目概述

本工程专门用于SMD2727 RGB LED灯珠配合单颗非球面透镜的光学性能模拟和分析，适用于：
- RGB调光器光学设计
- LED照明系统优化
- 光强分布分析
- 色度性能评估

## 🚀 快速开始

### 1. 环境准备
- 安装TracePro 7.x或更高版本
- 安装Python 3.7+及相关依赖包
- 确保系统满足硬件要求

### 2. 运行模拟
```bash
# 方法一：使用Python自动化脚本
cd Scripts
python tracepro_automation.py

# 方法二：手动执行TracePro脚本
# 打开Scripts/tracepro_smd2727_lens_script.txt
# 按照注释逐步执行
```

### 3. 查看结果
- 分析结果保存在 `Results/` 目录
- 详细报告在 `Analysis/` 目录
- 使用指南在 `Documentation/` 目录

## 📊 主要功能

- **SMD2727灯珠建模**: 精确的物理和光学参数
- **非球面透镜设计**: 可调参数的光学系统
- **RGB光源模拟**: 多色光混合和色温控制
- **光强分布分析**: 远场和近场光强分布
- **色度性能评估**: CCT、显色指数分析
- **效率计算**: 光学效率和能量利用率
- **IES文件导出**: 标准格式光强分布文件

## ⚙️ 技术参数

### SMD2727灯珠
- 封装尺寸: 2.7mm × 2.7mm × 1.1mm
- LED芯片: 1.0mm × 1.0mm × 0.1mm
- 发光面积: 0.8mm × 0.8mm
- 光谱特性: RGB三色LED

### 单颗透镜
- 直径: 8.0mm
- 厚度: 3.5mm
- 焦距: 6.0mm
- 材料: PMMA (折射率1.49)
- 表面: 非球面前表面 + 平面后表面

## 📈 预期结果

- **峰值光强**: 5000-8000 cd
- **半功率角**: 20° ± 5°
- **光强均匀性**: >85%
- **色温范围**: 2700K - 6500K
- **光学效率**: >85%

## 🔧 自定义配置

可以通过修改以下文件来调整参数：
- `Scripts/tracepro_automation.py`: Python脚本参数
- `Scripts/tracepro_smd2727_lens_script.txt`: TracePro命令参数
- `Documentation/tracepro_smd2727_lens_simulation.txt`: 详细参数说明

## 📞 技术支持

如有问题，请参考：
- `Documentation/TRACEPRO_USAGE_GUIDE.md`: 详细使用指南
- `Documentation/tracepro_smd2727_lens_simulation.txt`: 技术说明文档

## 📝 更新日志

- **v1.0**: 初始版本，基础建模功能
- **v1.1**: 添加自动化脚本
- **v1.2**: 优化参数配置
- **v1.3**: 完善项目结构

---

**项目位置**: E:\Works\TPlens  
**创建时间**: 2024年12月  
**版本**: v1.3  
**作者**: AI Assistant
