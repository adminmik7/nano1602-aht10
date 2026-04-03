# 🔌 nano1602 — Arduino Nano + LCD1602 PC Monitor

CPU load + RAM usage on a 16×2 I2C display. Data sent via USB Serial.

---

## 📁 Файлы

| Файл | Назначение |
|---|---|
| `nano1602.ino` | Прошивка для Arduino Nano |
| `sender.py` | Отправка данных с Linux-ПК |
| `start.sh` | Один скрипт: ставит Python + зависимости + запускает |
| `requirements.txt` | Зависимости для `pip install -r` |

## 🔌 Подключение

```
Arduino Nano          LCD1602 (I2C)
──────────────────────────────────────
A4 (SDA)      ─────►  SDA
A5 (SCL)      ─────►  SCL
5V            ─────►  VCC
GND           ─────►  GND
USB           ─────►  PC (данные + питание)
```

I2C адрес: **0x27** (если не работает — попробуй 0x3F)

## 🖥️ Экран

```
CPU:45% [██████    ]
RAM:62% [█████     ]
```

Формат данных: `CPU:XX|RAM:XX\n`

## 📥 Установка и Запуск

### Шаг 1: Скачивание
Склонируй репозиторий:
```bash
git clone https://github.com/adminmik7/nano1602.git
cd nano1602
```

### Шаг 2: Прошивка Arduino
1. Открой `nano1602.ino` в **Arduino IDE**.
2. Установи библиотеку `LiquidCrystal_I2C` через менеджер библиотек.
3. Залей скетч в плату **Arduino Nano**. (Не забудь выбрать правильный порт и плату в меню "Инструменты").

### Шаг 3: Запуск на ПК (Linux)
Теперь нужно запустить скрипт на компьютере, который будет отправлять данные на Arduino.

**Самый простой способ (через `start.sh`):**
```bash
chmod +x start.sh
./start.sh
```
> Скрипт сам проверит наличие Python3, установит `pyserial` и `psutil`, а затем найдет Arduino-порт.

**Запуск конкретного порта:**
```bash
./start.sh /dev/ttyUSB0
```

**Только настройка (без запуска):**
```bash
./start.sh setup
```

---

## 📊 Зависимости

| Для чего | Что нужно |
|---|---|
| **Arduino** | Библиотека `LiquidCrystal_I2C` |
| **Python (Linux)** | `pyserial` и `psutil` (устанавливаются автоматически) |

## 📊 Зависимости

**Arduino:**
- `LiquidCrystal_I2C` (менеджер библиотек Arduino IDE)

**Python:**
- `pyserial` — авто-установка
- `psutil` — авто-установка (без него работает из `/proc` — но менее точно)
