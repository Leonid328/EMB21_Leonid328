# hw_4.7: SWD debug, register blink (module 4 lesson 7)

STM32F411CEU6 **Black Pill**. Blink on PC13. Key on **PA0** cycles the half-period: 100 / 250 / 500 / 1000 ms (equal on/off). **No HAL** in the blink/button logic — RCC + GPIO + SysTick registers only (`stm32f411xe.h`).

## Wiring

| Pin | Function |
|-----|----------|
| PC13 | onboard LED (active low) |
| PA0 | KEY to GND (internal pull-up) |
| 3V3 / GND | ST-Link |
| PA13 SWDIO, PA14 SWCLK | SWD |

## Build / flash / SWD

```bash
pio run -d hw_4.7
pio run -d hw_4.7 -t upload
pio debug -d hw_4.7
```

Break on `nextPeriod` or `g_half_ms` to show the period change in the debugger (lesson: SWD).
