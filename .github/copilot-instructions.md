# GamePad Project - AI Coding Agent Instructions

## Project Overview
ESP32-S3 embedded Bluetooth gamepad controller with web UI, originally for converting Guitar Hero controllers to wireless BLE gamepads. Dual-architecture: C++/Arduino firmware + JavaScript/HTML web interface.

## Architecture & Key Components

### Dual-Core ESP32-S3 Design
- **Core 0**: Runs WiFi, Bluetooth, and background tasks
- **Core 1**: Main loop, LED processing (default), input handling
- FastLED updates run as separate FreeRTOS task (see [GamePad.cpp](src/GamePad.cpp#L78))

### Controller Configuration System
- Device-specific configs live in `src/devices/{360,DualArcadeStick,Peak,XPlorer Flash}/`
- Each device has `ControllerConfig.h` and `ControllerConfig.cpp`
- Active device selected in `include/DeviceConfig.h` via `#include` directive
- **Convention**: Controller configs define hardware only (pins, LEDs, buttons). No controller config should require code changes outside its directory.

### Profile System
- **Profiles** (multiple): User-selectable configurations per device (WiFi, BLE name, etc.)
- **Prefs** (singleton): Device-wide settings, boot count, statistics
- Prefs stored in NVS (Preferences API) + LittleFS for larger data like stats

### Web Server Architecture
- Dual servers: HTTP (port 80) + HTTPS (port 443) with self-signed certs
- Files served from LittleFS `/data/` folder: `*.html`, `*.js`, `*.css`
- Minified versions (`.min.*`) are production files
- HTML replacement system via `Web::HTMLReplacements` map for dynamic content injection
- See [Web.cpp](src/Web.cpp) for handler registration pattern

### Input Processing Pattern
- `Input` struct (see [Structs.h](include/Structs.h)) defines digital/analog inputs
- Each Input has:
  - Bluetooth operation pointers (press/release/setValue)
  - Optional custom operations (controller-specific)
  - LED config (onboard + external LED arrays)
  - Render operations (screen display)
  - Long-press child inputs (alternative actions on hold)
  - Statistics tracking pointers
- State machine tracked in `Input.ValueState` with debouncing (`DEBOUNCE_DELAY = 10ms`)

## Critical Build & Debug Workflows

### Building & Flashing
```powershell
# PlatformIO commands (not Arduino IDE - compilation is MUCH faster)
pio run                    # Build
pio run -t upload          # Flash firmware
pio run -t uploadfs        # Flash LittleFS filesystem (data/)
```

### Crash Debugging System
The project has a sophisticated crash capture system:

1. **Build flag**: `-Wl,--wrap=esp_panic_handler` wraps panic handler ([platformio.ini](platformio.ini#L18))
2. **Crash capture**: Custom `__wrap_esp_panic_handler` in [Debug.cpp](src/Debug.cpp) saves:
   - Full register dump to RTC memory (survives reboot)
   - Backtrace (up to 32 frames)
   - ELF SHA256 hash
   - Debug marks (breadcrumbs via `Debug::Mark()`)
3. **ELF archiving**: `archive_elf.py` post-build script saves ELF with SHA256 to `artifacts/elf/`
4. **Analysis**: Use `crash.ps1` PowerShell script:
   ```powershell
   .\crash.ps1 -LogPath crash.log  # Matches ELF by SHA256, runs addr2line
   ```
5. Crash logs stored in LittleFS, viewable via web interface at `/debug`

**Critical**: Always use `Debug::Mark()` breadcrumbs in complex code paths. Example:
```cpp
#ifdef DEBUG_MARKS
  Debug::Mark(500, __LINE__, __FILE__, __func__, "Bluetooth Connected");
#endif
```

### Serial Monitor Setup
- Baud: **921600** (defined in [Config.h](include/Config.h))
- Colored output enabled with `SERIAL_USE_COLOURS`
- Monitor filters: `esp32_exception_decoder, colorize` ([platformio.ini](platformio.ini#L40))
- **Must set**: `monitor_rts = 0` and `monitor_dtr = 0` to prevent reboot on serial connect

## Development Patterns & Conventions

### Conditional Compilation Guards
Extensive use of `#ifdef` for feature toggles in [Config.h](include/Config.h):
- `EXTRA_SERIAL_DEBUG` - Verbose logging (floods serial)
- `INCLUDE_BENCHMARKS` - Performance stats
- `DEBUG_MARKS` - Crash breadcrumbs (always use in production!)
- `STRAIGHT_TO_CONFIG_MENU` - Testing shortcut
- **Pattern**: Features not compiled = smaller/faster code

### Timing & Throttling
All timing uses **delta-time**, not frame-based:
- Display: `DISPLAY_UPDATE_RATE = 0.016` (~60 FPS)
- LED: `LED_UPDATE_RATE = 0.005` (~200 FPS for propagation effects)
- Stats: `SUB_SECOND_COUNT = 30` samples/second for click-rate calculations
- Never hardcode frame counts; always use time deltas

### LED System (FastLED)
- Array: `CRGB ExternalLeds[ExternalLED_Count]`
- Fading: `EXTERNAL_LED_FADE_RATE` applied per `LED_UPDATE_RATE` interval
- **Bottleneck**: `FastLED.show()` - this is why LED updates are on separate task
- LED configs use `ExternalLEDConfig` structs with ranges, colors, fade rates

### Web File Workflow
1. Edit source files in `data/` (e.g., `test.js`, `main.html`)
2. Minify manually or via build tool → `.min.` versions
3. Run `pio run -t uploadfs` to flash filesystem
4. Web server serves minified versions by default

### RREFont Custom Icons
- Icons defined in `include/CustomIcons.16x16.h` as bitmap data
- Icon mappings in `include/IconMappings.h` (e.g., `Icon_Web_Enabled = 0x14`)
- Font library patched: **Set `CONVERT_PL_CHARS = 0` in `RREFont.h`** or character mappings corrupt

### WiFi Testing Pattern
Auto-tests WiFi credentials when changed (see [WIFI_TEST_IMPLEMENTATION.md](WIFI_TEST_IMPLEMENTATION.md)):
- `Network::TestWiFiConnection()` runs async test
- Results displayed in menu with timeout (15s)
- Pattern: Non-blocking state machine, not `delay()` blocking

## File Organization

- `include/` - All headers (Config.h, Structs.h, Device.h, etc.)
- `src/` - Implementation files matching headers
- `src/devices/` - Controller-specific configs (change `include/DeviceConfig.h` to select)
- `data/` - Web UI files served via LittleFS
- `gfx/` - Source PBM files for custom icons
- `artifacts/elf/` - Auto-archived ELF files with SHA256 for crash debugging
- `storage/` - Runtime LittleFS mount point (crash logs, stats)

## Common Pitfalls

1. **Don't edit `include/DeviceConfig.h` content** - only change the `#include` path
2. **LittleFS changes require reflash** - `pio run -t uploadfs` after web file edits
3. **Core affinity matters** - LED task on Core 1 by default; moving to Core 0 can affect BLE stability
4. **Debug marks use RTC memory** - limited to 100 circular buffer (see `DebugMarkCount`)
5. **Profile ID** - Set `Input.ProfileId > 0` to include input only in specific profiles
6. **Long-press pattern** - Parent input must have `LongPressChildInput` pointer; child auto-gets `LongPressParentInput` back-reference

## Testing & Validation

- **Crash on purpose**: `Debug::CrashDeviceOnPurpose()` for testing crash capture
- **Benchmarks**: Enable `INCLUDE_BENCHMARKS` for FPS/latency stats
- **Test web page**: `data/test.html` has GPU fire simulation for WebGL testing
- **WiFi hotspot mode**: Boots into secure config AP if no WiFi configured

## External Dependencies

- ESP32-BLE-Gamepad: Bluetooth HID gamepad profile
- FastLED: WS2812B/Neopixel LED control
- Adafruit SH110X: OLED display driver (I2C)
- RREFont: Custom bitmap font rendering
- RapidJSON: JSON parsing for web API (in `lib/rapidjson-1.1.0/`)

## Quick Reference

- **VID/PID**: Xbox 360 `1430:4748` (see [Config.h](include/Config.h#L23))
- **Device naming**: Auto-generated from `DeviceNames[]` array + ESP32 chip ID
- **Serial output colors**: Use `Serial_INFO`, `Serial_ERROR` macros (see Utils.h)
- **Bluetooth naming**: `{FullDeviceName}` set in `GamePad.cpp` setup
