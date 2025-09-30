#include "temperature.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_shared.h"
#include "esp_log.h"
#include <math.h>
#include <inttypes.h>

static const char *TAG = "TEMP";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;
static adc_channel_t s_temp_channel = ADC_CHANNEL_0;
static float s_ref_res_ohm = 10000.0f;
static float s_vcc = 3.3f;
static int s_last_adc_raw = 0;
static int s_last_voltage_mv = 0;
// 采样与简易滤波配置
#define TEMP_NUM_SAMPLES 8

// 初始化温度传感器 (使用ADC读取热敏电阻或其他传感器)
void temperature_init(adc_channel_t temp_channel, float ref_res_ohm, float vcc_volt) {
    ESP_LOGI(TAG, "初始化温度传感器");
    s_temp_channel = temp_channel;
    s_ref_res_ohm = ref_res_ohm;
    s_vcc = vcc_volt;
    
    // 创建/复用全局 Oneshot ADC 单元
    adc_shared_init_unit();
    s_adc_handle = adc_shared_unit();

    // 配置通道（12-bit 默认位宽，12dB 衰减）
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(s_adc_handle, s_temp_channel, &chan_cfg);
    // 仅配置温度通道

    // 校准（全局）
    adc_shared_init_cali();
    s_cali_handle = adc_shared_cali();
    
    ESP_LOGI(TAG, "温度传感器初始化完成");
}

// 读取温度 - NTC 10K-3950 精确计算
float temperature_read(void) {
    // 读取ADC值（多次采样平均，降低噪声）
    int adc_reading = 0;
    int sum = 0;
    for (int i = 0; i < TEMP_NUM_SAMPLES; ++i) {
        int v = 0;
        adc_oneshot_read(s_adc_handle, s_temp_channel, &v);
        sum += v;
    }
    adc_reading = sum / TEMP_NUM_SAMPLES;
    
    // 转换为电压 (mV)
    int voltage_mv = 0;
    if (s_cali_handle) {
        adc_cali_raw_to_voltage(s_cali_handle, adc_reading, &voltage_mv);
    } else {
        // 12-bit, 12dB 近似 0~3300mV
        voltage_mv = (int)((adc_reading * 3300) / 4095);
    }
    s_last_adc_raw = adc_reading;
    s_last_voltage_mv = voltage_mv;
    
    // 计算热敏电阻阻值
    float voltage_v = voltage_mv / 1000.0f;
    float vcc = s_vcc;
    float r4 = s_ref_res_ohm;
    
    if (voltage_v >= vcc - 0.001f) {
        ESP_LOGW(TAG, "电压过高，可能传感器开路");
        return 999.0f;
    }
    if (voltage_v <= 0.001f) {
        ESP_LOGW(TAG, "电压过低，可能传感器短路");
        return -999.0f;
    }
    
    float rt = r4 * voltage_v / (vcc - voltage_v);
    
    // NTC 10K-3950 温度计算
    const float beta = 3950.0f;
    const float R0 = 10000.0f;
    const float T0 = 25.0f;
    const float T0_K = T0 + 273.15f;
    
    float T_K;
    if (rt > 0) {
        T_K = (beta * T0_K) / (T0_K * logf(rt / R0) + beta);
    } else {
        T_K = T0_K;
    }
    
    float temp_c = T_K - 273.15f;
    ESP_LOGI(TAG, "ADC=%d, 电压=%dmV, 阻值=%.0fΩ, 温度=%.1f°C", adc_reading, voltage_mv, rt, temp_c);
    return temp_c;
}



int temperature_get_last_raw(void) { return s_last_adc_raw; }
int temperature_get_last_mv(void) { return s_last_voltage_mv; }
