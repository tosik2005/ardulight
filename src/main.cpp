#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     6
#define NUM_LEDS    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

// --- Налаштування плавності ---
int brightness0 = 0;
int brightness1 = 0;
int fadeSpeed = 15;
unsigned long lastFadeTime = 0;

struct RadarState {
  byte buffer[48];
  byte index = 0;
  bool isDetected = false;
  unsigned long lastPacketTime = 0;
};

RadarState radar0State; // Serial2 (16 TX / 17 RX) -> Pixel 0
RadarState radar1State; // Serial1 (18 TX / 19 RX) -> Pixel 1

bool processRadar(HardwareSerial& radarSerial, RadarState& state);

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);   
  Serial1.begin(256000);  // Радар 1
  Serial2.begin(256000);  // Радар 0

  Serial.println("System started with updated LD2410 parser!");
}

void loop() {
  // КРОК 1: Опитування радарів
  bool radar0_active = processRadar(Serial2, radar0State); 
  bool radar1_active = processRadar(Serial1, radar1State); 

  // КРОК 2: Плавне керування світлом (50 FPS)
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastFadeTime >= 20) {
    lastFadeTime = currentMillis;

    // Піксель 0 (Радар 0)
    if (radar0_active) {
      brightness0 = min(255, brightness0 + fadeSpeed);
    } else {
      brightness0 = max(0, brightness0 - fadeSpeed);
    }

    // Піксель 1 (Радар 1)
    if (radar1_active) {
      brightness1 = min(255, brightness1 + fadeSpeed);
    } else {
      brightness1 = max(0, brightness1 - fadeSpeed);
    }

    leds[0] = CRGB(brightness0, brightness0, brightness0);
    leds[1] = CRGB(brightness1, brightness1, brightness1);

    FastLED.show();
  }

  // КРОК 3: Діагностика в консоль
  static unsigned long lastLog = 0;
  if (currentMillis - lastLog >= 300) {
    lastLog = millis();
    Serial.print("Pixel 0: ");
    Serial.print(radar0_active ? "[ON ] " : "[OFF] ");
    Serial.print(" | Pixel 1: ");
    Serial.println(radar1_active ? "[ON ]" : "[OFF]");
  }
}

// =========================================================
// УНІВЕРСАЛЬНИЙ ПАРСЕР LD2410 (Звичайний + Інженерний режим)
// =========================================================
bool processRadar(HardwareSerial& radarSerial, RadarState& state) {
  while (radarSerial.available()) {
    byte b = radarSerial.read();

    // Перевірка заголовка: F4 F3 F2 F1
    if (state.index == 0 && b != 0xF4) continue;
    if (state.index == 1 && b != 0xF3) { state.index = 0; continue; }
    if (state.index == 2 && b != 0xF2) { state.index = 0; continue; }
    if (state.index == 3 && b != 0xF1) { state.index = 0; continue; }

    state.buffer[state.index++] = b;

    // Мінімальна довжина кадру для зчитування стану — 9 байт
    if (state.index >= 9) {
      // Маска 0x7F прибирає інженерний прапор 0x80 (128 -> 0, 129 -> 1, 130 -> 2)
      byte targetStatus = state.buffer[8] & 0x7F; 
      
      if (targetStatus > 0x00 && targetStatus <= 0x03) {
        state.isDetected = true;
        state.lastPacketTime = millis();
      }
    }

    // Перевірка кінця кадру (F8 F7 F6 F5) або переповнення буфера
    if (state.index >= 8) {
      if (state.buffer[state.index - 4] == 0xF8 && 
          state.buffer[state.index - 3] == 0xF7 && 
          state.buffer[state.index - 2] == 0xF6 && 
          state.buffer[state.index - 1] == 0xF5) {
        state.index = 0; // Кадр успішно закритий
      }
    }

    if (state.index >= 45) {
      state.index = 0; // Захист від виходу за межі
    }
  }

  // Таймаут 2000 мс (перекриває паузи радарів у 1.4 с)
  if (millis() - state.lastPacketTime > 2000) {
    state.isDetected = false;
  }

  return state.isDetected;
}