#include <FastLED.h>

#define NUM_LEDS 6
#define LED_PIN 2
#define TOUCH_THRESHOLD 5000 /* Lower the value, more the sensitivity */

RTC_DATA_ATTR int bootCount = 0;
touch_pad_t touchPin;
CRGB leds[NUM_LEDS];
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

CHSV get_led_rgb(uint16_t ms, uint16_t phase_offset) {
  int f = 20;
  int16_t sin = sin16(ms * f + phase_offset);
  uint8_t v = lerp8by16(144, 255, sin);
  float h = 30.0;

  return CHSV(hue_to_int8(h), 255, v);
}

void animate() {
  uint16_t ms = millis();

  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = get_led_rgb(ms, i * 20000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  print_wakeup_reason();

  //Setup sleep and wakeup on Touch Pad 3 (GPIO3/D2)
  touchSleepWakeUpEnable(T3, TOUCH_THRESHOLD);
  touchAttachInterrupt(T3, touch_interrupt, TOUCH_THRESHOLD);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
}

void loop() {
  if (go_sleep) {
    sleep();
  }

  animate();
  FastLED.show();
}
