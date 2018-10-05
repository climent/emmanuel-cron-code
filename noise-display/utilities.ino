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


// ----- DISPLAY UTILITIES

// Input a value 0 to 255 to get a color value.
// The colours are a VU Meter (blue --> green --> red).
// Returned value is scaled down by the current brightness.
//
// Over this value, all mapped pixels will be red. This gives an ability to
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


// ----- STRIPS UTILITIES

// Sets the color of a pixel position on all strips.
void setPixelColor(uint8_t pixel, uint32_t color) {
  pixel = constrain(pixel, 0, LEDS_PER_STRIP_COMMON - 1);
  if (REVERSE_ALL_EFFECTS) {
    pixel = LEDS_PER_STRIP_COMMON - 1 - pixel;
  }
  strips[0].setPixelColor(pixel, color);
  strips[1].setPixelColor(pixel, color);
  strips[2].setPixelColor(pixel, color);
}

// Clears the strips memory (does not actually switch off the LEDs).
void clearStrips() {
  for (uint8_t s = 0; s < STRIPS_COUNT; s++) {
    strips[s].clear();
  }
}

