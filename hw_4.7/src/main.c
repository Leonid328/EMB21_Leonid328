#include "stm32f411xe.h"

/* WeAct Black Pill F411CE: LED PC13 active-low, KEY PA0 active-low. */
#define LED_PIN 13U
#define KEY_PIN 0U

static volatile uint32_t g_ms;
static volatile uint32_t g_half_ms = 250U;

static const uint32_t kPeriods[] = {100U, 250U, 500U, 1000U};
static uint8_t s_period_i;

void SysTick_Handler(void) { g_ms++; }

static void clockGpio(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
}

static void ledInit(void) {
    GPIOC->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOC->MODER |= (1U << (LED_PIN * 2U));
    GPIOC->OTYPER &= ~(1U << LED_PIN);
    GPIOC->OSPEEDR &= ~(3U << (LED_PIN * 2U));
    GPIOC->PUPDR &= ~(3U << (LED_PIN * 2U));
    GPIOC->BSRR = (1U << LED_PIN);
}

static void keyInit(void) {
    GPIOA->MODER &= ~(3U << (KEY_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (KEY_PIN * 2U));
    GPIOA->PUPDR |= (1U << (KEY_PIN * 2U));
}

static void ledOn(void) { GPIOC->BSRR = (1U << (LED_PIN + 16U)); }

static void ledOff(void) { GPIOC->BSRR = (1U << LED_PIN); }

static int keyDown(void) { return (GPIOA->IDR & (1U << KEY_PIN)) == 0U; }

static void waitMs(uint32_t ms) {
    const uint32_t start = g_ms;
    while ((g_ms - start) < ms) {
    }
}

static void nextPeriod(void) {
    s_period_i = (uint8_t)((s_period_i + 1U) % 4U);
    g_half_ms = kPeriods[s_period_i];
}

int main(void) {
    SystemInit();
    clockGpio();
    ledInit();
    keyInit();

    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    uint32_t last_toggle = g_ms;
    uint8_t on = 0U;
    uint8_t was_down = 0U;

    for (;;) {
        const int down = keyDown();
        if (down && !was_down) {
            waitMs(20U);
            if (keyDown()) {
                nextPeriod();
                while (keyDown()) {
                }
                waitMs(20U);
            }
        }
        was_down = (uint8_t)down;

        if ((g_ms - last_toggle) >= g_half_ms) {
            last_toggle = g_ms;
            on = (uint8_t)!on;
            if (on) {
                ledOn();
            } else {
                ledOff();
            }
        }
    }
}
