#pragma once
#include <FastLED.h>
#include "core/ProducerModule.h"
#include "FlmPixels.h"

// WaveRainbow2D — animated 2D diagonal rainbow with a sine-wave ripple.
//
// Each pixel's hue is computed from its column (x) position plus a sin8()
// ripple that shifts with y and time, producing a smooth flowing pattern
// across the panel. Uses FastLED's integer sin8() — no floating-point in loop().
//
// Extends ProducerModule: registers flm_leds[] as the pixel buffer so
// ConsumerModule subclasses can read it via bufferPtr() / bufferLen().
//
// Controls (visible in the web UI and adjustable at runtime):
//   speed       (1-10)   — animation rate
//   hue_offset  (0-255)  — global hue shift; set it to rotate the colour palette

class WaveRainbow2DEffect : public ProducerModule {
public:
    const char* name()     const override { return "WaveRainbow2D"; }
    const char* category() const override { return "effect"; }
    const char* tags()     const override { return "🔥🟦"; }
    uint8_t     preferredCore() const override { return 0; }

    void setup() override {
        declareBuffer(flm_leds, FLM_NUM_LEDS, sizeof(CRGB));
        addControl(speed_,     "speed",      "range", 1.0f, 10.0f);
        addControl(hueOffset_, "hue_offset", "range", 0.0f, 255.0f);
    }

    void loop() override {
        uint8_t phase = static_cast<uint8_t>(millis() * static_cast<uint32_t>(speed_) >> 6);
        for (uint8_t y = 0; y < FLM_HEIGHT; ++y) {
            for (uint8_t x = 0; x < FLM_WIDTH; ++x) {
                uint8_t ripple = sin8((x << 4) + (y << 3) + phase);
                uint8_t hue    = static_cast<uint8_t>(hueOffset_) + (x << 4) + (ripple >> 2);
                flm_leds[flm_idx(x, y)] = CHSV(hue, 240, 255);
            }
        }
    }

    void teardown() override {}

    void healthReport(char* buf, size_t len) const override {
        snprintf(buf, len, "speed=%.0f hue=%.0f", speed_, hueOffset_);
    }

    size_t classSize() const override { return sizeof(*this); }

private:
    float speed_     = 3.0f;
    float hueOffset_ = 0.0f;
};
