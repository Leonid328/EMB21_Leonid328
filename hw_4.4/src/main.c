#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_HZ 400000

#define SSD1306_ADDR 0x3C
#define DS1307_ADDR 0x68

#define OLED_W 128
#define OLED_H 64

#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCLK 12
#define SPI_CS 10

#define BME_REG_ID 0xD0
#define BME_REG_RESET 0xE0
#define BME_REG_CTRL_HUM 0xF2
#define BME_REG_STATUS 0xF3
#define BME_REG_CTRL_MEAS 0xF4
#define BME_REG_CONFIG 0xF5
#define BME_REG_PRESS_MSB 0xF7
#define BME_CHIP_ID 0x60

#define SAMPLE_US 1000000ULL

static const char *TAG = "hw_4.4";

static uint8_t s_fb[OLED_W * OLED_H / 8];
static spi_device_handle_t s_bme;
static int32_t s_t_fine;

static const uint8_t kFont5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x41, 0x22, 0x14, 0x08, 0x00}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x32}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

static const char *kDow[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} BmeCal;

typedef struct {
    int ok;
    float t_c;
    float rh;
    float p_hpa;
} BmeData;

static BmeCal s_cal;
static int s_bme_ok;

static uint8_t bcdToBin(uint8_t v) { return (uint8_t)(((v >> 4) * 10) + (v & 0x0F)); }

static uint8_t binToBcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static esp_err_t i2cWrite(uint8_t addr, const uint8_t *data, size_t len) {
    return i2c_master_write_to_device(I2C_PORT, addr, data, len, pdMS_TO_TICKS(50));
}

static esp_err_t i2cWriteRead(uint8_t addr, const uint8_t *w, size_t wlen, uint8_t *r, size_t rlen) {
    return i2c_master_write_read_device(I2C_PORT, addr, w, wlen, r, rlen, pdMS_TO_TICKS(50));
}

static void oledCmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    ESP_ERROR_CHECK(i2cWrite(SSD1306_ADDR, buf, 2));
}

static void oledCmd2(uint8_t a, uint8_t b) {
    oledCmd(a);
    oledCmd(b);
}

static void oledInit(void) {
    oledCmd(0xAE);
    oledCmd2(0xD5, 0x80);
    oledCmd2(0xA8, 0x3F);
    oledCmd2(0xD3, 0x00);
    oledCmd(0x40);
    oledCmd2(0x8D, 0x14);
    oledCmd2(0x20, 0x00);
    oledCmd(0xA1);
    oledCmd(0xC8);
    oledCmd2(0xDA, 0x12);
    oledCmd2(0x81, 0xCF);
    oledCmd2(0xD9, 0xF1);
    oledCmd2(0xDB, 0x40);
    oledCmd(0xA4);
    oledCmd(0xA6);
    oledCmd(0xAF);
}

static void fbClear(void) { memset(s_fb, 0, sizeof(s_fb)); }

static void fbPixel(uint8_t x, uint8_t y, uint8_t on) {
    if (x >= OLED_W || y >= OLED_H) {
        return;
    }
    const uint16_t i = (uint16_t)x + (uint16_t)(y / 8) * OLED_W;
    const uint8_t bit = (uint8_t)(1U << (y % 8));
    if (on) {
        s_fb[i] = (uint8_t)(s_fb[i] | bit);
    } else {
        s_fb[i] = (uint8_t)(s_fb[i] & (uint8_t)~bit);
    }
}

static uint8_t glyphIndex(char c) {
    if (c >= ' ' && c <= 'Z') {
        return (uint8_t)(c - ' ');
    }
    if (c >= 'a' && c <= 'z') {
        return (uint8_t)(c - 'a' + ('A' - ' '));
    }
    return 0;
}

static void fbChar(uint8_t x, uint8_t y, char c, uint8_t scale) {
    const uint8_t *g = kFont5x7[glyphIndex(c)];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = g[col];
        for (uint8_t row = 0; row < 7; row++) {
            const uint8_t on = (uint8_t)((bits >> row) & 1U);
            for (uint8_t sx = 0; sx < scale; sx++) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    fbPixel((uint8_t)(x + col * scale + sx), (uint8_t)(y + row * scale + sy), on);
                }
            }
        }
    }
}

static void fbText(uint8_t x, uint8_t y, const char *s, uint8_t scale) {
    while (*s) {
        fbChar(x, y, *s, scale);
        x = (uint8_t)(x + (uint8_t)(6 * scale));
        s++;
    }
}

static void oledFlush(void) {
    oledCmd(0x21);
    oledCmd(0);
    oledCmd(127);
    oledCmd(0x22);
    oledCmd(0);
    oledCmd(7);
    uint8_t packet[17];
    packet[0] = 0x40;
    for (uint16_t i = 0; i < sizeof(s_fb); i += 16) {
        memcpy(&packet[1], &s_fb[i], 16);
        ESP_ERROR_CHECK(i2cWrite(SSD1306_ADDR, packet, 17));
    }
}

static void rtcInit(void) {
    uint8_t reg = 0x00;
    uint8_t sec = 0;
    ESP_ERROR_CHECK(i2cWriteRead(DS1307_ADDR, &reg, 1, &sec, 1));
    if ((sec & 0x80U) != 0) {
        uint8_t start[8] = {0x00, binToBcd(0), binToBcd(0), binToBcd(12), 0x06, binToBcd(14),
                            binToBcd(11), binToBcd(26)};
        ESP_ERROR_CHECK(i2cWrite(DS1307_ADDR, start, sizeof(start)));
        ESP_LOGW(TAG, "rtc started");
    }
}

typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} RtcTime;

static void rtcRead(RtcTime *t) {
    uint8_t reg = 0x00;
    uint8_t buf[7];
    ESP_ERROR_CHECK(i2cWriteRead(DS1307_ADDR, &reg, 1, buf, 7));
    t->sec = bcdToBin((uint8_t)(buf[0] & 0x7FU));
    t->min = bcdToBin(buf[1]);
    t->hour = bcdToBin((uint8_t)(buf[2] & 0x3FU));
    {
        uint8_t d = (uint8_t)(buf[3] & 0x07U);
        if (d == 0) {
            d = 1;
        }
        t->dow = (uint8_t)(d - 1U);
    }
    t->day = bcdToBin(buf[4]);
    t->month = bcdToBin((uint8_t)(buf[5] & 0x1FU));
    t->year = (uint16_t)(2000 + bcdToBin(buf[6]));
}

static esp_err_t bmeWrite(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = {(uint8_t)(reg & 0x7FU), val};
    spi_transaction_t t = {0};
    t.length = 16;
    t.tx_buffer = tx;
    return spi_device_transmit(s_bme, &t);
}

static esp_err_t bmeRead(uint8_t reg, uint8_t *dst, size_t n) {
    uint8_t tx[16];
    uint8_t rx[16];
    if (n + 1U > sizeof(tx)) {
        return ESP_ERR_INVALID_SIZE;
    }
    memset(tx, 0, sizeof(tx));
    tx[0] = (uint8_t)(reg | 0x80U);
    spi_transaction_t t = {0};
    t.length = (n + 1U) * 8U;
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    esp_err_t e = spi_device_transmit(s_bme, &t);
    if (e != ESP_OK) {
        return e;
    }
    memcpy(dst, &rx[1], n);
    return ESP_OK;
}

static int16_t leS16(const uint8_t *p) { return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }

static uint16_t leU16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }

static int bmeLoadCal(void) {
    uint8_t a[26];
    uint8_t h1;
    uint8_t h[7];
    if (bmeRead(0x88, a, 26) != ESP_OK) {
        return 0;
    }
    if (bmeRead(0xA1, &h1, 1) != ESP_OK) {
        return 0;
    }
    if (bmeRead(0xE1, h, 7) != ESP_OK) {
        return 0;
    }
    s_cal.dig_T1 = leU16(&a[0]);
    s_cal.dig_T2 = leS16(&a[2]);
    s_cal.dig_T3 = leS16(&a[4]);
    s_cal.dig_P1 = leU16(&a[6]);
    s_cal.dig_P2 = leS16(&a[8]);
    s_cal.dig_P3 = leS16(&a[10]);
    s_cal.dig_P4 = leS16(&a[12]);
    s_cal.dig_P5 = leS16(&a[14]);
    s_cal.dig_P6 = leS16(&a[16]);
    s_cal.dig_P7 = leS16(&a[18]);
    s_cal.dig_P8 = leS16(&a[20]);
    s_cal.dig_P9 = leS16(&a[22]);
    s_cal.dig_H1 = h1;
    s_cal.dig_H2 = leS16(&h[0]);
    s_cal.dig_H3 = h[2];
    s_cal.dig_H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0F));
    s_cal.dig_H5 = (int16_t)((h[5] << 4) | (h[4] >> 4));
    s_cal.dig_H6 = (int8_t)h[6];
    return 1;
}

static float compensateT(int32_t adc_T) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)s_cal.dig_T1 << 1))) * ((int32_t)s_cal.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)s_cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)s_cal.dig_T1))) >> 12) *
                    ((int32_t)s_cal.dig_T3)) >>
                   14;
    s_t_fine = var1 + var2;
    return (s_t_fine * 5 + 128) / 256 / 100.0f;
}

static float compensateP(int32_t adc_P) {
    int64_t var1 = ((int64_t)s_t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)s_cal.dig_P6;
    var2 = var2 + ((var1 * (int64_t)s_cal.dig_P5) << 17);
    var2 = var2 + (((int64_t)s_cal.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)s_cal.dig_P3) >> 8) + ((var1 * (int64_t)s_cal.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)s_cal.dig_P1) >> 33;
    if (var1 == 0) {
        return 0;
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)s_cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_cal.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_cal.dig_P7) << 4);
    return p / 25600.0f;
}

static float compensateH(int32_t adc_H) {
    int32_t v = s_t_fine - ((int32_t)76800);
    v = (((((adc_H << 14) - (((int32_t)s_cal.dig_H4) << 20) - (((int32_t)s_cal.dig_H5) * v)) + ((int32_t)16384)) >>
          15) *
         (((((((v * ((int32_t)s_cal.dig_H6)) >> 10) * (((v * ((int32_t)s_cal.dig_H3)) >> 11) + ((int32_t)32768))) >>
             10) +
            ((int32_t)2097152)) *
               ((int32_t)s_cal.dig_H2) +
           8192) >>
          14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)s_cal.dig_H1)) >> 4);
    if (v < 0) {
        v = 0;
    }
    if (v > 419430400) {
        v = 419430400;
    }
    return (v >> 12) / 1024.0f;
}

static int bmeInit(void) {
    spi_bus_config_t bus = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .sclk_io_num = SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 500000,
        .mode = 0,
        .spics_io_num = SPI_CS,
        .queue_size = 1,
        .flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &s_bme));

    uint8_t id = 0;
    if (bmeRead(BME_REG_ID, &id, 1) != ESP_OK || id != BME_CHIP_ID) {
        ESP_LOGE(TAG, "bme280 id=0x%02X (need 0x60)", id);
        return 0;
    }
    ESP_ERROR_CHECK(bmeWrite(BME_REG_RESET, 0xB6));
    {
        int64_t t0 = esp_timer_get_time();
        while (esp_timer_get_time() - t0 < 10000) {
            vTaskDelay(1);
        }
    }
    if (!bmeLoadCal()) {
        ESP_LOGE(TAG, "bme280 cal fail");
        return 0;
    }
    ESP_ERROR_CHECK(bmeWrite(BME_REG_CTRL_HUM, 0x01));
    ESP_ERROR_CHECK(bmeWrite(BME_REG_CONFIG, 0x00));
    ESP_ERROR_CHECK(bmeWrite(BME_REG_CTRL_MEAS, 0x27));
    ESP_LOGI(TAG, "bme280 spi ok id=0x%02X CS=%d", id, SPI_CS);
    return 1;
}

static BmeData bmeSample(void) {
    BmeData d = {0};
    if (!s_bme_ok) {
        return d;
    }
    uint8_t raw[8];
    if (bmeRead(BME_REG_PRESS_MSB, raw, 8) != ESP_OK) {
        return d;
    }
    int32_t adc_P = (int32_t)(((uint32_t)raw[0] << 12) | ((uint32_t)raw[1] << 4) | (raw[2] >> 4));
    int32_t adc_T = (int32_t)(((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) | (raw[5] >> 4));
    int32_t adc_H = (int32_t)(((uint32_t)raw[6] << 8) | raw[7]);
    d.t_c = compensateT(adc_T);
    d.p_hpa = compensateP(adc_P);
    d.rh = compensateH(adc_H);
    d.ok = 1;
    return d;
}

static void displayUpdate(const RtcTime *t, const BmeData *b) {
    char time_s[16];
    char date_s[24];
    char t_s[24];
    char rh_s[24];
    char p_s[24];
    snprintf(time_s, sizeof(time_s), "%02u:%02u:%02u", (unsigned)t->hour, (unsigned)t->min,
             (unsigned)t->sec);
    snprintf(date_s, sizeof(date_s), "%s %02u.%02u.%04u", kDow[t->dow], (unsigned)t->day,
             (unsigned)t->month, (unsigned)t->year);
    if (b->ok) {
        snprintf(t_s, sizeof(t_s), "T: %.1fC", (double)b->t_c);
        snprintf(rh_s, sizeof(rh_s), "RH: %.0f%%", (double)b->rh);
        snprintf(p_s, sizeof(p_s), "P: %.0f hPa", (double)b->p_hpa);
    } else {
        snprintf(t_s, sizeof(t_s), "T: --.-C");
        snprintf(rh_s, sizeof(rh_s), "RH: --%%");
        snprintf(p_s, sizeof(p_s), "P: ---- hPa");
    }

    fbClear();
    fbText(2, 0, time_s, 1);
    fbText(2, 10, date_s, 1);
    fbText(2, 22, t_s, 1);
    fbText(2, 32, rh_s, 1);
    fbText(2, 42, p_s, 1);
    const uint8_t bar = (uint8_t)((t->sec * (OLED_W - 4)) / 59);
    for (uint8_t x = 0; x < bar; x++) {
        fbPixel((uint8_t)(2 + x), 60, 1);
        fbPixel((uint8_t)(2 + x), 61, 1);
    }
    oledFlush();

    ESP_LOGI(TAG, "screen %s | %s | %s | %s | %s", time_s, date_s, t_s, rh_s, p_s);
}

static void busInit(void) {
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master =
            {
                .clk_speed = I2C_HZ,
            },
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0));
}

void app_main(void) {
    busInit();
    oledInit();
    rtcInit();
    s_bme_ok = bmeInit();

    int64_t last = 0;
    while (1) {
        const int64_t now = esp_timer_get_time();
        if (last == 0 || (now - last) >= (int64_t)SAMPLE_US) {
            last = now;
            RtcTime t;
            rtcRead(&t);
            BmeData b = bmeSample();
            displayUpdate(&t, &b);
        }
        vTaskDelay(1);
    }
}
