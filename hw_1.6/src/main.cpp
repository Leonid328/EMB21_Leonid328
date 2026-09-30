#include <Arduino.h>
#include <math.h>
#include <stdint.h>

constexpr uint8_t LDR_PIN = 4;
constexpr uint8_t ADC_BITS = 12;
constexpr uint16_t ADC_MAX = 4095;
constexpr float UREF_V = 3.1f;
constexpr uint32_t SAMPLE_DELAY_MS = 100;

void setup() {
  Serial.begin(UART_SPEED);
  pinMode(LDR_PIN, INPUT);
  analogReadResolution(ADC_BITS);
}

void loop() {
  uint16_t raw = analogRead(LDR_PIN);
  float u_calc = ((float)raw / ADC_MAX) * UREF_V;
  uint32_t mv = analogReadMilliVolts(LDR_PIN);
  float u_meas = mv / 1000.0f;
  float err = 0;

  if (u_meas > 0) {
    err = fabs(u_calc - u_meas) / u_meas * 100.0f;
  }

  Serial.println("--------------------------------");
  Serial.print("RAW ADC: ");
  Serial.println(raw);
  Serial.print("Calculated voltage: ");
  Serial.print(u_calc, 3);
  Serial.println(" V");
  Serial.print("Measured voltage: ");
  Serial.print(u_meas, 3);
  Serial.println(" V");
  Serial.print("Measured millivolts: ");
  Serial.print(mv);
  Serial.println(" mV");
  Serial.print("Error: ");
  Serial.print(err, 2);
  Serial.println(" %");

  delay(SAMPLE_DELAY_MS);
}
