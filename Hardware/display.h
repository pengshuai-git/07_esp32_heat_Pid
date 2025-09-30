#ifndef DISPLAY_H
#define DISPLAY_H

// 需要 I2C 类型定义
#include "driver/i2c.h"

// 初始化OLED（I2C 端口/引脚/频率/设备地址）
void display_init(i2c_port_t port, int sda_io, int scl_io, uint32_t clk_hz, uint8_t addr);

// 更新显示 (温度, 设定, 电池)
void display_update(float temp, float setpoint, float battery);

// 可选：清屏
void display_clear(void);

// 显示三行任意文本（超长自动截断到屏宽）
void display_show_text(const char *line1, const char *line2, const char *line3);

#endif
