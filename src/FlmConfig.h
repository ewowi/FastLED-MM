#pragma once

// Hardware configuration — edit these to match your setup.
constexpr uint8_t  FLM_PIN      = 2;    // data pin to the first LED strip
constexpr uint16_t FLM_WIDTH    = 16;   // panel width in pixels
constexpr uint16_t FLM_HEIGHT   = 16;   // panel height in pixels
constexpr uint16_t FLM_NUM_LEDS = FLM_WIDTH * FLM_HEIGHT;
