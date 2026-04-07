/*
 * PC & Environment Monitor — Arduino Nano + LCD1602 (I2C) + AHT10
 * Line 1: CPU load + RAM usage
 * Line 2: Temperature + Humidity (from AHT10 sensor)
 * Data via USB Serial @ 9600 baud
 *
 * Format:  CPU:XX|RAM:XX|TEMP:XX|HUM:XX
 * Example: CPU:45|RAM:62|TEMP:24.5|HUM:40.1
 *
 * Wiring:
 *   SDA -> A4 (LCD + AHT10)
 *   SCL -> A5 (LCD + AHT10)
 *   VCC -> 5V
 *   GND -> GND
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>

// LCD1602 — адрес 0x27 (если не работает, попробуй 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_AHTX0 aht;

// ─── Состояние ───────────────────────────────────────────
int cpuLoad  = 0;
int ramUsage = 0;
float tempVal = 0.0;
float humVal = 0.0;

unsigned long lastUpdate = 0;
bool usbConnected = false; // Статус USB-соединения

static const unsigned long TIMEOUT_MS = 3000; // Таймаут 3 секунды

// ─── Буфер (C-стиль, без String, без фрагментации) ─────
static const byte MAX_BUF = 64;
char buffer[MAX_BUF];
byte bufPos = 0;

// ─── Экран ожидания ─────────────────────────────────────
static bool waitingShown = false;

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();

  // AHT10
  if (!aht.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("AHT10 Error!");
    while (1) delay(10);
  }

  // Приветствие
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  PC & Env Mon  ");
  lcd.setCursor(0, 1);
  lcd.print(" Waiting for PC ");
  delay(2000);

  // Очищаем буфер
  memset(buffer, 0, sizeof(buffer));
  bufPos = 0;
  waitingShown = false;

  Serial.println("Nano1602-AHT10 v1.0 — Ready");
}

// ─── LOOP ────────────────────────────────────────────────
void loop() {
  // Читаем Serial
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Если получили хоть что-то, считаем что связь есть
    if (!usbConnected) {
      usbConnected = true;
      lcd.clear();
    }

    if (c == '\n') {
      buffer[bufPos] = '\0';
      parseData(buffer);
      bufPos = 0;
      memset(buffer, 0, sizeof(buffer));
    } else if (bufPos < MAX_BUF - 1) {
      buffer[bufPos++] = c;
    }
  }

  // Проверка потери связи
  if (millis() - lastUpdate > TIMEOUT_MS) {
    if (usbConnected) {
      usbConnected = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("USB Lost!");
      lcd.setCursor(0, 1);
      lcd.print("Waiting...");
    }
  }

  if (usbConnected) {
    updateDisplay();
  }
  
  delay(200);
}

// ─── Парсинг данных ─────────────────────────────────────
void parseData(char* data) {
  // Ищем метки
  char* cpuPtr = strstr(data, "CPU:");
  char* ramPtr = strstr(data, "RAM:");
  char* tempPtr = strstr(data, "TEMP:");
  char* humPtr = strstr(data, "HUM:");

  if (cpuPtr) cpuLoad = atoi(cpuPtr + 4);
  if (ramPtr) ramUsage = atoi(ramPtr + 4);
  
  if (tempPtr) {
    char tempStr[8];
    strncpy(tempStr, tempPtr + 5, 7);
    tempStr[7] = '\0';
    tempVal = atof(tempStr);
  }
  
  if (humPtr) {
    char humStr[8];
    strncpy(humStr, humPtr + 4, 7);
    humStr[7] = '\0';
    humVal = atof(humStr);
  }

  lastUpdate = millis();
  usbConnected = true;
  Serial.println("OK");
}

// ─── Главный цикл дисплея ──────────────────────────────
void updateDisplay() {
  if (!usbConnected) {
    if (!waitingShown) {
      lcd.clear();
      waitingShown = true;
    }
    return;
  }
  waitingShown = false;

  // ─── Строка 1: CPU и RAM ─────────────────────────────
  lcd.setCursor(0, 0);
  lcd.print("CPU:");
  if (cpuLoad < 10) lcd.print(" ");
  lcd.print(cpuLoad);
  lcd.print("%  RAM:");
  if (ramUsage < 10) lcd.print(" ");
  lcd.print(ramUsage);
  lcd.print("% ");

  // ─── Строка 2: Температура и Влажность (с AHT10) ─────
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(tempVal, 1);
  lcd.print((char)223); // Символ градуса
  lcd.print("C  H:");
  lcd.print(humVal, 1);
  lcd.print("% ");
}
