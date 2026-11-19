# hw_4.4: SPI BME280 + OLED clock (module 4 lesson 4)

ESP32-S3. Previous HW screen (SSD1306 + DS1307) plus BME280 over **SPI** (master).

## What it shows

Same clock as `hw_4.3`, plus three fields:

- `T: 23.4C`
- `RH: 45%`
- `P: 1013 hPa`

All of those lines are also sent to `ESP_LOGI` (`hw_4.4`).

DS18B20 is not used, so only BME280 temperature is on the display.

## Timing

BME280 + RTC + OLED refresh **≥ 1 Hz** with `esp_timer_get_time()` stamp. **No `delay()`**. Loop only yields with `vTaskDelay(1)`.

## Wiring (ESP32-S3 DevKitC)

| Signal | GPIO |
|--------|------|
| I2C SDA (OLED + DS1307) | 8 |
| I2C SCL | 9 |
| SPI MOSI (BME280 SDI) | 11 |
| SPI MISO (BME280 SDO) | 13 |
| SPI SCLK | 12 |
| SPI CS | 10 |
| BME280 VCC | 3V3 |
| GND | GND |

BME280 CS must be low for SPI mode (module CS pin, not I2C ADDR).

## Build

```bash
pio run -d hw_4.4
pio run -d hw_4.4 -t upload
pio device monitor -d hw_4.4
```
