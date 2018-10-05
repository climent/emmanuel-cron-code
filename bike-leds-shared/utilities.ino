////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// ----- READ UTILITIES

uint16_t analogRead1000(uint8_t pin, uint16_t minValue, uint16_t maxValue) {
  // Analog values are normally from 0-1023, but potentiometers are not...
  // the most reliable values. This method reads values from 10 to 1000 (a
  // range of 1,000 values) and maps them to the given min and max values.
  uint16_t value = map(analogRead(pin), 0, 1000, minValue, maxValue);
  return constrain(value, minValue, maxValue);
}

uint8_t average(uint8_t buffer[], uint8_t count, uint8_t lastValue) {
  uint16_t total = 0;
  for (uint8_t b = 0; b < count - 1; b++) {
    buffer[b] = buffer[b + 1];
    total += buffer[b];
  }
  buffer[count - 1] = lastValue;
  total += lastValue;
  return total / count;
}

uint16_t average(uint16_t buffer[], uint8_t count, uint16_t lastValue) {
  uint32_t total = 0;
  for (uint8_t b = 0; b < count - 1; b++) {
    buffer[b] = buffer[b + 1];
    total += buffer[b];
  }
  buffer[count - 1] = lastValue;
  total += lastValue;
  return total / count;
}


// ----- FRAME UTILITIES

void increaseFrameCounter(uint16_t *frameCounter, uint16_t increment) {
  // UINT16_MAX = 65535. Next value loops back and is 0.
  if (UINT16_MAX - increment < *frameCounter) {
    increment -= UINT16_MAX - *frameCounter;
    *frameCounter = increment;
  } else {
    *frameCounter = *frameCounter + increment;
  }
}

bool frameMultipleOf(uint16_t frameCounter, uint16_t mod) {
  return frameCounter % mod == 0;
}

// Returns the current frame step given a frame rate and a number of steps.
// Returns a value between 0 and (steps - 1).
//
// This is used when a method wants to know "where it is" in an animation. For
// example, if an animation lights 30 pixels in succession, it would have 30
// steps. The time between each animation would be the frame rate (e.g. 3
// frames).
uint16_t frameStep(uint16_t frameRate, uint16_t steps) {
  // Suppose we're at frame 28, the frame rate of the method is 3 and
  // there are 6 steps in the animation:
  //   (28 / 3) % 6 = 9 % 6 = 3
  return (videoFrameCounter / frameRate) % steps;
}

// Returns a value within a range based on the value of param.
// If param is < CUSTOM_ANALOG_OPTION_1_THRESHOLD, it returns a range between
// minValue and maxValue based on the current beat level. The higher the beat
// level, the closer to maxValue the returned value is.
// If param is >= CUSTOM_ANALOG_OPTION_1_THRESHOLD, it returns a value between
// minValue and maxValue mapped on the value of param.
uint16_t getValueInRangePotentiallyMappedByBeat(uint16_t param,
    uint16_t minValue, uint16_t maxValue) {
  return getValueInRangePotentiallyMappedByBeat(
           param, minValue, maxValue, false /* reverseBeat */);
}

// Same as above, but if value is returned by beat and the beat is high, the
// minimum value is returned instead of the maximum value.
uint16_t getValueInRangePotentiallyMappedByBeatReverse(uint16_t param,
    uint16_t minValue, uint16_t maxValue) {
  return getValueInRangePotentiallyMappedByBeat(
           param, minValue, maxValue, true /* reverseBeat */);
}

uint16_t getValueInRangePotentiallyMappedByBeat(uint16_t param,
    uint16_t minValue, uint16_t maxValue, boolean reverseBeat) {
  uint16_t value;
  if (param < CUSTOM_ANALOG_OPTION_1_THRESHOLD) {
    uint16_t minMappedValue;
    uint16_t maxMappedValue;
    if (reverseBeat) {
      minMappedValue = maxValue;
      // Goes under to facilitate accessing first value.
      maxMappedValue = minValue > 0 ? minValue - 1 : 0;
    } else {
      minMappedValue = minValue;
      // Goes over to facilitate accessing last value.
      maxMappedValue = maxValue + 1;
    }
    value = map(beatLevel,
                0,
                MICROPHONE_DC_OFFSET * 2,
                minMappedValue,
                maxMappedValue);
  } else {
    value = map(param,
                CUSTOM_ANALOG_OPTION_1_THRESHOLD,
                MAX_ANALOG_VALUE_PARAM,
                minValue,
                // Goes over to facilitate accessing last value.
                maxValue + 1);
  }
  return constrain(value, minValue, maxValue);
}


// ----- DISPLAY UTILITIES

// Returns the same as scaledWheel() but with a chance of returning black.
// The 'blackChance' value must be between 0 and 100.
uint32_t scaledWheel(uint8_t wheelPos, uint8_t blackChance) {
  blackChance = constrain(blackChance, 0, 100);
  if (random(1, 101) <= blackChance) {
    return 0;
  }
  return scaledWheel(wheelPos);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// Returned value is scaled down by the current brightness.
uint32_t scaledWheel(uint8_t wheelPos) {
  return scaledColor(wheel(wheelPos));
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return Adafruit_NeoPixel::Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return Adafruit_NeoPixel::Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return Adafruit_NeoPixel::Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

// Input a value 0 to 255 to get a color value.
// The colours are a VU Meter (blue --> green --> red).
// Returned value is scaled down by the current brightness.
//
// Over the value below, all mapped pixels will be red. This gives an ability to
// show a "tail" of red pixels.
const uint8_t RED_THRESHOLD = 220;
uint32_t scaledVuMeter(uint8_t vuMeterPos) {
  uint8_t r, g, b;
  if (vuMeterPos >= RED_THRESHOLD) {
    r = 255;
    g = 0;
    b = 0;
  } else if (vuMeterPos < RED_THRESHOLD / 2) {
    r = 0;
    g = map(vuMeterPos, 0, RED_THRESHOLD / 2, 0, 255);
    b = 255 - g;
  } else {
    r = map(vuMeterPos, RED_THRESHOLD / 2, RED_THRESHOLD, 0, 255);
    g = 255 - r;
    b = 0;
  }
  return scaledColor(r, g, b);
}

// Returns the given color expressed as its components, scaled down by the
// current brightness.
uint32_t scaledColor(uint8_t r, uint8_t g, uint8_t b) {
  float brightnessRatio = (float) brightness / (float) MAX_BRIGHTNESS;
  float scaledr = brightnessRatio * (float) r;
  float scaledg = brightnessRatio * (float) g;
  float scaledb = brightnessRatio * (float) b;
  return Adafruit_NeoPixel::Color(
           (uint8_t) scaledr, (uint8_t) scaledg, (uint8_t) scaledb);
}

// Returns the given color expressed as a 32 bits int, scaled down by the
// current brightness.
uint32_t scaledColor(uint32_t color) {
  return scaledColor((color & 0x00ff0000UL) >> 16,
                     (color & 0x0000ff00UL) >> 8,
                     (color & 0x000000ffUL));
}

// Fades the given color using the given threshold and decay. If any of the
// components is equal or lower than the threshold, sets it to 0. Otherwise,
// lowers the color by max(1, component x decay / 256).
uint32_t fadeColor(uint32_t color, uint8_t threshold, uint8_t decay) {
  uint8_t r = (color & 0x00ff0000UL) >> 16;
  uint8_t g = (color & 0x0000ff00UL) >> 8;
  uint8_t b = (color & 0x000000ffUL);

  r = (r <= threshold) ? 0 : r - max(1, r * decay / 256);
  g = (g <= threshold) ? 0 : g - max(1, g * decay / 256);
  b = (b <= threshold) ? 0 : b - max(1, b * decay / 256);

  return Adafruit_NeoPixel::Color(r, g, b);
}


// ----- STRIPS UTILITIES

void setSharedPixelColor(uint8_t pixel, uint32_t color) {
  pixel = constrain(pixel, 0, PIXELS_PER_EFFECT - 1);
  if (REVERSE_ALL_EFFECTS) {
    pixel = PIXELS_PER_EFFECT - 1 - pixel;
  }

  // === Pixels ===
  // Top rear: 13
  // Bottom rear: 14
  // Middle bottom & top: 19
  // Front: 14
  // Vertical under saddle: 11 each side
  //
  // The top rear LEDs correspond to the bottom rear 1-13 pixel indices
  // (excluding index 0). So if this method is called with pixel == 0, only
  // one LED will be displayed on the rear side (on each side).

  // LEDs are indexed starting at 0, so if pixel == 0, this displays the color
  // on the 14th LED.
  setSideStripsPixelColor(pixel + 13, color);

  if (pixel <= 13) {
    setSideStripsPixelColor(13 - pixel, color);

    uint8_t vertical = UINT8_MAX;
    switch(pixel) {
      case 1:
        vertical = 0;
        break;
      case 3:
        vertical = 1;
        break;
      case 6:
        vertical = 2;
        break;
      case 8:
        vertical = 3;
        break;
      case 11:
        vertical = 4;
        break;
      case 13:
        vertical = 5;
        break;
    }
    if (vertical != UINT8_MAX) {
      setCenterStripPixelColor(5 + vertical, color);
      if (vertical != 0) {
        setCenterStripPixelColor(5 - vertical, color);
      }
    }
  } else if (pixel <= 32) {
    // Index 14 of bottom bar is first index shown on top bar.
    // First index on top bar is 11.
    // Thus, actual pixel index is: pixel - 14 + 11 = pixel - 3.
    // So if pixel == 32, actual pixel on center strip is 29 as expected.
    setCenterStripPixelColor(pixel - 3, color);
  }
}

uint32_t getSharedPixelColor(uint8_t pixel) {
  pixel = constrain(pixel, 0, PIXELS_PER_EFFECT - 1);

  // Only side strips have all the colors, so we'll use one for our purpose.
  return strips[1].getPixelColor(pixel + 13);
}

void setCenterStripPixelColor(uint8_t pixel, uint32_t color) {
  pixel = constrain(pixel, 0, (LEDS_PER_STRIP_COMMON / 2) - 1);
  if (REVERSE_ALL_EFFECTS) {
    pixel = (LEDS_PER_STRIP_COMMON / 2) - 1 - pixel;
  }
  _setCenterStripPixelColor(pixel, color);
}

void setSideStripsPixelColor(uint8_t pixel, uint32_t color) {
  pixel = constrain(pixel, 0, LEDS_PER_STRIP_COMMON - 1);
  if (REVERSE_ALL_EFFECTS) {
    pixel = LEDS_PER_STRIP_COMMON - 1 - pixel;
  }
  _setSideStripsPixelColor(pixel, color);
}

// Sets the color of a pixel position on the center strips. Translates the given
// position from 0-29 to its actual position based on how the strip is tied
// around the bike.
void _setCenterStripPixelColor(uint8_t pixel, uint32_t color) {
  // Translate pixel position to actual position on physical strip.
  // 38 first pixels are on right/left center bar (19 each side), then
  // 22 next pixels are on bar under saddle (11 each side).
  // Thus the following positions represent each side:
  //   - Start (vertical bar) 49...59; 0...18 End (center bar)
  //   - Start (vertical bar) 48...19 End (center bar)

  // First side
  if (pixel < 11) {
    strips[0].setPixelColor(49 + pixel, color);
  } else {
    // Pixel can be 11-29
    strips[0].setPixelColor(pixel - 11, color);
  }

  // Second side
  // Pixel can be 0-29
  strips[0].setPixelColor(48 - pixel, color);
}

// Sets the color of a pixel position on the side strips.
void _setSideStripsPixelColor(uint8_t pixel, uint32_t color) {
  strips[1].setPixelColor(pixel, color);
  strips[2].setPixelColor(pixel, color);
}

// Clears the strips memory (does not actually switch off the LEDs).
void clearStrips() {
  for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
    strips[s].clear();
  }
}


// ---- SOUND PROCESSING UTILITIES

// Calculation of constants:
//   http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
// Original code was for a Butterworth filter running at 5,000 Hz:
//   http://damian.pecke.tt/beat-detection-on-the-arduino

// 20 - 200hz Single Pole Bandpass IIR Filter (Butterworth 1500Hz sampling rate)
float butter1500_bandpass_20hz_200hz(float sample) {
  static float bassSamplesX[3] = {0.f, 0.f, 0.f};
  static float bassSamplesY[3] = {0.f, 0.f, 0.f};

  bassSamplesX[0] = bassSamplesX[1];
  bassSamplesX[1] = bassSamplesX[2];
  bassSamplesX[2] = sample / 3.291072859e+00f;
  bassSamplesY[0] = bassSamplesY[1];
  bassSamplesY[1] = bassSamplesY[2];
  bassSamplesY[2] = (bassSamplesX[2] - bassSamplesX[0])
                    + (-0.4327386423f * bassSamplesY[0])
                    + (1.3802466192f * bassSamplesY[1]);
  return bassSamplesY[2];
}

// 10hz Single Pole Lowpass IIR Filter (Butterworth 1500Hz sampling rate)
float butter1500_lowpass_10hz(float sample) { //10hz low pass
  static float envSamplesX[2] = {0.f, 0.f};
  static float envSamplesY[2] = {0.f, 0.f};

  envSamplesX[0] = envSamplesX[1];
  envSamplesX[1] = sample / 4.873950141e+01f;
  envSamplesY[0] = envSamplesY[1];
  envSamplesY[1] = (envSamplesX[0] + envSamplesX[1])
                   + (0.9589655220f * envSamplesY[0]);
  return envSamplesY[1];
}
