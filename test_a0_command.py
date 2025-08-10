#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
0xA0命令测试脚本
用于验证串口命令格式和校验和计算
"""

def calculate_checksum(data):
    """计算校验和"""
    return sum(data) & 0xFF

def test_a0_command(brightness, color_temp, duv):
    """
    测试0xA0命令
    
    参数:
    - brightness: 亮度值 (0-1000)
    - color_temp: 色温值 (1-61)
    - duv: DUV值 (0x01-0x05)
    """
    # 计算亮度的高8位和低8位
    brightness_high = (brightness >> 8) & 0xFF
    brightness_low = brightness & 0xFF
    
    # 构建命令数据
    frame_header = 0x01
    command = 0xA0
    length = 0x04
    data = [brightness_high, brightness_low, color_temp, duv]
    
    # 计算校验和
    checksum = calculate_checksum([frame_header, command, length] + data)
    
    # 构建完整命令
    full_command = [frame_header, command, length] + data + [checksum]
    
    print(f"0xA0命令测试:")
    print(f"亮度: {brightness} (0-1000)")
    print(f"色温: {color_temp} (1-61)")
    print(f"DUV: 0x{duv:02X} (0x01-0x05)")
    print()
    
    print("发送命令:")
    print(f"帧头: 0x{frame_header:02X}")
    print(f"命令: 0x{command:02X}")
    print(f"长度: 0x{length:02X}")
    print(f"数据: {' '.join([f'0x{x:02X}' for x in data])}")
    print(f"校验: 0x{checksum:02X}")
    print()
    
    print("完整命令 (十六进制):")
    print(" ".join([f"{x:02X}" for x in full_command]))
    print()
    
    # 计算响应
    response_header = 0x06
    response_checksum = calculate_checksum([response_header, command, length, command])
    
    print("预期响应:")
    print(f"帧头: 0x{response_header:02X}")
    print(f"命令: 0x{command:02X}")
    print(f"长度: 0x{length:02X}")
    print(f"数据: 0x{command:02X}")
    print(f"校验: 0x{response_checksum:02X}")
    print()
    
    print("完整响应 (十六进制):")
    response = [response_header, command, length, command, response_checksum]
    print(" ".join([f"{x:02X}" for x in response]))
    
    return full_command, response

def main():
    """主函数"""
    print("=" * 50)
    print("0xA0串口命令测试工具")
    print("=" * 50)
    print()
    
    # 测试用例1: 亮度500, 色温20, DUV 0x02
    print("测试用例1:")
    test_a0_command(500, 20, 0x02)
    print()
    print("-" * 50)
    print()
    
    # 测试用例2: 亮度1000, 色温61, DUV 0x05
    print("测试用例2:")
    test_a0_command(1000, 61, 0x05)
    print()
    print("-" * 50)
    print()
    
    # 测试用例3: 亮度0, 色温1, DUV 0x01
    print("测试用例3:")
    test_a0_command(0, 1, 0x01)
    print()
    print("-" * 50)
    print()
    
    # 测试用例4: 亮度250, 色温30, DUV 0x03
    print("测试用例4:")
    test_a0_command(250, 30, 0x03)

if __name__ == "__main__":
    main() 