#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t PIN_RED    = 15;
constexpr uint8_t PIN_YELLOW = 16;
constexpr uint8_t PIN_GREEN  = 17;
constexpr uint8_t PIN_BTN    = 4;

constexpr uint32_t GREEN_MS       = 4000;
constexpr uint32_t GREEN_BLINK_MS = 2000;
constexpr uint32_t YELLOW_MS      = 1500;
constexpr uint32_t RED_MS         = 4000;
constexpr uint32_t RED_YELLOW_MS  = 1500;
constexpr uint32_t BLINK_MS       = 400;
constexpr uint32_t DEBOUNCE_MS    = 40;

enum class Phase : uint8_t {
    Green,
    GreenBlink,
    Yellow,
    Red,
    RedYellow,
    NightYellow,
};

Phase    phase     = Phase::Green;
uint32_t t_phase   = 0;
uint32_t t_blink   = 0;
bool     green_on  = true;
bool     night     = false;
bool     btn_last  = true;
uint32_t t_btn     = 0;

void setLights(bool red, bool yellow, bool green) {
    digitalWrite(PIN_RED, red ? HIGH : LOW);
    digitalWrite(PIN_YELLOW, yellow ? HIGH : LOW);
    digitalWrite(PIN_GREEN, green ? HIGH : LOW);
}

void printPhase() {
    Serial.print("phase ");
    switch (phase) {
        case Phase::Green:
            Serial.println("green");
            break;
        case Phase::GreenBlink:
            Serial.println("green blink");
            break;
        case Phase::Yellow:
            Serial.println("yellow");
            break;
        case Phase::Red:
            Serial.println("red");
            break;
        case Phase::RedYellow:
            Serial.println("red+yellow");
            break;
        case Phase::NightYellow:
            Serial.println("yellow blink");
            break;
    }
}

void enter(Phase next) {
    phase    = next;
    t_phase  = millis();
    t_blink  = millis();
    green_on = true;
    printPhase();
}

void handleButton() {
    bool     now_up = digitalRead(PIN_BTN) == HIGH;
    uint32_t now    = millis();
    if (now_up == btn_last) {
        return;
    }
    if ((now - t_btn) < DEBOUNCE_MS) {
        return;
    }
    t_btn    = now;
    btn_last = now_up;
    if (now_up) {
        return;
    }
    night = !night;
    if (night) {
        enter(Phase::NightYellow);
    } else {
        enter(Phase::Green);
    }
}

void setup() {
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BTN, INPUT_PULLUP);
    Serial.begin(UART_SPEED);
    enter(Phase::Green);
}

void loop() {
    handleButton();
    uint32_t now = millis();

    if (phase == Phase::NightYellow) {
        if ((now - t_blink) >= BLINK_MS) {
            t_blink = now;
            digitalWrite(PIN_YELLOW, !digitalRead(PIN_YELLOW));
            digitalWrite(PIN_RED, LOW);
            digitalWrite(PIN_GREEN, LOW);
        }
        return;
    }

    uint32_t dt = now - t_phase;

    switch (phase) {
        case Phase::Green:
            setLights(false, false, true);
            if (dt >= GREEN_MS) {
                enter(Phase::GreenBlink);
            }
            break;
        case Phase::GreenBlink:
            if ((now - t_blink) >= BLINK_MS) {
                t_blink  = now;
                green_on = !green_on;
                setLights(false, false, green_on);
            }
            if (dt >= GREEN_BLINK_MS) {
                enter(Phase::Yellow);
            }
            break;
        case Phase::Yellow:
            setLights(false, true, false);
            if (dt >= YELLOW_MS) {
                enter(Phase::Red);
            }
            break;
        case Phase::Red:
            setLights(true, false, false);
            if (dt >= RED_MS) {
                enter(Phase::RedYellow);
            }
            break;
        case Phase::RedYellow:
            setLights(true, true, false);
            if (dt >= RED_YELLOW_MS) {
                enter(Phase::Green);
            }
            break;
        case Phase::NightYellow:
            break;
    }
}
