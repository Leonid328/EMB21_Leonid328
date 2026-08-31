# 🛠️ Інструменти та їх використання

Цей файл містить інструкції щодо використання основних інструментів, які використовуються у проєкті.

## 🔧 PlatformIO

### Встановлення
1. Встановіть [VSCode](https://code.visualstudio.com/).
2. Додайте розширення **PlatformIO**.
3. Відкрийте проєкт і переконайтесь, що все встановлено правильно.

### Основні команди
- `platformio run`: Збірка проєкту.
- `platformio run --target upload`: Завантаження коду на мікроконтролер.
- `platformio device monitor`: Відкриття терміналу для моніторингу виводу.

### Налаштування
Конфігурація проєкту описується у файлі `platformio.ini`.

**Приклад:**
```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

## 📝 Clang-Format

### Встановлення
1. Встановіть розширення **Clang-Format** у VSCode.
2. Додайте налаштування у `.vscode/settings.json`:
   ```json
   {
       "editor.defaultFormatter": "xaver.clang-format",
       "editor.formatOnSave": true,
       "[c]": {
           "editor.defaultFormatter": "xaver.clang-format"
       },
       "[cpp]": {
           "editor.defaultFormatter": "xaver.clang-format"
       }
   }
   ```

### Автоматичне форматування
- Виконайте команду `clang-format -i filename.c` для форматування файлу.
- Використовуйте скрипт `scripts/format_code.sh` для форматування всіх файлів:
  ```bash
  ./scripts/format_code.sh
  ```

## 🔄 Git

### Створення нової гілки
```bash
git checkout -b hw1-blinky
```

### Коміти та пуш
```bash
git add .
git commit -m "HW1: Блимання світлодіодом"
git push origin hw1-blinky
```

### Pull Request (PR)
1. Відкрийте Pull Request у `main`.
2. Додайте інформативну назву та опис.
3. Додайте скріншот або лог виводу, якщо це доречно.

## 📦 Скрипти

### Очистка проєкту
Виконайте команду:
```bash
./scripts/clean.sh
```

### Створення нової гілки та папки для завдання
Виконайте команду:
```bash
./scripts/create_hw_branch.sh hw1-blinky
```

Цей скрипт автоматично створить гілку та папку для завдання.

Виконайте команду:
```bash
./scripts/create_hw_branch.sh hw1-blinky
```

Цей скрипт автоматично створить гілку та папку для завдання.

## 📚 Додаткові ресурси

### Офіційна документація
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [STM32CubeMX Documentation](https://www.st.com/en/development-tools/stm32cubemx.html)

### Приклади коду
- Папка `examples/` містить приклади роботи зі світлодіодами, сенсорами та інтерфейсами.

## 🔴 Розв'язання проблем

### Проблема: Код не компілюється через `-Werror`
**Розв'язання:**
1. Видаліть прапорець `-Werror` у `platformio.ini`.
2. Виправте всі попередження.
3. Поверніть прапорець `-Werror` після виправлення.

### Проблема: Не вдається завантажити код на мікроконтролер
**Розв'язання:**
1. Перевірте кабель USB.
2. Увімкніть режиму завантаження (для ESP32S3).
3. Виконайте команду `platformio run --target upload`.

### Проблема: Не працює моніторинг порту
**Розв'язання:**
1. Перевірте швидкість порту у `platformio.ini` (наприклад, `monitor_speed = 115200`).
2. Виконайте команду `platformio device monitor`.
3. Увімкніть вивід даних у коді:
   ```c
   Serial.begin(115200);
   Serial.println("Hello, World!");
   ```
