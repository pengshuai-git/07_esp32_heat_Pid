#include "display.h"
// #include "ssd1306.h"  // 外部库暂时注释掉
#include "esp_log.h"
#include "driver/i2c.h"
#include <stdio.h>

static const char *TAG = "DISPLAY";

// 初始化I2C和OLED
void display_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // TODO: 需要添加SSD1306库或实现相应功能
    // ssd1306_init(I2C_NUM_0, 0x3C, SSD1306_128X64);  // 地址0x3C, 128x64
    // ssd1306_clear_screen();
    ESP_LOGI(TAG, "OLED initialized (placeholder)");
}

// 更新显示
void display_update(float temp, float setpoint, float battery) {
    // TODO: 需要添加SSD1306库或实现相应功能
    // ssd1306_clear_screen();
    char buf[32];
    sprintf(buf, "Temp: %.1f C", temp);
    // ssd1306_print(0, 0, buf);
    sprintf(buf, "Set: %.1f C", setpoint);
    // ssd1306_print(0, 16, buf);
    sprintf(buf, "Battery: %.0f%%", battery);
    // ssd1306_print(0, 32, buf);
    // ssd1306_update_screen();
    ESP_LOGD(TAG, "Display updated (placeholder) - Temp: %.1f, Set: %.1f, Battery: %.0f%%", 
             temp, setpoint, battery);
}