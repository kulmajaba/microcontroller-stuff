#include <FastLED.h>

#include "led.h"

#define TOUCH_THRESHOLD 2000 /* Lower the value, more the sensitivity */

#define NUM_LEDS 16
#define GUARD_LED_PIN D3
#define BOLSTER_LED_PIN D4
#define FORK_LED_PIN_1 D5
#define FORK_LED_PIN_2 D6

#define LED_POWER_PIN D10

CRGB leds[NUM_LEDS];
led_group led_groups[3] = {
  { {2, 1, 0, 3, 4, 5}, 3, 0, 20, 50000, 0 },
  { {0, 1, 2, 3, 4, 5}, 1, 6, 20, 0, 0 },
  { {3, 2, 1, 0}, 4, 12, 20, 50000, 0 }
};

RTC_DATA_ATTR int bootCount = 0;
bool go_sleep = false;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  esp_reset_reason_t reset_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  reset_reason = esp_reset_reason();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d, reset reason: %d\n", wakeup_reason, reset_reason); break;
  }
}

void touch_interrupt () {
  go_sleep = true;
}

void sleep() {
  Serial.println("Going to sleep");
  FastLED.clear(true);
  digitalWrite(LED_POWER_PIN, HIGH);
  delay(2000);
  esp_deep_sleep_start();
}

uint8_t hue_to_int8(float hue) {
  return hue / 360 * 255;
}

uint8_t lerp8by16(uint8_t min, uint8_t max, int16_t posRaw) {
  float pos = posRaw / 65535.0 + 0.5;
  return min + pos * (max - min);
}

uint8_t clamped_sin(uint16_t ms, int freq, int phase, uint8_t min, uint8_t max) {
  int16_t sin = sin16(ms * freq + phase);
  return lerp8by16(144, 255, sin);
}

CHSV get_led_rgb(uint16_t ms, int freq, uint16_t phase) {
  float h = 30.0;
  uint8_t s = 255;
  uint8_t v = clamped_sin(ms, freq, phase, 144, 255);

  return CHSV(hue_to_int8(h), s, v);
}

void animate() {
  uint16_t ms = millis();

  for (led_group group : led_groups) {
    for (int i = 0; i < group.animated_leds; ++i) {
      CHSV new_rgb = get_led_rgb(ms, group.fx_freq, group.fx_initial_phase + i * group.fx_phase);
      for (int j = i; j < group.led_indexes.size(); j += group.animated_leds) {
        int led_index = group.leds_start_index + group.led_indexes[j];
        leds[led_index] = new_rgb;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  //delay(1000);  //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  //Setup sleep and wakeup on Touch Pad 3 (GPIO3/D2)
  touchSleepWakeUpEnable(T3, TOUCH_THRESHOLD);
  touchAttachInterrupt(T3, touch_interrupt, TOUCH_THRESHOLD);

  FastLED.addLeds<NEOPIXEL, GUARD_LED_PIN>(leds, led_groups[0].leds_start_index, static_cast<int>(led_groups[0].led_indexes.size()));
  FastLED.addLeds<NEOPIXEL, BOLSTER_LED_PIN>(leds, led_groups[1].leds_start_index, static_cast<int>(led_groups[1].led_indexes.size()));
  FastLED.addLeds<NEOPIXEL, FORK_LED_PIN_1>(leds, led_groups[2].leds_start_index, static_cast<int>(led_groups[2].led_indexes.size()));
  FastLED.addLeds<NEOPIXEL, FORK_LED_PIN_2>(leds, led_groups[2].leds_start_index, static_cast<int>(led_groups[2].led_indexes.size()));

  pinMode(LED_POWER_PIN, OUTPUT);
  digitalWrite(LED_POWER_PIN, LOW);
}

void loop() {
  if (go_sleep) {
    sleep();
  }

  animate();
  FastLED.show();
}
