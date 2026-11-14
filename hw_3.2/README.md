# hw_3.2 — Калібрування даних з ADC

Потенціометр ~10 кОм як подільник, ESP32-S3, ADC1 oneshot + калібрування.

## Схема

```
3V3 ---- [край потенціометра]
GND ---- [інший край]
GPIO4 (ADC1_CH3) ---- [середній вивід]
```

## Параметри АЦП

| Параметр | Значення |
|---|---|
| Vref (для формули) | 3300 mV |
| Атенюація | 12 dB (`ADC_ATTEN_DB_12`) |
| Розрядність | 12 bit (0…4095) |
| Період | 100 ms |
| Калібрування | curve fitting (ESP32-S3) |

`U_manual(mV) = RAW * 3300 / 4095`

`U_cali` — `adc_cali_raw_to_voltage`.

`Error(%) = |U_manual - U_cali| / U_cali * 100`

## Збірка

```bash
pio run -d hw_3.2
pio run -d hw_3.2 -t upload
pio device monitor -d hw_3.2
```

Крути потенціометр під час 16 відліків, щоб таблиця покрила діапазон.

## Результат

Див. `serial_log.txt`. Графік (опційно): `python3 extra/plot_adc.py serial_log.txt`
