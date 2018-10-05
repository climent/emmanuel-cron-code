////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Set to true to enable debugging mode.
#define DEBUG false
// Set to true to reverse all effects.
#define REVERSE_ALL_EFFECTS false
#define BAUDS_SPEED 1000000

#include <Adafruit_NeoPixel.h>
// *INDENT-OFF*
#ifdef __AVR__
 #include <avr/power.h>
#endif
// *INDENT-ON*
#include <elapsedMillis.h>
#include <float.h>

// *INDENT-OFF*
#ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
// *INDENT-ON*


// ANALOG
// Potentiometers
const uint8_t BRIGHTNESS_POT_PIN = A3;

// Microphone
const uint8_t MICROPHONE_PIN = A4;
const uint16_t MICROPHONE_DC_OFFSET = 255;


// DIGITAL
// LED strips
// Strip 1 is middle strip (both sides of the bike)
const uint8_t CENTER_STRIP_PIN = 9;
// Strips 2 and 3 are on each sides of the bike.
const uint8_t SIDE_STRIP_1_PIN = 5;
const uint8_t SIDE_STRIP_2_PIN = 6;
const uint8_t LEDS_PER_STRIP_COMMON = 60;


// FRAMES
// Inputs FPS at which the program should run.
// This includes all inputs except the microphone.
const uint16_t INPUTS_FRAMES_PER_SECOND = 30;
const float MILLIS_PER_INPUTS_FRAME =
  (float) 1000 / (float) INPUTS_FRAMES_PER_SECOND;

// Sound FPS at which the program should run.
const uint16_t SOUND_FRAMES_PER_SECOND = 1500;
const float MICROS_PER_SOUND_FRAME =
  (float) 1000000 / (float) SOUND_FRAMES_PER_SECOND;

// Video FPS at which the program should run.
const uint16_t VIDEO_FRAMES_PER_SECOND = 60;
const float MILLIS_PER_VIDEO_FRAME =
  (float) 1000 / (float) VIDEO_FRAMES_PER_SECOND;


// OTHER
const uint8_t MIN_BRIGHTNESS = 10;
const uint8_t MAX_BRIGHTNESS = 255;

// Strips to light up.
const uint8_t STRIPS_COUNT = 3;
Adafruit_NeoPixel strips[STRIPS_COUNT];

// Current values on the inputs, updated on each loop.
uint8_t brightness;
// Don't use int8_t here since value range is:
//   [-MICROPHONE_DC_OFFSET, MICROPHONE_DC_OFFSET]
// Note that min and max aren't guaranteed.
// Used to calculate beat signal.
int16_t signal;
uint16_t absSignal;

elapsedMillis inputsFrameElapsedMillis;

// Sound frame that is currently being processed.
elapsedMicros soundFrameElapsedMicros;

// Video frame that is currently being displayed.
uint16_t videoFrameCounter;
elapsedMillis videoFrameElapsedMillis;


// ----- SETUP

void setup() {
  // *INDENT-OFF*
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) {
      clock_prescale_set(clock_div_1);
    }
  #endif
  // *INDENT-ON*

  // Set ADC to 77khz, max for 10bit.
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);

  // Input pins.
  pinMode(BRIGHTNESS_POT_PIN, INPUT);
  pinMode(MICROPHONE_PIN, INPUT);

  // Output pins.
  pinMode(CENTER_STRIP_PIN, OUTPUT);
  pinMode(SIDE_STRIP_1_PIN, OUTPUT);
  pinMode(SIDE_STRIP_2_PIN, OUTPUT);

  strips[0].setPin(CENTER_STRIP_PIN);
  strips[1].setPin(SIDE_STRIP_1_PIN);
  strips[2].setPin(SIDE_STRIP_2_PIN);

  for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
    // Number of pixels in strip.
    strips[s].updateLength(LEDS_PER_STRIP_COMMON);
    // Pixel type flags, add together as needed:
    //  NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //  NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //  NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //  NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    //  NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
    strips[s].updateType(NEO_GRB + NEO_KHZ800);

    strips[s].begin();
    // Use max brightness by default. Use scaledColor() to dim LEDs with the
    // current brightness.
    strips[s].setBrightness(MAX_BRIGHTNESS);
    // Initialize all pixels to 'off'.
    strips[s].show();
  }

  // Initialize frame counters and timers.
  // First frame will be 0 as counter increases at beginning of each relevant
  // processing method.
  videoFrameCounter = UINT16_MAX;
  // "Start" the frame timer (= reset to 0).
  // Don't reset inputs timer as we want to read them ASAP.
  soundFrameElapsedMicros = 0;
  videoFrameElapsedMillis = 0;
}


// ----- LOOP

void loop() {
  if (inputsFrameElapsedMillis > MILLIS_PER_INPUTS_FRAME) {
    inputsProcessing();
  }

  if (soundFrameElapsedMicros > MICROS_PER_SOUND_FRAME) {
    soundProcessing();
  }

  if (videoFrameElapsedMillis > MILLIS_PER_VIDEO_FRAME) {
    videoProcessing();
  }
}

void inputsProcessing() {
  // Using n readings to average the value over a short period of time.
  // 5 * 1/30 ~= 166.6ms
  const static uint8_t bufSize = 5;
  static uint8_t brightnessBuf[bufSize];

  uint8_t brightnessRead = (uint8_t) analogRead1000(
                             BRIGHTNESS_POT_PIN,
                             MIN_BRIGHTNESS,
                             MAX_BRIGHTNESS);
  // Smooth it down even more.
  brightnessRead -= brightnessRead % 2;
  brightness = average(brightnessBuf, bufSize, brightnessRead);

  // We're not counting frames, so simply reset the timer and wait until the
  // next trigger.
  inputsFrameElapsedMillis = 0;
}

void soundProcessing() {
  soundFrameElapsedMicros =
    soundFrameElapsedMicros % (uint32_t) MICROS_PER_SOUND_FRAME;

  // Raw analog read for microphone.
  signal = analogRead(MICROPHONE_PIN) - MICROPHONE_DC_OFFSET;
  absSignal = abs(signal);
}

void videoProcessing() {
  increaseFrameCounter(&videoFrameCounter, 1);
  videoFrameElapsedMillis -= MILLIS_PER_VIDEO_FRAME;

  // If true, frame took more than allowed time to render.
  // Let's skip frames until we're in time again.
  if (videoFrameElapsedMillis > MILLIS_PER_VIDEO_FRAME) {
    uint16_t toSkip =
      (float) videoFrameElapsedMillis / MILLIS_PER_VIDEO_FRAME;
    increaseFrameCounter(&videoFrameCounter, toSkip);
    videoFrameElapsedMillis =
      videoFrameElapsedMillis % (uint16_t) MILLIS_PER_VIDEO_FRAME;
  }

  bool mustShow = noiseDisplayMode();

  if (mustShow) {
    // Show all strips.
    for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
      strips[s].show();
    }
  }
}
