/*
 * PC Monitor — Arduino Nano + LCD1602 (I2C)
 * Line 1: CPU load + progress bar
 * Line 2: RAM usage + progress bar
 * Data via USB Serial @ 9600 baud
 *
 * Format:  CPU:XX|RAM:XX
 * Example: CPU:45|RAM:62
 *
 * Wiring:
 *   SDA -> A4
 *   SCL -> A5
 *   VCC -> 5V
 *   GND -> GND
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD1602 — адрес 0x27 (если не работает, попробуй 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ─── Кастомные символы ───────────────────────────────────
byte blockFull[8] = {
  B11111, B11111, B11111, B11111,
  B11111, B11111, B11111, B11111
};

byte blockEmpty[8] = {
  B11111, B10001, B10001, B10001,
  B10001, B10001, B10001, B11111
};

// ─── Состояние ───────────────────────────────────────────
float cpuLoad  = 0.0;
float ramUsage = 0.0;

unsigned long lastUpdate = 0;
bool connected = false;

static const unsigned long TIMEOUT_MS = 5000;

// ─── Буфер (C-стиль, без String, без фрагментации) ─────
static const byte MAX_BUF = 64;
char buffer[MAX_BUF];
byte bufPos = 0;

// ─── Экран ожидания ─────────────────────────────────────
static bool waitingShown = false;
static byte dots = 0;
static unsigned long lastWaitingDraw = 0;

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, blockFull);
  lcd.createChar(1, blockEmpty);

  // Приветствие
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   PC Monitor    ");
  lcd.setCursor(0, 1);
  lcd.print(" Waiting for PC  ");
  delay(2000);

  // Очищаем буфер
  memset(buffer, 0, sizeof(buffer));
  bufPos = 0;
  waitingShown = false;

  Serial.println("Nano1602 PC Monitor v1.0 — Ready");
}

// ─── LOOP ────────────────────────────────────────────────
void loop() {
  // Читаем Serial
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      buffer[bufPos] = '\0';
      parseData(buffer);
      bufPos = 0;
      memset(buffer, 0, sizeof(buffer));
    } else if (bufPos < MAX_BUF - 1) {
      buffer[bufPos++] = c;
    }
  }

  // Таймаут — 5 сек без данных
  if (millis() - lastUpdate > TIMEOUT_MS) {
    connected = false;
  }

  updateDisplay();
  delay(200);
}

// ─── Парсинг данных ─────────────────────────────────────
void parseData(char* data) {
  // Ищем CPU: и RAM:
  char* cpuPtr = strstr(data, "CPU:");
  char* ramPtr = strstr(data, "RAM:");

  if (cpuPtr != NULL) {
    cpuPtr += 4;
    char* end = strchr(cpuPtr, '|');
    if (end == NULL) end = cpuPtr + strlen(cpuPtr);
    char temp[8];
    int len = min((int)(end - cpuPtr), 7);
    strncpy(temp, cpuPtr, len);
    temp[len] = '\0';
    cpuLoad = atof(temp);
  }

  if (ramPtr != NULL) {
    ramPtr += 4;
    char* end = strchr(ramPtr, '|');
    if (end == NULL) end = ramPtr + strlen(ramPtr);
    char temp[8];
    int len = min((int)(end - ramPtr), 7);
    strncpy(temp, ramPtr, len);
    temp[len] = '\0';
    ramUsage = atof(temp);
  }

  lastUpdate = millis();
  connected = true;
  Serial.println("OK");
}

// ─── Прогресс-бар ───────────────────────────────────────
void drawBar(int col, int row, float value, int bars) {
  int filled = map(constrain((int)value, 0, 100), 0, 100, 0, bars);

  lcd.setCursor(col, row);
  for (int i = 0; i < bars; i++) {
    lcd.write((byte)(i < filled ? 0 : 1));
  }
}

// ─── Экран ожидания (без lcd.clear!) ────────────────────
void showWaitingScreen() {
  // Рисуем только раз, потом обновляем точки
  if (!waitingShown) {
    lcd.setCursor(0, 0);
    lcd.print("Waiting for PC  ");
    lcd.setCursor(0, 1);
    lcd.print("Connecting");
    waitingShown = true;
  }

  // Обновляем точки каждые 500мс
  if (millis() - lastWaitingDraw > 500) {
    lastWaitingDraw = millis();
    // Очищаем область точек (6 символов)
    lcd.setCursor(10, 1);
    for (int i = 0; i < 6; i++) lcd.print(' ');
    // Рисуем точки
    lcd.setCursor(10, 1);
    for (int i = 0; i < (dots % 4); i++) lcd.print('.');
    dots++;
  }
}

// ─── Главный цикл дисплея ──────────────────────────────
void updateDisplay() {
  if (!connected) {
    showWaitingScreen();
    return;
  }

  // Первичная очистка при возврате из режима ожидания
  if (waitingShown) {
    lcd.clear();
    waitingShown = false;
  }

  // ─── Строка 1: CPU ───────────────────────────────────
  lcd.setCursor(0, 0);
  int cpu = constrain((int)cpuLoad, 0, 100);
  lcd.print("CPU:");
  if (cpu < 10) lcd.print(" ");
  lcd.print(cpu);
  lcd.print("% ");
  drawBar(9, 0, cpuLoad, 5); // Сдвинули на 1 влево, уменьшили бар до 5

  // ─── Строка 2: RAM ───────────────────────────────────
  lcd.setCursor(0, 1);
  int ram = constrain((int)ramUsage, 0, 100);
  lcd.print("RAM:");
  if (ram < 10) lcd.print(" ");
  lcd.print(ram);
  lcd.print("% ");
  drawBar(9, 1, ramUsage, 5);
}
