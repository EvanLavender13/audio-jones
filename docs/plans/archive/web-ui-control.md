# Web UI Control Interface

HTTP server with WebSocket streaming for browser-based remote control of AudioJones visualizer.

## Goal

Enable remote control of AudioJones from any device with a web browser. Start with Audio panel (channel mode dropdown) and real-time analysis data streaming (beat intensity, band energies) to validate the architecture before migrating remaining panels.

This provides mobile/tablet control without native client requirements and enables future features like preset gallery, performance mode (visualizer-only fullscreen), and multi-instance monitoring.

## Current State

### Main Loop and Threading
- `src/main.cpp:145-200` - Single-threaded 60 FPS main loop
- `src/main.cpp:159` - Audio analysis runs every frame
- `src/main.cpp:162-165` - Visual updates at 20Hz using accumulator pattern
- Audio capture runs on separate miniaudio thread via lock-free ring buffer

### Configuration System
- `src/config/app_configs.h:12-22` - `AppConfigs` aggregates pointers to all config structs
- `src/audio/audio_config.h:4-15` - `AudioConfig` with `ChannelMode` enum (6 values: LEFT, RIGHT, MAX, MIX, SIDE, INTERLEAVED)
- `src/config/preset.cpp:11-32` - JSON serialization via nlohmann/json macros
- Pattern: POD config structs with in-class defaults, direct field access

### Analysis Data
- `src/analysis/beat.h:10-29` - `BeatDetector` with `beatIntensity` (0.0-1.0)
- `src/analysis/bands.h:17-32` - `BandEnergies` with bass/mid/treb smooth values
- `src/main.cpp:175` - Beat intensity accessed via `BeatDetectorGetIntensity()`

### UI Structure
- `src/ui/ui_main.cpp:121-153` - Five accordion panels: Analysis, Waveforms, Spectrum, Audio, Effects
- `src/ui/ui_panel_audio.cpp:6-30` - Audio panel: single dropdown for channel mode
- Both UIs will remain active simultaneously during migration

## Algorithm

### Threading Model

cpp-httplib runs HTTP server on its own thread pool. Main thread writes WebSocket messages at 20Hz (same rate as visual updates). Config updates queued via callback, applied at start of each frame.

```
┌─────────────────────────┐
│ Main Thread (60 FPS)    │
│ - Render loop           │
│ - Poll config queue     │◄──── config updates
│ - Push analysis data ───────► WebSocket broadcast
└─────────────────────────┘
         ▲
         │ callback
┌─────────────────────────┐
│ HTTP Server Thread      │
│ - Static file serving   │
│ - WebSocket connections │
│ - Parse incoming JSON   │
└─────────────────────────┘
```

### WebSocket Message Protocol

**Server → Client (Analysis Stream at 20Hz):**
```json
{
  "type": "analysis",
  "beat": 0.83,
  "bass": 1.24,
  "mid": 0.67,
  "treb": 0.45
}
```

**Server → Client (Config Snapshot on change):**
```json
{
  "type": "config",
  "audio": {"channelMode": 2}
}
```

**Client → Server (Config Update):**
```json
{
  "cmd": "setAudioChannel",
  "value": 2
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `beat` | float | Beat intensity 0.0-1.0 |
| `bass/mid/treb` | float | Normalized band energy (smoothed / avg) |
| `channelMode` | int | 0=Left, 1=Right, 2=Max, 3=Mix, 4=Side, 5=Interleaved |

### Config Update Flow

1. Browser sends JSON command via WebSocket
2. cpp-httplib thread parses JSON, validates command
3. Command queued to thread-safe vector (mutex protected)
4. Main thread polls queue at start of frame
5. Config struct updated, takes effect immediately

## Integration

### 1. Add WebServer to AppContext

**File:** `src/main.cpp:22-38`

```cpp
typedef struct AppContext {
    // ... existing fields ...
    WebServer* webServer;
} AppContext;
```

### 2. Initialize in AppContextInit

**File:** `src/main.cpp:74-94`

```cpp
INIT_OR_FAIL(ctx->webServer, WebServerInit("web", 8080));

// Setup callback after configs are ready
AppConfigs configs = { /* ... */ };
WebServerSetup(ctx->webServer, &configs);
```

### 3. Main Loop Integration

**File:** `src/main.cpp:145-200`

```cpp
while (!WindowShouldClose()) {
    const float deltaTime = GetFrameTime();

    // Process web commands at start of frame
    WebServerProcessCommands(ctx->webServer);

    // ... existing analysis and render code ...

    // Broadcast analysis at 20Hz (reuse existing accumulator)
    if (ctx->waveformAccumulator >= waveformUpdateInterval) {
        UpdateVisuals(ctx);
        WebServerBroadcastAnalysis(ctx->webServer,
                                    &ctx->analysis.beat,
                                    &ctx->analysis.bands);
        ctx->waveformAccumulator = 0.0f;
    }
}
```

### 4. Cleanup

**File:** `src/main.cpp:40-64`

```cpp
if (ctx->webServer != NULL) {
    WebServerUninit(ctx->webServer);
}
```

## Component Design

### WebServer Module

**Files:** `src/web/web_server.h`, `src/web/web_server.cpp`

```cpp
typedef struct WebServer WebServer;

WebServer* WebServerInit(const char* webRoot, int port);
void WebServerUninit(WebServer* server);

// Setup routes and config pointer (call after configs ready)
void WebServerSetup(WebServer* server, AppConfigs* configs);

// Process queued commands (call at start of frame)
void WebServerProcessCommands(WebServer* server);

// Broadcast analysis to all WebSocket clients
void WebServerBroadcastAnalysis(WebServer* server,
                                 const BeatDetector* beat,
                                 const BandEnergies* bands);
```

**Responsibilities:**
- Start HTTP server on port 8080
- Serve static files from `web/` directory
- Handle WebSocket connections at `/ws`
- Queue incoming commands for main thread
- Broadcast analysis JSON to connected clients

**Dependencies:** cpp-httplib (header-only), nlohmann/json

### WebBridge Module

**Files:** `src/web/web_bridge.h`, `src/web/web_bridge.cpp`

```cpp
// Serialize analysis state to JSON
void WebBridgeSerializeAnalysis(const BeatDetector* beat,
                                 const BandEnergies* bands,
                                 char* outJson, int maxLen);

// Parse and apply config command
// Returns true if valid command applied
bool WebBridgeApplyCommand(AppConfigs* configs, const char* json);

// Serialize current config to JSON (for new client sync)
void WebBridgeSerializeConfig(const AppConfigs* configs,
                               char* outJson, int maxLen);
```

**Responsibilities:**
- JSON serialization/deserialization
- Command parsing and validation
- Config application

### Web Frontend

**Files:** `web/index.html`, `web/app.js`, `web/style.css`

Single-page app with:
- Audio panel: Channel mode dropdown (6 options)
- Analysis display: Beat intensity bar, band energy meters
- WebSocket auto-reconnect on disconnect
- Mobile-responsive layout

## File Changes

| File | Change |
|------|--------|
| `src/web/web_server.h` | Create - WebServer API |
| `src/web/web_server.cpp` | Create - cpp-httplib HTTP/WebSocket implementation |
| `src/web/web_bridge.h` | Create - JSON serialization API |
| `src/web/web_bridge.cpp` | Create - Message parsing and config application |
| `src/main.cpp` | Modify - Add WebServer to AppContext, init/process/uninit calls |
| `CMakeLists.txt` | Modify - Add cpp-httplib dependency, WEB_SOURCES |
| `web/index.html` | Create - Audio panel UI with analysis display |
| `web/app.js` | Create - WebSocket client and event handlers |
| `web/style.css` | Create - Mobile-responsive styling |

## Build Sequence

### Phase 1: Server Infrastructure

- [x] Add cpp-httplib to CMakeLists.txt via FetchContent
- [x] Create `src/web/web_server.h` with API declarations
- [x] Create `src/web/web_server.cpp` with HTTP server (static files only)
- [x] Create `src/web/web_bridge.h` with serialization API
- [x] Create `src/web/web_bridge.cpp` stub (analysis serialization)
- [x] Add WEB_SOURCES to CMakeLists.txt
- [x] Add WebServer to AppContext in main.cpp
- [x] Build and verify server starts on port 8080

### Phase 2: WebSocket Streaming

- [x] Add WebSocket endpoint `/ws` to web_server.cpp
- [x] Implement `WebBridgeSerializeAnalysis()`
- [x] Implement `WebServerBroadcastAnalysis()`
- [x] Call broadcast in main loop at 20Hz
- [x] Create `web/index.html` with placeholder content
- [x] Create `web/app.js` with WebSocket connection
- [x] Verify analysis data streams to browser console

### Phase 3: Config Updates

- [x] Implement command queue in web_server.cpp
- [x] Implement `WebBridgeApplyCommand()` for audio channel
- [x] Implement `WebServerProcessCommands()`
- [x] Add channel mode dropdown to web/index.html
- [x] Wire dropdown change → WebSocket send in app.js
- [ ] Verify channel mode changes from browser update visualizer

### Phase 4: Polish

- [x] Create `web/style.css` with dark theme
- [x] Add beat intensity bar visualization
- [x] Add band energy meters (bass/mid/treb)
- [x] Add WebSocket reconnection logic
- [x] Add config broadcast on new client connect
- [x] ~~Test bidirectional sync (desktop ↔ web)~~ Skipped - replacing raygui with web UI

## Validation

- [ ] Server starts on port 8080 without blocking render loop
- [ ] Application maintains 60 FPS with server active
- [ ] `http://localhost:8080/` serves web/index.html
- [ ] WebSocket connects at `ws://localhost:8080/ws`
- [ ] Analysis data streams at ~20Hz (beat, bass, mid, treb)
- [ ] Channel mode dropdown shows all 6 options
- [ ] Changing dropdown updates visualizer immediately
- [ ] Changing desktop UI updates web dropdown
- [ ] Multiple browser tabs can connect simultaneously
- [ ] WebSocket reconnects after temporary disconnect
- [ ] Server shuts down cleanly on app exit

## Future Extensions

After validating with Audio panel:

1. **Waveform panel** - List management, sliders, color picker
2. **Spectrum panel** - Toggle, sliders, rotation controls
3. **Effects panel** - All effect sliders, conditional sections
4. **Analysis panel** - Read-only display matching desktop
5. **Preset panel** - Load/save, preset gallery

Each panel follows same pattern:
1. Add commands to `WebBridgeApplyCommand()`
2. Add HTML controls to index.html
3. Wire JS event handlers in app.js

## References

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - Header-only HTTP/WebSocket server
- [WebSocket RFC 6455](https://tools.ietf.org/html/rfc6455) - Protocol specification
- Existing JSON serialization: `src/config/preset.cpp:11-32`
