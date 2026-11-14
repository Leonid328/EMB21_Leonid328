# hw_2.9 STM32 VSCode

BlackPill F411CE. Blink PC13.

Business logic is in `include/app.h` and `src/app.c` as `setup()` and `loop()`.
`src/main.c` only inits HAL/clock and calls those functions.

`app.h` uses include guards so it can be included from several units without a cyclic import.
