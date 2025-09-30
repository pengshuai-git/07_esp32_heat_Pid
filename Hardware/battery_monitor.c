/**
 * 电池电压监控模块
 * 实时监控电池电压并通过串口输出
 */

#include "battery_monitor.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_shared.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

static adc_oneshot_unit_handle_t S_ADC_HANDLE = NULL;
static adc_cali_handle_t S_CALI_HANDLE = NULL;
static adc_channel_t S_BATT_CH = ADC_CHANNEL_4;
static float S_DIVIDER = 2.0f;
static float S_VMIN = 3.0f;
static float S_VMAX = 4.2f;
static uint32_t S_INTERVAL_MS = 2000;

void battery_monitor_init(adc_channel_t channel, float divider, float vmin, float vmax) {
    ESP_LOGI(TAG, "初始化电池监控ADC");
    S_BATT_CH = channel;
    S_DIVIDER = divider;
    S_VMIN = vmin;
    S_VMAX = vmax;
    
    // 复用全局 Oneshot ADC
    adc_shared_init_unit();
    S_ADC_HANDLE = adc_shared_unit();

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(S_ADC_HANDLE, S_BATT_CH, &chan_cfg);

    // 复用全局校准
    adc_shared_init_cali();
    S_CALI_HANDLE = adc_shared_cali();
    
    ESP_LOGI(TAG, "电池监控ADC初始化完成");
}

float battery_read_voltage(void) {
    int adc_raw = 0;
    adc_oneshot_read(S_ADC_HANDLE, S_BATT_CH, &adc_raw);
    int voltage_mv = 0;
    if (S_CALI_HANDLE) {
        adc_cali_raw_to_voltage(S_CALI_HANDLE, adc_raw, &voltage_mv);
    } else {
        voltage_mv = (int)((adc_raw * 3300) / 4095);
    }
    float battery_voltage = (voltage_mv * S_DIVIDER) / 1000.0f;
    return battery_voltage;
}

float battery_voltage_to_percentage(float voltage) {
    float percentage = ((voltage - S_VMIN) / (S_VMAX - S_VMIN)) * 100.0f;
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    return percentage;
}

void battery_monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "电池监控任务启动");
    while (1) {
        float voltage = battery_read_voltage();
        float percentage = battery_voltage_to_percentage(voltage);
        int adc_raw = 0;
        adc_oneshot_read(S_ADC_HANDLE, S_BATT_CH, &adc_raw);
        printf("=== 电池状态监控 ===\n");
        printf("电池电压: %.2f V\n", voltage);
        printf("电池电量: %.0f%%\n", percentage);
        printf("ADC原始值: %d\n", adc_raw);
        printf("-------------------\n");
        ESP_LOGI(TAG, "电池电压: %.2fV, 电量: %.0f%%, ADC: %d", voltage, percentage, adc_raw);
        if (percentage > 80) {
            ESP_LOGI(TAG, "电池电量充足 ✓");
        } else if (percentage > 50) {
            ESP_LOGI(TAG, "电池电量良好");
        } else if (percentage > 20) {
            ESP_LOGW(TAG, "电池电量偏低 ⚠️");
        } else {
            ESP_LOGE(TAG, "电池电量严重不足！请立即充电 ⚠️⚠️⚠️");
        }
        vTaskDelay(pdMS_TO_TICKS(S_INTERVAL_MS));
    }
}

void start_battery_monitor(uint32_t interval_ms) {
    S_INTERVAL_MS = interval_ms;
    xTaskCreate(battery_monitor_task, "battery_monitor", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "电池监控已启动，每%d秒更新一次", (int)(S_INTERVAL_MS / 1000));
}
