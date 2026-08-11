#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     6
#define NUM_LEDS    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

// --- ЗМІННІ ДЛЯ ПЛАВНОГО СВІТЛА ---
int brightness0 = 0; 
int brightness1 = 0; 
int fadeSpeed = 8;          // Швидкість зміни яскравості
unsigned long lastFadeTime = 0;

// Структура станiв для кожного радара
struct RadarState {
  byte buffer[30];
  byte index = 0;
  bool isDetected = false;
};

RadarState radar1State; // Радар 1 (Serial1) -> Піксель 1
RadarState radar2State; // Радар 2 (Serial2) -> Піксель 0

bool processRadar(HardwareSerial& radarSerial, RadarState& state);

void setup() {
  // 1. Ініціалізація світлодіодів
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // 2. Ініціалізація портів
  Serial.begin(115200);   // ПК Монітор
  Serial1.begin(256000);  // Радар 1 (піни 18, 19)
  Serial2.begin(256000);  // Радар 2 (піни 16, 17)

  Serial.println("System initialized successfully!");
}

void loop() {
  // КРОК 1: Читаємо стан радарів
  bool target0_detected = processRadar(Serial2, radar2State); // Піксель 0
  bool target1_detected = processRadar(Serial1, radar1State); // Піксель 1

  // КРОК 2: Плавне регулювання яскравості (Fade In / Fade Out)
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastFadeTime >= 15) {
    lastFadeTime = currentMillis;

    // Логіка для Пікселя 0
    if (target0_detected) {
      brightness0 += fadeSpeed;
      if (brightness0 > 255) brightness0 = 255;
    } else {
      brightness0 -= fadeSpeed;
      if (brightness0 < 0) brightness0 = 0;
    }

    // Логіка для Пікселя 1
    if (target1_detected) {
      brightness1 += fadeSpeed;
      if (brightness1 > 255) brightness1 = 255;
    } else {
      brightness1 -= fadeSpeed;
      if (brightness1 < 0) brightness1 = 0;
    }

    // Застосовуємо яскравість до пікселів
    leds[0] = CRGB(brightness0, brightness0, brightness0);
    leds[1] = CRGB(brightness1, brightness1, brightness1);

    FastLED.show();
  }

  // КРОК 3: Моніторинг у консоль (раз на пів секунди)
  static unsigned long lastDebugTime = 0;
  if (currentMillis - lastDebugTime >= 500) {
    lastDebugTime = currentMillis;
    Serial.print("Pixel 0 (Serial2): ");
    Serial.print(target0_detected ? "DETECTED " : "Empty    ");
    Serial.print(" | Pixel 1 (Serial1): ");
    Serial.println(target1_detected ? "DETECTED" : "Empty");
  }
}

// ==========================================================
// ПАРСЕР ДЛЯ СТАНДАРТНОГО КАДРУ LD2410 (Header: F4 F3 F2 F1)
// ==========================================================
bool processRadar(HardwareSerial& radarSerial, RadarState& state) {
  while (radarSerial.available()) {
    byte incomingByte = radarSerial.read();

    // 1. Пошук заголовка: 0xF4 0xF3 0xF2 0xF1
    if (state.index == 0 && incomingByte != 0xF4) continue;
    if (state.index == 1 && incomingByte != 0xF3) { state.index = 0; continue; }
    if (state.index == 2 && incomingByte != 0xF2) { state.index = 0; continue; }
    if (state.index == 3 && incomingByte != 0xF1) { state.index = 0; continue; }

    state.buffer[state.index] = incomingByte;
    state.index++;

    // 2. Стандартна довжина каду LD2410 — 23 байти
    if (state.index >= 23) {
      // Байт 8 — стан цілі: 0x00 = Немає, 0x01 = Рух, 0x02 = Спокій, 0x03 = Рух+Спокій
      byte targetState = state.buffer[8]; 

      // Якщо стан != 0, радар бачить людину/рух
      state.isDetected = (targetState != 0x00);

      state.index = 0; // Скидаємо буфер після отримання пакета
    }
  }
  return state.isDetected;
}