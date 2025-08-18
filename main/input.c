#include "input.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "INPUT";

#define BUTTON1_GPIO 4  // 增加
#define BUTTON2_GPIO 3  // 减少
#define BUTTON3_GPIO 2  // 确认/模式

// 初始化按键 (内部上拉)
void input_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<BUTTON1_GPIO) | (1ULL<<BUTTON2_GPIO) | (1ULL<<BUTTON3_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Buttons initialized");
}

// 扫描按键，返回温度delta (步进1°C)
float input_get_delta(void) {
    float delta = 0.0;
    if (gpio_get_level(BUTTON1_GPIO) == 0) {
        delta += 1.0;
        ESP_LOGI(TAG, "Button1 pressed: +1°C");
        vTaskDelay(pdMS_TO_TICKS(200));  // 去抖
    }
    if (gpio_get_level(BUTTON2_GPIO) == 0) {
        delta -= 1.0;
        ESP_LOGI(TAG, "Button2 pressed: -1°C");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (gpio_get_level(BUTTON3_GPIO) == 0) {
        // 模式切换或确认, 这里假设无
        ESP_LOGI(TAG, "Button3 pressed: Mode/Confirm");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return delta;
}