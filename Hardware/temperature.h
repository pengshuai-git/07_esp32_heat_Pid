#ifndef TEMPERATURE_H
#define TEMPERATURE_H

// 需要 ADC 类型定义（使用新HAL类型，避免旧驱动告警）
#include "hal/adc_types.h"
// 温度传感器常量
#define NTC_R25 10000.0     // 25°C时的电阻值 (10kΩ)
#define NTC_B 3950.0        // B常数
#ifndef NTC_REF_RES
#define NTC_REF_RES 10000.0 // 参考电阻 (10kΩ)
#endif

// 函数声明
// 初始化温度传感器（ADC通道、参考电阻、供电电压）
void temperature_init(adc_channel_t temp_channel, float ref_res_ohm, float vcc_volt);
float temperature_read(void);         // 读取当前温度 (°C)
int temperature_get_last_raw(void);   // 最近一次温度ADC原始值
int temperature_get_last_mv(void);    // 最近一次温度等效电压(mV)

// 温度报警阈值
#define TEMP_ALARM_HIGH 80.0    // 高温报警阈值 (°C)
#define TEMP_ALARM_LOW  5.0     // 低温报警阈值 (°C)

// 电池电压常量
#define BATTERY_VOLTAGE_MAX 4.2  // 最大电池电压 (V)
#define BATTERY_VOLTAGE_MIN 3.0  // 最小电池电压 (V)
#define BATTERY_LOW_THRESHOLD 20 // 低电量报警阈值 (%)

#endif // TEMPERATURE_H
