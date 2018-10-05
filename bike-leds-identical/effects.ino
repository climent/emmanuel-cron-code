////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// ----- EFFECTS

const uint16_t VU_METER_FRAME_RATE = 2;
// Must be a multiple of the above.
const uint16_t VU_METER_PEAK_UPDATE_FRAME_RATE = 4;
bool vuMeterEffect(uint16_t refLevel) {
  static uint8_t peakDot = 0;

  if (!frameMultipleOf(videoFrameCounter, VU_METER_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  uint8_t pixelsToFill = map(refLevel,
                             0,
                             MICROPHONE_DC_OFFSET * 2,
                             0,
                             // Allow to go slightly off scale.
                             LEDS_PER_STRIP_COMMON + 2);
  pixelsToFill = constrain(pixelsToFill, 0, LEDS_PER_STRIP_COMMON);

  if (pixelsToFill > peakDot) {
    peakDot = pixelsToFill;
  }

  for (uint16_t p = 0; p < pixelsToFill; p++) {
    uint32_t color = scaledVuMeter(map(p, 0, LEDS_PER_STRIP_COMMON, 0, 255));
    if (p % 2 == 0) {
      setCenterPixelColor(p / 2, color);
    }
    setSidePixelColor(p, color);
  }

  if (peakDot > 0) {
    uint32_t color = scaledVuMeter(
                       map(peakDot, 0, LEDS_PER_STRIP_COMMON, 0, 255));
    setCenterPixelColor(peakDot / 2, color);
    setSidePixelColor(peakDot, color);
  }

  if (frameMultipleOf(videoFrameCounter, VU_METER_PEAK_UPDATE_FRAME_RATE)
      && peakDot > 0) {
    peakDot--;
  }

  return true;
}

// Fill the dots one after the other with a color
const uint16_t COLOR_WIPE_FRAME_RATE = 1;
const uint8_t COLOR_WIPE_TOTAL_STEPS = LEDS_PER_STRIP_COMMON * 2;
bool colorWipeEffect(uint32_t scaledColor, uint16_t speedInFrames) {
  static uint8_t step = COLOR_WIPE_TOTAL_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, COLOR_WIPE_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  if (modeChanged) {
    step = 0;
  } else if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == COLOR_WIPE_TOTAL_STEPS - 1 ? 0 : step + 1;
  }

  uint16_t pixelsToFill;
  uint16_t offset;
  if (step < LEDS_PER_STRIP_COMMON) {
    offset = 0;
    pixelsToFill = step + 1;
  } else {
    offset = step % LEDS_PER_STRIP_COMMON;
    pixelsToFill = LEDS_PER_STRIP_COMMON - offset;
  }

  for (uint16_t p = 0; p < pixelsToFill; p++) {
    if  (p % 2 == 0) {
      setCenterPixelColor((LEDS_PER_STRIP_COMMON / 2)
                          - (offset / 2)
                          - (p / 2)
                          - 1,
                          scaledColor);
    }
    setSidePixelColor(LEDS_PER_STRIP_COMMON - offset - p - 1, scaledColor);
  }
  return true;
}

const uint16_t RAINBOW_FRAME_RATE = 1;
const uint8_t RAINBOW_TOTAL_STEPS = 256;
bool rainbowEffect(uint16_t speedInFrames) {
  static uint8_t step = RAINBOW_TOTAL_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, RAINBOW_FRAME_RATE)) {
    return false;
  }

  if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == RAINBOW_TOTAL_STEPS - 1 ? 0 : step + 1;
  }

  for (uint16_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    uint32_t color = scaledWheel((p + step) & 255);
    if  (p % 2 == 0) {
      setCenterPixelColor(p / 2, color);
    }
    setSidePixelColor(p, color);
  }
  return true;
}

// Slightly different, this makes the rainbow equally distributed throughout
const uint16_t RAINBOW_CYCLE_FRAME_RATE = 1;
const uint8_t RAINBOW_CYCLE_TOTAL_STEPS = 256;
bool rainbowCycleEffect(uint16_t speedInFrames) {
  static uint16_t step = RAINBOW_CYCLE_TOTAL_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, RAINBOW_CYCLE_FRAME_RATE)) {
    return false;
  }

  if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == RAINBOW_CYCLE_TOTAL_STEPS - 1 ? 0 : step + 1;
  }

  for (uint16_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    uint32_t color = scaledWheel(
                       ((p * 256 / LEDS_PER_STRIP_COMMON) + step) & 255);
    if  (p % 2 == 0) {
      setCenterPixelColor(p / 2, color);
    }
    setSidePixelColor(p, color);
  }

  return true;
}

// Theatre-style crawling lights.
const uint16_t THEATER_CHASE_FRAME_RATE = 1;
const uint8_t THEATER_CHASE_MAX_STEPS = 3;
bool theaterChaseEffect(uint32_t scaledColor, uint16_t speedInFrames) {
  static uint8_t step = THEATER_CHASE_MAX_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, THEATER_CHASE_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == THEATER_CHASE_MAX_STEPS - 1 ? 0 : step + 1;
  }

  // Inverse direction.
  uint8_t invStep = 2 - step;

  for (uint16_t p = 0; p < LEDS_PER_STRIP_COMMON; p = p + 3) {
    // Turn every third pixel on.
    if (p % 6 == 0) {
      setCenterPixelColor((p / 2) + invStep, scaledColor);
    }
    setSidePixelColor(p + invStep, scaledColor);
  }
  return true;
}

// Theatre-style crawling lights with rainbow effect
const uint8_t THEATER_CHASE_RAINBOW_MAX_STEPS = 3;
const uint8_t THEATER_CHASE_RAINBOW_MAX_COLOR_STEPS = 256;
bool theaterChaseRainbowEffect(uint16_t speedInFrames) {
  static uint8_t step = THEATER_CHASE_RAINBOW_MAX_STEPS - 1;
  static uint16_t colorStep = THEATER_CHASE_RAINBOW_MAX_COLOR_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, speedInFrames)) {
    return false;
  }

  clearStrips();

  if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == THEATER_CHASE_RAINBOW_MAX_STEPS - 1 ? 0 : step + 1;
    colorStep = colorStep == THEATER_CHASE_RAINBOW_MAX_COLOR_STEPS - 1 ?
                0 : colorStep + 1;
  }

  // Inverse direction.
  uint8_t invStep = 2 - step;

  for (uint16_t p = 0; p < LEDS_PER_STRIP_COMMON; p = p + 3) {
    uint32_t color = scaledWheel((p + colorStep) % 255);
    // Turn every third pixel on.
    if (p % 6 == 0) {
      setCenterPixelColor((p / 2) + invStep, color);
    }
    setSidePixelColor(p + invStep, color);
  }
  return true;
}

// Cooling indicates how fast a flame cools down. More cooling means shorter
// flames, and the recommended values are between 20 and 100. 50 seems the
// nicest.
//
// Sparking indicates the chance (out of 255) that a spark will ignite. A
// higher value makes the fire more active. Suggested values lay between 50
// and 200, with a good value being 120.
const uint16_t FIRE_FRAME_RATE = 1;
bool fireEffect(uint8_t cooling, uint8_t sparking) {
  static uint8_t stripsHeat[LEDS_PER_STRIP_COMMON];

  if (!frameMultipleOf(videoFrameCounter, FIRE_FRAME_RATE)) {
    return false;
  }

  if (modeChanged) {
    for (uint8_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
      stripsHeat[p] = 0;
    }
  }

  // Step 1.  Cool down every cell a little
  uint8_t cooldown;
  for (int p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    cooldown = random(0, ((cooling * 10) / LEDS_PER_STRIP_COMMON) + 2);

    if (cooldown > stripsHeat[p]) {
      stripsHeat[p] = 0;
    } else {
      stripsHeat[p] = stripsHeat[p] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int p = LEDS_PER_STRIP_COMMON - 1; p >= 2; p--) {
    stripsHeat[p] = (stripsHeat[p - 1]
                     + stripsHeat[p - 2]
                     + stripsHeat[p - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if (random(255) < sparking) {
    int p = random(7);
    stripsHeat[p] = stripsHeat[p] + random(160, 255);
  }

  // Step 4.  Convert heat to LED colors
  for (int p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    uint8_t temperature = stripsHeat[p];

    // Scale 'heat' down from 0-255 to 0-191
    byte t192 = round((temperature / 255.0) * 191);

    // Calculate ramp up from
    byte heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252

    // Figure out which third of the spectrum we're in.
    uint32_t color;
    if ( t192 > 0x80) {                    // hottest
      color = scaledColor(255, 255, heatramp);
    } else if ( t192 > 0x40 ) {            // middle
      color = scaledColor(255, heatramp, 0);
    } else {                               // coolest
      color = scaledColor(heatramp, 0, 0);
    }
    if (p % 2 == 0) {
      setCenterPixelColor(p / 2, color);
    }
    setSidePixelColor(p, color);
  }
  return true;
}

// Meteor trail decay sets how fast the meteor tail decays/ disappears.
// A larger number makes the tail short and/or disappear faster. Theoretically
// a value of 64 should reduce the brightness by 25% for each time the meteor
// gets drawn.
//
// Since meteors are not perfect, the 6th parameter allows to mimic some sorts
// of difference in debris by making the decay a little random. If this value
// is set to “true” then some randomness is added to the rail. If you set the
// value to “false” then the tail will be very smooth.
const uint8_t METEOR_MAX_STEPS = LEDS_PER_STRIP_COMMON * 2;
const uint16_t METEOR_FRAME_RATE = 2;
bool meteorEffect(uint32_t scaledColor,
                  uint8_t meteorSize,
                  uint8_t meteorTrailDecay,
                  bool meteorRandomDecay) {
  static uint8_t step = METEOR_MAX_STEPS - 1;

  if (!frameMultipleOf(videoFrameCounter, METEOR_FRAME_RATE)) {
    return false;
  }

  if (modeChanged) {
    step = 0;
  } else if (frameMultipleOf(videoFrameCounter, METEOR_FRAME_RATE)) {
    step = step == METEOR_MAX_STEPS - 1 ? 0 : step + 1;
  }
  uint8_t threshold = 10 * brightness / MAX_BRIGHTNESS;
  threshold = constrain(threshold, 1, MAX_BRIGHTNESS);

  // Fade brightness all LEDs one step.
  for (uint8_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    if ( (!meteorRandomDecay) || (random(10) > 5) ) {
      // Incrementally fade pixels to black based on previous value.
      uint32_t dimmedColor = fadeColor(getSideStrip()->getPixelColor(p),
                                       threshold, meteorTrailDecay);
      if (p % 2 == 0) {
        setCenterPixelColor(p / 2, dimmedColor);
      }
      setSidePixelColor(p, dimmedColor);
    }
  }

  // Draw meteor.
  if (step < LEDS_PER_STRIP_COMMON) {
    for (uint8_t p = 0; p < meteorSize; p++) {
      uint8_t position = LEDS_PER_STRIP_COMMON - step + p - 1;
      if (position < LEDS_PER_STRIP_COMMON) {
        if (position % 2 == 0) {
          setCenterPixelColor(position / 2, scaledColor);
        }
        setSidePixelColor(position, scaledColor);
      }
    }
  }

  return true;
}

const uint16_t LIGHT_FLOW_2_FRAME_RATE = 1;
// Minus one because we display two LEDs next to each other.
const uint8_t LIGHT_FLOW_2_MAX_STEPS = LEDS_PER_STRIP_COMMON * 2 - 1;
bool lightFlow2Effect(uint16_t speedInFrames) {
  static uint8_t step = LIGHT_FLOW_2_MAX_STEPS - 1;
  static float j, f, k;

  if (!frameMultipleOf(videoFrameCounter, LIGHT_FLOW_2_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  if (modeChanged) {
    step = 0;
  } else if (frameMultipleOf(videoFrameCounter, speedInFrames)) {
    step = step == LIGHT_FLOW_2_MAX_STEPS - 1 ? 0 : step + 1;
  }

  //  uint16_t step = frameStep(speedInFrames, LEDS_PER_STRIP_COMMON * 2 - 1);
  uint8_t refStep = step;
  if (refStep >= LEDS_PER_STRIP_COMMON) {
    refStep = LEDS_PER_STRIP_COMMON - refStep % LEDS_PER_STRIP_COMMON - 2;
  }

  uint8_t red =   64 * (1 + sin(refStep / 2.0 + j / 4.0       ));
  uint8_t green = 64 * (1 + sin(refStep / 1.0 + f / 9.0  + 2.1));
  uint8_t blue =  64 * (1 + sin(refStep / 3.0 + k / 14.0 + 4.2));
  uint32_t color = scaledColor(red, green, blue);

  for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
    if (s % 2 == 0) {
      setCenterPixelColor(refStep / 2, color);
      if ((refStep / 2) < (LEDS_PER_STRIP_COMMON / 2) - 1) {
        setCenterPixelColor((refStep / 2) + 1, color);
      }
    }
    setSidePixelColor(refStep, color);
    if (refStep < LEDS_PER_STRIP_COMMON - 1) {
      setSidePixelColor(refStep + 1, color);
    }
  }

  // Just to keep it sane.
  if (j >= FLT_MAX - 6.0) {
    j = 0;
  }
  if (f  >= FLT_MAX - 6.0) {
    f = 0;
  }
  if (k  >= FLT_MAX - 6.0) {
    k = 0;
  }

  j += random(1, 6) / 6.0;
  f += random(1, 6) / 6.0;
  k += random(1, 6) / 6.0;

  return true;
}

const uint16_t SNOW_SPARKLE_FRAME_RATE = 1;
bool snowSparkleEffect(uint8_t numLedsToSpark, bool sparkle,
                       uint32_t unscaledColor) {
  if (!frameMultipleOf(videoFrameCounter, SNOW_SPARKLE_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  // Dim all LEDs.
  uint8_t r = (unscaledColor & 0x00ff0000UL) >> 16;
  uint8_t g = (unscaledColor & 0x0000ff00UL) >> 8;
  uint8_t b = (unscaledColor & 0x000000ffUL);

  uint8_t rDimmed = r * 16 / 255;
  uint8_t gDimmed = g * 16 / 255;
  uint8_t bDimmed = b * 16 / 255;

  uint32_t colorDimmed = scaledColor(rDimmed, gDimmed, bDimmed);
  for (int8_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    if (p % 2 == 0) {
      setCenterPixelColor(p / 2, colorDimmed);
    }
    setSidePixelColor(p, colorDimmed);
  }

  if (sparkle) {
    uint8_t randomPos;
    uint32_t color = scaledColor(r, g, b);
    for (int n = 0; n < numLedsToSpark; n++) {
      randomPos = random(0, LEDS_PER_STRIP_COMMON);
      if (randomPos % 2 == 0) {
        setCenterPixelColor(randomPos / 2, color);
      }
      setSidePixelColor(randomPos, color);
    }
  }

  return true;
}

// Gradually fills the strip LED by LED.
// If 'fill' is true, will add a LED at the end and will move it down one LED
// every frame until it disappears. If this method is called in a row with
// 'fill' set to true, it will use the same color to fill a LED until the
// method is called again with 'fill' set to false. The next time it will
// choose a random color.
//
// If 'fill' is false, then 'fillInFrames' is looked at. If its value is 0, it
// is ignored. If its value is greater than 0, it will fill the strip the
// number of given frames then leave it empty the same number of frames, and so
// on.
const uint16_t GRADUAL_FILL_FRAME_RATE = 1;
bool gradualFillEffect(bool fill, uint8_t fillInFrames) {
  static uint32_t unscaledStripsGradualFill[LEDS_PER_STRIP_COMMON];

  if (!frameMultipleOf(videoFrameCounter, GRADUAL_FILL_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  // Move all LEDs one pixel down.
  bool hasMoved = false;
  // Remove last pixel, always, so that things move down constantly.
  if (unscaledStripsGradualFill[0] != 0) {
    unscaledStripsGradualFill[0] = 0;
  }
  for (uint8_t p = 1; p < LEDS_PER_STRIP_COMMON; p++) {
    if (unscaledStripsGradualFill[p] != 0 && unscaledStripsGradualFill[p - 1]
        == 0) {
      if (!hasMoved) {
        hasMoved = true;
      }
      unscaledStripsGradualFill[p - 1] = unscaledStripsGradualFill[p];
      unscaledStripsGradualFill[p] = 0;
    }
  }

  if (fillInFrames > 0) {
    uint16_t step = frameStep(GRADUAL_FILL_FRAME_RATE, fillInFrames * 2);
    if (step < fillInFrames) {
      fill = true;
    }
  } else if (!hasMoved) {
    fill = true;
  }

  // Fill if requested or if nothing has moved.
  if (fill) {
    uint32_t color;
    if (unscaledStripsGradualFill[LEDS_PER_STRIP_COMMON - 2] != 0) {
      color = unscaledStripsGradualFill[LEDS_PER_STRIP_COMMON - 2];
    } else {
      color = wheel(random(0, 255));
    }
    unscaledStripsGradualFill[LEDS_PER_STRIP_COMMON - 1] = color;
  }

  // Show strips.
  for (uint8_t p = 0; p < LEDS_PER_STRIP_COMMON; p++) {
    if (p % 2 == 0) {
      setCenterPixelColor(p / 2, scaledColor(unscaledStripsGradualFill[p]));
    }
    setSidePixelColor(p, scaledColor(unscaledStripsGradualFill[p]));
  }

  return true;
}
