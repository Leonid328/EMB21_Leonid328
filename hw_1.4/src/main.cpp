#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t LED1_PIN = 15;
constexpr uint8_t LED2_PIN = 16;
constexpr uint8_t BUTTON_PIN = 21;
constexpr uint8_t BOOT_BUTTON_PIN = 0;
constexpr uint32_t FAST_DELAY_MS = 200;
constexpr uint32_t SLOW_DELAY_MS = 1000;
constexpr uint32_t DEBOUNCE_MS = 50;

enum class Mode : uint8_t {
  Fast,
  Slow,
  Count,
};

Mode mode = Mode::Fast;

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    mode = Mode::Fast;
    delay(DEBOUNCE_MS);
  }

  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    mode = Mode::Slow;
    delay(DEBOUNCE_MS);
  }

  if (mode == Mode::Fast) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, HIGH);
    delay(FAST_DELAY_MS);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    delay(FAST_DELAY_MS);
  }

  if (mode == Mode::Slow) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);
    delay(SLOW_DELAY_MS);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH);
    delay(SLOW_DELAY_MS);
  }
}
