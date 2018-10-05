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
                             PIXELS_PER_EFFECT + 2);
  pixelsToFill = constrain(pixelsToFill, 0, PIXELS_PER_EFFECT);

  if (pixelsToFill > peakDot) {
    peakDot = pixelsToFill;
  }
  if (pixelsToFill == 0 && peakDot == 0) {
    peakDot = PIXELS_PER_EFFECT - 1;
  }

  for (uint16_t p = 0; p < pixelsToFill; p++) {
    uint32_t color = scaledVuMeter(map(p, 0, PIXELS_PER_EFFECT, 0, 255));
    setSharedPixelColor(p, color);
  }

  if (peakDot > 0) {
    uint32_t color = scaledVuMeter(map(peakDot, 0, PIXELS_PER_EFFECT, 0, 255));
    setSharedPixelColor(peakDot, color);
  }

  if (frameMultipleOf(videoFrameCounter, VU_METER_PEAK_UPDATE_FRAME_RATE)
      && peakDot > 0) {
    peakDot--;
  }

  return true;
}

// Fill the dots one after the other with a color
const uint16_t COLOR_WIPE_FRAME_RATE = 1;
const uint8_t COLOR_WIPE_TOTAL_STEPS = PIXELS_PER_EFFECT * 2;
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
  if (step < PIXELS_PER_EFFECT) {
    offset = 0;
    pixelsToFill = step + 1;
  } else {
    offset = step % PIXELS_PER_EFFECT;
    pixelsToFill = PIXELS_PER_EFFECT - offset;
  }

  for (uint16_t p = 0; p < pixelsToFill; p++) {
    setSharedPixelColor(PIXELS_PER_EFFECT - offset - p - 1, scaledColor);
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

  for (uint16_t p = 0; p < PIXELS_PER_EFFECT; p++) {
    uint32_t color = scaledWheel((p + step) & 255);
    setSharedPixelColor(p, color);
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

  for (uint16_t p = 0; p < PIXELS_PER_EFFECT; p++) {
    uint32_t color = scaledWheel(((p * 256 / PIXELS_PER_EFFECT) + step) & 255);
    setSharedPixelColor(p, color);
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

  for (uint16_t p = 0; p < PIXELS_PER_EFFECT; p = p + 3) {
    setSharedPixelColor(p + invStep, scaledColor);
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

  for (uint16_t p = 0; p < PIXELS_PER_EFFECT; p = p + 3) {
    uint32_t color = scaledWheel((p + colorStep) % 255);
    // Turn every third pixel on.
    setSharedPixelColor(p + invStep, color);
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
  static uint8_t stripsHeat[PIXELS_PER_EFFECT];

  if (!frameMultipleOf(videoFrameCounter, FIRE_FRAME_RATE)) {
    return false;
  }

  if (modeChanged) {
    for (uint8_t p = 0; p < PIXELS_PER_EFFECT; p++) {
      stripsHeat[p] = 0;
    }
  }

  // Step 1.  Cool down every cell a little
  uint8_t cooldown;
  for (int p = 0; p < PIXELS_PER_EFFECT; p++) {
    cooldown = random(0, ((cooling * 10) / PIXELS_PER_EFFECT) + 2);

    if (cooldown > stripsHeat[p]) {
      stripsHeat[p] = 0;
    } else {
      stripsHeat[p] = stripsHeat[p] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int p = PIXELS_PER_EFFECT - 1; p >= 2; p--) {
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
  for (int p = 0; p < PIXELS_PER_EFFECT; p++) {
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
    setSharedPixelColor(p, color);
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
const uint8_t METEOR_MAX_STEPS = PIXELS_PER_EFFECT * 2;
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
  for (uint8_t p = 0; p < PIXELS_PER_EFFECT; p++) {
    if ( (!meteorRandomDecay) || (random(10) > 5) ) {
      // Incrementally fade pixels to black based on previous value.
      uint32_t dimmedColor = fadeColor(getSharedPixelColor(p),
                                       threshold,
                                       meteorTrailDecay);
      setSharedPixelColor(p, dimmedColor);
    }
  }

  // Draw meteor.
  if (step < PIXELS_PER_EFFECT) {
    for (uint8_t p = 0; p < meteorSize; p++) {
      uint8_t position = PIXELS_PER_EFFECT - step + p - 1;
      if (position < PIXELS_PER_EFFECT) {
        setSharedPixelColor(position, scaledColor);
      }
    }
  }

  return true;
}

const uint16_t LIGHT_FLOW_2_FRAME_RATE = 1;
// Minus one because we display two LEDs next to each other.
const uint8_t LIGHT_FLOW_2_MAX_STEPS = PIXELS_PER_EFFECT * 2 - 1;
// Code found on (?):
// http://wiki.makeblock.cc/index.php?title=LED_RGB_Strip-Addressable%2C_Sealed
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

  uint8_t refStep = step;
  if (refStep >= PIXELS_PER_EFFECT) {
    refStep = PIXELS_PER_EFFECT - refStep % PIXELS_PER_EFFECT - 2;
  }

  uint8_t red =   64 * (1 + sin(refStep / 2.0 + j / 4.0       ));
  uint8_t green = 64 * (1 + sin(refStep / 1.0 + f / 9.0  + 2.1));
  uint8_t blue =  64 * (1 + sin(refStep / 3.0 + k / 14.0 + 4.2));
  uint32_t color = scaledColor(red, green, blue);

  for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
    setSharedPixelColor(refStep, color);
    if (refStep < PIXELS_PER_EFFECT - 1) {
      setSharedPixelColor(refStep + 1, color);
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
  for (int8_t p = 0; p < PIXELS_PER_EFFECT; p++) {
    setSharedPixelColor(p, colorDimmed);
  }

  if (sparkle) {
    uint8_t randomPos;
    uint32_t color = scaledColor(r, g, b);
    for (int n = 0; n < numLedsToSpark; n++) {
      randomPos = random(0, PIXELS_PER_EFFECT);
      setSharedPixelColor(randomPos, color);
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
  static uint32_t unscaledStripsGradualFill[PIXELS_PER_EFFECT];

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
  for (uint8_t p = 1; p < PIXELS_PER_EFFECT; p++) {
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
    if (unscaledStripsGradualFill[PIXELS_PER_EFFECT - 2] != 0) {
      color = unscaledStripsGradualFill[PIXELS_PER_EFFECT - 2];
    } else {
      color = wheel(random(0, 256));
    }
    unscaledStripsGradualFill[PIXELS_PER_EFFECT - 1] = color;
  }

  // Show strips.
  for (uint8_t p = 0; p < PIXELS_PER_EFFECT; p++) {
    setSharedPixelColor(p, scaledColor(unscaledStripsGradualFill[p]));
  }

  return true;
}

const uint16_t BIKE_SHAPES_FRAME_RATE = 1;
const uint8_t BLACK_CHANCE = 50; /* 0-100 */
bool bikeShapesEffect(bool newFrontColor,
                      bool newMiddleColor,
                      bool newRearColor) {
  // 0: Front, 1: Middle, 2: Rear
  static uint32_t shapeColors[3] = {0, 0, 0};

  if (!frameMultipleOf(videoFrameCounter, BIKE_SHAPES_FRAME_RATE)) {
    return false;
  }

  if (!newFrontColor && !newMiddleColor && !newRearColor) {
    return false;
  }

  if (newFrontColor) {
    shapeColors[0] = scaledWheel(random(0, 256), BLACK_CHANCE);
  }
  if (newMiddleColor) {
    shapeColors[1] = scaledWheel(random(0, 256), BLACK_CHANCE);
  }
  if (newRearColor) {
    shapeColors[2] = scaledWheel(random(0, 256), BLACK_CHANCE);
  }
  // Make sure at least one shape is always on.
  if (shapeColors[0] == 0 && shapeColors[1] == 0 && shapeColors[2] == 0) {
    shapeColors[random(0, 3)] = scaledWheel(random(0, 256));
  }

  clearStrips();

  // Front.
  if (shapeColors[0] > 0) {
    for (uint8_t p = 33; p < PIXELS_PER_EFFECT; p++) {
      setSharedPixelColor(p, shapeColors[0]);
    }
  }
  
  // Do rear before middle as we want to cover vertical
  // bar with middle color if we have one.

  // Rear.
  if (shapeColors[2] > 0) {
    for (uint8_t p = 0; p < 14; p++) {
      setSharedPixelColor(p, shapeColors[2]);
    }
  }

  // Middle.
  if (shapeColors[1] > 0) {
    for (uint8_t p = 14; p < 33; p++) {
      setSharedPixelColor(p, shapeColors[1]);
    }
    for (uint8_t p = 0; p < 11; p++) {
      setCenterStripPixelColor(p, shapeColors[1]);
    }
  }
  
  return true;
}

