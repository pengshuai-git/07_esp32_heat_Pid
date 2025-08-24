#ifndef TEMPERATURE_H
#define TEMPERATURE_H

// 温度传感器常量
#define NTC_R25 10000.0     // 25°C时的电阻值 (10kΩ)
#define NTC_B 3950.0        // B常数
#define NTC_REF_RES 10000.0 // 参考电阻 (10kΩ)

// 函数声明
void temperature_init(void);          // 初始化温度传感器
float temperature_read(void);         // 读取当前温度 (°C)
float get_battery_level(void);        // 获取电池电量百分比 (0-100%)

// 温度报警阈值
#define TEMP_ALARM_HIGH 80.0    // 高温报警阈值 (°C)
#define TEMP_ALARM_LOW  5.0     // 低温报警阈值 (°C)

// 电池电压常量
#define BATTERY_VOLTAGE_MAX 4.2  // 最大电池电压 (V)
#define BATTERY_VOLTAGE_MIN 3.0  // 最小电池电压 (V)
#define BATTERY_LOW_THRESHOLD 20 // 低电量报警阈值 (%)

#endif // TEMPERATURE_H