// FastLED-MM — FastLED effects inside the projectMM module runtime.
//
// What you get for free by adding projectMM as a dependency:
//   Web UI on port 80, WebSocket state push, dual-core dispatch,
//   persistent settings (LittleFS), REST API, WiFi AP + STA.
//
// To add your own effect:
//   1. Create src/effects/MyEffect.h — subclass ProducerModule, write to flm_leds[]
//   2. #include it below and add REGISTER_MODULE(MyEffect)
//   3. Flash and open the web UI to wire it into the pipeline

#include <Arduino.h>
#include <FastLED.h>
#include "core/AppSetup.h"
#include "core/Scheduler.h"
#include "core/ModuleManager.h"
#include "core/HttpServer.h"
#include "core/WsServer.h"
#include "FlmConfig.h"
#include "FlmPixels.h"
#include "effects/WaveRainbow2D.h"
#include "drivers/FastLEDDriver.h"

// Shared pixel array — written by effects, read by the driver.
CRGB flm_leds[FLM_NUM_LEDS];

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
