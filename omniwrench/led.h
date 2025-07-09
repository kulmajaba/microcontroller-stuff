#include <vector>
#include <FastLED.h>

struct led_group {
    const std::vector<int> led_indexes;
    const int animated_leds;
    const int leds_start_index;
    int fx_freq;
    int fx_phase;
    int fx_initial_phase;
};