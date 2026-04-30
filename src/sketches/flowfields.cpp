// FastLED-MM — FastLED effects inside the projectMM module runtime.
//
// What you get for free by adding projectMM as a dependency:
//   Web UI on port 80, WebSocket state push, dual-core dispatch,
//   persistent settings (LittleFS), REST API, WiFi AP + STA.
//

#include <Arduino.h>
#include <FastLED.h>
#include <projectMM.h>

#include "FlowFieldsEngine.h"

constexpr uint8_t PIN = 2;       // data pin to the first LED strip
constexpr uint16_t WIDTH = 44;   // panel width in pixels
constexpr uint16_t HEIGHT = 44;  // panel height in pixels
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// XY mapping for this panel — replace with your own.
static uint32_t XY(uint16_t x, uint16_t y) { return (uint32_t)y * WIDTH + x; }

// Logical pixel array — written by effects in row-major order, read by PreviewModule.
CRGB leds[NUM_LEDS];

class FlowFieldsEffect : public ProducerModule {
 public:
  // ── Identity ─────────────────────────────────────────────────────
  const char* name() const override { return "FlowFields"; }
  const char* category() const override { return "source"; }
  uint8_t preferredCore() const override { return 0; }

  uint16_t pixelWidth() const override { return WIDTH; }
  uint16_t pixelHeight() const override { return HEIGHT; }

  // ── Setup ────────────────────────────────────────────────────────
  void setup() override {
    declareBuffer(leds, NUM_LEDS, sizeof(CRGB));

    // 1. Register controls — projectMM owns and updates these floats.
    addControl(emitterIdx_, "emitter", "select");
    for (int i = 0; i < EMITTER_COUNT; i++) addControlValue(EMITTER_NAMES[i]);

    addControl(flowIdx_, "flow", "select");
    for (int i = 0; i < FLOW_COUNT; i++) addControlValue(FLOW_NAMES[i]);

    addControl(globalSpeed_, "globalSpeed", "slider", 0.1f, 5.0f);
    addControl(persistence_, "persistence", "slider", 0.01f, 2.0f);
    addControl(colorShift_, "colorShift", "slider", 0.0f, 1.0f);
    addControl(numDots_, "numDots", "slider", 1.0f, 20.0f);
    addControl(dotDiam_, "dotDiam", "slider", 0.5f, 5.0f);
    addControl(orbitSpeed_, "orbitSpeed", "slider", 0.01f, 25.0f);
    addControl(orbitDiam_, "orbitDiam", "slider", 1.0f, 30.0f);
    addControl(swarmSpeed_, "swarmSpeed", "slider", 0.01f, 5.0f);
    addControl(swarmSpread_, "swarmSpread", "slider", 0.0f, 1.0f);
    addControl(lineSpeed_, "lineSpeed", "slider", 0.01f, 5.0f);
    addControl(lineAmp_, "lineAmp", "slider", 1.0f, 20.0f);
    addControl(driftSpeed_, "driftSpeed", "slider", 0.0f, 2.0f);
    addControl(noiseScale_, "noiseScale", "slider", 0.001f, 0.2f);
    addControl(noiseBand_, "noiseBand", "slider", 0.01f, 0.5f);
    addControl(scale_, "scale", "slider", 0.1f, 3.0f);
    addControl(rotateSpeedX_, "rotateSpeedX", "slider", -5.0f, 5.0f);
    addControl(rotateSpeedY_, "rotateSpeedY", "slider", -5.0f, 5.0f);
    addControl(rotateSpeedZ_, "rotateSpeedZ", "slider", -5.0f, 5.0f);
    addControl(jetForce_, "jetForce", "slider", 0.0f, 2.0f);
    addControl(jetAngle_, "jetAngle", "slider", -3.14f, 3.14f);
    addControl(xSpeed_, "xSpeed", "slider", -5.0f, 5.0f);
    addControl(ySpeed_, "ySpeed", "slider", -5.0f, 5.0f);
    addControl(xShift_, "xShift", "slider", 0.0f, 5.0f);
    addControl(yShift_, "yShift", "slider", 0.0f, 5.0f);
    addControl(xFreq_, "xFreq", "slider", 0.01f, 2.0f);
    addControl(yFreq_, "yFreq", "slider", 0.01f, 2.0f);
    addControl(blendFactor_, "blendFactor", "slider", 0.0f, 1.0f);
    addControl(innerSwirl_, "innerSwirl", "slider", -1.0f, 1.0f);
    addControl(outerSwirl_, "outerSwirl", "slider", -1.0f, 1.0f);
    addControl(midDrift_, "midDrift", "slider", 0.0f, 1.0f);
    addControl(angularStep_, "angularStep", "slider", 0.0f, 1.0f);
    addControl(viscosity_, "viscosity", "slider", 0.0f, 0.01f);
    addControl(vorticity_, "vorticity", "slider", 0.0f, 20.0f);
    addControl(gravity_, "gravity", "slider", -2.0f, 2.0f);

    // 2. Initialise the engine for this panel.
    engine_.setup(WIDTH, HEIGHT, NUM_LEDS, XY);

    // 3. Bind: redirect engine cVars to the floats projectMM updates.
    //    Called once here — run() pays only a pointer dereference per param.
    engine_.bindParam("emitter", &emitterIdxFloat_);
    engine_.bindParam("flow", &flowIdxFloat_);
    engine_.bindParam("globalSpeed", &globalSpeed_);
    engine_.bindParam("persistence", &persistence_);
    engine_.bindParam("colorShift", &colorShift_);
    engine_.bindParam("dotDiam", &dotDiam_);
    engine_.bindParam("orbitSpeed", &orbitSpeed_);
    engine_.bindParam("orbitDiam", &orbitDiam_);
    engine_.bindParam("swarmSpeed", &swarmSpeed_);
    engine_.bindParam("swarmSpread", &swarmSpread_);
    engine_.bindParam("lineSpeed", &lineSpeed_);
    engine_.bindParam("lineAmp", &lineAmp_);
    engine_.bindParam("driftSpeed", &driftSpeed_);
    engine_.bindParam("noiseScale", &noiseScale_);
    engine_.bindParam("noiseBand", &noiseBand_);
    engine_.bindParam("scale", &scale_);
    engine_.bindParam("rotateSpeedX", &rotateSpeedX_);
    engine_.bindParam("rotateSpeedY", &rotateSpeedY_);
    engine_.bindParam("rotateSpeedZ", &rotateSpeedZ_);
    engine_.bindParam("jetForce", &jetForce_);
    engine_.bindParam("jetAngle", &jetAngle_);
    engine_.bindParam("xSpeed", &xSpeed_);
    engine_.bindParam("ySpeed", &ySpeed_);
    engine_.bindParam("xShift", &xShift_);
    engine_.bindParam("yShift", &yShift_);
    engine_.bindParam("xFreq", &xFreq_);
    engine_.bindParam("yFreq", &yFreq_);
    engine_.bindParam("blendFactor", &blendFactor_);
    engine_.bindParam("innerSwirl", &innerSwirl_);
    engine_.bindParam("outerSwirl", &outerSwirl_);
    engine_.bindParam("midDrift", &midDrift_);
    engine_.bindParam("angularStep", &angularStep_);
    engine_.bindParam("viscosity", &viscosity_);
    engine_.bindParam("vorticity", &vorticity_);
    engine_.bindParam("gravity", &gravity_);

    onSizeChanged();  // (onSizeChanged not implemented for ProducerModule yet in projectMM)
  }

  void onSizeChanged() {  // override (onSizeChanged not implemented for ProducerModule yet in projectMM)
    engine_.teardown();
    engine_.setup(pixelWidth(), pixelHeight(), pixelWidth() * pixelHeight(), XY);
    // Bindings survive teardown/setup — they point into this object's
    // own member floats, not into the (now-freed) grid buffers.
  }

  void onUpdate(const char* key) override {
    if (strcmp(key, "emitter") == 0) {
      emitterIdxFloat_ = emitterIdx_;
    }
    if (strcmp(key, "flow") == 0) {
      flowIdxFloat_ = flowIdx_;
    }
  }

  // ── Hot path ─────────────────────────────────────────────────────
  void loop() override { engine_.run(leds); }

  void teardown() override { engine_.teardown(); }
  size_t classSize() const override { return sizeof(*this); }

 private:
  flowFields::FlowFieldsEngine engine_;

  // ── projectMM-owned floats — one per bound parameter ─────────────
  uint8_t emitterIdx_ = 0.0f;
  uint8_t flowIdx_ = 0.0f;
  float emitterIdxFloat_ = 0.0f;
  float flowIdxFloat_ = 0.0f;
  float globalSpeed_ = 1.0f;
  float persistence_ = 0.05f;
  float colorShift_ = 0.20f;
  float numDots_ = 3.0f;
  float dotDiam_ = 1.5f;
  float orbitSpeed_ = 2.0f;
  float orbitDiam_ = 6.6f;
  float swarmSpeed_ = 0.5f;
  float swarmSpread_ = 0.5f;
  float lineSpeed_ = 0.35f;
  float lineAmp_ = 13.5f;
  float driftSpeed_ = 0.35f;
  float noiseScale_ = 0.0375f;
  float noiseBand_ = 0.1f;
  float scale_ = 1.0f;
  float rotateSpeedX_ = 0.6f;
  float rotateSpeedY_ = 0.9f;
  float rotateSpeedZ_ = 0.3f;
  float jetForce_ = 0.35f;
  float jetAngle_ = 0.0f;
  float xSpeed_ = 0.15f;
  float ySpeed_ = 0.15f;
  float xShift_ = 1.5f;
  float yShift_ = 1.5f;
  float xFreq_ = 0.33f;
  float yFreq_ = 0.32f;
  float blendFactor_ = 0.45f;
  float innerSwirl_ = -0.2f;
  float outerSwirl_ = 0.2f;
  float midDrift_ = 0.3f;
  float angularStep_ = 0.28f;
  float viscosity_ = 0.0005f;
  float vorticity_ = 7.0f;
  float gravity_ = 0.3f;
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
    mm.addModule("FlowFieldsEffect", "fx1", ep, ep, 0, "");
  }

  if (!pal::hasModuleType(mm, "FastLEDDriverModule")) {
    mm.addModule("FastLEDDriverModule", "driver1", ep, ep, 1, "");
  }

  // Add PreviewModule wired to fx1 (WaveRainbow2DEffect / ProducerModule).
  if (!pal::hasModuleType(mm, "PreviewModule")) {
    JsonDocument inp;  // No width/height props needed: PreviewModule reads geometry from source via pixelBuf().
    inp["source"] = "fx1";
    mm.addModule("PreviewModule", "preview1", ep, inp.as<JsonObjectConst>(), 1, "");
  }

  mm.saveAllState();
}

void setup() {
  Serial.begin(115200);
  projectMM::setup(firstBoot);
}

void loop() { projectMM::loop(); }