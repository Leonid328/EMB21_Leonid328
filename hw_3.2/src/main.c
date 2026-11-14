#include <stdint.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define POT_ADC_UNIT ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_3
#define ADC_BITS ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_MAX_RAW 4095
#define VREF_MV 3300
#define SAMPLE_PERIOD_MS 100U
#define SAMPLE_COUNT 16U

static const char *TAG = "hw_3.2";

static uint32_t voltageManualMv(int32_t raw) {
    if (raw < 0) {
        raw = 0;
    }
    return (uint32_t)raw * (uint32_t)VREF_MV / (uint32_t)ADC_MAX_RAW;
}

static adc_oneshot_unit_handle_t initAdc(void) {
    adc_oneshot_unit_handle_t   handle   = NULL;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = POT_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITS,
        .atten    = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, POT_ADC_CHANNEL, &chan_cfg));
    return handle;
}

static adc_cali_handle_t initCali(void) {
    adc_cali_handle_t handle = NULL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = POT_ADC_UNIT,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITS,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &handle));
#else
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = POT_ADC_UNIT,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITS,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &handle));
#endif
    return handle;
}

static void printTableHeader(void) {
    ESP_LOGI(TAG, "Vref=%d mV  atten=12 dB  bits=12  full-scale raw=%d", VREF_MV, ADC_MAX_RAW);
    ESP_LOGI(TAG, "U_manual = RAW * Vref / (2^bits - 1)");
    ESP_LOGI(TAG, "RAW   U_manual(mV)   U_cali(mV)   Error(%%)");
    ESP_LOGI(TAG, "------------------------------------------");
}

void app_main(void) {
    adc_oneshot_unit_handle_t adc1 = initAdc();
    adc_cali_handle_t         cali = initCali();

    printTableHeader();

    for (uint8_t n = 0; n < SAMPLE_COUNT; n++) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1, POT_ADC_CHANNEL, &raw));

        uint32_t u_manual = voltageManualMv((int32_t)raw);

        int u_cali = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali, raw, &u_cali));

        uint32_t err_x100 = 0;
        if (u_cali != 0) {
            int32_t diff = (int32_t)u_manual - (int32_t)u_cali;
            if (diff < 0) {
                diff = -diff;
            }
            err_x100 = (uint32_t)diff * 10000U / (uint32_t)u_cali;
        }

        ESP_LOGI(TAG, "%4d      %7lu          %4d        %lu.%02lu", raw, (unsigned long)u_manual,
                 u_cali, (unsigned long)(err_x100 / 100U), (unsigned long)(err_x100 % 100U));

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
