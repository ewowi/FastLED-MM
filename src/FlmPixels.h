#pragma once
#include <FastLED.h>
#include "FlmConfig.h"

// Shared pixel array. Defined once in main.cpp; declared extern here so
// both the effect and driver modules can access it without duplication.
extern CRGB flm_leds[FLM_NUM_LEDS];

// Row-major index helper: (x=column, y=row) to flat array index.
// Matches the layout written by most 2D panel drivers (no serpentine).
inline uint16_t flm_idx(uint8_t x, uint8_t y) {
    return static_cast<uint16_t>(y) * FLM_WIDTH + x;
}
