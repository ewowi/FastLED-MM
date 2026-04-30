// Minimal sketch: WaveRainbow2D effect on a 16x16 serpentine panel.
// Effects write to logical leds[] (row-major); the driver remaps to
// physLeds[] (serpentine) so PreviewModule sees a correct grid.
#include <Arduino.h>
#include <FastLED.h>
#include <projectMM.h>

constexpr uint8_t PIN = 2;
constexpr uint16_t WIDTH = 16;
constexpr uint16_t HEIGHT = 16;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// Logical pixel array — written by effects in row-major order, read by PreviewModule.
CRGB leds[NUM_LEDS];
// Physical pixel array — serpentine-remapped from leds[] by the driver before FastLED.show().
CRGB physLeds[NUM_LEDS];

// Row-major index: (x, y) → flat array index.
inline uint32_t XY(uint16_t x, uint16_t y) { return (uint32_t)y * WIDTH + x; }
// Serpentine index: odd rows run right→left; delegates to XY().
inline uint32_t serpentineXY(uint16_t x, uint16_t y) { return XY(y & 1 ? (WIDTH - 1 - x) : x, y); }

class WaveRainbow2DEffect : public ProducerModule {
 public:
  const char* name() const override { return "WaveRainbow2D"; }
  const char* category() const override { return "source"; }
  const char* tags() const override { return "🔥🟦"; }
  uint8_t preferredCore() const override { return 0; }

  uint16_t pixelWidth() const override { return WIDTH; }
  uint16_t pixelHeight() const override { return HEIGHT; }

  void setup() override {
    declareBuffer(leds, NUM_LEDS, sizeof(CRGB));
    addControl(speed_, "speed", "slider", 1.0f, 10.0f);
    addControl(hueOffset_, "hue_offset", "slider", 0.0f, 255.0f);
  }

  void loop() override {
    uint8_t phase = static_cast<uint8_t>(millis() * static_cast<uint32_t>(speed_) >> 6);
    for (uint8_t y = 0; y < HEIGHT; ++y)
      for (uint8_t x = 0; x < WIDTH; ++x) {
        uint8_t ripple = sin8((x << 4) + (y << 3) + phase);
        uint8_t hue = static_cast<uint8_t>(hueOffset_) + (x << 4) + (ripple >> 2);
        leds[XY(x, y)] = CHSV(hue, 240, 255);
      }
  }

  size_t classSize() const override { return sizeof(*this); }

 private:
  float speed_ = 3.0f;
  float hueOffset_ = 0.0f;
};

REGISTER_MODULE(WaveRainbow2DEffect)

class FastLEDDriverModule : public ConsumerModule {
 public:
  const char* name() const override { return "FastLEDDriver"; }
  const char* category() const override { return "driver"; }
  uint8_t preferredCore() const override { return 1; }

  void setup() override {
    FastLED.addLeds<WS2812B, PIN, GRB>(physLeds, NUM_LEDS);
    FastLED.setBrightness(static_cast<uint8_t>(brightness_));
    // Cap at 30fps: at 90fps the RMT peripheral occupies ~70% of radio
    // time on ESP32-S3, starving WiFi beacon transmission and making the
    // AP invisible in scans. 30fps drops duty cycle to ~23%.
    FastLED.setMaxRefreshRate(30);
    addControl(brightness_, "brightness", "slider", 0.0f, 255.0f);
  }

  void loop() override {
    // Remap logical row-major leds[] to physical serpentine physLeds[] before pushing.
    // Safe: AppSetup's semaphore ensures Core 0 has finished writing leds[] before this runs.
    for (uint16_t y = 0; y < HEIGHT; ++y)  // cache-friendly: fits in L1; odd rows reversed within 2 hot cache lines
      for (uint16_t x = 0; x < WIDTH; ++x)
        physLeds[serpentineXY(x, y)] = leds[XY(x, y)];
    FastLED.show();
  }

  void teardown() override { FastLED.clear(true); }

  void onUpdate(const char* key) override {
    if (strcmp(key, "brightness") == 0) FastLED.setBrightness(static_cast<uint8_t>(brightness_));
  }

 private:
  float brightness_ = 64.0f;
};

REGISTER_MODULE(FastLEDDriverModule)
REGISTER_MODULE(PreviewModule)

static void firstBoot(ModuleManager& mm) {
  if (pal::hasModuleType(mm, "WaveRainbow2DEffect")) return;
  JsonDocument d;
  auto ep = d.as<JsonObjectConst>();

  mm.addModule("WaveRainbow2DEffect", "fx1", ep, ep, 0, "");
  mm.addModule("FastLEDDriverModule", "driver1", ep, ep, 1, "");

  JsonDocument inp;  // No width/height props needed: PreviewModule reads geometry from source via pixelBuf().
  inp["source"] = "fx1";
  mm.addModule("PreviewModule", "preview1", ep, inp.as<JsonObjectConst>(), 1, "");

  mm.saveAllState();
}

void setup() {
  Serial.begin(115200);
  projectMM::setup(firstBoot);
}

void loop() { projectMM::loop(); }
