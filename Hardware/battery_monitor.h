#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

// 需要 ADC 类型与 uint32_t（使用 HAL 类型，避免旧驱动告警）
#include "hal/adc_types.h"
#include <stdint.h>

// 电池监控配置（由外部传入，避免硬编码）

// 函数声明
void battery_monitor_init(adc_channel_t channel, float divider, float vmin, float vmax);
float battery_read_voltage(void);                   // 读取电池电压 (V)
float battery_voltage_to_percentage(float voltage); // 电压转百分比
void battery_monitor_task(void *pvParameters);      // 电池监控任务（需先 init）
void start_battery_monitor(uint32_t interval_ms);   // 启动电池监控（周期）

#endif // BATTERY_MONITOR_H
