#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

// 组件头文件
#include "../Hardware/temperature.h"
#include "../Hardware/display.h"
#include "../Hardware/rgb.h"
#include "../Hardware/key.h"
#include "../Hardware/buzzer.h"
#include "../Hardware/uart.h"
#include "../Hardware/battery_monitor.h"
#include "../Hardware/relay.h"
#include "web_server.h"

static const char *TAG = "MAIN";
// Wi-Fi SoftAP 配置（如需 STA，可后续扩展）
#define WIFI_AP_SSID     "ESP32-PID"
#define WIFI_AP_PASS     "12345678"  // 至少8位
#define WIFI_AP_MAX_CONN 4

static void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_cfg = { 0 };
    snprintf((char*)ap_cfg.ap.ssid, sizeof(ap_cfg.ap.ssid), "%s", WIFI_AP_SSID);
    ap_cfg.ap.ssid_len = strlen(WIFI_AP_SSID);
    snprintf((char*)ap_cfg.ap.password, sizeof(ap_cfg.ap.password), "%s", WIFI_AP_PASS);
    ap_cfg.ap.max_connection = WIFI_AP_MAX_CONN;
    ap_cfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(WIFI_AP_PASS) == 0) ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi SoftAP 启动完成，SSID: %s，IP: 192.168.4.1", WIFI_AP_SSID);
}

// === 硬件引脚集中配置（便于复用移植） ===
#define RELAY_GPIO 10

// UART1
#define UART_PORT_CFG UART_NUM_1
#define UART_TX_GPIO 20
#define UART_RX_GPIO 21
#define UART_BAUD    115200

// I2C0 for OLED
#define I2C_PORT_CFG I2C_NUM_0
#define I2C_SDA_IO   8
#define I2C_SCL_IO   9
#define I2C_CLK_HZ   400000
#define OLED_ADDR    0x3C

// RGB
#define RGB_R_GPIO   6
#define RGB_G_GPIO   5
#define RGB_B_GPIO   7

// Buttons
#define BUTTON1_GPIO 4
#define BUTTON2_GPIO 3
#define BUTTON3_GPIO 2

// Temperature/Battery ADC
#define TEMP_ADC_CH   ADC_CHANNEL_0
#define BATT_ADC_CH   ADC_CHANNEL_4
#define NTC_REF_RES_CFG   100000.0f    // 若上拉电阻为1kΩ，这里设为1000
#define VCC_SUPPLY    3.3f

static void hardware_init(void);

// 已移除旧的任务逻辑，仅保留硬件自检

void app_main(void) {
    ESP_LOGI(TAG, "ESP32硬件自检模式启动");

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 仅初始化自检所需模块
    ESP_LOGI(TAG, "初始化自检相关硬件...");
    hardware_init();

    // 启动 Web 服务（提供前端与 API）
    wifi_init_softap();
    web_server_start();

    ESP_LOGI(TAG, "初始化完成，进入待机/WEB服务模式");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static void hardware_init(void) {
    uart_init(UART_PORT_CFG, UART_TX_GPIO, UART_RX_GPIO, UART_BAUD);
    key_init(BUTTON1_GPIO, BUTTON2_GPIO, BUTTON3_GPIO);
    rgb_init(RGB_R_GPIO, RGB_G_GPIO, RGB_B_GPIO);
    buzzer_init(7);
    display_init(I2C_PORT_CFG, I2C_SDA_IO, I2C_SCL_IO, I2C_CLK_HZ, OLED_ADDR);
    temperature_init(TEMP_ADC_CH, NTC_REF_RES_CFG, VCC_SUPPLY);
    // 补充：继电器 PWM 与电池监控
    relay_init_pwm(RELAY_GPIO, 1000);
    battery_monitor_init(BATT_ADC_CH, 2.0f, 3.0f, 4.2f);
}