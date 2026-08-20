#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     6
#define NUM_LEDS    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

int currentBright0 = 0;
int currentBright1 = 0;
int targetBright0  = 0;
int targetBright1  = 0;

int fadeSpeed = 15; 
unsigned long lastFadeTime = 0;

struct RadarState {
  byte buffer[64];
  byte index = 0;
  bool isDetected = false;
  uint16_t distance = 0;
  unsigned long lastPacketTime = 0;
};

RadarState radar0State; // Serial2 (16 TX / 17 RX) -> Pixel 0
RadarState radar1State; // Serial1 (18 TX / 19 RX) -> Pixel 1

void processRadarDynamic(HardwareSerial& radarSerial, RadarState& state);
int calculateTargetBrightness(bool isDetected, uint16_t distance);

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);   
  Serial1.begin(256000);  // Радар 1
  Serial2.begin(256000);  // Радар 0

  Serial.println("System ready: Dynamic Stream Parser");
}

void loop() {
  // 1. Зчитуємо дані з радарів
  processRadarDynamic(Serial2, radar0State); 
  processRadarDynamic(Serial1, radar1State); 

  // 2. Вираховуємо необхідну яскравість
  targetBright0 = calculateTargetBrightness(radar0State.isDetected, radar0State.distance);
  targetBright1 = calculateTargetBrightness(radar1State.isDetected, radar1State.distance);

  // 3. Плавна зміна яскравості (50 кадрів/сек)
  unsigned long currentMillis = millis();
  if (currentMillis - lastFadeTime >= 20) {
    lastFadeTime = currentMillis;

    // Піксель 0
    if (currentBright0 < targetBright0) currentBright0 = min(targetBright0, currentBright0 + fadeSpeed);
    else if (currentBright0 > targetBright0) currentBright0 = max(targetBright0, currentBright0 - fadeSpeed);

    // Піксель 1
    if (currentBright1 < targetBright1) currentBright1 = min(targetBright1, currentBright1 + fadeSpeed);
    else if (currentBright1 > targetBright1) currentBright1 = max(targetBright1, currentBright1 - fadeSpeed);

    // Використовуємо HSV (Hue=0, Saturation=0, Value=Brightness) для чистого білого світла
    leds[0] = CHSV(0, 0, currentBright0);
    leds[1] = CHSV(0, 0, currentBright1);

    FastLED.show();
  }

  // 4. Монітор порту
  static unsigned long lastLog = 0;
  if (currentMillis - lastLog >= 300) {
    lastLog = currentMillis;
    
    Serial.print("R0: ");
    if (radar0State.isDetected) {
      Serial.print("DET ("); Serial.print(radar0State.distance); 
      Serial.print("cm) -> Br:"); Serial.print(targetBright0);
    } else {
      Serial.print("OFF                ");
    }

    Serial.print(" | R1: ");
    if (radar1State.isDetected) {
      Serial.print("DET ("); Serial.print(radar1State.distance); 
      Serial.print("cm) -> Br:"); Serial.println(targetBright1);
    } else {
      Serial.println("OFF");
    }
  }
}

int calculateTargetBrightness(bool isDetected, uint16_t distance) {
  if (!isDetected) return 0;

  if (distance <= 40) {
    return 255;
  } else if (distance >= 120) {
    return 10;
  } else {
    return map(distance, 40, 120, 255, 10);
  }
}

// Потоковий парсер з динамічним пошуком маркера кінця
void processRadarDynamic(HardwareSerial& radarSerial, RadarState& state) {
  while (radarSerial.available()) {
    byte b = radarSerial.read();

    // Заголовок кадру: F4 F3 F2 F1
    if (state.index == 0 && b != 0xF4) continue;
    if (state.index == 1 && b != 0xF3) { state.index = 0; continue; }
    if (state.index == 2 && b != 0xF2) { state.index = 0; continue; }
    if (state.index == 3 && b != 0xF1) { state.index = 0; continue; }

    state.buffer[state.index++] = b;

    // Перевіряємо останні 4 байти на маркер кінця F8 F7 F6 F5
    if (state.index >= 8) {
      if (state.buffer[state.index - 4] == 0xF8 &&
          state.buffer[state.index - 3] == 0xF7 &&
          state.buffer[state.index - 2] == 0xF6 &&
          state.buffer[state.index - 1] == 0xF5) {
        
        // Кадр повністю зібрано!
        if (state.index >= 14) { // Перевірка наявності даних про стан і дистанцію
          byte targetStatus = state.buffer[8] & 0x7F;

          if (targetStatus > 0x00 && targetStatus <= 0x03) {
            state.isDetected = true;
            state.lastPacketTime = millis();

            uint16_t moveDist = state.buffer[9] | (state.buffer[10] << 8);
            uint16_t staticDist = state.buffer[12] | (state.buffer[13] << 8);

            if (targetStatus == 0x01) state.distance = moveDist;
            else if (targetStatus == 0x02) state.distance = staticDist;
            else if (targetStatus == 0x03) state.distance = min(moveDist, staticDist);
          } else {
            state.isDetected = false;
          }
        }
        state.index = 0; // Скидаємо буфер для наступного кадру
      }
    }

    // Захист від переповнення
    if (state.index >= 60) {
      state.index = 0;
    }
  }

  // Таймаут втрати зв'язку (1.2 сек)
  if (millis() - state.lastPacketTime > 1200) {
    state.isDetected = false;
  }
}