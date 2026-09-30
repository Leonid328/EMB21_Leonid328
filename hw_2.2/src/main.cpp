#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t RELAY_PIN = 5;
constexpr uint8_t CONTACT_PIN = 4;
constexpr uint32_t PAUSE_MS = 1000;
constexpr uint32_t DEBOUNCE_US = 5000;
constexpr uint8_t MEASUREMENTS = 10;

volatile bool contact_event = false;
volatile uint32_t contact_time = 0;

void IRAM_ATTR contactISR() {
  static uint32_t last_time = 0;
  uint32_t now = micros();
  if (now - last_time < DEBOUNCE_US) {
    return;
  }
  last_time = now;
  contact_time = now;
  contact_event = true;
}

void setup() {
  Serial.begin(UART_SPEED);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), contactISR, CHANGE);
}

void loop() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;

  uint64_t total_on = 0;
  uint64_t total_off = 0;

  for (uint8_t i = 0; i < MEASUREMENTS; i++) {
    contact_event = false;
    uint32_t t0 = micros();
    digitalWrite(RELAY_PIN, HIGH);
    while (!contact_event) {
    }
    uint32_t on_time = contact_time - t0;
    total_on += on_time;
    Serial.print("ON ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(on_time);
    Serial.println(" us");
    delay(PAUSE_MS);

    contact_event = false;
    uint32_t t1 = micros();
    digitalWrite(RELAY_PIN, LOW);
    while (!contact_event) {
    }
    uint32_t off_time = contact_time - t1;
    total_off += off_time;
    Serial.print("OFF ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(off_time);
    Serial.println(" us");
    delay(PAUSE_MS);
  }

  float avg_on = (float)total_on / MEASUREMENTS;
  float avg_off = (float)total_off / MEASUREMENTS;
  Serial.println();
  Serial.print("Average ON: ");
  Serial.print(avg_on);
  Serial.println(" us");
  Serial.print("Average OFF: ");
  Serial.print(avg_off);
  Serial.println(" us");
}
