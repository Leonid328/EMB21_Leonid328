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

#define ADC_CHANNEL ADC_CHANNEL_8
#define ADC_UNIT ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define SERVO_PIN 14
#define SERVO_FREQ 50
#define SERVO_PERIOD_US 20000
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1U << 12) - 1U)
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_CHANNEL LEDC_CHANNEL_0

#define SUPERLOOP_DELAY_MS 100
#define SMA_WINDOW 8
#define DEG_DEADBAND 2

#define SERVO_MIN_DEG 10
#define SERVO_MAX_DEG 170
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
#define LIGHT_MIN_MV 50
#define LIGHT_MAX_MV 3000

static const char *TAG = "hw_miniproject_7";

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static int32_t s_sma_buf[SMA_WINDOW];
static uint8_t s_sma_idx;
static uint8_t s_sma_count;
static int32_t s_last_deg = -1;

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

static int32_t smaPush(int32_t sample) {
    s_sma_buf[s_sma_idx] = sample;
    s_sma_idx = (uint8_t)((s_sma_idx + 1U) % SMA_WINDOW);
    if (s_sma_count < SMA_WINDOW) {
        s_sma_count++;
    }
    int32_t sum = 0;
    for (uint8_t i = 0; i < s_sma_count; i++) {
        sum += s_sma_buf[i];
    }
    return sum / (int32_t)s_sma_count;
}

static void ldrInit(void) {
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &s_adc));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, ADC_CHANNEL, &channel_config));

    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &s_cali));
}

static void servoInit(void) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = SERVO_TIMER,
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = SERVO_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static void applyAngle(int32_t deg) {
    const int32_t pulse_us = map_i32(deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
    const uint32_t duty =
        (uint32_t)((int64_t)pulse_us * (int64_t)SERVO_MAX_DUTY / SERVO_PERIOD_US);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
    ESP_LOGI(TAG, "deg=%" PRId32 " us=%" PRId32 " duty=%" PRIu32, deg, pulse_us, duty);
}

static void setAngleFromMv(int32_t mv) {
    mv = clamp_i32(mv, LIGHT_MIN_MV, LIGHT_MAX_MV);
    const int32_t deg = map_i32(mv, LIGHT_MIN_MV, LIGHT_MAX_MV, SERVO_MIN_DEG, SERVO_MAX_DEG);
    if (s_last_deg >= 0) {
        int32_t delta = deg - s_last_deg;
        if (delta < 0) {
            delta = -delta;
        }
        if (delta < DEG_DEADBAND) {
            return;
        }
    }
    s_last_deg = deg;
    applyAngle(deg);
}

void app_main(void) {
    ldrInit();
    servoInit();

    while (1) {
        int raw = 0;
        int mv = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, ADC_CHANNEL, &raw));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, raw, &mv));
        const int32_t filtered = smaPush((int32_t)mv);
        ESP_LOGI(TAG, "raw=%d mv=%d sma=%" PRId32, raw, mv, filtered);
        setAngleFromMv(filtered);
        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}
