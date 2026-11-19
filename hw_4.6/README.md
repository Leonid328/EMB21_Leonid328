# hw_4.6: ADC DMA + UART DMA (module 4 lesson 6)

ESP32-S3. Potentiometer on **GPIO4 / ADC1_CH3** (same as `hw_3.2`).

1. ADC continuous converter uses **GDMA** (`adc_continuous_*`).
2. Converted samples are sent to the USB/UART console with the **UART driver TX buffer** (ESP32-S3 UART TX path uses GDMA). No byte-by-byte CPU copy in the app loop.
3. Optional scope line: **Vmin Vmax Vpp Vavg** in millivolts.

## Wiring

```
3V3 ---- pot end
GND ---- pot end
GPIO4 (ADC1_CH3) ---- pot wiper
```

USB serial 115200.

## Build

```bash
pio run -d hw_4.6
pio run -d hw_4.6 -t upload
pio device monitor -d hw_4.6
```
