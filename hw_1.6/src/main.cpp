#include <Arduino.h>
#include <stdint.h>
#include <cmath>

constexpr uint8_t  LDR_PIN         = 4;
constexpr uint8_t  ADC_BITS        = 12;
constexpr uint16_t ADC_MAX         = 4095;
constexpr float    UREF_V          = 3.1f;
constexpr uint32_t SAMPLE_DELAY_MS = 100;
constexpr float    MV_TO_V         = 1000.0f;

void setup() {
    Serial.begin(UART_SPEED);
    pinMode(LDR_PIN, INPUT);
    analogReadResolution(ADC_BITS);
}

void loop() {
    const uint16_t raw_adc = static_cast<uint16_t>(analogRead(LDR_PIN));
    const float    calculated_voltage =
        (static_cast<float>(raw_adc) / static_cast<float>(ADC_MAX)) * UREF_V;
    const uint32_t measured_millivolts = analogReadMilliVolts(LDR_PIN);
    const float    measured_voltage    = static_cast<float>(measured_millivolts) / MV_TO_V;
    float          error_percent       = 0.0f;

    if (measured_voltage > 0.0f) {
        error_percent =
            std::fabs(calculated_voltage - measured_voltage) / measured_voltage * 100.0f;
    }

    Serial.println("--------------------------------");
    Serial.print("RAW ADC: ");
    Serial.println(raw_adc);
    Serial.print("Calculated voltage: ");
    Serial.print(calculated_voltage, 3);
    Serial.println(" V");
    Serial.print("Measured voltage: ");
    Serial.print(measured_voltage, 3);
    Serial.println(" V");
    Serial.print("Measured millivolts: ");
    Serial.print(measured_millivolts);
    Serial.println(" mV");
    Serial.print("Error: ");
    Serial.print(error_percent, 2);
    Serial.println(" %");

    delay(SAMPLE_DELAY_MS);
}
