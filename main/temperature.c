#include "temperature.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include <math.h>
#include <inttypes.h>  // 添加这个头文件以支持PRIu32

static const char *TAG = "TEMP";

static esp_adc_cal_characteristics_t adc_chars;

// 初始化温度传感器 (使用ADC读取热敏电阻或其他传感器)
void temperature_init(void) {
    ESP_LOGI(TAG, "初始化温度传感器");
    
    // 配置ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);  // GPIO0 - 修改为新宏名
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12);  // GPIO1 - 修改为新宏名
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);  // 修改为新宏名
    
    ESP_LOGI(TAG, "温度传感器初始化完成");
}

// 读取温度 - NTC 10K-3950 精确计算
float temperature_read(void) {
    // 读取ADC值
    int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
    
    // 转换为电压 (mV)
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    
    // 计算热敏电阻阻值 (分压电路: Vout = Vcc * Rt / (R4 + Rt))
    // Rt = R4 * Vout / (Vcc - Vout)
    float voltage_v = voltage_mv / 1000.0f;
    float vcc = 3.3f;  // ESP32 供电电压
    float r4 = 10000.0f;  // 上拉电阻 10KΩ (你的电路)
    
    if (voltage_v >= vcc - 0.001f) {
        ESP_LOGW(TAG, "电压过高，可能传感器开路");
        return 999.0f;  // 开路指示
    }
    if (voltage_v <= 0.001f) {
        ESP_LOGW(TAG, "电压过低，可能传感器短路");
        return -999.0f;  // 短路指示
    }
    
    float rt = r4 * voltage_v / (vcc - voltage_v);
    
    // NTC 10K-3950 温度计算 (参考STM32代码的正确公式)
    // 使用简化的Beta方程: T_K = (beta * T0_K) / (T0_K * ln(R/R0) + beta)
    const float beta = 3950.0f;       // Beta系数
    const float R0 = 10000.0f;       // 25°C时的阻值
    const float T0 = 25.0f;          // 参考温度(°C)
    const float T0_K = T0 + 273.15f; // 参考温度(K)
    
    float T_K;
    if (rt > 0) {
        T_K = (beta * T0_K) / (T0_K * logf(rt / R0) + beta);
    } else {
        T_K = T0_K;  // 处理无效阻值
    }
    
    float temp_c = T_K - 273.15f;  // 转换为摄氏度
    
    // 误差校正 (根据你的经验，通常需要1-2°C校正)

    
    ESP_LOGI(TAG, "ADC: %d, 电压: %" PRIu32 "mV, 阻值: %.0fΩ, 温度: %.1f°C", 
             adc_reading, voltage_mv, rt, temp_c);
    
    return temp_c;
}

// 获取电池电量百分比
float get_battery_level(void) {
    // 读取电池电压ADC (假设通过分压电路连接到ADC1_CHANNEL_1)
    int adc_reading = adc1_get_raw(ADC1_CHANNEL_1);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    
    // 假设电池电压通过2:1分压电路连接
    float battery_voltage = (voltage * 2.0) / 1000.0;  // 转换为伏特
    
    // 计算电池百分比 (3.0V-4.2V 对应 0-100%)
    float percentage = ((battery_voltage - 3.0) / 1.2) * 100.0;
    
    // 限制范围
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    ESP_LOGD(TAG, "电池电压: %.2fV, 电量: %.0f%%", battery_voltage, percentage);
    
    return percentage;
}