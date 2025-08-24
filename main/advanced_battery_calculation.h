/**
 * 高级电池电量计算模块
 * 基于电池特性曲线和多种补偿算法
 */

#ifndef ADVANCED_BATTERY_CALCULATION_H
#define ADVANCED_BATTERY_CALCULATION_H

#include <stdint.h>
#include <stdbool.h>

// 电池类型定义
typedef enum {
    BATTERY_TYPE_LIPO_1S,      // 单节锂聚合物电池 (3.0~4.2V)
    BATTERY_TYPE_LION_18650,   // 18650锂离子电池 (2.5~4.2V)
    BATTERY_TYPE_NIMH,         // 镍氢电池 (0.9~1.4V)
    BATTERY_TYPE_CUSTOM        // 自定义电池
} battery_type_t;

// 电池放电曲线点
typedef struct {
    float voltage;             // 电压 (V)
    float soc;                // 电量百分比 (0-100%)
} battery_curve_point_t;

// 电池状态信息
typedef struct {
    float voltage_filtered;    // 滤波后电压 (V)
    float voltage_no_load;     // 估算空载电压 (V)
    float current_ma;          // 负载电流 (mA, 如果可测)
    float temperature;         // 温度 (°C, 如果可测)
    float soc_percentage;      // 电量百分比 (0-100%)
    float remaining_mah;       // 剩余容量 (mAh)
    int health_percentage;     // 电池健康度 (0-100%)
    bool is_charging;          // 是否在充电
    bool is_valid;             // 数据是否有效
} battery_status_t;

// 电池配置
typedef struct {
    battery_type_t type;                    // 电池类型
    float nominal_capacity_mah;             // 标称容量 (mAh)
    float cutoff_voltage;                   // 截止电压 (V)
    float full_voltage;                     // 满电电压 (V)
    float internal_resistance_mohm;         // 内阻 (mΩ)
    const battery_curve_point_t* curve;     // 放电曲线
    int curve_points;                       // 曲线点数
    float temp_coefficient;                 // 温度系数 (%/°C)
} battery_config_t;

// 预定义的电池配置
extern const battery_config_t CONFIG_LIPO_1S_2000MAH;
extern const battery_config_t CONFIG_18650_3000MAH;
extern const battery_config_t CONFIG_CUSTOM_5V_SYSTEM;

// 函数声明
void battery_calc_init(const battery_config_t* config);
battery_status_t battery_calc_update(float voltage, float current_ma, float temp_c);
float battery_calc_soc_from_voltage(float voltage, const battery_config_t* config);
float battery_calc_compensate_load(float loaded_voltage, float current_ma, float internal_resistance);
float battery_calc_compensate_temperature(float soc, float temp_c, float temp_coefficient);
void battery_calc_print_status(const battery_status_t* status);

// 高级功能
bool battery_calc_is_low_battery(const battery_status_t* status, float threshold);
float battery_calc_estimate_runtime_hours(const battery_status_t* status, float avg_current_ma);
int battery_calc_health_assessment(float current_capacity, float nominal_capacity);

#endif // ADVANCED_BATTERY_CALCULATION_H 