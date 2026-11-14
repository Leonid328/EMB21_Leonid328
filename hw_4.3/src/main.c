#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
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

static const char *TAG = "hw_4.3";

static uint8_t s_fb[OLED_W * OLED_H / 8];

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

static void displayUpdate(const RtcTime *t) {
    char time_s[16];
    char date_s[24];
    snprintf(time_s, sizeof(time_s), "%02u:%02u:%02u", (unsigned)t->hour, (unsigned)t->min,
             (unsigned)t->sec);
    snprintf(date_s, sizeof(date_s), "%s %02u.%02u.%04u", kDow[t->dow], (unsigned)t->day,
             (unsigned)t->month, (unsigned)t->year);

    fbClear();
    fbText(8, 8, time_s, 2);
    fbText(4, 40, date_s, 1);
    const uint8_t bar = (uint8_t)((t->sec * (OLED_W - 4)) / 59);
    for (uint8_t x = 0; x < bar; x++) {
        fbPixel((uint8_t)(2 + x), 60, 1);
        fbPixel((uint8_t)(2 + x), 61, 1);
    }
    oledFlush();
    ESP_LOGI(TAG, "%s %s", time_s, date_s);
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

    while (1) {
        RtcTime t;
        rtcRead(&t);
        displayUpdate(&t);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
