#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ENC_A_GPIO 4
#define ENC_B_GPIO 5
#define ENC_SW_GPIO 6
#define SERVO_GPIO 14
#define BUZZER_GPIO 7

#define PIN_LEN 4
#define MAX_ATTEMPTS 3
#define SECRET_PIN "2580"

#define SERVO_FREQ 50
#define SERVO_RES LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1U << 12) - 1U)
#define SERVO_PERIOD_US 20000
#define LOCK_DEG 10
#define UNLOCK_DEG 90

static const char *TAG = "hw_miniproject_5";

static volatile int32_t s_enc_delta;
static uint8_t s_last_a;
static uint8_t s_sw_prev = 1;

static char s_code[PIN_LEN + 1];
static uint8_t s_pos;
static uint8_t s_digit;
static uint8_t s_attempts;
static uint8_t s_blocked;
static int8_t s_dir;

static void IRAM_ATTR encIsr(void *arg) {
    (void)arg;
    const uint8_t a = (uint8_t)gpio_get_level(ENC_A_GPIO);
    const uint8_t b = (uint8_t)gpio_get_level(ENC_B_GPIO);
    if (a != s_last_a) {
        s_enc_delta += (a == b) ? 1 : -1;
        s_last_a = a;
    }
}

static int32_t map_i32(int32_t value, int32_t in_lo, int32_t in_hi, int32_t out_lo,
                       int32_t out_hi) {
    if (in_hi == in_lo) {
        return out_lo;
    }
    return out_lo + (value - in_lo) * (out_hi - out_lo) / (in_hi - in_lo);
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
        .freq_hz = 880,
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
    const int32_t pulse = map_i32(deg, 0, 180, 500, 2500);
    const uint32_t duty =
        (uint32_t)((int64_t)pulse * (int64_t)SERVO_MAX_DUTY / SERVO_PERIOD_US);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void toneMs(uint16_t freq, uint16_t ms) {
    if (freq == 0) {
        vTaskDelay(pdMS_TO_TICKS(ms));
        return;
    }
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 4096));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    vTaskDelay(pdMS_TO_TICKS(ms));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

static void melodyOk(void) {
    toneMs(523, 150);
    toneMs(659, 150);
    toneMs(784, 300);
}

static void melodyAlarm(void) {
    for (uint8_t i = 0; i < 6; i++) {
        toneMs(880, 120);
        toneMs(0, 80);
    }
}

static void printLive(void) {
    printf("\rcode: ");
    for (uint8_t i = 0; i < s_pos; i++) {
        putchar(s_code[i]);
    }
    if (s_pos < PIN_LEN) {
        putchar((char)('0' + s_digit));
        putchar('\b');
    }
    fflush(stdout);
}

static void resetEntry(void) {
    memset(s_code, 0, sizeof(s_code));
    s_pos = 0;
    s_digit = 0;
    s_dir = 0;
    printf("\n");
    printLive();
}

static void checkPin(void) {
    s_code[PIN_LEN] = '\0';
    s_attempts = (uint8_t)(s_attempts + 1U);
    if (strcmp(s_code, SECRET_PIN) == 0) {
        printf("\nUNLOCK\n");
        ESP_LOGI(TAG, "ok");
        melodyOk();
        setServoDeg(UNLOCK_DEG);
        return;
    }
    printf("\nFAIL %u/%u\n", (unsigned)s_attempts, (unsigned)MAX_ATTEMPTS);
    if (s_attempts >= MAX_ATTEMPTS) {
        s_blocked = 1;
        melodyAlarm();
        printf("BLOCKED\n");
        return;
    }
    resetEntry();
}

static void confirmDigit(void) {
    if (s_pos >= PIN_LEN) {
        return;
    }
    s_code[s_pos] = (char)('0' + s_digit);
    s_pos++;
    s_digit = 0;
    if (s_pos >= PIN_LEN) {
        checkPin();
    } else {
        printLive();
    }
}

void app_main(void) {
    gpioInit();
    pwmInit();
    setServoDeg(LOCK_DEG);
    resetEntry();

    while (1) {
        if (s_blocked) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int32_t delta = s_enc_delta;
        s_enc_delta = 0;
        if (delta != 0 && s_pos < PIN_LEN) {
            const int8_t dir = (delta > 0) ? 1 : -1;
            if (s_dir == 0) {
                s_dir = dir;
            }
            if (dir != s_dir) {
                confirmDigit();
                s_dir = dir;
            } else {
                uint8_t steps = (uint8_t)((delta < 0) ? -delta : delta);
                while (steps > 0 && s_pos < PIN_LEN) {
                    s_digit = (uint8_t)((s_digit + 1U) % 10U);
                    steps--;
                }
                printLive();
            }
        }

        const uint8_t sw = (uint8_t)gpio_get_level(ENC_SW_GPIO);
        if (s_sw_prev == 1 && sw == 0) {
            s_attempts = (uint8_t)(s_attempts + 1U);
            printf("\nRESET %u/%u\n", (unsigned)s_attempts, (unsigned)MAX_ATTEMPTS);
            if (s_attempts >= MAX_ATTEMPTS) {
                s_blocked = 1;
                melodyAlarm();
                printf("BLOCKED\n");
            } else {
                resetEntry();
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        s_sw_prev = sw;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
