#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pid_controller.h"
#include "temperature.h"
#include "display.h"
#include "input.h"
#include "rgb.h"
#include "buzzer.h"
#include "uart.h"
#include "driver/gpio.h"

static const char *TAG = "MAIN";

PID_t pid;
float current_temp = 0.0;
float battery = 0.0;

// PID任务 (每1s运行)
static void task_pid(void *pvParameters) {
    uint32_t window_start = 0;
    while (1) {
        current_temp = get_temperature();
        battery = get_battery_level();
        float output = pid_compute(&pid, current_temp);

        // 时间比例控制继电器
        if (xTaskGetTickCount() - window_start > pdMS_TO_TICKS(WINDOW_SIZE)) window_start = xTaskGetTickCount();
        uint32_t on_time = (uint32_t)(output / 100.0 * WINDOW_SIZE);
        gpio_set_level(GPIO_NUM_10, (xTaskGetTickCount() - window_start < pdMS_TO_TICKS(on_time)) ? 1 : 0);

        // 高温报警
        if (current_temp > ALARM_THRESHOLD) {
            buzzer_alarm();
            set_rgb(0, 0, 255);  // 蓝报警
        } else if (output > 0) {
            set_rgb(255, 0, 0);  // 红加热
        } else {
            set_rgb(0, 255, 0);  // 绿保温
        }

        // UART日志
        char buf[64];
        sprintf(buf, "Temp: %.2f, Output: %.2f\n", current_temp, output);
        uart_log(buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 显示任务 (每2s更新)
static void task_display(void *pvParameters) {
    while (1) {
        display_update(current_temp, pid.setpoint, battery);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// 输入任务 (每200ms扫描)
static void task_input(void *pvParameters) {
    while (1) {
        float delta = input_get_delta();
        pid.setpoint += delta;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// 主函数
void app_main(void) {
    // 初始化模块
    temperature_init();
    display_init();
    input_init();
    rgb_init();
    buzzer_init();
    uart_init();

    // 继电器初始化
    gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_10, 0);

    // PID初始化
    pid_init(&pid, 10.0, 0.1, 1.0, 60.0);

    // 创建任务
    xTaskCreate(task_pid, "pid_task", 2048, NULL, 5, NULL);
    xTaskCreate(task_display, "display_task", 2048, NULL, 4, NULL);
    xTaskCreate(task_input, "input_task", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "System started");
}