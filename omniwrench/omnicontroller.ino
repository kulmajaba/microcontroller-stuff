#include <FastLED.h>
#include <limits.h>

#define NUM_LEDS 1
#define DATA_PIN 2

CRGB leds[NUM_LEDS];

void setup()
{
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void loop()
{
  animate();
  FastLED.show();

}

int hue_to_int8(float hue)
{
  return hue / 360 * 255;
}

uint8_t lerp8by16(uint8_t min, uint8_t max, int16_t posRaw)
{
  float pos = posRaw / 65535.0 + 0.5;
  return min + pos * (max - min);
}

void animate()
{
  int f = 10;
  int a = INT_MAX;
  uint16_t ms = millis();
  int16_t sin = sin16(ms * f);
  uint8_t v = lerp8by16(192, 255, sin);
  leds[0] = CHSV(hue_to_int8(33), 255, v);
}
