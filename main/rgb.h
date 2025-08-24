#ifndef RGB_H
#define RGB_H

#include <stdint.h>  // 添加uint8_t类型定义

// 初始化RGB LED PWM
void rgb_init(void);

// 设置RGB颜色 (0-255)
void set_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif