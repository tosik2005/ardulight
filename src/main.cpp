#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     6
#define NUM_LEDS    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

// --- Змінні для плавного світла ---
int brightness0 = 0; 
int brightness1 = 0; 
int fadeSpeed = 8;   // Швидкість зміни яскравості
unsigned long lastFadeTime = 0;

// Структура для збору даних з UART
struct RadarState {
  byte buffer[32];
  byte index = 0;
  bool isDetected = false;
  uint16_t distance = 999;      // Беззнакове ціле (ніколи не буде від'ємним)
  uint16_t smoothedDistance = 999; // Згладжена відстань для стабільності
};

RadarState radar0State; // Serial2 (Піни 16, 17) -> Піксель 0
RadarState radar1State; // Serial1 (Піни 18, 19) -> Піксель 1

bool processRadar(HardwareSerial& radarSerial, RadarState& state);
int getBrightnessForDistance(uint16_t dist);

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);
  Serial1.begin(256000); 
  Serial2.begin(256000);

  Serial.println("System started! Stabilized distance tracking active.");
}

void loop() {
  // КРОК 1: Збір даних
  bool r0_active = processRadar(Serial2, radar0State); 
  bool r1_active = processRadar(Serial1, radar1State); 

  // КРОК 2: Неблокуюче плавне керування пікселями
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastFadeTime >= 20) {
    lastFadeTime = currentMillis;

    // --- Логіка для Пікселя 0 ---
    int target0 = r0_active ? getBrightnessForDistance(radar0State.smoothedDistance) : 0;
    
    if (brightness0 < target0) {
      brightness0 += fadeSpeed;
      if (brightness0 > target0) brightness0 = target0;
    } else if (brightness0 > target0) {
      brightness0 -= fadeSpeed;
      if (brightness0 < target0) brightness0 = target0;
    }

    // --- Логіка для Пікселя 1 ---
    int target1 = r1_active ? getBrightnessForDistance(radar1State.smoothedDistance) : 0;
    
    if (brightness1 < target1) {
      brightness1 += fadeSpeed;
      if (brightness1 > target1) brightness1 = target1;
    } else if (brightness1 > target1) {
      brightness1 -= fadeSpeed;
      if (brightness1 < target1) brightness1 = target1;
    }

    leds[0] = CRGB(brightness0, brightness0, brightness0);
    leds[1] = CRGB(brightness1, brightness1, brightness1);
    FastLED.show();
  }

  // КРОК 3: Діагностика в консоль (раз на 500 мс)
  static unsigned long lastLog = 0;
  if (currentMillis - lastLog >= 500) {
    lastLog = currentMillis;
    Serial.print("R0: "); 
    if (r0_active) {
      Serial.print("[ON  dist="); Serial.print(radar0State.distance); 
      Serial.print(" smooth="); Serial.print(radar0State.smoothedDistance);
      Serial.print(" br="); Serial.print(getBrightnessForDistance(radar0State.smoothedDistance)); Serial.print("] ");
    } else {
      Serial.print("[OFF] ");
    }
    
    Serial.print("| R1: "); 
    if (r1_active) {
      Serial.print("[ON  dist="); Serial.print(radar1State.distance); 
      Serial.print(" smooth="); Serial.print(radar1State.smoothedDistance);
      Serial.print(" br="); Serial.print(getBrightnessForDistance(radar1State.smoothedDistance)); Serial.print("]");
    } else {
      Serial.print("[OFF]");
    }
    Serial.println();
  }
}

// =========================================================
// ФУНКЦІЯ РОЗРАХУНКУ ЯСКРАВОСТІ ЗА ВІДСТАННЮ
// =========================================================
int getBrightnessForDistance(uint16_t dist) {
  if (dist <= 90)  return 255; // 100%
  if (dist <= 130) return 204; // 80%
  if (dist <= 170) return 153; // 60%
  if (dist <= 210) return 102; // 40%
  if (dist <= 250) return 51;  // 20%
  return 0;                    // > 250 см (0%)
}

// =========================================================
// ФУНКЦІЯ ПАРСИНГУ ДАНИХ RADAR HLK-LD2410C (Стабілізована)
// =========================================================
bool processRadar(HardwareSerial& radarSerial, RadarState& state) {
  while (radarSerial.available()) {
    byte b = radarSerial.read();

    // 1. Пошук заголовка
    if (state.index == 0 && b != 0xF4) continue;
    if (state.index == 1 && b != 0xF3) { state.index = 0; continue; }
    if (state.index == 2 && b != 0xF2) { state.index = 0; continue; }
    if (state.index == 3 && b != 0xF1) { state.index = 0; continue; }

    state.buffer[state.index] = b;
    state.index++;

    // 2. Перевірка довжини пакету
    if (state.index >= 18) {
      if (state.buffer[6] == 0x02) { 
        
        byte targetStatus = state.buffer[8] & 0x03; 
        
        // ВИПРАВЛЕННЯ 1: Використовуємо uint16_t, щоб уникнути від'ємних чисел
        uint16_t movingDist = state.buffer[9] | (state.buffer[10] << 8);
        byte movingEnergy = state.buffer[11];
        
        uint16_t staticDist = state.buffer[13] | (state.buffer[14] << 8);
        byte staticEnergy = state.buffer[15];

        uint16_t activeDist = 999;
        byte activeEnergy = 0;

        if ((targetStatus & 0x01) != 0) { 
          activeDist = movingDist;
          activeEnergy = movingEnergy;
        } else if ((targetStatus & 0x02) != 0) { 
          activeDist = staticDist;
          activeEnergy = staticEnergy;
        }

        // ВИПРАВЛЕННЯ 2: Суворіша перевірка. Відстань має бути > 0 і <= 250.
        // Енергію трохи підвищили до 8, щоб відсікти фоновий шум на 0 см.
        if (activeDist > 0 && activeDist <= 250 && activeEnergy > 8) {
          state.isDetected = true;
          state.distance = activeDist;
          
          // ВИПРАВЛЕННЯ 3: Просте експоненційне згладжування (70% старе значення + 30% нове)
          // Це усуває різкі стрибки від одного помилкового пакету
          state.smoothedDistance = (state.smoothedDistance * 7 + activeDist * 3) / 10;
          
        } else {
          state.isDetected = false;
          state.distance = 0;
          state.smoothedDistance = 999; // Скидаємо згладжене значення
        }
      }
      state.index = 0; 
    }
  }
  return state.isDetected;
}