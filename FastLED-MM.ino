// FastLED-MM — Arduino IDE entry point.
//
// This file mirrors src/main.cpp for Arduino IDE users.
// PlatformIO users: use src/main.cpp via platformio.ini.
//
// Arduino IDE setup:
//   1. Install the projectMM library (Sketch > Include Library > Add .ZIP Library)
//   2. Install FastLED via the Library Manager
//   3. Open this sketch and flash

#include <Arduino.h>
#include <FastLED.h>
#include "src/core/AppSetup.h"
#include "src/core/Scheduler.h"
#include "src/core/ModuleManager.h"
#include "src/core/HttpServer.h"
#include "src/core/WsServer.h"
#include "src/core/ProducerModule.h"
#include "src/core/ConsumerModule.h"

constexpr uint8_t  PIN      = 2;    // data pin to the first LED strip
constexpr uint16_t WIDTH    = 16;   // panel width in pixels
constexpr uint16_t HEIGHT   = 16;   // panel height in pixels
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// Row-major index helper: (x=column, y=row) to flat array index.
// Matches the layout written by most 2D panel drivers (no serpentine).
inline uint16_t idx(uint8_t x, uint8_t y) {
    return static_cast<uint16_t>(y) * WIDTH + x;
}

// Shared pixel array — written by effects, read by the driver.
CRGB leds[NUM_LEDS];

class WaveRainbow2DEffect : public ProducerModule {
public:
    const char* name()     const override { return "WaveRainbow2D"; }
    const char* category() const override { return "source"; }
    const char* tags()     const override { return "🔥🟦"; }
    uint8_t     preferredCore() const override { return 0; }

    void setup() override {
        declareBuffer(leds, NUM_LEDS, sizeof(CRGB));
        addControl(speed_,     "speed",      "slider", 1.0f, 10.0f);
        addControl(hueOffset_, "hue_offset", "slider", 0.0f, 255.0f);
    }

    void loop() override {
        uint8_t phase = static_cast<uint8_t>(millis() * static_cast<uint32_t>(speed_) >> 6);
        for (uint8_t y = 0; y < HEIGHT; ++y) {
            for (uint8_t x = 0; x < WIDTH; ++x) {
                uint8_t ripple = sin8((x << 4) + (y << 3) + phase);
                uint8_t hue    = static_cast<uint8_t>(hueOffset_) + (x << 4) + (ripple >> 2);
                leds[idx(x, y)] = CHSV(hue, 240, 255);
            }
        }
    }

    void teardown() override {}

    // Suppress ProducerModule::snapshot() — FastLEDDriverModule owns the preview.
    bool snapshot(std::vector<uint8_t>&) const override { return false; }

    void healthReport(char* buf, size_t len) const override {
        snprintf(buf, len, "speed=%.0f hue=%.0f", speed_, hueOffset_);
    }

    size_t classSize() const override { return sizeof(*this); }

private:
    float speed_     = 3.0f;
    float hueOffset_ = 0.0f;
};

class FastLEDDriverModule : public ConsumerModule {
public:
    const char* name()     const override { return "FastLEDDriver"; }
    const char* category() const override { return "driver"; }
    uint8_t     preferredCore() const override { return 1; }

    void setup() override {
        FastLED.addLeds<WS2812B, PIN, GRB>(leds, NUM_LEDS);
        FastLED.setBrightness(static_cast<uint8_t>(brightness_));
        // Cap at 30fps: at 90fps the RMT peripheral occupies ~70% of radio
        // time on ESP32-S3, starving WiFi beacon transmission and making the
        // AP invisible in scans. 30fps drops duty cycle to ~23%.
        FastLED.setMaxRefreshRate(30);
        addControl(brightness_, "brightness", "slider", 0.0f, 255.0f);
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

    bool snapshot(std::vector<uint8_t>& buf) const override {
        buf.resize(7 + NUM_LEDS * 3);
        buf[0] = 0x02;
        buf[1] = WIDTH & 0xFF;  buf[2] = WIDTH >> 8;
        buf[3] = HEIGHT & 0xFF; buf[4] = HEIGHT >> 8;
        buf[5] = 1;             buf[6] = 0;
        for (uint16_t i = 0; i < NUM_LEDS; ++i) {
            buf[7 + i*3 + 0] = leds[i].r;
            buf[7 + i*3 + 1] = leds[i].g;
            buf[7 + i*3 + 2] = leds[i].b;
        }
        return true;
    }

    void healthReport(char* buf, size_t len) const override {
        snprintf(buf, len, "brightness=%.0f fps=%u", brightness_, FastLED.getFPS());
    }

    size_t classSize() const override { return sizeof(*this); }

private:
    float brightness_ = 64.0f;
};

// Register modules so the web UI and REST API can instantiate them.
REGISTER_MODULE(WaveRainbow2DEffect)
REGISTER_MODULE(FastLEDDriverModule)

// projectMM runtime objects.
static Scheduler     scheduler;
static ModuleManager mm(scheduler);
static HttpServer    server(80);
static WsServer      ws;

// Called once on first boot to populate the default pipeline.
// Guard with hasModuleType so subsequent boots skip this.
static void firstBoot(ModuleManager& mm) {
    if (pal::hasModuleType(mm, "WaveRainbow2DEffect")) return;
    if (pal::hasModuleType(mm, "FastLEDDriverModule")) return;
    JsonDocument d; auto ep = d.as<JsonObjectConst>();
    mm.addModule("WaveRainbow2DEffect", "fx1",     ep, ep, 0, "");
    mm.addModule("FastLEDDriverModule", "driver1", ep, ep, 1, "");
    mm.saveAllState();
}

void setup() {
    Serial.begin(115200);
    pal::embeddedSetup(mm, scheduler, server, ws, nullptr, firstBoot);
}

void loop() { pal::suspend_forever(); }
