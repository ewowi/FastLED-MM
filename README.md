<img width="640" height="360" alt="FastLED-MM" src="https://github.com/user-attachments/assets/ce71829c-8563-4d4b-8969-7d3a0e17cbe0" />

# FastLED-MM

FastLED effects and drivers running inside the [projectMM](https://github.com/ewowi/projectMM) module runtime.

## What this is

FastLED-MM is a PlatformIO project that wires two things together:

- **FastLED** — the pixel math and hardware driver library you already know. We use the latest master branch to get all the new features (e.g. [FastLED Audio](https://github.com/FastLED/FastLED/blob/master/src/fl/audio/README.md), Fixed point math, [FastLED Channels API](https://github.com/FastLED/FastLED/blob/master/src/fl/channels/README.md))
- **projectMM** — a module runtime that adds a web UI, live control, dual-core dispatch, and persistent state on top of your effect

You write an effect that fills a shared `CRGB leds[]` array, exactly like a regular FastLED sketch. projectMM handles the rest: WiFi, [ASP Async WebServer](https://github.com/ESP32Async/ESPAsyncWebServer), WebSocket state push, REST API, and per-module timing.

projectMM is a brand new project, as is FastLED-MM (April 2026) and are both under heavy development so expect more features soon. Like the repo and log a [FastLED-MM issue](https://github.com/MoonModules/FastLED-MM/issues) of you want to participate.

FastLED-MM is an example of using projectMM as a standalone library. Any ESP32 project can use projectMM to embed custom code in a full featured application. Log a [projectMM issue](https://github.com/ewowi/projectMM/issues) if you need help.

## What you get for free

| Feature | How |
|---------|-----|
| Web UI | Adjust effect controls live in the browser |
| WebSocket push | Control values update in the browser |
| Dual-core dispatch | Effect on Core 0, `FastLED.show()` on Core 1 |
| Persistent state | All changes made survive reboots (LittleFS) |
| REST API | `GET /api/modules`, `POST /api/control`, `GET /api/system` |
| Per-module timing | See exact frame cost per effect in the web UI |
| ESP32 devices | Currently ESP32dev and ESP32-S3, P4 follows soon |
| ESP-IDF 5.5 | Latest ESP32 firmware |

## Quick start

### 1. Configure hardware

Edit `main.cpp` (VSCode/PlatformIO) or `FastLED-MM.ino` (Arduino IDE):

```cpp
constexpr uint8_t  PIN      = 2;   // data pin
constexpr uint16_t WIDTH    = 16;  // panel width
constexpr uint16_t HEIGHT   = 16;  // panel height
```

### 2. Flash

Upload in VSCode platformIO or Arduino IDE

### 3. Connect

- Connect to the WiFi AP `MM-<device-id>` (password: `moonlight`)
- Open `http://4.3.2.1` in a browser
- Connect to your local network in the Network module

The `WaveRainbow2D` effect is running. Use the web UI to adjust parameters live.

## Adding your own effect

Create a new subclass of StatefulModule in `src/main.cpp` or `FastLED-MM.ino`:

```cpp
class MyEffect : public StatefulModule {
public:
    const char* name()     const override { return "MyEffect"; }
    const char* category() const override { return "effect"; }
    uint8_t     preferredCore() const override { return 0; }

    void setup() override {
        addControl(speed_, "speed", "range", 1.0f, 20.0f);
    }

    void loop() override {
        uint8_t hue = millis() / 10;
        for (uint16_t i = 0; i < NUM_LEDS; ++i)
            leds[i] = CHSV(hue + i, 255, 255);
    }

    void teardown() override {}
    void healthReport(char* buf, size_t len) const override { snprintf(buf, len, "ok"); }
    size_t classSize() const override { return sizeof(*this); }

private:
    float speed_ = 5.0f;
};
```

Then , register the effect:

```cpp
REGISTER_MODULE(MyEffect)
```

Optionally add the effect to run at boot in firstBoot()

```cpp
static void firstBoot(ModuleManager& mm) {
    //...
    mm.addModule("WaveRainbow2DEffect", "fx1",     ep, ep, 0, "");
    //...
}
```

You can also add it in the UI using create module

Flash and use the web UI to add your effect to the pipeline.

## Architecture

```
leds[]  (shared CRGB array in main.cpp)
     |
     +-- WaveRainbow2DEffect (Core 0) — writes pixels each tick
     |
     +-- FastLEDDriverModule  (Core 1) — calls FastLED.show() each tick
```

Both modules are `StatefulModule` subclasses managed by projectMM's `Scheduler` and `ModuleManager`. The shared array is the pixel handoff between effect and driver. In more advances scripts (as done in projectMM) double buffering, blending and such can be done to exploit parallelism

## Roadmap

- **Audio module**: a third `StatefulModule` that reads microphone data from the new FastLED Audio and publishes to `KvStore` for effects to react to. Can be dispatched in a separate task.
- **FastLED Channels API**: use the newer `FastLED::addLeds()` channels API when targeting multi-strip setups.
- Further tuning of `src/main.cpp` and `FastLED-MM.ino` moving any code which is not user-friendly into the projectMM library.

## Requirements

- PlatformIO or Arduino IDE
- ESP32 (classic using 1.75MB partitions or S3; see `platformio.ini` for both supported environments)
- FastLED-compatible LED strip (WS2812B by default; change in `FastLEDDriver.h`)
