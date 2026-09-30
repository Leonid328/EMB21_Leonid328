#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t LED1_PIN = 15;
constexpr uint8_t LED2_PIN = 16;
constexpr uint8_t LED3_PIN = 17;
constexpr uint32_t LED1_MS = 200;
constexpr uint32_t LED2_MS = 500;
constexpr uint32_t LED3_MS = 1000;

uint32_t t1 = 0;
uint32_t t2 = 0;
uint32_t t3 = 0;

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  Serial.begin(UART_SPEED);
}

void loop() {
  uint32_t now = millis();

  if (now - t1 >= LED1_MS) {
    t1 = now;
    digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
    Serial.print(now);
    Serial.print(" LED1 ");
    Serial.println(digitalRead(LED1_PIN));
  }

  if (now - t2 >= LED2_MS) {
    t2 = now;
    digitalWrite(LED2_PIN, !digitalRead(LED2_PIN));
    Serial.print(now);
    Serial.print(" LED2 ");
    Serial.println(digitalRead(LED2_PIN));
  }

  if (now - t3 >= LED3_MS) {
    t3 = now;
    digitalWrite(LED3_PIN, !digitalRead(LED3_PIN));
    Serial.print(now);
    Serial.print(" LED3 ");
    Serial.println(digitalRead(LED3_PIN));
  }
}
