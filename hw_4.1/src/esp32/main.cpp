#include <Arduino.h>
#include <stdint.h>

static const uint8_t kBtnPin = 0;
static const uint8_t kLedPin = 2;
static const uint8_t kCmdToggle = 0xA5;

static uint8_t s_btn_prev = 1;
static uint8_t s_led;

void setup() {
    pinMode(kBtnPin, INPUT_PULLUP);
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);
    Serial.begin(UART_SPEED);
    Serial1.begin(UART_SPEED, SERIAL_8N1, 18, 17);
}

void loop() {
    const uint8_t btn = (uint8_t)digitalRead(kBtnPin);
    if (s_btn_prev == 1 && btn == 0) {
        Serial1.write(kCmdToggle);
        Serial.println("tx toggle");
        delay(40);
    }
    s_btn_prev = btn;

    if (Serial1.available() > 0) {
        const uint8_t cmd = (uint8_t)Serial1.read();
        if (cmd == kCmdToggle) {
            s_led = (uint8_t)(s_led ? 0 : 1);
            digitalWrite(kLedPin, s_led ? HIGH : LOW);
            Serial.println("rx toggle");
        }
    }
}
