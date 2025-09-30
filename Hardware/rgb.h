#ifndef RGB_H
#define RGB_H

#include <stdint.h>

// 初始化RGB LED PWM（传入三个GPIO，如 7,6,5）
void rgb_init(int gpio_r, int gpio_g, int gpio_b);

// 设置RGB颜色 (0-255)
void set_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif
