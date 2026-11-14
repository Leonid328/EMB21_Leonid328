#include "app.h"

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define LED_PIN GPIO_PIN_13
#define LED_PORT GPIOC
#define BLINK_MS 500

static uint32_t s_last_toggle_ms;
static uint8_t s_led_on;

void setup(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);

    s_led_on = 0;
    s_last_toggle_ms = HAL_GetTick();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void loop(void) {
    const uint32_t now = HAL_GetTick();
    if ((now - s_last_toggle_ms) < BLINK_MS) {
        return;
    }
    s_last_toggle_ms = now;
    s_led_on = (uint8_t)(s_led_on ? 0 : 1);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, s_led_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
