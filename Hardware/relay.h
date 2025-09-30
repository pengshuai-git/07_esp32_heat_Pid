#ifndef RELAY_H
#define RELAY_H

#include <stdbool.h>

void relay_init(int gpio);
void relay_set(bool on);
void relay_toggle(void);
bool relay_get(void);

// PWM 调光模式（基于 LEDC）
// 初始化为 PWM 输出（freq_hz 典型 1000Hz），占空默认 0%
void relay_init_pwm(int gpio, int freq_hz);
// 设置占空（0-100）。若未 init_pwm，则自动回退为阈值控制（>=50% 置 ON，否则 OFF）
void relay_set_pwm_percent(int percent);
int relay_get_pwm_percent(void);

#endif

