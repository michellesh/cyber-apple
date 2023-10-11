#include <FastLED.h>

#define DATA_PIN 4
#define LED_TYPE NEOPIXEL
#define COLOR_ORDER GRB
#define BRIGHTNESS 255

#define NUM_LEDS 51
#define NUM_DRIPS 3

CRGB leds[NUM_LEDS];

float mapf(float value, float inMin, float inMax, float outMin, float outMax) {
  float percentage = (value - inMin) / (inMax - inMin);
  return outMin + (outMax - outMin) * percentage;
}

struct Drip {
  float headLength;
  float tailLength;
  float index;

  void show() {
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
        //leds[i] = CRGB::Black;
      }
    }
  }

  void update() {
    index += 0.02;
    if (index > NUM_LEDS) {
      index = 0;
    }
  }
};

Drip drips[NUM_DRIPS];

void setup() {
  Serial.begin(115200);
  delay(500);

  FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  for (int i = 0; i < NUM_DRIPS; i++) {
    Drip d = {1, 10, i * 10 + 2};
    drips[i] = d;
  }
}

void loop() {
  FastLED.clear();

  for (int i = 0; i < NUM_DRIPS; i++) {
    drips[i].show();
    drips[i].update();
  }

  FastLED.show();
}
