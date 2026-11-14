#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LDR_ADC_UNIT ADC_UNIT_1
#define LDR_ADC_CHANNEL ADC_CHANNEL_3
#define LED_GPIO GPIO_NUM_2
#define SMA_WINDOW 16U
#define THRESH_DARK 1400
#define THRESH_LIGHT 2200
#define SAMPLE_PERIOD_MS 50U

static const char *TAG = "hw_3.1";

static int32_t  sma_buf[SMA_WINDOW];
static uint8_t  sma_index;
static uint8_t  sma_count;
static uint8_t  led_is_on;

static int32_t smaUpdate(int32_t sample) {
    sma_buf[sma_index] = sample;
    sma_index          = (uint8_t)((sma_index + 1U) % SMA_WINDOW);
    if (sma_count < SMA_WINDOW) {
        sma_count++;
    }

    int32_t sum = 0;
    for (uint8_t i = 0; i < sma_count; i++) {
        sum += sma_buf[i];
    }
    return sum / (int32_t)sma_count;
}

static void applyHysteresis(int32_t filtered) {
    if ((led_is_on == 0U) && (filtered < THRESH_DARK)) {
        led_is_on = 1U;
    } else if ((led_is_on != 0U) && (filtered > THRESH_LIGHT)) {
        led_is_on = 0U;
    }
    gpio_set_level(LED_GPIO, led_is_on);
}

static void initLed(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);
}

static adc_oneshot_unit_handle_t initAdc(void) {
    adc_oneshot_unit_handle_t     handle   = NULL;
    adc_oneshot_unit_init_cfg_t   unit_cfg = {
          .unit_id  = LDR_ADC_UNIT,
          .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, LDR_ADC_CHANNEL, &chan_cfg));
    return handle;
}

void app_main(void) {
    initLed();
    adc_oneshot_unit_handle_t adc1 = initAdc();

    ESP_LOGI(TAG, "SMA window=%u dark<%d light>%d ADC1_CH3 GPIO4 LED GPIO2", SMA_WINDOW, THRESH_DARK,
             THRESH_LIGHT);

    while (1) {
        /* adc_oneshot_read() takes int* (ESP-IDF API). */
        int     raw      = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1, LDR_ADC_CHANNEL, &raw));
        int32_t filtered = smaUpdate((int32_t)raw);
        applyHysteresis(filtered);
        ESP_LOGI(TAG, "raw=%d sma=%ld led=%u", raw, (long)filtered, (unsigned)led_is_on);
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
