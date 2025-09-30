#include "adc_shared.h"
#include "esp_log.h"

static const char *TAG = "ADC_SHARED";

static adc_oneshot_unit_handle_t s_unit = NULL;
static adc_cali_handle_t s_cali = NULL;

esp_err_t adc_shared_init_unit(void) {
    if (s_unit) return ESP_OK;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_cfg, &s_unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %d", err);
    }
    return err;
}

esp_err_t adc_shared_init_cali(void) {
    if (s_cali) return ESP_OK;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) != ESP_OK) {
        s_cali = NULL;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali) != ESP_OK) {
        s_cali = NULL;
    }
#else
    s_cali = NULL;
#endif
    return ESP_OK;
}

adc_oneshot_unit_handle_t adc_shared_unit(void) {
    return s_unit;
}

adc_cali_handle_t adc_shared_cali(void) {
    return s_cali;
}


