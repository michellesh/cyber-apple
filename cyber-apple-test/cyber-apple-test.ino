#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>

#define PIN_1 2
#define PIN_2 4
#define PIN_3 7
#define PIN_4 8
#define PIN_5 6
#define PIN_6 5
#define PIN_STEM 3

#define LED_TYPE NEOPIXEL
#define COLOR_ORDER GRB
#define BRIGHTNESS 180

#define NUM_LEDS_STRIP 177
#define NUM_LEDS_STEM 31

#define DRIP_STICK_INDEX 65
#define DRIP_STICK_LENGTH 36

CRGB leds1[NUM_LEDS_STRIP];
CRGB leds2[NUM_LEDS_STRIP];
CRGB leds3[NUM_LEDS_STRIP];
CRGB leds4[NUM_LEDS_STRIP];
CRGB leds5[NUM_LEDS_STRIP];
CRGB leds6[NUM_LEDS_STRIP];
CRGB ledsStem[NUM_LEDS_STRIP];

void setup() {
  Serial.begin(115200);
  delay(500);

  FastLED.addLeds<LED_TYPE, PIN_1>(leds1, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_2>(leds2, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_3>(leds3, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_4>(leds4, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_5>(leds5, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_6>(leds6, NUM_LEDS_STRIP);
  FastLED.addLeds<LED_TYPE, PIN_STEM>(ledsStem, NUM_LEDS_STRIP);

  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  FastLED.clear();

  for (int i = 0; i < NUM_LEDS_STRIP; i++) {
    CRGB color = CRGB::Red;
    if (i >= DRIP_STICK_INDEX && i < DRIP_STICK_INDEX + DRIP_STICK_LENGTH) {
      color = CRGB::Black;
    }
    leds1[i] = color;
    leds2[i] = color;
    leds3[i] = color;
    if (i > 0) {
      leds4[i - 1] = color;
    }
    leds5[i] = color;
    leds6[i] = color;
  }

  for (int i = 0; i < NUM_LEDS_STEM; i++) {
    ledsStem[i] = CRGB::SaddleBrown;
  }

  FastLED.show();
}
