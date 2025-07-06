#include <limits.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#include <FastLED.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/touch_pad.h>
#include <esp_log.h>
#include <esp_check.h>

#define NUM_LEDS 1
#define DATA_PIN 2
#define TOUCH_PAD TOUCH_PAD_NUM1

#if CONFIG_IDF_TARGET_ESP32
#define THRESHOLD 40 /* Greater the value, more the sensitivity */
#elif (CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3)
#define THRESHOLD 5000 /* Lower the value, more the sensitivity */
#else                  // ESP32-P4 + default for other chips (to be adjusted) */
#define THRESHOLD 500  /* Lower the value, more the sensitivity */
#endif

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

CRGB leds[NUM_LEDS];

uint32_t touch_value;

/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
static void tp_example_read_task(void *pvParameter)
{
    /* Wait touch sensor init done */
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("Touch Sensor read, the output format is: \nTouchpad num:[raw data]\n\n");

    while (1) {
        touch_pad_read_raw_data(TOUCH_PAD_NUM1, &touch_value);    // read raw data.
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void setup()
{
  Serial.begin (115200);

  touch_pad_init();
  touch_pad_config(TOUCH_PAD);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  /* Denoise setting at TouchSensor 0. */
  touch_pad_denoise_t denoise = {
      /* The bits to be cancelled are determined according to the noise level. */
      .grade = TOUCH_PAD_DENOISE_BIT4,
      .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
  };
  touch_pad_denoise_set_config(&denoise);
  touch_pad_denoise_enable();
  ESP_LOGI(TAG, "Denoise function init");

  /* Enable touch sensor clock. Work mode is "timer trigger". */
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_fsm_start();

  /* Start task to read values by pads. */
  xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
}

void loop()
{
  animate();
  FastLED.show();
}

uint8_t hue_to_int8(float hue)
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

  /* Range is about 29200...38000 */
  float touch_hue = clamp((touch_value - 29200) / 25.0, 0.0, 360.0);
  Serial.printf("Hue: [%4"PRIu32"] ", touch_hue);
  Serial.printf("\n");

  leds[0] = CHSV(hue_to_int8(touch_hue), 255, v);
}
