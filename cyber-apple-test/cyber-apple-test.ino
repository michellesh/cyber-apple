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
#define BRIGHTNESS 255

#define NUM_LEDS 177
#define NUM_LEDS_STEM 31
#define NUM_STRANDS 6

#define DRIP_STICK_INDEX 65
#define DRIP_STICK_LENGTH 18 // 36

CRGB *leds;
CRGB ledsStem[NUM_LEDS_STEM];

CRGB colors[] = {
    CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple,
};

void setup() {
  Serial.begin(115200);
  delay(500);

  leds = new CRGB[NUM_STRANDS * NUM_LEDS];

  FastLED.addLeds<LED_TYPE, PIN_1>(leds, 0, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_2>(leds, NUM_LEDS, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_3>(leds, NUM_LEDS * 2, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_4>(leds, NUM_LEDS * 3 + 1,
                                   NUM_LEDS); // strand 4 is shorter by 1 pixel
  FastLED.addLeds<LED_TYPE, PIN_5>(leds, NUM_LEDS * 4, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_6>(leds, NUM_LEDS * 5, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_STEM>(ledsStem, NUM_LEDS_STEM);

  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  FastLED.clear();

  for (int i = 0; i < NUM_STRANDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      bool dripStick = j >= DRIP_STICK_INDEX && j < DRIP_STICK_INDEX + DRIP_STICK_LENGTH * 2;
      leds[i * NUM_LEDS + j] = dripStick ? CRGB::Black : colors[i];
    }
  }

  for (int i = 0; i < NUM_LEDS_STEM; i++) {
    ledsStem[i] = CRGB::SaddleBrown;
  }

  FastLED.show();
}
