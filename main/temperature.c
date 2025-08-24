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

// 读取温度
float temperature_read(void) {
    // 读取ADC值
    int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
    
    // 转换为电压
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    
    // 简单线性转换为温度 (实际应根据传感器特性调整)
    // 假设0V对应0°C, 3.3V对应100°C
    float temperature = (voltage / 3300.0) * 100.0;
    
    // 添加一些随机噪声来模拟真实传感器
    temperature += ((float)(adc_reading % 10) - 5) * 0.1;
    
    ESP_LOGD(TAG, "ADC: %d, 电压: %" PRIu32 "mV, 温度: %.1f°C", adc_reading, voltage, temperature);
    
    return temperature;
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