#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ENC_A_GPIO 4
#define ENC_B_GPIO 5
#define ENC_SW_GPIO 6
#define SERVO_GPIO 14
#define BUZZER_GPIO 7

#define SERVO_FREQ 50
#define SERVO_RES LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1U << 12) - 1U)
#define SERVO_PERIOD_US 20000
#define SERVO_MIN_DEG 10
#define SERVO_MAX_DEG 170
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
#define SERVO_CENTER_DEG 90

#define STEP_COARSE 10
#define STEP_FINE 5
#define LONG_PRESS_US 800000
#define BEEP_US 120000

static const char *TAG = "hw_miniproject_4";

static volatile int32_t s_enc_delta;
static int32_t s_angle_deg = SERVO_CENTER_DEG;
static uint8_t s_fine;
static uint8_t s_last_a;
static uint8_t s_sw_prev = 1;
static int64_t s_sw_down_us;
static uint8_t s_long_done;

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

static void IRAM_ATTR encIsr(void *arg) {
    (void)arg;
    const uint8_t a = (uint8_t)gpio_get_level(ENC_A_GPIO);
    const uint8_t b = (uint8_t)gpio_get_level(ENC_B_GPIO);
    if (a != s_last_a) {
        if (a == b) {
            s_enc_delta++;
        } else {
            s_enc_delta--;
        }
        s_last_a = a;
    }
}

static void gpioInit(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_A_GPIO) | (1ULL << ENC_B_GPIO) | (1ULL << ENC_SW_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    ESP_ERROR_CHECK(gpio_set_intr_type(ENC_A_GPIO, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENC_A_GPIO, encIsr, NULL));
    s_last_a = (uint8_t)gpio_get_level(ENC_A_GPIO);
}

static void pwmInit(void) {
    ledc_timer_config_t servo_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = SERVO_RES,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&servo_timer));

    ledc_channel_config_t servo_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&servo_ch));

    ledc_timer_config_t buzz_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&buzz_timer));

    ledc_channel_config_t buzz_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&buzz_ch));
}

static void setServoDeg(int32_t deg) {
    const int32_t pulse =
        map_i32(deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
    const uint32_t duty =
        (uint32_t)((int64_t)pulse * (int64_t)SERVO_MAX_DUTY / SERVO_PERIOD_US);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void beep(void) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 4096));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    const int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < BEEP_US) {
    }
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

void app_main(void) {
    gpioInit();
    pwmInit();
    setServoDeg(s_angle_deg);

    while (1) {
        int32_t delta = s_enc_delta;
        s_enc_delta = 0;
        if (delta != 0) {
            const int32_t step = s_fine ? STEP_FINE : STEP_COARSE;
            const int32_t next = s_angle_deg + delta * step;
            if (next < SERVO_MIN_DEG || next > SERVO_MAX_DEG) {
                beep();
                s_angle_deg = clamp_i32(next, SERVO_MIN_DEG, SERVO_MAX_DEG);
            } else {
                s_angle_deg = next;
            }
            setServoDeg(s_angle_deg);
            ESP_LOGI(TAG, "angle=%" PRId32 " fine=%u", s_angle_deg, (unsigned)s_fine);
        }

        const uint8_t sw = (uint8_t)gpio_get_level(ENC_SW_GPIO);
        const int64_t now = esp_timer_get_time();
        if (s_sw_prev == 1 && sw == 0) {
            s_sw_down_us = now;
            s_long_done = 0;
        }
        if (sw == 0 && s_long_done == 0 && (now - s_sw_down_us) > LONG_PRESS_US) {
            s_angle_deg = SERVO_CENTER_DEG;
            setServoDeg(s_angle_deg);
            s_long_done = 1;
            ESP_LOGI(TAG, "center");
        }
        if (s_sw_prev == 0 && sw == 1 && s_long_done == 0) {
            s_fine = (uint8_t)(s_fine ? 0 : 1);
            ESP_LOGI(TAG, "fine=%u", (unsigned)s_fine);
        }
        s_sw_prev = sw;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
