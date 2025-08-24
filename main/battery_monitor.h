#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

// 电池监控配置常量
#define BATTERY_ADC_CHANNEL ADC1_CHANNEL_4  // GPIO4
#define BATTERY_VOLTAGE_DIVIDER 2.0         // 分压比 2:1
#define BATTERY_VOLTAGE_MIN 3.0             // 最小电压 (V)
#define BATTERY_VOLTAGE_MAX 4.2             // 最大电压 (V)
#define BATTERY_MONITOR_INTERVAL 2000       // 监控间隔 (ms)

// 函数声明
void battery_monitor_init(void);                    // 初始化电池监控ADC
float battery_read_voltage(void);                   // 读取电池电压 (V)
float battery_voltage_to_percentage(float voltage); // 电压转百分比
void battery_monitor_task(void *pvParameters);      // 电池监控任务
void start_battery_monitor(void);                   // 启动电池监控

#endif // BATTERY_MONITOR_H 