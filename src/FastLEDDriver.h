#pragma once
#include <FastLED.h>
#include "core/ConsumerModule.h"
#include "FlmPixels.h"
#include "FlmConfig.h"

// FastLEDDriver — pushes flm_leds[] to the physical strip via FastLED.show().
//
// Runs on Core 1 (output core) by preferredCore().
// Brightness is adjustable at runtime from the web UI.
//
// Extends ConsumerModule: wired to a WaveRainbow2DEffect (or any ProducerModule)
// via setInput("producer", &effect). The producer's buffer is accessible via
// producer_->bufferPtr() if needed; this driver reads flm_leds[] directly for
// compatibility with the shared-array pattern.
//
// This module owns the FastLED hardware binding: it calls FastLED.addLeds()
// in setup(). Exactly one FastLEDDriver should be in any pipeline.
//
// To change LED type (e.g., SK6812, APA102) or pin, update FlmConfig.h
// and change the template arguments in setup() below.

class FastLEDDriverModule : public ConsumerModule {
public:
    const char* name()     const override { return "FastLEDDriver"; }
    const char* category() const override { return "driver"; }
    uint8_t     preferredCore() const override { return 1; }

    void setup() override {
        FastLED.addLeds<WS2812B, FLM_PIN, GRB>(flm_leds, FLM_NUM_LEDS);
        FastLED.setBrightness(static_cast<uint8_t>(brightness_));
        addControl(brightness_, "brightness", "range", 0.0f, 255.0f);
    }

    void loop() override {
        FastLED.show();
    }

    void teardown() override {
        FastLED.clear(true);
    }

    void onUpdate(const char* key) override {
        if (strcmp(key, "brightness") == 0)
            FastLED.setBrightness(static_cast<uint8_t>(brightness_));
    }

    void healthReport(char* buf, size_t len) const override {
        snprintf(buf, len, "brightness=%.0f fps=%u", brightness_, FastLED.getFPS());
    }

    size_t classSize() const override { return sizeof(*this); }

private:
    float brightness_ = 64.0f;
};
