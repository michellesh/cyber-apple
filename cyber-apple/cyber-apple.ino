#include <FastLED.h>

#define DATA_PIN 4
#define LED_TYPE NEOPIXEL
#define COLOR_ORDER GRB
#define NUM_LEDS 51
#define BRIGHTNESS 255

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  delay(500);

  FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  FastLED.clear();

  static float index = 0;

  float headLength = 1;
  float tailLength = 10;

  for (int i = 0; i < NUM_LEDS; i++) {
    float dist = i - index;
    if (dist > 0 && dist < headLength) {
      // ahead of the drip
      int brightness = mapf(dist, 0, headLength, 255, 0);
      leds[i] = CHSV(HUE_RED, 255, brightness);
    } else if (dist < 0 && abs(dist) < tailLength) {
      // tail behind the drip
      int brightness = mapf(abs(dist), 0, tailLength, 255, 0);
      leds[i] = CHSV(HUE_RED, 255, brightness);
    } else {
      leds[i] = CRGB::Black;
    }
  }

  index += 0.02;
  if (index > NUM_LEDS) {
    index = 0;
  }

  FastLED.show();
}

float mapf(float value, float inMin, float inMax, float outMin, float outMax) {
  float percentage = (value - inMin) / (inMax - inMin);
  return outMin + (outMax - outMin) * percentage;
}
