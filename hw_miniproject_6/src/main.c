#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ENC_A_GPIO 4
#define ENC_B_GPIO 5
#define ENC_SW_GPIO 6

typedef enum {
    ST_A = 0,
    ST_OP = 1,
    ST_B = 2,
    ST_RES = 3,
} CalcState;

static volatile int32_t s_enc_delta;
static uint8_t s_last_a;
static uint8_t s_sw_prev = 1;

static CalcState s_state;
static int32_t s_a;
static int32_t s_b;
static uint8_t s_op;
static int32_t s_res;

static const char kOps[] = {'+', '-', '*', '/'};

static void IRAM_ATTR encIsr(void *arg) {
    (void)arg;
    const uint8_t a = (uint8_t)gpio_get_level(ENC_A_GPIO);
    const uint8_t b = (uint8_t)gpio_get_level(ENC_B_GPIO);
    if (a != s_last_a) {
        s_enc_delta += (a == b) ? 1 : -1;
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

static void printExpr(void) {
    if (s_state == ST_A) {
        printf("\r%" PRId32 "          \r", s_a);
    } else if (s_state == ST_OP) {
        printf("\r%" PRId32 " %c          \r", s_a, kOps[s_op]);
    } else if (s_state == ST_B) {
        printf("\r%" PRId32 " %c %" PRId32 "          \r", s_a, kOps[s_op], s_b);
    } else {
        printf("\r%" PRId32 " %c %" PRId32 " = %" PRId32 "\n", s_a, kOps[s_op], s_b, s_res);
    }
    fflush(stdout);
}

static int32_t compute(void) {
    if (s_op == 0) {
        return s_a + s_b;
    }
    if (s_op == 1) {
        return s_a - s_b;
    }
    if (s_op == 2) {
        return s_a * s_b;
    }
    if (s_b == 0) {
        return 0;
    }
    return s_a / s_b;
}

void app_main(void) {
    gpioInit();
    printExpr();

    while (1) {
        int32_t delta = s_enc_delta;
        s_enc_delta = 0;
        if (delta != 0) {
            if (s_state == ST_A) {
                s_a += delta;
            } else if (s_state == ST_OP) {
                int32_t next = (int32_t)s_op + delta;
                while (next < 0) {
                    next += 4;
                }
                s_op = (uint8_t)(next % 4);
            } else if (s_state == ST_B) {
                s_b += delta;
            }
            printExpr();
        }

        const uint8_t sw = (uint8_t)gpio_get_level(ENC_SW_GPIO);
        if (s_sw_prev == 1 && sw == 0) {
            if (s_state == ST_A) {
                s_state = ST_OP;
            } else if (s_state == ST_OP) {
                s_state = ST_B;
            } else if (s_state == ST_B) {
                s_res = compute();
                s_state = ST_RES;
                printExpr();
                s_state = ST_A;
                s_a = 0;
                s_b = 0;
                s_op = 0;
            }
            printExpr();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        s_sw_prev = sw;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
