// FastLED-MM — FastLED effects inside the projectMM module runtime.
//
// What you get for free by adding projectMM as a dependency:
//   - Web UI on port 80: adjust effect parameters live, add/remove modules
//   - WebSocket state push: see control values update in real time
//   - Dual-core dispatch: effect on Core 0, driver on Core 1
//   - Persistent state: settings survive reboots (stored on LittleFS)
//   - REST API: GET /api/modules, POST /api/control, GET /api/system
//
// To add your own effect:
//   1. Create MyEffect.h — subclass StatefulModule, write to flm_leds[]
//   2. Add  #include "MyEffect.h"  and  REGISTER_MODULE(MyEffect)  below
//   3. Flash and open the web UI to add it to the pipeline
//
// Note: this file will shrink significantly once projectMM's setup boilerplate
// moves to a reusable helper (tracked in projectMM R04S10).

#include <Arduino.h>
#include "core/Scheduler.h"
#include "core/ModuleManager.h"
#include "core/TypeRegistry.h"
#include "core/HttpServer.h"
#include "core/WsServer.h"
#include "core/Logger.h"
#include "pal/Pal.h"
#include "frontend/frontend_bundle.h"
#include <ArduinoJson.h>

#include "FlmPixels.h"
#include "WaveRainbow2D.h"
#include "FastLEDDriver.h"

// ---------- Shared pixel array (one definition) --------------------------------
CRGB flm_leds[FLM_NUM_LEDS];

// ---------- Register modules for the web UI and REST API ----------------------
// Add your own effects and drivers here.
REGISTER_MODULE(WaveRainbow2DEffect)
REGISTER_MODULE(FastLEDDriverModule)

// ---------- projectMM runtime -------------------------------------------------
Scheduler     scheduler;
ModuleManager mm(scheduler);
HttpServer    server(80);
WsServer      ws;

static void registerRoutes() {
    server.onGetStatic("/", "text/html", FRONTEND_HTML);

    server.onGet("/api/modules", [](const std::string&) {
        JsonDocument doc;
        mm.getModulesJson(doc.to<JsonArray>());
        std::string body; serializeJson(doc, body);
        return HttpResponse{200, "application/json", body};
    });

    server.onGet("/api/types", [](const std::string&) {
        JsonDocument doc;
        mm.getTypesJson(doc.to<JsonArray>());
        std::string body; serializeJson(doc, body);
        return HttpResponse{200, "application/json", body};
    });

    server.onPost("/api/control", [](const std::string& body) {
        JsonDocument doc;
        if (deserializeJson(doc, body))
            return HttpResponse{400, "application/json", R"({"ok":false,"error":"bad json"})"};
        bool ok = mm.setModuleControl(doc["id"] | "", doc["key"] | "",
                                      doc["value"].as<JsonVariantConst>());
        return HttpResponse{ok ? 200 : 404, "application/json",
                            ok ? R"({"ok":true})" : R"({"ok":false,"error":"not found"})"};
    });

    server.onGet("/api/system", [](const std::string&) {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        mm.fillSystemJson(root);
        mm.getSystemModulesJson(root["modules"].to<JsonArray>());
        std::string body; serializeJson(doc, body);
        return HttpResponse{200, "application/json", body};
    });
}

static void ensureDefaultPipeline() {
    // On first boot (no state file) add the default effect and driver.
    // On subsequent boots modulemanager.json restores the previous state.
    JsonDocument doc;
    mm.getModulesJson(doc.to<JsonArray>());
    if (doc.as<JsonArray>().size() == 0) {
        JsonDocument empty;
        auto ep = empty.as<JsonObjectConst>();
        mm.addModule("WaveRainbow2DEffect", "fx1",     ep, ep, 0, "");
        mm.addModule("FastLEDDriverModule", "driver1", ep, ep, 1, "");
        mm.saveAllState();
    }
}

// ---------- FreeRTOS tasks (dual-core) ----------------------------------------
static void* s_frameSem = nullptr;

static void effectsTask(void*) {
    for (;;) {
        { auto lk = mm.beginTick(); scheduler.loopCore(0); }
        pal::sem_give(s_frameSem);
        pal::yield();
    }
}

static void driverTask(void*) {
    uint32_t lastWsMs = 0;
    for (;;) {
        pal::sem_take(s_frameSem, 100);
        { auto lk = mm.beginTick(); scheduler.loopCore(1); scheduler.tickPeriodic(); }
        mm.flushIfDirty();
        ws.tick();

        uint32_t now = pal::millis();
        if (ws.hasClients() && now - lastWsMs >= 20) {
            lastWsMs = now;
            JsonDocument doc;
            mm.getStateJson(doc.to<JsonArray>());
            std::string msg; serializeJson(doc, msg);
            ws.broadcastText(msg);
        }
        pal::yield();
    }
}

// ---------- Arduino entry points ----------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    printf("[FastLED-MM] starting\n");
    pal::fs_begin();

    mm.setup();
    scheduler.setup();
    ensureDefaultPipeline();

    registerRoutes();
    server.begin();
    ws.begin();
    printf("[FastLED-MM] HTTP on :80  WS on :81\n");

    s_frameSem = pal::sem_binary_create();
    pal::task_create_pinned(effectsTask, "effects", 8192, nullptr, 1, 0);
    pal::task_create_pinned(driverTask,  "drivers", 8192, nullptr, 2, 1);
}

void loop() {
    pal::suspend_forever();
}
