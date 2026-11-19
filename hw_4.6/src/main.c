#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_adc/adc_continuous.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define TAG "hw_4.6"
#define UART_PORT UART_NUM_0
#define UART_BAUD 115200
#define UART_TX_BUF 4096
#define UART_RX_BUF 256

#define ADC_UNIT_USED ADC_UNIT_1
#define ADC_CHANNEL_USED ADC_CHANNEL_3
#define ADC_ATTEN_USED ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED ADC_BITWIDTH_12
#define ADC_MAX_RAW 4095
#define VREF_MV 3300
#define SAMPLE_HZ 20000
#define FRAME_BYTES 256
#define READ_BUF 512

static TaskHandle_t s_task;
static adc_continuous_handle_t s_adc;

static uint32_t rawToMv(uint32_t raw) {
    if (raw > (uint32_t)ADC_MAX_RAW) {
        raw = (uint32_t)ADC_MAX_RAW;
    }
    return raw * (uint32_t)VREF_MV / (uint32_t)ADC_MAX_RAW;
}

static bool IRAM_ATTR onConvDone(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata,
                                 void *user_data) {
    (void)handle;
    (void)edata;
    (void)user_data;
    BaseType_t hp = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &hp);
    return hp == pdTRUE;
}

static void uartDmaInit(void) {
    uart_config_t cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUF, UART_TX_BUF, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
}

static void uartDmaSend(const char *s) {
    const size_t n = strlen(s);
    (void)uart_write_bytes(UART_PORT, s, n);
}

static void adcDmaInit(void) {
    adc_continuous_handle_cfg_t hcfg = {
        .max_store_buf_size = 1024,
        .conv_frame_size = FRAME_BYTES,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&hcfg, &s_adc));

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_USED,
        .channel = ADC_CHANNEL_USED,
        .unit = ADC_UNIT_USED,
        .bit_width = ADC_BITWIDTH_USED,
    };
    adc_continuous_config_t dcfg = {
        .sample_freq_hz = SAMPLE_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 1,
        .adc_pattern = &pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(s_adc, &dcfg));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = onConvDone,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(s_adc, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(s_adc));
}

void app_main(void) {
    s_task = xTaskGetCurrentTaskHandle();
    uartDmaInit();
    adcDmaInit();

    uartDmaSend("hw_4.6 ADC-DMA + UART-DMA  GPIO4/ADC1_CH3\r\n");
    uartDmaSend("turn the pot; lines are DMA-TX\r\n");

    uint8_t buf[READ_BUF];
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t got = 0;
        esp_err_t err = adc_continuous_read(s_adc, buf, sizeof(buf), &got, 0);
        if (err != ESP_OK || got < sizeof(adc_digi_output_data_t)) {
            continue;
        }

        uint32_t vmin = 0xFFFFFFFFu;
        uint32_t vmax = 0;
        uint64_t sum = 0;
        uint32_t n = 0;
        uint32_t last_raw = 0;

        for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= got; i += sizeof(adc_digi_output_data_t)) {
            adc_digi_output_data_t sample;
            memcpy(&sample, &buf[i], sizeof(sample));
            if (sample.type2.unit != ADC_UNIT_USED) {
                continue;
            }
            if (sample.type2.channel != ADC_CHANNEL_USED) {
                continue;
            }
            last_raw = sample.type2.data;
            if (last_raw > (uint32_t)ADC_MAX_RAW) {
                last_raw = (uint32_t)ADC_MAX_RAW;
            }
            if (last_raw < vmin) {
                vmin = last_raw;
            }
            if (last_raw > vmax) {
                vmax = last_raw;
            }
            sum += last_raw;
            n++;
        }
        if (n == 0) {
            continue;
        }

        const uint32_t v_last = rawToMv(last_raw);
        const uint32_t v_min = rawToMv(vmin);
        const uint32_t v_max = rawToMv(vmax);
        const uint32_t v_pp = v_max - v_min;
        const uint32_t v_avg = rawToMv((uint32_t)(sum / n));

        char line[160];
        snprintf(line, sizeof(line),
                 "raw=%4lu V=%4lu mV | Vmin=%4lu Vmax=%4lu Vpp=%4lu Vavg=%4lu n=%lu\r\n",
                 (unsigned long)last_raw, (unsigned long)v_last, (unsigned long)v_min,
                 (unsigned long)v_max, (unsigned long)v_pp, (unsigned long)v_avg, (unsigned long)n);
        uartDmaSend(line);
    }
}
