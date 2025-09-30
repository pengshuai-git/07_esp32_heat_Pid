#include "hardware_test.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "../Hardware/uart.h"
#include "../Hardware/key.h"
#include "../Hardware/rgb.h"
#include "../Hardware/buzzer.h"
#include "../Hardware/display.h"
#include "../Hardware/temperature.h"
#include "../Hardware/battery_monitor.h"

static const char *TAG = "MAIN";

// 与 main.c 相同的集中硬件配置（可后续抽到公共头）
#define RELAY_GPIO 10
#define BUTTON1_GPIO 4
#define BUTTON2_GPIO 3
#define BUTTON3_GPIO 2

void run_hardware_self_test(void) {
	ESP_LOGI(TAG, "===== 硬件自检开始 =====");

	// 1) 电池电压检测
	ESP_LOGI(TAG, "[1/7] 电池电压检测");
	battery_monitor_init(ADC_CHANNEL_4, 2.0f, 3.0f, 4.2f);
	float battery_voltage = battery_read_voltage();
	float battery_percent = battery_voltage_to_percentage(battery_voltage);
	ESP_LOGI(TAG, "电池电压: %.2fV (%.0f%%)", battery_voltage, battery_percent);

	// 2) 按键测试
	ESP_LOGI(TAG, "[2/7] 按键测试：请依次按下 按键1(加)+ 按键2(减)- 按键3(确认)");
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

	// 4) 小灯/LED 测试
	ESP_LOGI(TAG, "[4/7] 小灯测试：GPIO5(红) 与 GPIO6(绿) 分别间隔1s亮2s，结束后常亮");
	set_rgb(0, 0, 255);
	vTaskDelay(pdMS_TO_TICKS(2000));
	set_rgb(0, 0, 0);
	vTaskDelay(pdMS_TO_TICKS(1000));
	set_rgb(0, 255, 0);
	vTaskDelay(pdMS_TO_TICKS(2000));
	set_rgb(0, 0, 0);
	vTaskDelay(pdMS_TO_TICKS(1000));
	set_rgb(0, 255, 0);
	set_rgb(0, 255, 255);

	// 5) 蜂鸣器 2 秒
	ESP_LOGI(TAG, "[5/7] 蜂鸣器测试：响2秒");
	buzzer_alarm();
	buzzer_alarm();

	// 6) OLED 显示 HELLO
	ESP_LOGI(TAG, "[6/7] OLED 测试：显示 HELLO 并保持");
	display_show_text("HELLO", "", "");
	vTaskDelay(pdMS_TO_TICKS(1500));

	// 7) 继电器测试
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


