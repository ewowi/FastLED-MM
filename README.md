# FastLED-MM

FastLED effects and drivers running inside the [projectMM](https://github.com/ewowi/projectMM) module runtime.

## What this is

FastLED-MM is a PlatformIO project that wires two things together:

- **FastLED** — the pixel math and hardware driver library you already know
- **projectMM** — a module runtime that adds a web UI, live control, dual-core dispatch, and persistent state on top of your effect

You write an effect that fills a shared `CRGB flm_leds[]` array, exactly like a regular FastLED sketch. projectMM handles the rest: WiFi AP, web server on port 80, WebSocket state push, REST API, and per-module timing.

## What you get for free

| Feature | How |
|---------|-----|
| Web UI on port 80 | Adjust `speed`, `hue_offset`, brightness live in the browser |
| WebSocket push | Control values update in the browser at 50 Hz |
| Dual-core dispatch | Effect on Core 0, `FastLED.show()` on Core 1 |
| Persistent state | Brightness, speed settings survive reboots (LittleFS) |
| REST API | `GET /api/modules`, `POST /api/control`, `GET /api/system` |
| Per-module timing | See exact frame cost per effect in the web UI |

## Quick start

### 1. Configure hardware

Edit `src/FlmConfig.h`:

```cpp
constexpr uint8_t  FLM_PIN      = 2;   // data pin
constexpr uint16_t FLM_WIDTH    = 16;  // panel width
constexpr uint16_t FLM_HEIGHT   = 16;  // panel height
```

### 2. Flash

```sh
pio run -t upload -e esp32dev
```

### 3. Connect

- Connect to the WiFi AP `MM-<device-id>` (password: `moonlight`)
- Open `http://192.168.4.1` in a browser

The `WaveRainbow2D` effect is running. Use the web UI to adjust parameters live.

## Adding your own effect

Create `src/MyEffect.h`:

```cpp
#pragma once
#include "core/StatefulModule.h"
#include "FlmPixels.h"

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
        for (uint16_t i = 0; i < FLM_NUM_LEDS; ++i)
            flm_leds[i] = CHSV(hue + i, 255, 255);
    }

    void teardown() override {}
    void healthReport(char* buf, size_t len) const override { snprintf(buf, len, "ok"); }
    size_t classSize() const override { return sizeof(*this); }

private:
    float speed_ = 5.0f;
};
```

Then in `src/main.cpp`, add two lines:

```cpp
#include "MyEffect.h"
REGISTER_MODULE(MyEffect)
```

Flash and use the web UI to add your effect to the pipeline.

## Architecture

```
flm_leds[]  (shared CRGB array in main.cpp)
     |
     +-- WaveRainbow2DEffect (Core 0) — writes pixels each tick
     |
     +-- FastLEDDriverModule  (Core 1) — calls FastLED.show() each tick
```

Both modules are `StatefulModule` subclasses managed by projectMM's `Scheduler` and `ModuleManager`. The shared array is the pixel handoff between effect and driver.

## Roadmap

- **ProducerModule / ConsumerModule base classes** (projectMM R05S05): replace the shared global array with typed base classes so the compiler enforces the producer/consumer contract.
- **`embeddedSetup()` helper** (projectMM R04S10): move projectMM boilerplate out of `main.cpp` so this file shrinks to ~15 lines.
- **Audio module**: a third `StatefulModule` that reads microphone data and publishes to `KvStore` for effects to react to.
- **FastLED Channels API**: use the newer `FastLED::addLeds()` channels API when targeting multi-strip setups.

## Requirements

- PlatformIO
- ESP32 (classic or S3; see `platformio.ini` for both environments)
- FastLED-compatible LED strip (WS2812B by default; change in `FastLEDDriver.h`)
