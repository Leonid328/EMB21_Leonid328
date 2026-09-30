#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t  POT_PIN       = 4;
constexpr uint8_t  MOTOR_PIN     = 5;
constexpr uint16_t ADC_MAX       = 4095;
constexpr uint32_t PWM_PERIOD_US = 2000;

void setup() {
    pinMode(MOTOR_PIN, OUTPUT);
    analogReadResolution(12);
}

void loop() {
    const uint16_t adc_value = static_cast<uint16_t>(analogRead(POT_PIN));
    const uint32_t high_time = static_cast<uint32_t>(adc_value) * PWM_PERIOD_US / ADC_MAX;
    const uint32_t low_time  = PWM_PERIOD_US - high_time;

    if (high_time > 0) {
        digitalWrite(MOTOR_PIN, HIGH);
        delayMicroseconds(high_time);
    }
    if (low_time > 0) {
        digitalWrite(MOTOR_PIN, LOW);
        delayMicroseconds(low_time);
    }
}
