#include "buzzer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

// s_buzzer_gpio 用于记录蜂鸣器所用的GPIO编号，初始值为-1，表示尚未初始化（无效值）
static int s_buzzer_gpio = -1;

void buzzer_init(int gpio) {
    s_buzzer_gpio = gpio;

    // 若 RGB 占用了对应 LEDC 通道/引脚，可在外层工程处理，这里仅保证为GPIO输出
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << s_buzzer_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(s_buzzer_gpio, 0);
    ESP_LOGI(TAG, "Active buzzer on GPIO%d initialized (GPIO output)", s_buzzer_gpio);
}

// 有源蜂鸣器：拉高即响，这里响 1 秒
void buzzer_alarm(void) {
    if (s_buzzer_gpio < 0) {
        ESP_LOGE(TAG, "buzzer not initialized");
        return;
    }
    gpio_set_level(s_buzzer_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(s_buzzer_gpio, 0);
    ESP_LOGI(TAG, "Active buzzer beeped 1s");
}
