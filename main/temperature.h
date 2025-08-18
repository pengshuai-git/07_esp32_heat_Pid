#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <stdint.h>

// NTC参数 (10kΩ, B=3950)
#define NTC_REF_RES 10000.0  // 分压电阻
#define NTC_B 3950.0
#define NTC_R25 10000.0
#define NTC_A 3.9083e-3
#define NTC_B_CONST -5.775e-7

// 高温报警阈值 (°C)
#define ALARM_THRESHOLD 80.0

// 初始化ADC
void temperature_init(void);

// 读取温度 (°C)
float get_temperature(void);

// 读取电池电平 (%)
float get_battery_level(void);

#endif