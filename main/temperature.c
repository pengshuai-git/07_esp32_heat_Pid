#include "temperature.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "TEMP";

static esp_adc_cal_characteristics_t adc_chars;

// 初始化ADC (GPIO0 for NTC, GPIO1 for battery)
void temperature_init(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);  // GPIO0
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);  // GPIO1
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    ESP_LOGI(TAG, "ADC initialized for temperature and battery monitoring");
}

// 读取温度 (Steinhart-Hart公式)
float get_temperature(void) {
    int adc = adc1_get_raw(ADC1_CHANNEL_0);
    float voltage = esp_adc_cal_raw_to_voltage(adc, &adc_chars) / 1000.0;  // V
    if (voltage == 0) return 0.0;  // 避免除零
    float resistance = NTC_REF_RES * voltage / (3.3 - voltage);

    float lnR = log(resistance / NTC_R25);
    float temp_k = 1.0 / (1.0/298.15 + lnR / NTC_B);  // Kelvin
    float temp = temp_k - 273.15;  // Celsius

    ESP_LOGD(TAG, "NTC ADC: %d, Voltage: %.2fV, Resistance: %.0fΩ, Temp: %.2f°C", adc, voltage, resistance, temp);
    return temp;
}

// 读取电池电平 (假设3.0-4.2V范围, 分压电路调整)
float get_battery_level(void) {
    int adc = adc1_get_raw(ADC1_CHANNEL_1);
    float voltage = esp_adc_cal_raw_to_voltage(adc, &adc_chars) / 1000.0 * 2.0;  // 假设2x分压
    float level = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
    if (level > 100.0) level = 100.0;
    if (level < 0.0) level = 0.0;
    ESP_LOGD(TAG, "Battery ADC: %d, Voltage: %.2fV, Level: %.0f%%", adc, voltage, level);
    return level;
}