#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/ledc.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_3
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define LED_GPIO 2
#define MOTOR_GPIO 5

#define PWM_FREQ_HZ 5000
#define PWM_RES LEDC_TIMER_12_BIT
#define PWM_MAX_DUTY ((1U << 12) - 1U)

#define LOOP_MS 20

static const char *TAG = "hw_3.3";

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

static int32_t clamp_i32(int32_t value, int32_t lo, int32_t hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static int32_t map_i32(int32_t value, int32_t in_lo, int32_t in_hi, int32_t out_lo,
                       int32_t out_hi) {
    value = clamp_i32(value, in_lo, in_hi);
    if (in_hi == in_lo) {
        return out_lo;
    }
    return out_lo + (value - in_lo) * (out_hi - out_lo) / (in_hi - in_lo);
}

static void adcInit(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, ADC_CHANNEL, &chan_cfg));

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali));
}

static void pwmInit(void) {
    ledc_timer_config_t led_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RES,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer));

    ledc_timer_config_t motor_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = PWM_RES,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&motor_timer));

    ledc_channel_config_t led_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&led_ch));

    ledc_channel_config_t motor_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = MOTOR_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&motor_ch));
}

static void setDuty(ledc_channel_t channel, uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
}

void app_main(void) {
    adcInit();
    pwmInit();

    while (1) {
        int raw = 0;
        int mv = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, ADC_CHANNEL, &raw));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, raw, &mv));

        uint32_t duty = (uint32_t)map_i32((int32_t)raw, 0, 4095, 0, (int32_t)PWM_MAX_DUTY);
        setDuty(LEDC_CHANNEL_0, duty);
        setDuty(LEDC_CHANNEL_1, duty);

        ESP_LOGI(TAG, "raw=%d mv=%d duty=%" PRIu32, raw, mv, duty);
        vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
    }
}
