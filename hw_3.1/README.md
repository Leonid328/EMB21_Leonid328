# hw_3.1 — Проста ковзна середня (SMA)

ESP32-S3, LDR на ADC1 (oneshot), LED з гістерезисом.

## Схема

```
3V3 ---- LDR ----+---- GPIO4 (ADC1_CH3)
                 |
                10 kΩ
                 |
                GND

LED: GPIO2 ---- резистор 220 Ω ---- GND
(анод до GPIO2, якщо LED з катодом на GND)
```

Темніше → більший опір LDR → нижча напруга на ADC → LED вмикається.

## Логіка

1. `adc_oneshot_read` — одне вимірювання ADC1.
2. SMA на 16 відліків.
3. Два пороги (12-bit, 0…4095):
   - `sma < 1400` — темно, LED ON
   - `sma > 2200` — світло, LED OFF
   - між порогами стан не змінюється (немає мерехтіння)

## Збірка

```bash
pio run -d hw_3.1
pio run -d hw_3.1 -t upload
pio device monitor -d hw_3.1
```

## Результат (лог UART)

Див. `serial_log.txt`.
