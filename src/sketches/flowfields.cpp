// FastLED-MM — FastLED effects inside the projectMM module runtime.
//
// What you get for free by adding projectMM as a dependency:
//   Web UI on port 80, WebSocket state push, dual-core dispatch,
//   persistent settings (LittleFS), REST API, WiFi AP + STA.
//

#include <Arduino.h>
#include <FastLED.h>
#include <projectMM.h>

#include "FlowFieldsModule.h"  // v2 façade — replaces FlowFieldsEngine.h

constexpr uint8_t PIN = 2;       // data pin to the first LED strip
constexpr uint16_t WIDTH = 16;   // panel width in pixels
constexpr uint16_t HEIGHT = 16;  // panel height in pixels
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// XY mapping for this panel — replace with your own.
static uint32_t XY(uint16_t x, uint16_t y) { return (uint32_t)y * WIDTH + x; }

// Logical pixel array — written by effects in row-major order, read by PreviewModule.
CRGB leds[NUM_LEDS];

// ── Parameter ranges — one row per slider registered in the UI ──────
// Mirrors VISUALIZER_PARAMETER_REGISTRY in index.html.
// Used by registerControls() to add controls by name without hardcoding
// each addControl() call per emitter/flow.
struct ParamRange {
  const char* name;
  float lo, hi;
};
static const ParamRange PARAM_RANGES[] = {
    {"globalSpeed", 0.1f, 5.0f}, {"persistence", 0.01f, 2.0f}, {"colorShift", 0.0f, 1.0f}, {"numDots", 1.0f, 20.0f}, {"dotDiam", 0.5f, 5.0f}, {"orbitSpeed", 0.01f, 25.0f}, {"orbitDiam", 1.0f, 30.0f}, {"swarmSpeed", 0.01f, 5.0f}, {"swarmSpread", 0.0f, 1.0f}, {"lineSpeed", 0.01f, 5.0f}, {"lineAmp", 1.0f, 20.0f}, {"driftSpeed", 0.0f, 2.0f}, {"noiseScale", 0.001f, 0.2f}, {"noiseBand", 0.01f, 0.5f}, {"scale", 0.1f, 3.0f}, {"rotateSpeedX", -5.0f, 5.0f}, {"rotateSpeedY", -5.0f, 5.0f}, {"rotateSpeedZ", -5.0f, 5.0f}, {"jetForce", 0.0f, 2.0f}, {"jetAngle", -3.14f, 3.14f}, {"xSpeed", -5.0f, 5.0f}, {"ySpeed", -5.0f, 5.0f}, {"xShift", 0.0f, 5.0f}, {"yShift", 0.0f, 5.0f}, {"xFreq", 0.01f, 2.0f}, {"yFreq", 0.01f, 2.0f}, {"blendFactor", 0.0f, 1.0f}, {"innerSwirl", -1.0f, 1.0f}, {"outerSwirl", -1.0f, 1.0f}, {"midDrift", 0.0f, 1.0f}, {"angularStep", 0.0f, 1.0f}, {"viscosity", 0.0f, 0.01f}, {"vorticity", 0.0f, 20.0f}, {"gravity", -2.0f, 2.0f},
};

// ── Member-variable lookup for onUpdate dispatch ─────────────────────
// Maps parameter name to the address of the corresponding float member.
// Kept as a pointer so the table can reference members via offsetof.
struct Binding {
  const char* name;
  float* ptr;
};

class FlowFieldsEffect : public ProducerModule {
 public:
  const char* name() const override { return "FlowFields"; }
  const char* category() const override { return "source"; }
  uint8_t preferredCore() const override { return 0; }

  uint16_t pixelWidth() const override { return WIDTH; }
  uint16_t pixelHeight() const override { return HEIGHT; }

  void setup() override {
    declareBuffer(leds, NUM_LEDS, sizeof(CRGB));
    ff_.setup(WIDTH, HEIGHT, XY, leds, NUM_LEDS);
    rebuildControls();
  }

  // Called by projectMM when emitter/flow changes — shows only relevant sliders.
  void rebuildControls() override {
    clearControls();

    addControl(emitterIdx_, "emitter", "select");
    for (uint8_t i = 0; i < ff_.emitterCount(); i++) addControlValue(ff_.emitterName(i));

    addControl(flowIdx_, "flow", "select");
    for (uint8_t i = 0; i < ff_.flowCount(); i++) addControlValue(ff_.flowName(i));

    auto globals = ff_.getGlobalParams();
    for (uint8_t i = 0; i < globals.count; i++) addControlByName(globals.names[i]);

    auto ep = ff_.getEmitterParams(emitterIdx_);
    for (uint8_t i = 0; i < ep.count; i++) addControlByName(ep.names[i]);

    auto fp = ff_.getFlowParams(flowIdx_);
    for (uint8_t i = 0; i < fp.count; i++) addControlByName(fp.names[i]);
  }

  void onUpdate(const char* name) override {
    if (strcmp(name, "emitter") == 0) { ff_.setEmitter(emitterIdx_); rebuildControls(); return; }
    if (strcmp(name, "flow")    == 0) { ff_.setFlow(flowIdx_);        rebuildControls(); return; }

    for (const auto& b : bindings_) {
      if (strcasecmp(name, b.name) == 0) { ff_.setParameter(name, *b.ptr); return; }
    }
  }

  void loop()     override { ff_.loop(); }
  void teardown() override { ff_.teardown(); }
  size_t classSize() const override { return sizeof(*this); }

 private:
  FlowFieldsModule ff_;

  uint8_t emitterIdx_ = 0;
  uint8_t flowIdx_    = 0;
  float globalSpeed_   = 1.0f;
  float persistence_   = 0.05f;
  float colorShift_    = 0.20f;
  float numDots_       = 3.0f;
  float dotDiam_       = 1.5f;
  float orbitSpeed_    = 2.0f;
  float orbitDiam_     = 6.6f;
  float swarmSpeed_    = 0.5f;
  float swarmSpread_   = 0.5f;
  float lineSpeed_     = 0.35f;
  float lineAmp_       = 13.5f;
  float driftSpeed_    = 0.35f;
  float noiseScale_    = 0.0375f;
  float noiseBand_     = 0.1f;
  float scale_         = 1.0f;
  float rotateSpeedX_  = 0.6f;
  float rotateSpeedY_  = 0.9f;
  float rotateSpeedZ_  = 0.3f;
  float jetForce_      = 0.35f;
  float jetAngle_      = 0.0f;
  float xSpeed_        = 0.15f;
  float ySpeed_        = 0.15f;
  float xShift_        = 1.5f;
  float yShift_        = 1.5f;
  float xFreq_         = 0.33f;
  float yFreq_         = 0.32f;
  float blendFactor_   = 0.45f;
  float innerSwirl_    = -0.2f;
  float outerSwirl_    = 0.2f;
  float midDrift_      = 0.3f;
  float angularStep_   = 0.28f;
  float viscosity_     = 0.0005f;
  float vorticity_     = 7.0f;
  float gravity_       = 0.3f;

  const Binding bindings_[34] = {
      {"globalSpeed",  &globalSpeed_},  {"persistence",  &persistence_},
      {"colorShift",   &colorShift_},   {"numDots",      &numDots_},
      {"dotDiam",      &dotDiam_},      {"orbitSpeed",   &orbitSpeed_},
      {"orbitDiam",    &orbitDiam_},    {"swarmSpeed",   &swarmSpeed_},
      {"swarmSpread",  &swarmSpread_},  {"lineSpeed",    &lineSpeed_},
      {"lineAmp",      &lineAmp_},      {"driftSpeed",   &driftSpeed_},
      {"noiseScale",   &noiseScale_},   {"noiseBand",    &noiseBand_},
      {"scale",        &scale_},        {"rotateSpeedX", &rotateSpeedX_},
      {"rotateSpeedY", &rotateSpeedY_}, {"rotateSpeedZ", &rotateSpeedZ_},
      {"jetForce",     &jetForce_},     {"jetAngle",     &jetAngle_},
      {"xSpeed",       &xSpeed_},       {"ySpeed",       &ySpeed_},
      {"xShift",       &xShift_},       {"yShift",       &yShift_},
      {"xFreq",        &xFreq_},        {"yFreq",        &yFreq_},
      {"blendFactor",  &blendFactor_},  {"innerSwirl",   &innerSwirl_},
      {"outerSwirl",   &outerSwirl_},   {"midDrift",     &midDrift_},
      {"angularStep",  &angularStep_},  {"viscosity",    &viscosity_},
      {"vorticity",    &vorticity_},    {"gravity",      &gravity_},
  };

  // Look up name in PARAM_RANGES + bindings_ and call addControl.
  void addControlByName(const char* name) {
    for (const auto& r : PARAM_RANGES) {
      if (strcmp(name, r.name) != 0) continue;
      for (const auto& b : bindings_) {
        if (strcmp(name, b.name) == 0) {
          addControl(*b.ptr, name, "slider", r.lo, r.hi);
          return;
        }
      }
    }
  }
};

REGISTER_MODULE(FlowFieldsEffect)

class WaveRainbow2DEffect : public ProducerModule {
 public:
  const char* name() const override { return "WaveRainbow2D"; }
  const char* category() const override { return "source"; }
  const char* tags() const override { return "🔥🟦"; }
  uint8_t preferredCore() const override { return 0; }

  uint16_t pixelWidth() const override { return WIDTH; }    // asked for by previerModule
  uint16_t pixelHeight() const override { return HEIGHT; }  // asked for by previerModule

  void setup() override {
    declareBuffer(leds, NUM_LEDS, sizeof(CRGB));
    addControl(speed_, "speed", "slider", 1.0f, 10.0f);
    addControl(hueOffset_, "hue_offset", "slider", 0.0f, 255.0f);
  }

  void loop() override {
    uint8_t phase = static_cast<uint8_t>(millis() * static_cast<uint32_t>(speed_) >> 6);
    for (uint8_t y = 0; y < HEIGHT; ++y) {
      for (uint8_t x = 0; x < WIDTH; ++x) {
        uint8_t ripple = sin8((x << 4) + (y << 3) + phase);
        uint8_t hue = static_cast<uint8_t>(hueOffset_) + (x << 4) + (ripple >> 2);
        leds[XY(x, y)] = CHSV(hue, 240, 255);
      }
    }
  }

  size_t classSize() const override { return sizeof(*this); }

 private:
  float speed_ = 3.0f;
  float hueOffset_ = 0.0f;
};

REGISTER_MODULE(WaveRainbow2DEffect)

// ************* Consumer

// Physical pixel array — serpentine-remapped from leds[] by the driver before FastLED.show().
CRGB physLeds[NUM_LEDS];

// Serpentine index: odd rows run right→left; delegates to XY() for the base calculation.
inline uint32_t serpentineXY(uint16_t x, uint16_t y) { return XY(y & 1 ? (WIDTH - 1 - x) : x, y); }

class FastLEDDriverModule : public ConsumerModule {
 public:
  const char* name() const override { return "FastLEDDriver"; }
  const char* category() const override { return "driver"; }
  uint8_t preferredCore() const override { return 1; }

  void setup() override {
    FastLED.addLeds<WS2812B, PIN, GRB>(physLeds, NUM_LEDS);
    FastLED.setBrightness(static_cast<uint8_t>(brightness_));
    addControl(brightness_, "brightness", "slider", 0.0f, 255.0f);
  }

  void loop() override {
    // Remap logical row-major leds[] to physical serpentine physLeds[] before pushing.
    // Safe: AppSetup's semaphore ensures Core 0 has finished writing leds[] before this runs.
    for (uint16_t y = 0; y < HEIGHT; ++y)  // cache-friendly: 1.5 KB fits in L1; even rows write sequentially, odd rows reversed but within 2 hot cache lines
      for (uint16_t x = 0; x < WIDTH; ++x) physLeds[serpentineXY(x, y)] = leds[XY(x, y)];
    FastLED.show();
  }

  void teardown() override { FastLED.clear(true); }

  void onUpdate(const char* key) override {
    if (strcmp(key, "brightness") == 0) FastLED.setBrightness(static_cast<uint8_t>(brightness_));
  }

 private:
  float brightness_ = 10.0f;
};

REGISTER_MODULE(FastLEDDriverModule)

// Register projectMM PreviewModule
REGISTER_MODULE(PreviewModule)

// Called once on first boot to populate the default pipeline.
// Guard with hasModuleType so subsequent boots skip this.
static void firstBoot(ModuleManager& mm) {
  JsonDocument d;
  auto ep = d.as<JsonObjectConst>();

  // if (!pal::hasModuleType(mm, "WaveRainbow2DEffect")) {
  //   mm.addModule("WaveRainbow2DEffect", "fx1", ep, ep, 0, "");
  // }

  
  if (!pal::hasModuleType(mm, "FlowFieldsEffect")) {
    Serial.printf("Add FlowFieldsEffect\n");
    mm.addModule("FlowFieldsEffect", "fx1", ep, ep, 0, "");
  }

  if (!pal::hasModuleType(mm, "FastLEDDriverModule")) {
    Serial.printf("Add FastLEDDriverModule\n");
    mm.addModule("FastLEDDriverModule", "driver1", ep, ep, 1, "");
  }

  // Add PreviewModule wired to fx1 (WaveRainbow2DEffect / ProducerModule).
  if (!pal::hasModuleType(mm, "PreviewModule")) {
    JsonDocument inp;  // No width/height props needed: PreviewModule reads geometry from source via pixelBuf().
    inp["source"] = "fx1";
    Serial.printf("Add PreviewModule\n");
    mm.addModule("PreviewModule", "preview1", ep, inp.as<JsonObjectConst>(), 1, "");
  }

  mm.saveAllState();
}

void setup() {
  Serial.begin(115200);
  projectMM::setup(firstBoot);
}

void loop() { projectMM::loop(); }