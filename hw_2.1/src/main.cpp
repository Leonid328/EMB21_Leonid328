#include <Arduino.h>
#include <stdint.h>

namespace {

enum class LedState : uint8_t {
    Off,
    On,
};

enum class LedMode : uint8_t {
    Blinking,
    AlwaysOn,
    AlwaysOff,
};

namespace Config {
constexpr uint8_t  kLedPin          = 5;
constexpr uint8_t  kButtonPin       = 4;
constexpr uint32_t kBlinkIntervalMs = 500;
constexpr uint32_t kDebounceMs      = 50;
}  // namespace Config

class Led {
public:
    explicit Led(uint8_t pin) : pin_(pin) {}

    void init() {
        pinMode(pin_, OUTPUT);
        set(LedState::Off);
    }

    void set(LedState state) {
        led_state_ = state;
        digitalWrite(pin_, state == LedState::On ? HIGH : LOW);
    }

    void toggle() {
        set(led_state_ == LedState::On ? LedState::Off : LedState::On);
    }

private:
    uint8_t   pin_;
    LedState  led_state_ = LedState::Off;
};

class Button {
public:
    Button(uint8_t pin, uint32_t debounce_ms) : pin_(pin), debounce_ms_(debounce_ms) {}

    void init() {
        pinMode(pin_, INPUT_PULLUP);
        last_stable_high_ = digitalRead(pin_) == HIGH;
        attachInterruptArg(digitalPinToInterrupt(pin_), isr, this, CHANGE);
    }

    bool wasPressed() {
        uint32_t last_irq_ms = 0;
        bool     irq_seen    = false;
        noInterrupts();
        irq_seen    = irq_seen_;
        last_irq_ms = last_irq_ms_;
        noInterrupts();

        if (!irq_seen) {
            return false;
        }

        const uint32_t now = millis();
        if ((now - last_irq_ms) < debounce_ms_) {
            return false;
        }

        noInterrupts();
        irq_seen_ = false;
        noInterrupts();

        const bool high    = digitalRead(pin_) == HIGH;
        const bool pressed = last_stable_high_ && !high;
        last_stable_high_  = high;
        return pressed;
    }

private:
    static void IRAM_ATTR isr(void* arg) {
        static_cast<Button*>(arg)->onInterrupt();
    }

    void IRAM_ATTR onInterrupt() {
        last_irq_ms_ = millis();
        irq_seen_    = true;
    }

    uint8_t           pin_;
    uint32_t          debounce_ms_;
    volatile uint32_t last_irq_ms_     = 0;
    volatile bool     irq_seen_        = false;
    bool              last_stable_high_ = true;
};

class App {
public:
    App() : led_(Config::kLedPin), button_(Config::kButtonPin, Config::kDebounceMs) {}

    void init() {
        Serial.begin(UART_SPEED);
        led_.init();
        button_.init();
    }

    void tick() {
        if (button_.wasPressed()) {
            cycleMode();
        }
        updateLed();
    }

private:
    void cycleMode() {
        switch (mode_) {
            case LedMode::Blinking:
                mode_ = LedMode::AlwaysOn;
                led_.set(LedState::On);
                break;
            case LedMode::AlwaysOn:
                mode_ = LedMode::AlwaysOff;
                led_.set(LedState::Off);
                break;
            case LedMode::AlwaysOff:
                mode_ = LedMode::Blinking;
                break;
        }
    }

    void updateLed() {
        if (mode_ != LedMode::Blinking) {
            return;
        }
        const uint32_t now = millis();
        if ((now - last_toggle_ms_) >= Config::kBlinkIntervalMs) {
            last_toggle_ms_ = now;
            led_.toggle();
        }
    }

    Led      led_;
    Button   button_;
    LedMode  mode_            = LedMode::Blinking;
    uint32_t last_toggle_ms_  = 0;
};

App& app() {
    static App instance;
    return instance;
}

}  // namespace

void setup() {
    app().init();
}

void loop() {
    app().tick();
}
