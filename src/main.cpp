#include <FastLED.h>

#define LED_PIN     6          // Пін Din від Arduino
#define NUM_LEDS    2          // 2 пікселі підключені послідовно
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // 1. Плавне розгоряння (від 0 до 255)
  for (int brightness = 0; brightness <= 255; brightness++) {
    // Встановлюємо однаковий рівень для всіх 3 каналів обох пікселів
    leds[0] = CRGB(brightness, brightness, brightness);
    leds[1] = CRGB(brightness, brightness, brightness);
    
    FastLED.show();
    delay(10); // Швидкість розгоряння (чим менше значення, тим швидше)
  }

  // 2. Плавне згасання (від 255 до 0)
  for (int brightness = 255; brightness >= 0; brightness--) {
    leds[0] = CRGB(brightness, brightness, brightness);
    leds[1] = CRGB(brightness, brightness, brightness);
    
    FastLED.show();
    delay(10); // Швидкість згасання
  }

  delay(500); // Пауза пів секунди у вимкненому стані перед наступним циклом
}