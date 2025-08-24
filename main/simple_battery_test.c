/**
 * 简单的电池电压监控测试程序
 * 专门用于测试电池电压监控功能
 * 支持0~5V电压范围，带平滑滤波
 */

#include <stdio.h>
#include <inttypes.h>  // 添加这个头文件以支持PRIu32
#include <math.h>      // 添加这个头文件以支持fabs函数
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "BATTERY_TEST";

// 配置常量
#define BATTERY_ADC_CHANNEL ADC1_CHANNEL_4  // GPIO4
#define BATTERY_VOLTAGE_MIN 0.0             // 最小电压 (V) - 支持0V
#define BATTERY_VOLTAGE_MAX 5.0             // 最大电压 (V) - 支持5V
#define BATTERY_FULL_CHARGE 4.2             // 满电电压 (V)
#define BATTERY_LOW_CHARGE 3.0              // 低电压 (V)

// 平滑滤波配置
#define FILTER_SIZE 10                      // 滤波窗口大小
#define ALPHA 0.1                           // 指数滤波系数 (0-1, 越小越平滑)

static esp_adc_cal_characteristics_t adc_chars;

// 滤波相关变量
static float voltage_buffer[FILTER_SIZE] = {0};    // 电压缓冲区
static int buffer_index = 0;                       // 缓冲区索引
static bool buffer_full = false;                   // 缓冲区是否已满
static float filtered_voltage = 0.0;               // 指数滤波后的电压

/**
 * 移动平均滤波
 * @param new_value 新的电压值
 * @return 滤波后的电压值
 */
float moving_average_filter(float new_value) {
    // 将新值添加到缓冲区
    voltage_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % FILTER_SIZE;
    
    if (!buffer_full && buffer_index == 0) {
        buffer_full = true;
    }
    
    // 计算平均值
    float sum = 0.0;
    int count = buffer_full ? FILTER_SIZE : buffer_index;
    
    for (int i = 0; i < count; i++) {
        sum += voltage_buffer[i];
    }
    
    return sum / count;
}

/**
 * 指数滤波 (更平滑)
 * @param new_value 新的电压值
 * @return 滤波后的电压值
 */
float exponential_filter(float new_value) {
    if (filtered_voltage == 0.0) {
        // 第一次初始化
        filtered_voltage = new_value;
    } else {
        // 指数平滑滤波: Y(n) = α * X(n) + (1-α) * Y(n-1)
        filtered_voltage = ALPHA * new_value + (1.0 - ALPHA) * filtered_voltage;
    }
    return filtered_voltage;
}

/**
 * 读取ADC并转换为实际电压 (0~5V范围)
 * @return 实际电压值 (V)
 */
float read_voltage_0_5v(void) {
    // 读取ADC原始值
    int adc_reading = adc1_get_raw(BATTERY_ADC_CHANNEL);
    
    // 转换为ADC电压值（mV）
    uint32_t adc_voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    
    // ESP32C3的ADC在3.3V参考电压下，最大读取约3.3V
    // 为了读取0~5V，需要分压电路：5V -> 3.3V 分压比约为 5/3.3 = 1.515
    float voltage_divider_ratio = 5.0 / 3.3;  // 分压比
    
    // 计算实际电压 (考虑分压比)
    float actual_voltage = (adc_voltage_mv / 1000.0) * voltage_divider_ratio;
    
    // 限制在0~5V范围内
    if (actual_voltage < 0) actual_voltage = 0;
    if (actual_voltage > 5.0) actual_voltage = 5.0;
    
    return actual_voltage;
}

/**
 * 电压转电池百分比 (基于4.2V满电，3.0V低电)
 * @param voltage 电压值 (V)
 * @return 电量百分比 (0-100%)
 */
float voltage_to_battery_percentage(float voltage) {
    float percentage = ((voltage - BATTERY_LOW_CHARGE) / 
                       (BATTERY_FULL_CHARGE - BATTERY_LOW_CHARGE)) * 100.0;
    
    // 限制范围
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    return percentage;
}

// 简单的电池监控任务
void simple_battery_monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "简单电池监控任务启动 - 支持0~5V范围，带平滑滤波");
    
    // 配置ADC - 使用DB_12 (12dB衰减) 以支持更大的电压范围
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_12);  // 12dB衰减，支持0~3.3V
    
    // ADC校准
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
    ESP_LOGI(TAG, "ADC配置完成：12位分辨率，12dB衰减，支持0~3.3V输入");
    ESP_LOGI(TAG, "通过分压电路支持0~5V电压测量");
    ESP_LOGI(TAG, "平滑滤波：移动平均(窗口%d) + 指数滤波(α=%.1f)", FILTER_SIZE, ALPHA);
    
    while (1) {
        // 读取原始电压
        float raw_voltage = read_voltage_0_5v();
        
        // 应用平滑滤波
        float avg_filtered = moving_average_filter(raw_voltage);
        float exp_filtered = exponential_filter(raw_voltage);
        
        // 读取ADC原始值用于调试
        int adc_reading = adc1_get_raw(BATTERY_ADC_CHANNEL);
        uint32_t adc_voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
        
        // 计算电池百分比（基于指数滤波后的电压）
        float battery_percent = voltage_to_battery_percentage(exp_filtered);
        
        // 串口实时打印 - 显示原始值和滤波后的值
        printf("========== 电池监控 (0~5V) ==========\n");
        printf("ADC原始值: %d\n", adc_reading);
        printf("ADC电压: %" PRIu32 " mV\n", adc_voltage_mv);
        printf("原始电压: %.3f V\n", raw_voltage);
        printf("移动平均滤波: %.3f V\n", avg_filtered);
        printf("指数滤波: %.3f V\n", exp_filtered);
        printf("电池电量: %.0f%% (基于%.3fV)\n", battery_percent, exp_filtered);
        printf("=====================================\n\n");
        
        // ESP日志输出
        ESP_LOGI(TAG, "原始:%.3fV 滤波:%.3fV 电量:%.0f%% ADC:%d", 
                 raw_voltage, exp_filtered, battery_percent, adc_reading);
        
        // 电压范围状态指示
        if (exp_filtered >= 4.5) {
            ESP_LOGI(TAG, "🔵 高电压 (%.3fV)", exp_filtered);
        } else if (exp_filtered >= 4.0) {
            ESP_LOGI(TAG, "🟢 电池电量充足 (%.3fV)", exp_filtered);
        } else if (exp_filtered >= 3.5) {
            ESP_LOGI(TAG, "🟡 电池电量良好 (%.3fV)", exp_filtered);
        } else if (exp_filtered >= 3.0) {
            ESP_LOGW(TAG, "🟠 电池电量偏低 (%.3fV)", exp_filtered);
        } else if (exp_filtered >= 1.0) {
            ESP_LOGE(TAG, "🔴 电池电量严重不足 (%.3fV)", exp_filtered);
        } else {
            ESP_LOGE(TAG, "⚫ 电压异常低 (%.3fV)", exp_filtered);
        }
        
        // 显示滤波效果
        float noise_reduction = fabs(raw_voltage - exp_filtered);
        if (noise_reduction > 0.05) {  // 如果滤波效果明显
            ESP_LOGI(TAG, "📊 滤波降噪: %.3fV", noise_reduction);
        }
        
        // 每2秒更新一次
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32电池监控测试程序启动 (0~5V范围)");
    
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "NVS初始化完成");
    
    // 创建电池监控任务
    xTaskCreate(simple_battery_monitor_task, "battery_test", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "电池监控任务已启动，支持0~5V电压测量");
    
    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // 每10秒输出一次心跳
        ESP_LOGI(TAG, "系统运行正常...");
    }
} 