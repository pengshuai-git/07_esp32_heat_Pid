#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// 组件头文件
#include "temperature.h"
#include "display.h"
#include "rgb.h"
#include "input.h"
#include "buzzer.h"
#include "uart.h"

static const char *TAG = "MAIN";

// === 测试相关定义（请按实际硬件修改） ===
#define RELAY_GPIO 10			// 继电器控制引脚（占位，按原理图修改）
#define BUTTON1_GPIO 4		// 与 input.c 保持一致（增加）
#define BUTTON2_GPIO 3		// 与 input.c 保持一致（减少）
#define BUTTON3_GPIO 2		// 与 input.c 保持一致（确认/模式）

static void hardware_self_test(void);

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
    uart_init();
    input_init();
    rgb_init();
    buzzer_init();
    display_init();
    temperature_init();

    ESP_LOGI(TAG, "初始化完成，开始自检");
    hardware_self_test();

    ESP_LOGI(TAG, "自检完成，系统待机");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// 硬件自检：电池→按键→温度→RGB→蜂鸣→OLED→继电器
static void hardware_self_test(void) {
	ESP_LOGI(TAG, "===== 硬件自检开始 =====");

	// 1) 电池电压检测
	ESP_LOGI(TAG, "[1/7] 电池电压检测");
	esp_adc_cal_characteristics_t adc_chars;
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_12);
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
	int adc_reading = adc1_get_raw(ADC1_CHANNEL_4);
	uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
	float battery_voltage = (voltage_mv * 2.0f) / 1000.0f; // 假设2:1分压
	float battery_percent = ((battery_voltage - 3.0f) / 1.2f) * 100.0f;
	if (battery_percent < 0) battery_percent = 0;
	if (battery_percent > 100) battery_percent = 100;
	ESP_LOGI(TAG, "电池电压: %.2fV (%.0f%%), ADC=%d", battery_voltage, battery_percent, adc_reading);

    // 2) 按键功能（等待用户分别按下1/2/3）
	ESP_LOGI(TAG, "[2/7] 按键测试：请依次按下 按键1(加)+ 按键2(减)- 按键3(确认)");
	// 确保GPIO已为输入上拉（input_init 已配置，但这里再设置一遍以防万一）
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = (1ULL<<BUTTON1_GPIO) | (1ULL<<BUTTON2_GPIO) | (1ULL<<BUTTON3_GPIO),
		.pull_down_en = 0,
		.pull_up_en = 1,
	};
	gpio_config(&io_conf);

	bool b1_ok = false, b2_ok = false, b3_ok = false;
	TickType_t start_ticks = xTaskGetTickCount();
	while (!(b1_ok && b2_ok && b3_ok)) {
		if (!b1_ok && gpio_get_level(BUTTON1_GPIO) == 0) { ESP_LOGI(TAG, "按键1 检测到"); b1_ok = true; vTaskDelay(pdMS_TO_TICKS(300)); }
		if (!b2_ok && gpio_get_level(BUTTON2_GPIO) == 0) { ESP_LOGI(TAG, "按键2 检测到"); b2_ok = true; vTaskDelay(pdMS_TO_TICKS(300)); }
		if (!b3_ok && gpio_get_level(BUTTON3_GPIO) == 0) { ESP_LOGI(TAG, "按键3 检测到"); b3_ok = true; vTaskDelay(pdMS_TO_TICKS(300)); }
		vTaskDelay(pdMS_TO_TICKS(20));
		// 可选：超时提示
		if (xTaskGetTickCount() - start_ticks > pdMS_TO_TICKS(30000)) {
			ESP_LOGW(TAG, "按键测试等待超时，请继续按键...");
			start_ticks = xTaskGetTickCount();
		}
	}

	// 3) 温度传感器
	ESP_LOGI(TAG, "[3/7] 温度传感器测试：读取3次");
	for (int i = 0; i < 3; i++) {
		float t = temperature_read();
		ESP_LOGI(TAG, "温度采集 #%d: %.2f°C", i+1, t);
		vTaskDelay(pdMS_TO_TICKS(500));
	}

    // 4) 小灯/LED 测试（GPIO5=红, GPIO6=绿）：分别间隔1s亮2s，结束后红绿常亮
    // 说明：当前 LEDC 绑定 GPIO5(通道2) 与 GPIO6(通道1)，用 set_rgb 控制占空
    ESP_LOGI(TAG, "[4/7] 小灯测试：GPIO5(红) 与 GPIO6(绿) 分别间隔1s亮2s，结束后常亮");
    // 红亮2s
    set_rgb(0, 0, 255); // 点亮 GPIO5（对应原BLUE通道，实际连到你的红灯）
    vTaskDelay(pdMS_TO_TICKS(2000));
    set_rgb(0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 绿亮2s
    set_rgb(0, 255, 0); // 点亮 GPIO6（绿）
    vTaskDelay(pdMS_TO_TICKS(2000));
    set_rgb(0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 红绿常亮
    // 这里用同时点亮 GPIO5 与 GPIO6（不点亮 GPIO7）
    // 先点亮绿
    set_rgb(0, 255, 0);
    // 再叠加点亮 GPIO5：由于 set_rgb 会覆盖三通道，这里再次调用组合值
    set_rgb(0, 255, 255); // GPIO6(绿) + GPIO5(原蓝通道)

	// 5) 蜂鸣器 2 秒
	ESP_LOGI(TAG, "[5/7] 蜂鸣器测试：响2秒");
	buzzer_alarm();
	buzzer_alarm();

    // 6) OLED 显示 HELLO（并保持显示）
    ESP_LOGI(TAG, "[6/7] OLED 测试：显示 HELLO 并保持");
    display_show_text("HELLO", "", "");
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 7) 继电器测试：拉高2秒 → 拉低2秒（手动观察指示灯）
	ESP_LOGI(TAG, "[7/7] 继电器测试：HIGH 2s -> LOW 2s");
	gpio_config_t rconf = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = (1ULL<<RELAY_GPIO),
		.pull_down_en = 0,
		.pull_up_en = 0,
	};
	gpio_config(&rconf);
	gpio_set_level(RELAY_GPIO, 1);
	vTaskDelay(pdMS_TO_TICKS(2000));
	gpio_set_level(RELAY_GPIO, 0);
	vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "===== 硬件自检完毕 =====");
}