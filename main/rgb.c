#include "rgb.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "RGB";

#define RGB_R_GPIO 7
#define RGB_G_GPIO 6
#define RGB_B_GPIO 5

// 初始化LEDC PWM (频率1kHz, 8-bit分辨率)
void rgb_init(void) {
    // 配置定时器
    ledc_timer_config_t timer_conf = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_LOW_SPEED_MODE,  // ESP32C3只支持低速模式
        .timer_num = LEDC_TIMER_0,
    };
    ledc_timer_config(&timer_conf);

    // 配置红色通道
    ledc_channel_config_t channel_r = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = RGB_R_GPIO,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&channel_r);

    // 配置绿色通道
    ledc_channel_config_t channel_g = {
        .channel = LEDC_CHANNEL_1,
        .duty = 0,
        .gpio_num = RGB_G_GPIO,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&channel_g);

    // 配置蓝色通道
    ledc_channel_config_t channel_b = {
        .channel = LEDC_CHANNEL_2,
        .duty = 0,
        .gpio_num = RGB_B_GPIO,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&channel_b);

    ESP_LOGI(TAG, "RGB PWM initialized");
}

// 设置颜色 (duty = value * 255 / 255)
void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    
    ESP_LOGD(TAG, "RGB set: R=%d, G=%d, B=%d", r, g, b);
}