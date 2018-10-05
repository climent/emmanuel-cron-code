////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// ----- MODES

bool randomMode() {
  static uint16_t lastModeChangeFrame;
  static uint8_t currentRandomMode = 0;

  if (currentRandomMode == 0
      || (abs(videoFrameCounter - lastModeChangeFrame)
          > FRAMES_PER_RANDOM_MODE)) {
    lastModeChangeFrame = videoFrameCounter;
    uint8_t newMode = currentRandomMode;
    while (newMode == currentRandomMode) {
      // Ignore random mode itself.
      newMode = random(1, modesCount);
    }

    modeChanged = true;
    currentRandomMode = newMode;
    clearStrips();
  }
  return (*modes[currentRandomMode])();
}

bool vuMeterMode() {
  return vuMeterEffect(beatLevel);
}

const uint16_t COLOR_WIPE_MIN_SPEED_IN_FRAMES = 1;
const uint16_t COLOR_WIPE_MAX_SPEED_IN_FRAMES = 10;
bool colorWipeMode() {
  uint16_t speedInFrames = getValueInRangePotentiallyMappedByBeatReverse(
                             param2,
                             COLOR_WIPE_MIN_SPEED_IN_FRAMES,
                             COLOR_WIPE_MAX_SPEED_IN_FRAMES);

  uint32_t color;
  if (param1 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    color = scaledVuMeter(beatLevel / 2);
  } else {
    uint16_t mappedParam1 = map(param1,
                                CUSTOM_ANALOG_OPTION_1_THRESHOLD,
                                MAX_ANALOG_VALUE_PARAM,
                                0,
                                255);
    color = scaledWheel(mappedParam1);
  }

  return colorWipeEffect(color, speedInFrames);
}

const uint16_t RAINBOW_MIN_SPEED_IN_FRAMES = 1;
const uint16_t RAINBOW_MAX_SPEED_IN_FRAMES = 4;
bool rainbowMode() {
  uint16_t speedInFrames = map(param2,
                               0,
                               MAX_ANALOG_VALUE_PARAM,
                               RAINBOW_MIN_SPEED_IN_FRAMES,
                               RAINBOW_MAX_SPEED_IN_FRAMES);

  if (param1 < MAX_ANALOG_VALUE_PARAM / 2) {
    return rainbowCycleEffect(speedInFrames);
  } else {
    return rainbowEffect(speedInFrames);
  }
}

const uint16_t THEATER_CHASE_MIN_SPEED_IN_FRAMES = 3;
const uint16_t THEATER_CHASE_MAX_SPEED_IN_FRAMES = 15;
bool theaterChaseMode() {
  uint16_t speedInFrames = getValueInRangePotentiallyMappedByBeatReverse(
                             param2,
                             THEATER_CHASE_MIN_SPEED_IN_FRAMES,
                             THEATER_CHASE_MAX_SPEED_IN_FRAMES);

  uint32_t color;
  if (param1 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    color = scaledWheel(beatLevel / 2);
  } else if (param1 < CUSTOM_ANALOG_OPTION_2_THRESHOLD) {
    return theaterChaseRainbowEffect(speedInFrames);
  } else {
    uint16_t mappedParam1 = map(param1,
                                CUSTOM_ANALOG_OPTION_2_THRESHOLD,
                                MAX_ANALOG_VALUE_PARAM,
                                0,
                                255);
    color = scaledWheel(mappedParam1);
  }
  return theaterChaseEffect(color, speedInFrames);
}

const uint16_t FIRE_SPARKING_MIN_VALUE = 20;
const uint16_t FIRE_SPARKING_MAX_VALUE = 200;
bool fireMode() {
  uint16_t sparking = getValueInRangePotentiallyMappedByBeat(
                        param2,
                        FIRE_SPARKING_MIN_VALUE,
                        FIRE_SPARKING_MAX_VALUE);
  return fireEffect(50 /* cooling: 20-100 */, sparking /* 50-200 */);
}

bool meteorMode() {
  uint32_t color;
  if (param1 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    color = scaledWheel(beatLevel / 2);
  } else {
    uint16_t mappedParam1 = map(param1,
                                CUSTOM_ANALOG_OPTION_1_THRESHOLD,
                                MAX_ANALOG_VALUE_PARAM,
                                0,
                                255);
    color = scaledWheel(mappedParam1);
  }

  return meteorEffect(color, 10, 64, true);
}

const uint16_t LIGHT_FLOW_2_MIN_SPEED_IN_FRAMES = 1;
const uint16_t LIGHT_FLOW_2_MAX_SPEED_IN_FRAMES = 4;
bool lightFlow2Mode() {
  uint16_t speedInFrames = getValueInRangePotentiallyMappedByBeatReverse(
                             param2,
                             LIGHT_FLOW_2_MIN_SPEED_IN_FRAMES,
                             LIGHT_FLOW_2_MAX_SPEED_IN_FRAMES);
  return lightFlow2Effect(speedInFrames);
}

const uint8_t SNOW_SPARKLE_LEDS_TO_SPARK = 3; // 1-10
const uint16_t SNOW_SPARKLE_MIN_RAND_FRAMES_BETWEEN_SPARKLES = 12;
const uint16_t SNOW_SPARKLE_MAX_RAND_FRAMES_BETWEEN_SPARKLES = 60;
const float SNOW_SPARKLE_BEAT_LEVEL_THRESHOLD_RATIO = 0.6; // orig: 100
bool snowSparkleMode() {
  static uint16_t lastSparkleFrame = 0;
  static uint8_t framesBetweenSparkles = 0;

  uint32_t unscaledColor;
  if (param1 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    unscaledColor = Adafruit_NeoPixel::Color(255, 255, 255);
  } else {
    uint8_t wheelColor = map(
                           param1,
                           CUSTOM_ANALOG_OPTION_1_THRESHOLD,
                           MAX_ANALOG_VALUE_PARAM,
                           0,
                           255);
    unscaledColor = wheel(wheelColor);
  }

  if (param2 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    return snowSparkleEffect(
             SNOW_SPARKLE_LEDS_TO_SPARK,
             beatLevel >= (decayingBeatLevelPeak *
                           SNOW_SPARKLE_BEAT_LEVEL_THRESHOLD_RATIO),
             unscaledColor);
  } else {
    bool sparkle = false;
    if (abs(videoFrameCounter - lastSparkleFrame) >= framesBetweenSparkles) {
      sparkle = true;
      lastSparkleFrame = videoFrameCounter;
      if (param2 < CUSTOM_ANALOG_OPTION_2_THRESHOLD) {
        framesBetweenSparkles =
          random(
            SNOW_SPARKLE_MIN_RAND_FRAMES_BETWEEN_SPARKLES,
            SNOW_SPARKLE_MAX_RAND_FRAMES_BETWEEN_SPARKLES + 1);
      } else {
        framesBetweenSparkles =
          map(param2,
              CUSTOM_ANALOG_OPTION_2_THRESHOLD,
              MAX_ANALOG_VALUE_PARAM,
              SNOW_SPARKLE_MIN_RAND_FRAMES_BETWEEN_SPARKLES,
              SNOW_SPARKLE_MAX_RAND_FRAMES_BETWEEN_SPARKLES);
      }
    }
    return snowSparkleEffect(SNOW_SPARKLE_LEDS_TO_SPARK,
                             sparkle,
                             unscaledColor);
  }
}

const uint16_t GRADUAL_FILL_MIN_FILL_IN_FRAMES = 8;
const uint16_t GRADUAL_FILL_MAX_FILL_IN_FRAMES = 15;
const float GRADUAL_FILL_BEAT_LEVEL_THRESHOLD_RATIO = 0.5; // orig: 170
bool gradualFillMode() {
  bool fill = false;
  uint8_t fillInFrames = 0;
  if (param2 < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    if (beatLevel >= (decayingBeatLevelPeak *
                      GRADUAL_FILL_BEAT_LEVEL_THRESHOLD_RATIO)) {
      fill = true;
    }
  } else {
    fillInFrames = map(param2,
                       CUSTOM_ANALOG_OPTION_1_THRESHOLD,
                       MAX_ANALOG_VALUE_PARAM,
                       GRADUAL_FILL_MIN_FILL_IN_FRAMES,
                       GRADUAL_FILL_MAX_FILL_IN_FRAMES);

  }
  return gradualFillEffect(fill, fillInFrames);
}
