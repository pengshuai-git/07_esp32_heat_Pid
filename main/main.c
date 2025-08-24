#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "nvs_flash.h"

// 组件头文件
#include "temperature.h"
#include "pid_controller.h"
#include "display.h"
#include "rgb.h"
#include "input.h"
#include "buzzer.h"
#include "uart.h"

static const char *TAG = "MAIN";

// 全局变量
static float target_temp = 25.0;  // 目标温度
static PID_t pid;                 // PID控制器 - 修正类型名

// 电池电压监控任务
void battery_monitor_task(void *pvParameters) {
    // 配置ADC用于电池电压监控（假设使用GPIO4，ADC1_CHANNEL_4）
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_12);  // 使用新的宏名
    
    // ADC校准
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
    while (1) {
        // 读取ADC原始值
        int adc_reading = adc1_get_raw(ADC1_CHANNEL_4);
        
        // 转换为电压值（mV）
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
        
        // 计算电池电压（假设分压比为2:1）
        float battery_voltage = (voltage_mv * 2.0) / 1000.0;  // 转换为V
        
        // 计算电池百分比（假设3.0V-4.2V对应0-100%）
        float battery_percent = ((battery_voltage - 3.0) / 1.2) * 100.0;
        if (battery_percent < 0) battery_percent = 0;
        if (battery_percent > 100) battery_percent = 100;
        
        // 串口打印电池信息 - 修复格式化错误
        printf("电池电压: %.2fV, 电量: %.0f%%, ADC原始值: %d\n", 
               battery_voltage, battery_percent, adc_reading);
        
        // 日志记录
        ESP_LOGI(TAG, "电池电压: %.2fV, 电量: %.0f%%", battery_voltage, battery_percent);
        
        // RGB LED指示电池状态
        if (battery_percent > 50) {
            set_rgb(0, 255, 0);  // 绿色 - 电量充足
        } else if (battery_percent > 20) {
            set_rgb(255, 255, 0);  // 黄色 - 电量中等
        } else {
            set_rgb(255, 0, 0);   // 红色 - 电量低
        }
        
        // 低电量报警
        if (battery_percent < 20) {
            ESP_LOGW(TAG, "电池电量低！请充电");
            buzzer_alarm();  // 蜂鸣器报警
        }
        
        // 每2秒更新一次
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 温度控制任务
void temperature_control_task(void *pvParameters) {
    float current_temp;
    float output;
    
    while (1) {
        // 读取当前温度
        current_temp = temperature_read();
        
        // 更新PID设定值
        pid.setpoint = target_temp;
        
        // PID计算 - 修正函数调用，只传入当前温度
        output = pid_compute(&pid, current_temp);
        
        // 输出控制信号（这里只是示例，实际应控制加热器）
        ESP_LOGI(TAG, "目标温度: %.1f°C, 当前温度: %.1f°C, PID输出: %.2f", 
                 target_temp, current_temp, output);
        
        // 检查用户输入
        float delta = input_get_delta();
        if (delta != 0) {
            target_temp += delta;
            ESP_LOGI(TAG, "目标温度调整为: %.1f°C", target_temp);
        }
        
        // 更新显示
        float battery_voltage = 3.7;  // 临时值，实际应从电池监控任务获取
        display_update(current_temp, target_temp, (battery_voltage - 3.0) / 1.2 * 100);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32智能温控器启动");
    
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 初始化所有模块
    ESP_LOGI(TAG, "初始化硬件模块...");
    
    temperature_init();   // 初始化温度传感器
    rgb_init();          // 初始化RGB LED
    input_init();        // 初始化按键输入
    buzzer_init();       // 初始化蜂鸣器
    uart_init();         // 初始化UART
    display_init();      // 初始化显示屏
    
    // 初始化PID控制器 - 修正函数调用，添加setpoint参数
    pid_init(&pid, 2.0, 0.1, 0.5, target_temp);  // Kp=2.0, Ki=0.1, Kd=0.5, setpoint=25.0
    
    ESP_LOGI(TAG, "所有模块初始化完成");
    
    // 创建电池监控任务
    xTaskCreate(battery_monitor_task, "battery_monitor", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "电池监控任务已启动");
    
    // 创建温度控制任务
    xTaskCreate(temperature_control_task, "temp_control", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "温度控制任务已启动");
    
    // 系统状态指示
    set_rgb(0, 0, 255);  // 蓝色表示系统启动完成
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "系统运行中，开始监控电池电压和温度控制");
    
    // 主循环 - 系统状态监控
    while (1) {
        // 系统心跳指示
        ESP_LOGI(TAG, "系统正常运行中...");
        vTaskDelay(pdMS_TO_TICKS(10000));  // 每10秒打印一次系统状态
    }
}