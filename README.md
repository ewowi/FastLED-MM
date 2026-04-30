<img width="640" height="360" alt="FastLED-MM" src="https://github.com/user-attachments/assets/ce71829c-8563-4d4b-8969-7d3a0e17cbe0" />

# FastLED-MM

FastLED effects and drivers running inside the [projectMM](https://github.com/ewowi/projectMM) module runtime on ESP32 devices.

## What this is

FastLED-MM is a PlatformIO project that wires two things together:

- **FastLED** — the pixel math and hardware driver library you already know. We use the latest master branch (The to be released FastLED 4 version) to get all the new features (e.g. [FastLED Audio](https://github.com/FastLED/FastLED/blob/master/src/fl/audio/README.md), Fixed point math, [FastLED Channels API](https://github.com/FastLED/FastLED/blob/master/src/fl/channels/README.md))
- **projectMM** — a module runtime that adds a web UI, live control, dual-core dispatch, and persistent state on top of your effect

You write an effect that fills a shared `CRGB leds[]` array, exactly like a regular FastLED sketch. projectMM handles the rest: WiFi, [ASP Async WebServer](https://github.com/ESP32Async/ESPAsyncWebServer), WebSocket state push, REST API, and per-module timing.

projectMM is a brand new project, as is FastLED-MM (April 2026) and are both under heavy development so expect more features soon. So don't expect evrything to work fluently yet. Like the repo and log a [FastLED-MM issue](https://github.com/MoonModules/FastLED-MM/issues) of you want to participate.

FastLED-MM is an example of using projectMM as a standalone library. Any ESP32 project can use projectMM to embed custom code in a full featured application. Log a [projectMM issue](https://github.com/ewowi/projectMM/issues) if you need help.

If you like projectMM or FastLED-MM, give it a ⭐️, fork it or open an issue or pull request. It helps the projects grow, improve and get noticed.

## What you get for free

| Feature | How |
|---------|-----|
| WiFi  support | add your home network credentials in the UI and the device joins it; the AP stays up as a fallback |
| Web UI | Adjust effect controls live in the browser |
| Live LED preview | see your panel animate in the browser before looking at the hardware |
| Log panel | real-time serial output in the browser tab, no USB required |
| Art-Net | receive and send Art-Net packages over the network and use in your FastLED pipeline |
| FastLED audio integration | Run the brand new FastLED audio |
| ESP32 devices | Currently ESP32dev and ESP32-S3, P4 follows soon |
| WebSocket push | Control values update in the browser |
| Dual-core dispatch | Effect on Core 0, `FastLED.show()` on Core 1 |
| Persistent state | All changes made survive reboots (LittleFS) |
| REST API | `GET /api/modules`, `POST /api/control`, `GET /api/system` |
| Per-module timing | See exact frame cost per effect in the web UI |
| ESP-IDF 5.5 | Latest ESP32 firmware |
| Much more | Much more to come |

Read the [user guide](https://ewowi.github.io/projectMM/user-guide/getting-started/) for more info

## Sketches

All runnable examples live in `src/sketches/`. Each sketch is a self-contained `.cpp` file with its own effect classes, panel config, and `setup()`/`loop()`.

| Sketch | Description | Panel |
|--------|-------------|-------|
| `minimal.cpp` | WaveRainbow2D effect with serpentine panel support | 16×16 |
| `flowfields.cpp` | FlowFields engine with full parameter set | 44×44 |

### Selecting a sketch in PlatformIO

`platformio.ini` has one environment per sketch. Pick the one matching your hardware and use **PlatformIO: Set Default Environment** or set `default_envs` in `platformio.ini`:

| Environment | Sketch | Board |
|-------------|--------|-------|
| `minimal` | minimal.cpp | esp32dev |
| `flowfields` | flowfields.cpp | esp32dev |
| `flowfields_s3` | flowfields.cpp | ESP32-S3 (16 MB) |

### Selecting a sketch in Arduino IDE

`FastLED-MM.ino` includes one sketch at a time. Swap the include at the bottom of the file:

```cpp
#include "src/sketches/minimal.cpp"     // simple 16x16 wave effect
// #include "src/sketches/flowfields.cpp"  // full FlowFields engine
```

### Adding your own sketch

1. Create `src/sketches/mysketch.cpp` — use `minimal.cpp` as a starting point.
2. Set `PIN`, `WIDTH`, `HEIGHT` and `NUM_LEDS` at the top to match your panel.
3. Add an environment to `platformio.ini`:

```ini
[env:mysketch]
board = esp32dev
src_filter = -<*> +<sketches/mysketch.cpp>
board_build.arduino.partitions = partitions/esp32dev.csv
```

## Quick start

### 1. Configure hardware

Open the sketch you want to use (`src/sketches/minimal.cpp` or another) and set the panel constants at the top:

```cpp
constexpr uint8_t  PIN    = 2;   // data pin
constexpr uint16_t WIDTH  = 16;  // panel width
constexpr uint16_t HEIGHT = 16;  // panel height
```

When using the new FastLED channels API, this can also be added in a Module and can be set in runtime!

### 2. Flash

Select the matching environment in PlatformIO and upload, or use Arduino IDE.

### 3. Connect

- Connect to the WiFi AP `MM-<device-id>` (password: `moonlight`)
- Open `http://4.3.2.1` in a browser
- Connect to your local network in the Network module

The effect from the selected sketch is running. Use the web UI to adjust parameters live.

## Adding your own effect

Create a new subclass of `ProducerModule` inside your sketch file:

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
src/sketches/minimal.cpp  (or flowfields.cpp, or your own)
     |
     +-- leds[]      — logical pixel array, row-major, read by PreviewModule
     +-- physLeds[]  — physical pixel array, serpentine-remapped for the strip
     |
     +-- YourEffect        (Core 0) — writes leds[] each tick
     +-- FastLEDDriverModule (Core 1) — remaps leds[]→physLeds[], calls FastLED.show()
     +-- PreviewModule     (Core 1) — streams leds[] to the browser over WebSocket
```

Each sketch is a self-contained `.cpp` selected at build time via `src_filter` in `platformio.ini`. The effect and driver are `StatefulModule` subclasses managed by projectMM's `Scheduler`. A semaphore in `AppSetup` ensures Core 0 finishes writing `leds[]` before Core 1 reads it — no extra locking needed in your effect code.

## Roadmap

- **Audio module**: a third `StatefulModule` that reads microphone data from the new FastLED Audio and publishes to `KvStore` for effects to react to. Can be dispatched in a separate task.
- **FastLED Channels API**: use the newer `FastLED::addLeds()` channels API when targeting multi-strip setups.
- Further tuning of `src/sketches/` and `FastLED-MM.ino` moving any code which is not user-friendly into the projectMM library.

## Requirements

- PlatformIO or Arduino IDE
- ESP32 (classic using 1.75MB partitions or S3; see `platformio.ini` for both supported environments)
- FastLED-compatible LED strip (WS2812B by default; change in `FastLEDDriver.h`)
