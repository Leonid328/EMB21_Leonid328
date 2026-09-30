#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t  LED1_PIN       = 15;
constexpr uint8_t  LED2_PIN       = 16;
constexpr uint8_t  LED3_PIN       = 17;
constexpr uint32_t LED1_PERIOD_MS = 200;
constexpr uint32_t LED2_PERIOD_MS = 500;
constexpr uint32_t LED3_PERIOD_MS = 1000;

static uint32_t led1_last_ms = 0;
static uint32_t led2_last_ms = 0;
static uint32_t led3_last_ms = 0;

void setup() {
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    Serial.begin(UART_SPEED);
}

void loop() {
    const uint32_t now = millis();

    if (now - led1_last_ms >= LED1_PERIOD_MS) {
        led1_last_ms = now;
        digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
        Serial.print(now);
        Serial.print(" LED1 ");
        Serial.println(digitalRead(LED1_PIN));
    }

    if (now - led2_last_ms >= LED2_PERIOD_MS) {
        led2_last_ms = now;
        digitalWrite(LED2_PIN, !digitalRead(LED2_PIN));
        Serial.print(now);
        Serial.print(" LED2 ");
        Serial.println(digitalRead(LED2_PIN));
    }

    if (now - led3_last_ms >= LED3_PERIOD_MS) {
        led3_last_ms = now;
        digitalWrite(LED3_PIN, !digitalRead(LED3_PIN));
        Serial.print(now);
        Serial.print(" LED3 ");
        Serial.println(digitalRead(LED3_PIN));
    }
}
