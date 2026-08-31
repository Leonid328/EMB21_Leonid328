#include <Arduino.h>
#include <stdint.h>

void setup() {
    Serial.begin(UART_SPEED);
    Serial.println("Hello Embedded World!");
}

void loop() {
    // Your code here
}
