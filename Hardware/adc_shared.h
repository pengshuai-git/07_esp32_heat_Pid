#ifndef ADC_SHARED_H
#define ADC_SHARED_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// 确保全局唯一的 ADC oneshot 单元已创建
esp_err_t adc_shared_init_unit(void);

// 确保全局校准句柄已创建（若硬件不支持则返回 ESP_OK 且句柄为 NULL）
esp_err_t adc_shared_init_cali(void);

// 获取全局 ADC oneshot 句柄（在 init_unit 之后调用）
adc_oneshot_unit_handle_t adc_shared_unit(void);

// 获取全局校准句柄（可能为 NULL）
adc_cali_handle_t adc_shared_cali(void);

#endif

