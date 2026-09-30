#include <Arduino.h>
#include <stdint.h>

enum class LedState : uint8_t {
  Off,
  On,
};

enum class LedMode : uint8_t {
  Blinking,
  AlwaysOn,
  AlwaysOff,
};

struct Config {
  static constexpr uint8_t LED_PIN = 5;
  static constexpr uint8_t BUTTON_PIN = 4;
  static constexpr uint32_t BLINK_INTERVAL_MS = 500;
  static constexpr uint32_t DEBOUNCE_MS = 50;
};

class Led {
public:
  Led(uint8_t pin) : pin(pin) {}

  void init() {
    pinMode(pin, OUTPUT);
    set(LedState::Off);
  }

  void set(LedState state) {
    led_state = state;
    digitalWrite(pin, state == LedState::On ? HIGH : LOW);
  }

  void toggle() {
    set(led_state == LedState::On ? LedState::Off : LedState::On);
  }

private:
  uint8_t pin;
  LedState led_state = LedState::Off;
};

Led led(Config::LED_PIN);
volatile bool button_pressed = false;
LedMode mode = LedMode::Blinking;

void IRAM_ATTR buttonISR() { button_pressed = true; }

void handleButton() {
  static uint32_t last_press = 0;
  if (!button_pressed) {
    return;
  }
  button_pressed = false;

  uint32_t now = millis();
  if (now - last_press < Config::DEBOUNCE_MS) {
    return;
  }
  last_press = now;

  switch (mode) {
  case LedMode::Blinking:
    mode = LedMode::AlwaysOn;
    led.set(LedState::On);
    break;
  case LedMode::AlwaysOn:
    mode = LedMode::AlwaysOff;
    led.set(LedState::Off);
    break;
  case LedMode::AlwaysOff:
    mode = LedMode::Blinking;
    break;
  }
}

void updateLed() {
  static uint32_t last_toggle = 0;
  if (mode != LedMode::Blinking) {
    return;
  }
  uint32_t now = millis();
  if (now - last_toggle >= Config::BLINK_INTERVAL_MS) {
    last_toggle = now;
    led.toggle();
  }
}

void setup() {
  Serial.begin(UART_SPEED);
  led.init();
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Config::BUTTON_PIN), buttonISR,
                  FALLING);
}

void loop() {
  handleButton();
  updateLed();
}
