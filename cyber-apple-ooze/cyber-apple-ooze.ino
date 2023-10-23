// Original code from Adafruit.
// Modified for FastLED with an ESP8266.
// https://learn.adafruit.com/ooze-master-3000-neopixel-simulated-liquid-physics
//
// -------
//
// SPDX-FileCopyrightText: 2019 Phillip Burgess for Adafruit Industries
//
// SPDX-License-Identifier: MIT

// OOZE MASTER 3000: NeoPixel simulated liquid physics. Up to 7 NeoPixel
// strands dribble light, while an 8th strand "catches the drips."
// Designed for the Adafruit Feather M0 or M4 with matching version of
// NeoPXL8 FeatherWing, or for RP2040 boards including SCORPIO. This can be
// adapted for other M0, M4, RP2040 or ESP32-S3 boards but you will need to
// do your own "pin sudoku" & level shifting (e.g. NeoPXL8 Friend breakout).
// See here: https://learn.adafruit.com/adafruit-neopxl8-featherwing-and-library
// Requires Adafruit_NeoPixel, Adafruit_NeoPXL8 and Adafruit_ZeroDMA libraries.

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
#define BRIGHTNESS 200

#define NUM_LEDS 177
#define NUM_LEDS_STEM 31
#define NUM_STRANDS 6

#define DRIP_STICK_INDEX 65
#define DRIP_STICK_LENGTH 13 // 36

#define NUM_DRIPS 6

#define PIXEL_PITCH (1.0 / 150.0) // 150 pixels/m
#define GAMMA 2.6                 // For linear brightness correction
#define G_CONST 9.806             // Standard acceleration due to gravity
// While the above G_CONST is correct for "real time" drips, you can dial it
// back for a more theatric effect / to slow the drips like they've still got
// a syrupy "drool string" attached (try much lower values like 2.0 to 3.0).

CRGB *leds;
CRGB ledsStem[NUM_LEDS_STEM];

float mapf(float value, float inMin, float inMax, float outMin, float outMax) {
  float percentage = (value - inMin) / (inMax - inMin);
  return outMin + (outMax - outMin) * percentage;
}

typedef enum {
  MODE_IDLE,
  MODE_OOZING,
  MODE_DRIBBLING_1,
  MODE_DRIBBLING_2,
  MODE_DRIPPING
} dropState;

uint8_t palette[][3] = {
    {255, 0, 0}, // Red
};
#define NUM_COLORS (sizeof palette / sizeof palette[0])

struct Drip {
  uint16_t startPixel;
  uint16_t length;            // Length of NeoPixel strip IN PIXELS
  uint16_t dribblePixel;      // Index of pixel where dribble pauses before drop (0
                              // to length-1)
  float height;               // Height IN METERS of dribblePixel above ground
  uint16_t palette_min;       // Lower color palette index for this strip
  uint16_t palette_max;       // Upper color palette index for this strip
  dropState mode;             // One of the above states (MODE_IDLE, etc.)
  uint32_t eventStartUsec;    // Starting time of current event
  uint32_t eventDurationUsec; // Duration of current event, in microseconds
  float eventDurationReal;    // Duration of current event, in seconds (float)
  uint32_t splatStartUsec;    // Starting time of most recent "splat"
  uint32_t splatDurationUsec; // Fade duration of splat
  float pos;                  // Position of drip on prior frame
  uint8_t color[3];           // RGB color (randomly picked from from palette[])
  uint8_t splatColor[3];      // RGB color of "splat" (may be from prior drip)
};

Drip drips[] = {
    // THIS TABLE CONTAINS INFO FOR UP TO 8 NEOPIXEL DRIPS
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
    {50, 15 + DRIP_STICK_LENGTH, 15, 2, 0, 0},
};

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

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

  for (int i = 0; i < NUM_DRIPS; i++) {
    drips[i].mode = MODE_IDLE; // Start all drips in idle mode
    drips[i].eventStartUsec = 0;
    drips[i].eventDurationUsec = random(500000, 2500000); // Initial idle 0.5-2.5 sec
    drips[i].eventDurationReal = (float)drips[i].eventDurationUsec / 1000000.0;
    drips[i].splatStartUsec = 0;
    drips[i].splatDurationUsec = 0;

    // Randomize initial color:
    memcpy(drips[i].color, palette[random(drips[i].palette_min, drips[i].palette_max + 1)],
           sizeof palette[0]);
    memcpy(drips[i].splatColor, drips[i].color, sizeof palette[0]);
  }
}

void loop() {
  uint32_t t = micros(); // Current time, in microseconds

  float x = 0.0; // multipurpose interim result

  setBackground();

  for (int i = 0; i < NUM_DRIPS; i++) {
    uint32_t dtUsec = t - drips[i].eventStartUsec; // Elapsed time, in microseconds, since
                                                   // start of current event
    float dtReal = (float)dtUsec / 1000000.0;      // Elapsed time, in seconds

    // Handle transitions between drip states (oozing, dribbling, dripping,
    // etc.)
    if (dtUsec >= drips[i].eventDurationUsec) {              // Are we past end of current event?
      drips[i].eventStartUsec += drips[i].eventDurationUsec; // Yes, next event starts here
      dtUsec -= drips[i].eventDurationUsec; // We're already this far into next event
      dtReal = (float)dtUsec / 1000000.0;
      switch (drips[i].mode) { // Current mode...about to switch to next mode...
      case MODE_IDLE:
        drips[i].mode = MODE_OOZING;                          // Idle to oozing transition
        drips[i].eventDurationUsec = random(800000, 1200000); // 0.8 to 1.2 sec ooze
        drips[i].eventDurationReal = (float)drips[i].eventDurationUsec / 1000000.0;
        // Randomize next drip color from palette settings:
        memcpy(drips[i].color, palette[random(drips[i].palette_min, drips[i].palette_max + 1)],
               sizeof palette[0]);
        break;
      case MODE_OOZING:
        if (drips[i].dribblePixel) {        // If dribblePixel is nonzero...
          drips[i].mode = MODE_DRIBBLING_1; // Oozing to dribbling transition
          drips[i].pos = (float)drips[i].dribblePixel;
          drips[i].eventDurationUsec = 250000 + drips[i].dribblePixel * random(30000, 40000);
          drips[i].eventDurationReal = (float)drips[i].eventDurationUsec / 1000000.0;
        } else {                                       // No dribblePixel...
          drips[i].pos = (float)drips[i].dribblePixel; // Oozing to dripping transition
          drips[i].mode = MODE_DRIPPING;
          drips[i].eventDurationReal = sqrt(drips[i].height * 2.0 / G_CONST); // SCIENCE
          drips[i].eventDurationUsec = (uint32_t)(drips[i].eventDurationReal * 1000000.0);
        }
        break;
      case MODE_DRIBBLING_1:
        drips[i].mode = MODE_DRIBBLING_2; // Dripping 1st half to 2nd half transition
        drips[i].eventDurationUsec =
            drips[i].eventDurationUsec * 3 / 2; // Second half is 1/3 slower
        drips[i].eventDurationReal = (float)drips[i].eventDurationUsec / 1000000.0;
        break;
      case MODE_DRIBBLING_2:
        drips[i].mode = MODE_DRIPPING; // Dribbling 2nd half to dripping transition
        drips[i].pos = (float)drips[i].dribblePixel;
        drips[i].eventDurationReal = sqrt(drips[i].height * 2.0 / G_CONST); // SCIENCE
        drips[i].eventDurationUsec = (uint32_t)(drips[i].eventDurationReal * 1000000.0);
        break;
      case MODE_DRIPPING:
        drips[i].mode = MODE_IDLE;                            // Dripping to idle transition
        drips[i].eventDurationUsec = random(500000, 1200000); // Idle for 0.5 to 1.2 seconds
        drips[i].eventDurationReal = (float)drips[i].eventDurationUsec / 1000000.0;
        drips[i].splatStartUsec = drips[i].eventStartUsec; // Splat starts now!
        drips[i].splatDurationUsec = random(900000, 1100000);
        memcpy(drips[i].splatColor, drips[i].color,
               sizeof palette[0]); // Save color for splat
        break;
      }
    }

    // Render drip state to NeoPixels...
    switch (drips[i].mode) {
    case MODE_IDLE:
      // Do nothing
      break;
    case MODE_OOZING:
      x = dtReal / drips[i].eventDurationReal; // 0.0 to 1.0 over ooze interval
      x = sqrt(x);                             // Perceived area increases linearly
      set(i, i, 0, x);
      break;
    case MODE_DRIBBLING_1:
      // Point b moves from first to second pixel over event time
      x = dtReal / drips[i].eventDurationReal; // 0.0 to 1.0 during move
      x = 3 * x * x - 2 * x * x * x;           // Easing function: 3*x^2-2*x^3 0.0 to 1.0
      dripDraw(i, 0.0, x * drips[i].dribblePixel, false);
      break;
    case MODE_DRIBBLING_2:
      // Point a moves from first to second pixel over event time
      x = dtReal / drips[i].eventDurationReal; // 0.0 to 1.0 during move
      x = 3 * x * x - 2 * x * x * x;           // Easing function: 3*x^2-2*x^3 0.0 to 1.0
      dripDraw(i, x * drips[i].dribblePixel, drips[i].dribblePixel, false);
      break;
    case MODE_DRIPPING:
      x = 0.5 * G_CONST * dtReal * dtReal;         // Position in meters
      x = drips[i].dribblePixel + x / PIXEL_PITCH; // Position in pixels
      dripDraw(i, drips[i].pos, x, true);
      drips[i].pos = x;
      break;
    }

    if (NUM_DRIPS < 8) {                    // Do splats unless there's an 8th drip defined
      dtUsec = t - drips[i].splatStartUsec; // Elapsed time, in microseconds,
                                            // since start of splat
      if (dtUsec < drips[i].splatDurationUsec) {
        x = 1.0 - sqrt((float)dtUsec / (float)drips[i].splatDurationUsec);
        // set(7, i, splatmap[i], x);
      }
    }
  }

  FastLED.show();
}

void setBackground() {
  // Set the background to super dim red
  FastLED.clear();
  for (int i = 0; i < NUM_STRANDS; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      leds[i * NUM_LEDS + j] = CRGB(5, 0, 0);
    }
  }
}

// This "draws" a drip in the NeoPixel buffer...zero to peak brightness
// at center and back to zero. Peak brightness diminishes with length,
// and drawn dimmer as pixels approach strand length.
void dripDraw(uint8_t dNum, float a, float b, bool fade) {
  if (a > b) { // Sort a,b inputs if needed so a<=b
    float t = a;
    a = b;
    b = t;
  }
  // Find range of pixels to draw. If first pixel is off end of strand,
  // nothing to draw. If last pixel is off end of strand, clip to strand length.
  int firstPixel = (int)a;
  if (firstPixel >= drips[dNum].length)
    return;
  int lastPixel = (int)b + 1;
  if (lastPixel >= drips[dNum].length)
    lastPixel = drips[dNum].length - 1;

  float center = (a + b) * 0.5;   // Midpoint of a-to-b
  float range = center - a + 1.0; // Distance from center to a, plus 1 pixel
  for (int i = firstPixel; i <= lastPixel; i++) {
    float x = fabs(center - (float)i); // Pixel distance from center point
    if (x < range) {                   // Inside drip
      x = (range - x) / range;         // 0.0 (edge) to 1.0 (center)
      if (fade) {
        int dLen = drips[dNum].length - drips[dNum].dribblePixel; // Length of drip
        if (dLen > 0) { // Scale x by 1.0 at top to 1/3 at bottom of drip
          int dPixel = i - drips[dNum].dribblePixel; // Pixel position along drip
          x *= 1.0 - ((float)dPixel / (float)dLen * 0.66);
        }
      }
    } else {
      x = 0.0;
    }
    set(dNum, dNum, drips[dNum].startPixel + i, x);
  }
}

// Set one pixel to a given brightness level (0.0 to 1.0).
// Strand # and drip # are BOTH passed in because "splats" are always
// on strand 7 but colors come from drip indices.
void set(uint8_t strand, uint8_t d, uint8_t pixel, float brightness) {
  leds[strand * NUM_LEDS + pixel] = CRGB((int)((float)drips[d].color[0] * brightness + 0.5),
                                         (int)((float)drips[d].color[1] * brightness + 0.5),
                                         (int)((float)drips[d].color[2] * brightness + 0.5));
}
