#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t  ADC_PIN         = 4;
constexpr uint8_t  RELAY_PIN       = 5;
constexpr uint8_t  ADC_BITS        = 12;
constexpr uint16_t THRESHOLD_DARK  = 1200;
constexpr uint16_t THRESHOLD_LIGHT = 2500;
constexpr uint32_t SAMPLE_DELAY_MS = 200;

void setup() {
    Serial.begin(UART_SPEED);
    analogReadResolution(ADC_BITS);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
}

void loop() {
    const uint16_t adc_value = static_cast<uint16_t>(analogRead(ADC_PIN));
    Serial.print("ADC: ");
    Serial.println(adc_value);

    if (adc_value < THRESHOLD_DARK) {
        digitalWrite(RELAY_PIN, HIGH);
    } else if (adc_value > THRESHOLD_LIGHT) {
        digitalWrite(RELAY_PIN, LOW);
    }

    delay(SAMPLE_DELAY_MS);
}
