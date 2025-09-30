#include "buzzer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

#define BUZZER_GPIO 7  // 有源蜂鸣器引脚（按你的硬件连接）

void buzzer_init(void) {
    // 若 RGB 使用了 LEDC_CHANNEL_0 绑定 GPIO7，先停止，释放引脚控制权
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(BUZZER_GPIO, 0);
    ESP_LOGI(TAG, "Active buzzer on GPIO%d initialized (GPIO output)", BUZZER_GPIO);
}

// 有源蜂鸣器：拉高即响，这里响 1 秒
void buzzer_alarm(void) {
    gpio_set_level(BUZZER_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(BUZZER_GPIO, 0);
    ESP_LOGI(TAG, "Active buzzer beeped 1s");
}