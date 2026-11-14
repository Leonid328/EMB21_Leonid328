# hw_4.2 I2C intro SSD1306 plan

Datasheets used:

- OLED SSD1306: https://files.waveshare.com/upload/a/af/SSD1306-Revision_1.1.pdf
- RTC DS1307: https://eu.mouser.com/datasheet/3/1014/1/DS1307.pdf
- EEPROM AT24C32E: https://eu.mouser.com/datasheet/3/282/1/AT24C32E_I2C_Compatible__Two_Wire__Serial_EEPROM_32_Kbit__4_096x8__20006109B.pdf

I2C 7-bit addresses (typical module):

- SSD1306: 0x3C (SA0 low) or 0x3D
- DS1307: 0x68
- AT24C32E: 0x50 plus A2..A0

Control bytes for SSD1306:

- Co=0, D/C#=0: command stream (0x00)
- Co=0, D/C#=1: data stream (0x40)

Init sequence for 128x64, horizontal addressing:

1. Display OFF `0xAE`
2. Set display clock `0xD5`, `0x80`
3. Set multiplex `0xA8`, `0x3F` (64 rows)
4. Set display offset `0xD3`, `0x00`
5. Set start line `0x40`
6. Charge pump `0x8D`, `0x14`
7. Memory addressing mode `0x20`, `0x00` (horizontal)
8. Segment remap `0xA1`
9. COM scan direction `0xC8`
10. COM pins `0xDA`, `0x12`
11. Contrast `0x81`, `0xCF`
12. Precharge `0xD9`, `0xF1`
13. VCOMH `0xDB`, `0x40`
14. Resume RAM `0xA4`
15. Normal display `0xA6`
16. Display ON `0xAF`

Framebuffer update:

1. Column range `0x21`, `0x00`, `0x7F`
2. Page range `0x22`, `0x00`, `0x07`
3. Write 1024 GDDRAM bytes with control `0x40`

DS1307 time registers from 0x00: sec, min, hour, day-of-week, date, month, year (BCD). Bit 7 of seconds is CH; clear it to run the oscillator.

AT24C32E: 16-bit word address, then data. Not used in hw_4.3 clock.

Work order:

1. Scan bus, confirm 0x3C and 0x68
2. Send SSD1306 init
3. Fill GDDRAM with a test pattern
4. Read DS1307, convert BCD
5. Render HH:MM:SS and EEE DD.MM.YYYY once per second
