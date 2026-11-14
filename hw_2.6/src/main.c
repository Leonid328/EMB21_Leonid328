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

#define SERVO_GPIO 14
#define SERVO_FREQ_HZ 50
#define SERVO_RES LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1U << 12) - 1U)
#define SERVO_PERIOD_US 20000

#define SERVO_MIN_DEG 10
#define SERVO_MAX_DEG 170
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500

#define LOOP_MS 50

static const char *TAG = "hw_2.6";

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

static void servoInit(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = SERVO_RES,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
}

static void setPulseUs(int32_t pulse_us) {
    const uint32_t duty =
        (uint32_t)((int64_t)pulse_us * (int64_t)SERVO_MAX_DUTY / SERVO_PERIOD_US);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void app_main(void) {
    adcInit();
    servoInit();

    while (1) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, ADC_CHANNEL, &raw));

        const int32_t deg =
            map_i32((int32_t)raw, 0, 4095, SERVO_MIN_DEG, SERVO_MAX_DEG);
        const int32_t pulse_us =
            map_i32(deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
        setPulseUs(pulse_us);

        const int32_t from_left = deg - SERVO_MIN_DEG;
        ESP_LOGI(TAG, "raw=%d angle=%" PRId32 " deg from_left=%" PRId32, raw, deg, from_left);

        vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
    }
}
