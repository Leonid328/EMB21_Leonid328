#include <Arduino.h>
#include <stdint.h>

static const uint8_t kBtnPin = PA0;
static const uint8_t kLedPin = PC13;
static const uint8_t kCmdToggle = 0xA5;

static uint8_t s_btn_prev = 1;
static uint8_t s_led;

void setup() {
    pinMode(kBtnPin, INPUT_PULLUP);
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, HIGH);
    Serial.begin(UART_SPEED);
}

void loop() {
    const uint8_t btn = (uint8_t)digitalRead(kBtnPin);
    if (s_btn_prev == 1 && btn == 0) {
        Serial.write(kCmdToggle);
        delay(40);
    }
    s_btn_prev = btn;

    if (Serial.available() > 0) {
        const uint8_t cmd = (uint8_t)Serial.read();
        if (cmd == kCmdToggle) {
            s_led = (uint8_t)(s_led ? 0 : 1);
            digitalWrite(kLedPin, s_led ? LOW : HIGH);
        }
    }
}
