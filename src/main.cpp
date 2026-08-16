#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     6
#define NUM_LEDS    2
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS];

void setup() {
  // Ініціалізація стрічки
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255); // Максимальна яскравість
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // 1. Плавне розгоряння (від 0 до 255)
  for (int brightness = 0; brightness <= 255; brightness++) {
    leds[0] = CRGB(brightness, brightness, brightness); // Білий
    leds[1] = CRGB(brightness, brightness, brightness); // Білий
    
    FastLED.show();
    delay(10); // Швидкість розгоряння (10 мс = ~2.5 секунди на повний цикл)
  }

  // 2. Пауза у ввімкненому стані
  delay(1000); // 1 секунда світить на максимумі

  // 3. Плавне згасання (від 255 до 0)
  for (int brightness = 255; brightness >= 0; brightness--) {
    leds[0] = CRGB(brightness, brightness, brightness);
    leds[1] = CRGB(brightness, brightness, brightness);
    
    FastLED.show();
    delay(10); // Швидкість згасання
  }

  // 4. Пауза у вимкненому стані
  delay(1000); // 1 секунда вимкнено
}