# hw_4.1 UART ESP32 STM32

ESP32-S3 UART1 GPIO17 TX / GPIO18 RX, button GPIO0, LED GPIO2.
STM32F411 UART Serial (PA9/PA10 or USB CDC depending on core), button PA0, LED PC13.

Byte 0xA5 toggles the remote LED. Cross-wire TX-RX-GND.

Build ESP32: `pio run -e esp32s3`
Build STM32: `pio run -e blackpill_f411ce`
