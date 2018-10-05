////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// ----- EFFECTS

const uint16_t NOISE_DISPLAY_FRAME_RATE = 2;
bool noiseDisplayEffect(uint16_t absSignal) {
  if (!frameMultipleOf(videoFrameCounter, NOISE_DISPLAY_FRAME_RATE)) {
    return false;
  }

  clearStrips();

  uint8_t pixelsToFill = map(absSignal,
                             0,
                             MICROPHONE_DC_OFFSET,
                             0,
                             LEDS_PER_STRIP_COMMON);

  for (uint16_t p = 0; p < pixelsToFill; p++) {
    uint32_t color = scaledVuMeter(map(p, 0, LEDS_PER_STRIP_COMMON, 0, 255));
    setPixelColor(p, color);
  }

  return true;
}
