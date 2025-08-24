/**
 * 电池电压监控模块
 * 实时监控电池电压并通过串口输出
 */

#include "battery_monitor.h"  // 添加对应的头文件
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

static esp_adc_cal_characteristics_t battery_adc_chars;

/**
 * 初始化电池监控ADC
 */
void battery_monitor_init(void) {
    ESP_LOGI(TAG, "初始化电池监控ADC");
    
    // 配置ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_12);
    
    // ADC校准
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &battery_adc_chars);
    
    ESP_LOGI(TAG, "电池监控ADC初始化完成");
}

/**
 * 读取电池电压
 * @return 电池电压 (V)
 */
float battery_read_voltage(void) {
    // 读取ADC原始值
    int adc_raw = adc1_get_raw(BATTERY_ADC_CHANNEL);
    
    // 转换为电压 (mV)
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &battery_adc_chars);
    
    // 考虑分压电路，计算实际电池电压
    float battery_voltage = (voltage_mv * BATTERY_VOLTAGE_DIVIDER) / 1000.0;
    
    return battery_voltage;
}

/**
 * 计算电池电量百分比
 * @param voltage 电池电压 (V)
 * @return 电量百分比 (0-100%)
 */
float battery_voltage_to_percentage(float voltage) {
    float percentage = ((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0;
    
    // 限制范围
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    return percentage;
}

/**
 * 电池监控任务
 */
void battery_monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "电池监控任务启动");
    
    while (1) {
        // 读取电池电压
        float voltage = battery_read_voltage();
        
        // 计算电量百分比
        float percentage = battery_voltage_to_percentage(voltage);
        
        // 读取ADC原始值用于调试
        int adc_raw = adc1_get_raw(BATTERY_ADC_CHANNEL);
        
        // 串口实时打印
        printf("=== 电池状态监控 ===\n");
        printf("电池电压: %.2f V\n", voltage);
        printf("电池电量: %.0f%%\n", percentage);
        printf("ADC原始值: %d\n", adc_raw);
        printf("-------------------\n");
        
        // ESP日志输出
        ESP_LOGI(TAG, "电池电压: %.2fV, 电量: %.0f%%, ADC: %d", voltage, percentage, adc_raw);
        
        // 电量状态指示
        if (percentage > 80) {
            ESP_LOGI(TAG, "电池电量充足 ✓");
        } else if (percentage > 50) {
            ESP_LOGI(TAG, "电池电量良好");
        } else if (percentage > 20) {
            ESP_LOGW(TAG, "电池电量偏低 ⚠️");
        } else {
            ESP_LOGE(TAG, "电池电量严重不足！请立即充电 ⚠️⚠️⚠️");
        }
        
        // 延时
        vTaskDelay(pdMS_TO_TICKS(BATTERY_MONITOR_INTERVAL));
    }
}

/**
 * 启动电池监控
 */
void start_battery_monitor(void) {
    // 初始化ADC
    battery_monitor_init();
    
    // 创建监控任务
    xTaskCreate(battery_monitor_task, "battery_monitor", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "电池监控已启动，每%d秒更新一次", BATTERY_MONITOR_INTERVAL / 1000);
} 