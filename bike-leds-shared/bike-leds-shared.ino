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
const uint8_t MODE_POT_PIN = A0;
const uint8_t PARAM1_POT_PIN = A1;
const uint8_t PARAM2_POT_PIN = A2;
const uint8_t BRIGHTNESS_POT_PIN = A3;
// Used to initialize pseudo random number generator.
const uint8_t RANDOM_ANALOG_PIN = A5;

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
// 14 front + 19 middle + 14 rear bottom = 47 pixels (LEDs)
const uint8_t PIXELS_PER_EFFECT = 47;


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
// How many sound frames should happen between updates to the beat level.
// 1500 fps / 60 fps = 25 frames
const uint8_t BEAT_LEVEL_UPDATE_FRAME_FRATE = 25; // Based on sound fps!

// Video FPS at which the program should run.
const uint16_t VIDEO_FRAMES_PER_SECOND = 60;
const float MILLIS_PER_VIDEO_FRAME =
  (float) 1000 / (float) VIDEO_FRAMES_PER_SECOND;
// 60 fps * 15s = 900 frames
const uint16_t FRAMES_PER_RANDOM_MODE = 900; // Based on video fps


// SOUND
// This will always be removed from the signal.
// Any signal lower than that value will be considered == 0.
const uint16_t SIGNAL_NOISE = 26;
// Beat signal peak will never be lower than this value.
// Increasing this value will make beat level less jupmy at low volume.
// Beat signal range is [0, (255 - SIGNAL_NOISE)] (max isn't guaranteed).
const uint16_t MIN_DECAYING_BEAT_SIGNAL_PEAK = 140;
// Beat level peak will never be lower than this value.
// Decreasing this value will make high precision beat detection more sensitive
// at low volume.
// Beat level range is [0, 510].
const uint16_t MIN_DECAYING_BEAT_LEVEL_PEAK = 160;


// OTHER
const uint16_t MAX_ANALOG_VALUE_COMMON = 1023;
const uint16_t MAX_ANALOG_VALUE_PARAM = 999;
// Used to define a custom first option on a pot.
const uint16_t CUSTOM_ANALOG_OPTION_1_THRESHOLD = 40;
// Used to define a custom second option on a pot.
const uint16_t CUSTOM_ANALOG_OPTION_2_THRESHOLD = 80;
const uint8_t MIN_BRIGHTNESS = 10;
const uint8_t MAX_BRIGHTNESS = 255;

// Strips to light up.
const uint8_t STRIPS_COUNT = 3;
Adafruit_NeoPixel strips[STRIPS_COUNT];

// Modes that can be used. Modes are methods that take no parameter and return a
// boolean. If the returned value is true, LED strips should be shown, otherwise
// nothing is done.
bool (*modes[11])();
uint8_t modesCount = sizeof(modes) / sizeof(int);

// Current values on the inputs, updated on each loop.
uint8_t mode;
uint16_t param1;
uint16_t param2;
uint8_t brightness;
// Don't use int8_t here since value range is:
//   [-MICROPHONE_DC_OFFSET, MICROPHONE_DC_OFFSET]
// Note that min and max aren't guaranteed.
// Used to calculate beat signal.
int16_t signal;

// Sound processing buffers and values.
//
// Records the highest value of the beat signal and decays it over time, so we
// always have an interesting sound graph whatever the beat signal we have.
// This is used to map the beat signal to the beat level. Useful to avoid having
// a very jumpy beat level at low volume.
// Minimum value is limited by MIN_DECAYING_BEAT_SIGNAL_PEAK.
// Maximum value should be MICROPHONE_DC_OFFSET but is not guaranteed.
uint16_t decayingBeatSignalPeak;
// Records the highest value of the beat level and decays it over time. This can
// be used to do more precise beat detection, e.g.:
//   beatLevel > 0.8 * decayingBeatLevelPeak
// Minimum value is limited by MIN_DECAYING_BEAT_LEVEL_PEAK.
// Maximum value is 510 (MICROPHONE_DC_OFFSET * 2 - 1).
uint16_t decayingBeatLevelPeak;
// Dampened reading of beat signal.
// Range: [0, 510] ([0, MICROPHONE_DC_OFFSET * 2 - 1])
uint16_t beatLevel;

elapsedMillis inputsFrameElapsedMillis;

// Sound frame that is currently being processed.
uint16_t soundFrameCounter;
uint16_t lastBeatLevelUpdateFrame;
elapsedMicros soundFrameElapsedMicros;

// Video frame that is currently being displayed.
uint16_t videoFrameCounter;
elapsedMillis videoFrameElapsedMillis;
// True on the frame where a mode change happens.
bool modeChanged;


// ----- SETUP

void setup() {
  // *INDENT-OFF*
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) {
      clock_prescale_set(clock_div_1);
    }
  #endif
  // *INDENT-ON*

  // *INDENT-OFF*
  #ifdef DEBUG
    // Open the serial port at the requested speed.
    Serial.begin(BAUDS_SPEED);
  #endif
  // *INDENT-ON*

  // Set ADC to 77khz, max for 10bit.
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);

  // Input pins.
  pinMode(MODE_POT_PIN, INPUT);
  pinMode(PARAM1_POT_PIN, INPUT);
  pinMode(PARAM2_POT_PIN, INPUT);
  pinMode(BRIGHTNESS_POT_PIN, INPUT);
  pinMode(MICROPHONE_PIN, INPUT);

  // Output pins.
  pinMode(CENTER_STRIP_PIN, OUTPUT);
  pinMode(SIDE_STRIP_1_PIN, OUTPUT);
  pinMode(SIDE_STRIP_2_PIN, OUTPUT);

  // Initialize random seed with an unconnected pin.
  randomSeed(analogRead(RANDOM_ANALOG_PIN));

  // All available modes. If you add a mode here, remember to increase the size
  // of the global array above.
  modes[0] = randomMode;
  modes[1] = vuMeterMode;
  modes[2] = colorWipeMode;
  modes[3] = rainbowMode;
  modes[4] = theaterChaseMode;
  modes[5] = fireMode;
  modes[6] = meteorMode;
  modes[7] = lightFlow2Mode;
  modes[8] = snowSparkleMode;
  modes[9] = gradualFillMode;
  modes[10] = bikeShapesMode;

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

  // Initialize beat level and beat peaks.
  beatLevel = 0;
  decayingBeatSignalPeak = MIN_DECAYING_BEAT_SIGNAL_PEAK;
  decayingBeatLevelPeak = MIN_DECAYING_BEAT_LEVEL_PEAK;

  // Initialize frame counters and timers.
  // First frame will be 0 as counter increases at beginning of each relevant
  // processing method.
  soundFrameCounter = UINT16_MAX;
  lastBeatLevelUpdateFrame = UINT16_MAX;
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
  static uint8_t modeBuf[bufSize];
  static uint16_t param1Buf[bufSize];
  static uint16_t param2Buf[bufSize];
  static uint8_t brightnessBuf[bufSize];

  mode = average(modeBuf,
                 bufSize,
                 (uint8_t) analogRead1000(MODE_POT_PIN, 0, modesCount - 1));
  param1 = average(param1Buf, bufSize, analogRead1000(
                     PARAM1_POT_PIN, 0, MAX_ANALOG_VALUE_PARAM));
  param2 = average(param2Buf, bufSize, analogRead1000(
                     PARAM2_POT_PIN, 0, MAX_ANALOG_VALUE_PARAM));

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
  increaseFrameCounter(&soundFrameCounter, 1);
  soundFrameElapsedMicros -= MICROS_PER_SOUND_FRAME;

  // If true, frame took more than allowed time to render.
  // Let's skip frames until we're in time again.
  if (soundFrameElapsedMicros > MICROS_PER_SOUND_FRAME) {
    uint16_t toSkip =
      (float) soundFrameElapsedMicros / MICROS_PER_SOUND_FRAME;
    increaseFrameCounter(&soundFrameCounter, toSkip);
    soundFrameElapsedMicros =
      soundFrameElapsedMicros % (uint32_t) MICROS_PER_SOUND_FRAME;
  }

  // Raw analog read for microphone.
  signal = analogRead(MICROPHONE_PIN) - MICROPHONE_DC_OFFSET;
  if (abs(signal) < SIGNAL_NOISE) {
    signal = 0;
  } else if (signal < 0) {
    signal += SIGNAL_NOISE;
  } else {
    signal -= SIGNAL_NOISE;
  }

  beatLevelSoundProcessing();
}

void beatLevelSoundProcessing() {
  // Always update beat detection envelope.
  // Filter only beat and bass component
  float beatAndBassSignal = butter1500_bandpass_20hz_200hz(signal);
  // Take signal amplitude
  beatAndBassSignal = abs(beatAndBassSignal);

  // Low pass the bass to filter them out and only keep the beats amplitude.
  uint16_t beatSignal = abs(butter1500_lowpass_10hz(beatAndBassSignal));

  if (abs(soundFrameCounter - lastBeatLevelUpdateFrame)
      >= BEAT_LEVEL_UPDATE_FRAME_FRATE) {
    // Lower decaying beat signal peak. If signal too loud, simply update peak.
    if (beatSignal >= decayingBeatSignalPeak) {
      decayingBeatSignalPeak = beatSignal;
    } else if (decayingBeatSignalPeak > MIN_DECAYING_BEAT_SIGNAL_PEAK) {
      decayingBeatSignalPeak -= 1;
    }

    beatSignal = map(beatSignal,
                     0,
                     decayingBeatSignalPeak,
                     0,
                     MICROPHONE_DC_OFFSET * 2 - 1);
    // Dampen the envelope too.
    // Note that this somewhat prevents the vuMeter to reach its max.
    beatLevel = ((beatLevel * 3) + (uint16_t) beatSignal) >> 2;

    // Lower decaying beat level peak. If level too loud, simply update peak.
    if (beatLevel >= decayingBeatLevelPeak) {
      decayingBeatLevelPeak = beatLevel;
    } else if (decayingBeatLevelPeak > MIN_DECAYING_BEAT_LEVEL_PEAK) {
      decayingBeatLevelPeak -= 2;
    }

    lastBeatLevelUpdateFrame = soundFrameCounter;
  }
}

void videoProcessing() {
  static uint8_t currentMode = 0;

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

  if (mode != currentMode) {
    modeChanged = true;
    currentMode = mode;

    clearStrips();
  }

  bool mustShow = (*modes[mode])();

  if (mustShow) {
    if (modeChanged) {
      modeChanged = false;
    }
    // Show all strips.
    for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
      strips[s].show();
    }
  }
}
