#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUZZER_GPIO 5
#define TICK_US 50000
#define PWM_RES LEDC_TIMER_13_BIT
#define PWM_DUTY_ON ((1U << 13) / 2U)

typedef struct {
    uint16_t freq_hz;
    uint16_t duration_ms;
} Note;

static const char *TAG = "hw_3.4";

static const Note kMelody[] = {
    {262, 400}, {262, 400}, {392, 400}, {392, 400}, {440, 400}, {440, 400}, {392, 800},
    {349, 400}, {349, 400}, {330, 400}, {330, 400}, {294, 400}, {294, 400}, {262, 800},
    {0, 600},
};

static uint8_t s_note_index;
static uint32_t s_note_elapsed_ms;
static int64_t s_last_tick_us;

static void buzzerInit(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RES,
        .freq_hz = 440,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
}

static void playFreq(uint16_t freq_hz) {
    if (freq_hz == 0) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        return;
    }
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_ON));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void audioTick(void) {
    const int64_t now = esp_timer_get_time();
    if ((now - s_last_tick_us) < TICK_US) {
        return;
    }
    s_last_tick_us = now;
    s_note_elapsed_ms += 50;

    const Note *note = &kMelody[s_note_index];
    if (s_note_elapsed_ms >= note->duration_ms) {
        s_note_elapsed_ms = 0;
        s_note_index = (uint8_t)((s_note_index + 1U) % (sizeof(kMelody) / sizeof(kMelody[0])));
        playFreq(kMelody[s_note_index].freq_hz);
        ESP_LOGI(TAG, "note=%u freq=%u", (unsigned)s_note_index, (unsigned)kMelody[s_note_index].freq_hz);
    }
}

void app_main(void) {
    buzzerInit();
    s_last_tick_us = esp_timer_get_time();
    playFreq(kMelody[0].freq_hz);

    while (1) {
        audioTick();
    }
}
