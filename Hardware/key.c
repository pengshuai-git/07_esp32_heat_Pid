#include "key.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INPUT";

static int s_btn_inc = -1;  // 增加
static int s_btn_dec = -1;  // 减少
static int s_btn_ok  = -1;  // 确认/模式

// 初始化按键 (内部上拉)
void key_init(int btn_inc_gpio, int btn_dec_gpio, int btn_ok_gpio) {
    s_btn_inc = btn_inc_gpio;
    s_btn_dec = btn_dec_gpio;
    s_btn_ok  = btn_ok_gpio;
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<s_btn_inc) | (1ULL<<s_btn_dec) | (1ULL<<s_btn_ok),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Buttons initialized");
}

// 扫描按键，返回温度delta (步进1°C)
float key_get_delta(void) {
    float delta = 0.0;
    if (gpio_get_level(s_btn_inc) == 0) {
        delta += 1.0;
        ESP_LOGI(TAG, "Button1 pressed: +1°C");
        vTaskDelay(pdMS_TO_TICKS(200));  // 去抖
    }
    if (gpio_get_level(s_btn_dec) == 0) {
        delta -= 1.0;
        ESP_LOGI(TAG, "Button2 pressed: -1°C");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (gpio_get_level(s_btn_ok) == 0) {
        // 模式切换或确认, 这里假设无
        ESP_LOGI(TAG, "Button3 pressed: Mode/Confirm");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return delta;
}
