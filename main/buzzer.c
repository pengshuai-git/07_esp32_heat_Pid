#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

#define BUZZER_GPIO 6  // 与RGB G共享, 报警时暂停RGB G

// 初始化 (LEDC channel 1, 共享GPIO6)
void buzzer_init(void) {
    // 复用LEDC channel1 for buzzer
    ESP_LOGI(TAG, "Buzzer initialized on GPIO6");
}

// 蜂鸣 (1000Hz, 1s)
void buzzer_alarm(void) {
    // 暂停RGB G
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    // 设置蜂鸣
    ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, 1000);  // 频率1000Hz
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 128);  // 50% duty
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    // 恢复RGB G频率
    ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, 1000);  // 恢复
    ESP_LOGI(TAG, "Alarm buzzed");
}