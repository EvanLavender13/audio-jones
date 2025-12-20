# Web Preset Panel

Migrate preset management (list, load, save, delete) from raygui to the web UI while keeping the native panel functional.

## Goal

Enable full preset control from the browser while maintaining the existing native raygui preset panel. Behavior matches raygui exactly: individual field changes take effect immediately, loading a preset overrides everything immediately. The web UI will eventually completely replace raygui.

**Key principle:** Presets are the sync boundary. Individual config commands modify the visualizer in real-time. Loading a preset broadcasts full config state so all web UI controls update.

## Current State

### Native Preset Panel

**`src/ui/ui_panel_preset.cpp:9-98`** - raygui preset panel:
- `PresetPanelState` (line 9-17): file list, selection, scroll position, name input
- `PresetPanelInit()` (line 19): calls `PresetListFiles()` for initial list
- Save button (line 59-69): creates `Preset`, calls `PresetFromAppConfigs()` + `PresetSave()`
- Auto-load (line 84-95): triggers on selection change via `PresetLoad()` + `PresetToAppConfigs()`

### Preset Core

**`src/config/preset.cpp:81-153`** - stateless file I/O:
- `PresetSave(preset, filepath)` (line 81): JSON serialization, returns `bool`
- `PresetLoad(preset, filepath)` (line 95): JSON deserialization, returns `bool`
- `PresetListFiles(directory, outFiles, maxFiles)` (line 109): lists `.json` files
- `PresetFromAppConfigs(preset, configs)` (line 133): copy live config to preset
- `PresetToAppConfigs(preset, configs)` (line 144): apply preset to live config

### Web Bridge

**`src/web/web_bridge.cpp:48-80`** - command processing:
- `WebBridgeApplyCommand(configs, jsonStr)`: parses JSON, executes commands
- Currently only handles `setAudioChannel` command
- Returns `bool` success

**`src/web/web_bridge.cpp:82-98`** - config sync:
- `WebBridgeSerializeConfig()`: sends config snapshot on client connect
- Currently only serializes `audio.channelMode`

### Web UI

**`web/app.js:6-47`** - Alpine.js store:
- Reactive state + computed getters + action methods
- `updateConfig()`: applies server config
- `window.sendCommand()`: sends JSON via WebSocket

**`web/index.html:28-52`** - panel pattern:
- `<section class="panel">` with header and content
- `x-model` bindings for two-way data
- `@change` handlers trigger store actions

## Algorithm

### Command Protocol

Four commands for preset operations:

| Command | Parameter | Backend Action |
|---------|-----------|----------------|
| `presetList` | none | Call `PresetListFiles()`, return file list |
| `presetLoad` | `filename` | `PresetLoad()` + `PresetToAppConfigs()` |
| `presetSave` | `name` | `PresetFromAppConfigs()` + `PresetSave()` |
| `presetDelete` | `filename` | `std::filesystem::remove()` |

### Response Format

New message type `presetStatus`:
```json
{
  "type": "presetStatus",
  "success": true,
  "message": "Loaded Cyberpunk.json",
  "presets": ["Default.json", "Cyberpunk.json", "Bass Heavy.json"]
}
```

Status message displays for 3 seconds then clears. Preset list always included for immediate sync.

### Frontend State

```javascript
Alpine.store('app', {
    // Preset state
    presets: [],              // ["Default.json", ...]
    selectedPreset: null,     // Currently loaded preset
    presetName: '',           // Name input for save
    presetStatus: '',         // Status message

    // Actions
    loadPreset(filename) {
        window.sendCommand('presetLoad', filename);
    },
    savePreset() {
        if (!this.presetName.trim()) return;
        window.sendCommand('presetSave', this.presetName);
    },
    deletePreset(filename) {
        if (!confirm(`Delete ${filename}?`)) return;
        window.sendCommand('presetDelete', filename);
    },
    refreshPresets() {
        window.sendCommand('presetList', null);
    },
    updatePresets(data) {
        if (data.presets) this.presets = data.presets;
        if (data.message) {
            this.presetStatus = data.message;
            setTimeout(() => { this.presetStatus = ''; }, 3000);
        }
        if (data.success && data.message?.startsWith('Loaded')) {
            // Extract preset name from "Loaded X.json"
            this.selectedPreset = data.message.replace('Loaded ', '');
        }
    }
})
```

## Architecture Decision

**Approach:** Pragmatic - dedicated `presetStatus` message type with four named commands, plus full config broadcast after preset load.

**Rationale:**
1. Clear protocol separation from config sync messages
2. Status + preset list combined in one response (efficiency)
3. Self-documenting command names
4. Minimal changes to existing infrastructure
5. Future-extensible for additional preset operations
6. Full config broadcast after load ensures web UI controls sync (matches raygui behavior)

**Key behavior:**
- Individual field commands (e.g., `setAudioChannel`) work exactly like raygui sliders - immediate effect
- Loading a preset overrides ALL config and broadcasts full state to web clients
- Web UI controls update to reflect loaded preset values

## Component Design

### WebBridge: Full Config Serialization

**File:** `src/web/web_bridge.cpp`

Replace `WebBridgeSerializeConfig()` to use existing preset serialization:

```cpp
void WebBridgeSerializeConfig(const AppConfigs* configs,
                               char* outJson, int maxLen)
{
    if (configs == NULL) {
        SerializeJsonToBuffer(json({}), outJson, maxLen);
        return;
    }

    // Reuse preset serialization - it already has to_json() for all config types
    Preset p;
    PresetFromAppConfigs(&p, configs);

    json msg = {
        {"type", "config"},
        {"preset", p}  // Uses existing to_json(json&, const Preset&)
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}
```

This reuses the existing `to_json()` from `preset.cpp:35-46` which already serializes all config structs. Buffer size needs increase from 512 to 8192 bytes.

### WebBridge: Preset Commands

**File:** `src/web/web_bridge.h`

Add declaration:
```cpp
// Serialize preset operation status with current file list
void WebBridgeSerializePresetStatus(bool success,
                                     const char* message,
                                     char outFiles[][PRESET_PATH_MAX],
                                     int fileCount,
                                     char* outJson,
                                     int maxLen);
```

**File:** `src/web/web_bridge.cpp`

Add includes:
```cpp
#include <filesystem>
#include "config/preset.h"
```

Implement `WebBridgeSerializePresetStatus()`:
```cpp
void WebBridgeSerializePresetStatus(bool success,
                                     const char* message,
                                     char outFiles[][PRESET_PATH_MAX],
                                     int fileCount,
                                     char* outJson,
                                     int maxLen)
{
    json presetArray = json::array();
    for (int i = 0; i < fileCount; i++) {
        presetArray.push_back(outFiles[i]);
    }

    json msg = {
        {"type", "presetStatus"},
        {"success", success},
        {"message", message ? message : ""},
        {"presets", presetArray}
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}
```

Extend `WebBridgeApplyCommand()` with preset handlers:
```cpp
if (cmd == "presetList") {
    // No parameters needed, just trigger status broadcast
    return true;
}
else if (cmd == "presetLoad") {
    if (!msg.contains("filename")) return false;
    std::string filename = msg["filename"].get<std::string>();
    char filepath[PRESET_PATH_MAX];
    snprintf(filepath, sizeof(filepath), "presets/%s", filename.c_str());
    Preset p;
    if (!PresetLoad(&p, filepath)) return false;
    PresetToAppConfigs(&p, configs);
    return true;
}
else if (cmd == "presetSave") {
    if (!msg.contains("name")) return false;
    std::string name = msg["name"].get<std::string>();
    if (name.empty()) return false;
    char filepath[PRESET_PATH_MAX];
    snprintf(filepath, sizeof(filepath), "presets/%s.json", name.c_str());
    Preset p;
    strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
    p.name[PRESET_NAME_MAX - 1] = '\0';
    PresetFromAppConfigs(&p, configs);
    return PresetSave(&p, filepath);
}
else if (cmd == "presetDelete") {
    if (!msg.contains("filename")) return false;
    std::string filename = msg["filename"].get<std::string>();
    char filepath[PRESET_PATH_MAX];
    snprintf(filepath, sizeof(filepath), "presets/%s", filename.c_str());
    std::error_code ec;
    return std::filesystem::remove(filepath, ec);
}
```

### WebServer: Preset Broadcast

**File:** `src/web/web_server.h`

Add declaration:
```cpp
// Broadcast preset status to all connected clients
void WebServerBroadcastPresetStatus(WebServer* server,
                                     bool success,
                                     const char* message);
```

**File:** `src/web/web_server.cpp`

Implement broadcast function:
```cpp
void WebServerBroadcastPresetStatus(WebServer* server,
                                     bool success,
                                     const char* message)
{
    if (server == NULL || !server->running) return;

    char files[MAX_PRESET_FILES][PRESET_PATH_MAX];
    int count = PresetListFiles("presets", files, MAX_PRESET_FILES);

    char json[4096];
    WebBridgeSerializePresetStatus(success, message, files, count, json, sizeof(json));
    BroadcastToClients(server->wsServer, json);
}
```

Modify `WebServerProcessCommands()` to broadcast after preset operations:
```cpp
void WebServerProcessCommands(WebServer* server)
{
    // ... existing queue drain logic ...

    for (const std::string& cmdStr : commands) {
        try {
            json msg = json::parse(cmdStr);
            if (!msg.contains("cmd")) continue;

            std::string cmd = msg["cmd"].get<std::string>();
            bool success = WebBridgeApplyCommand(server->configs, cmdStr.c_str());

            // Broadcast status for preset commands
            if (cmd == "presetList" || cmd == "presetLoad" ||
                cmd == "presetSave" || cmd == "presetDelete") {
                const char* message = NULL;
                if (cmd == "presetLoad" && success) {
                    static char loadMsg[128];
                    snprintf(loadMsg, sizeof(loadMsg), "Loaded %s",
                             msg["filename"].get<std::string>().c_str());
                    message = loadMsg;

                    // Broadcast full config so web UI controls update
                    char configJson[8192];
                    WebBridgeSerializeConfig(server->configs, configJson, sizeof(configJson));
                    BroadcastToClients(server->wsServer, configJson);
                } else if (cmd == "presetSave" && success) {
                    static char saveMsg[128];
                    snprintf(saveMsg, sizeof(saveMsg), "Saved %s.json",
                             msg["name"].get<std::string>().c_str());
                    message = saveMsg;
                } else if (cmd == "presetDelete" && success) {
                    static char deleteMsg[128];
                    snprintf(deleteMsg, sizeof(deleteMsg), "Deleted %s",
                             msg["filename"].get<std::string>().c_str());
                    message = deleteMsg;
                } else if (!success) {
                    message = "Operation failed";
                }
                WebServerBroadcastPresetStatus(server, success, message);
            }
        } catch (...) {
            continue;
        }
    }
}
```

**Key addition:** After successful `presetLoad`, broadcast full config via `WebBridgeSerializeConfig()` so all web UI controls update to reflect loaded preset values.

Add initial preset list on client connect (after config send):
```cpp
// In WebSocket Open handler, after sending config:
WebServerBroadcastPresetStatus(server, true, NULL);  // NULL message = silent list update
```

### Frontend: Alpine Store

**File:** `web/app.js`

Add preset state and config state to store (after line 17):
```javascript
// Preset state
presets: [],
selectedPreset: null,
presetName: '',
presetStatus: '',

// Config state (populated on preset load, used by future control panels)
effects: {},
waveformCount: 1,
waveforms: [],
spectrum: {},
bands: {},
```

Add action methods (after line 46):
```javascript
loadPreset(filename) {
    if (!this.connected) return;
    window.sendCommand('presetLoad', filename);
},

savePreset() {
    if (!this.connected) return;
    const name = this.presetName.trim();
    if (!name) return;
    window.sendCommand('presetSave', name);
},

deletePreset(filename) {
    if (!this.connected) return;
    if (!confirm('Delete ' + filename + '?')) return;
    window.sendCommand('presetDelete', filename);
},

refreshPresets() {
    if (!this.connected) return;
    window.sendCommand('presetList', null);
},

updatePresets(data) {
    if (data.presets) {
        this.presets = data.presets;
    }
    if (data.message) {
        this.presetStatus = data.message;
        setTimeout(() => { this.presetStatus = ''; }, 3000);
    }
    if (data.success && data.message && data.message.startsWith('Loaded ')) {
        this.selectedPreset = data.message.substring(7);
    }
},
```

Expand `updateConfig()` to apply full preset data (replace existing implementation):
```javascript
updateConfig(config) {
    // Full preset data comes nested under 'preset' key
    const p = config.preset;
    if (!p) return;

    // Audio
    if (p.audio) {
        this.channelMode = p.audio.channelMode;
    }

    // Effects
    if (p.effects) {
        this.effects = p.effects;  // Store entire effects object
    }

    // Waveforms
    if (p.waveforms && p.waveformCount != null) {
        this.waveformCount = p.waveformCount;
        this.waveforms = p.waveforms.slice(0, p.waveformCount);
    }

    // Spectrum
    if (p.spectrum) {
        this.spectrum = p.spectrum;
    }

    // Bands
    if (p.bands) {
        this.bands = p.bands;
    }
}
```

**Note:** This requires adding corresponding state properties to the store for each config section (effects, waveforms, spectrum, bands). These will be used by future web UI control panels.

Extend message handler in `handleMessage()` (after line 71):
```javascript
else if (msg.type === 'presetStatus') {
    Alpine.store('app').updatePresets(msg);
}
```

Add preset list refresh on connect (in `ws.onopen`, after line 93):
```javascript
// Request initial preset list
window.sendCommand('presetList', null);
```

Extend `window.sendCommand()` to handle preset command format (replace line 118-121):
```javascript
window.sendCommand = function(cmd, value) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        let payload = { cmd: cmd };
        if (cmd === 'presetLoad' || cmd === 'presetDelete') {
            payload.filename = value;
        } else if (cmd === 'presetSave') {
            payload.name = value;
        } else if (value !== null) {
            payload.value = value;
        }
        ws.send(JSON.stringify(payload));
    }
};
```

### Frontend: Preset Panel HTML

**File:** `web/index.html`

Add preset panel after Analysis panel (after line 75):
```html
<!-- Preset Panel -->
<section class="panel">
    <div class="panel-header">
        <span class="panel-title">Presets</span>
        <span class="panel-indicator"></span>
    </div>
    <div class="panel-content" x-data>
        <!-- Status message -->
        <div class="preset-status"
             x-show="$store.app.presetStatus"
             x-text="$store.app.presetStatus"></div>

        <!-- Save section -->
        <div class="control-row">
            <label class="control-label" for="preset-name">Name</label>
            <div class="preset-save-row">
                <input type="text"
                       id="preset-name"
                       class="preset-input"
                       x-model="$store.app.presetName"
                       placeholder="New preset"
                       :disabled="!$store.app.connected"
                       @keyup.enter="$store.app.savePreset()">
                <button class="btn-save"
                        @click="$store.app.savePreset()"
                        :disabled="!$store.app.connected || !$store.app.presetName.trim()">
                    Save
                </button>
            </div>
        </div>

        <!-- Preset list -->
        <div class="preset-list">
            <template x-for="preset in $store.app.presets" :key="preset">
                <div class="preset-item"
                     :class="{'selected': $store.app.selectedPreset === preset}">
                    <button class="preset-name"
                            @click="$store.app.loadPreset(preset)"
                            :disabled="!$store.app.connected"
                            x-text="preset.replace('.json', '')"></button>
                    <button class="btn-delete"
                            @click.stop="$store.app.deletePreset(preset)"
                            :disabled="!$store.app.connected"
                            title="Delete preset">
                        &times;
                    </button>
                </div>
            </template>
            <div class="preset-empty" x-show="$store.app.presets.length === 0">
                No presets saved
            </div>
        </div>
    </div>
</section>
```

### Frontend: Styles

**File:** `web/style.css`

Add preset panel styles (at end of file):
```css
/* Preset Panel */
.preset-status {
    background: rgba(48, 209, 88, 0.15);
    border-left: 2px solid var(--led-mid);
    color: var(--led-mid);
    padding: 8px 12px;
    margin-bottom: 12px;
    font-size: 11px;
    animation: fadeIn 0.2s ease-out;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(-4px); }
    to { opacity: 1; transform: translateY(0); }
}

.preset-save-row {
    display: flex;
    gap: 8px;
    flex: 1;
}

.preset-input {
    flex: 1;
    background: var(--bg-inset);
    border: 1px solid var(--border-subtle);
    border-radius: 4px;
    color: var(--text-primary);
    padding: 6px 10px;
    font-family: var(--font-mono);
    font-size: 11px;
}

.preset-input:focus {
    outline: none;
    border-color: var(--led-beat);
    box-shadow: 0 0 4px rgba(255, 149, 0, 0.3);
}

.preset-input:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.preset-input::placeholder {
    color: var(--text-dim);
}

.btn-save {
    background: var(--bg-elevated);
    border: 1px solid var(--border-subtle);
    border-radius: 4px;
    color: var(--text-primary);
    padding: 6px 16px;
    font-family: var(--font-mono);
    font-size: 11px;
    cursor: pointer;
    white-space: nowrap;
}

.btn-save:hover:not(:disabled) {
    background: var(--bg-highlight);
    border-color: var(--led-beat);
}

.btn-save:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.preset-list {
    margin-top: 12px;
    max-height: 200px;
    overflow-y: auto;
    border: 1px solid var(--border-subtle);
    border-radius: 4px;
    background: var(--bg-inset);
}

.preset-item {
    display: flex;
    align-items: center;
    border-bottom: 1px solid var(--border-subtle);
}

.preset-item:last-child {
    border-bottom: none;
}

.preset-item.selected {
    background: rgba(255, 149, 0, 0.1);
}

.preset-name {
    flex: 1;
    background: transparent;
    border: none;
    color: var(--text-primary);
    padding: 10px 12px;
    text-align: left;
    font-family: var(--font-mono);
    font-size: 11px;
    cursor: pointer;
}

.preset-name:hover:not(:disabled) {
    background: var(--bg-highlight);
}

.preset-name:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.btn-delete {
    width: 32px;
    height: 32px;
    background: transparent;
    border: none;
    color: var(--text-dim);
    font-size: 18px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
    margin-right: 4px;
}

.btn-delete:hover:not(:disabled) {
    color: var(--led-bass);
}

.btn-delete:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.preset-empty {
    color: var(--text-dim);
    font-size: 11px;
    text-align: center;
    padding: 24px;
}
```

## Integration

### Data Flow (Preset Load)

```
┌────────────────────────────────────────────────────────────┐
│ Web UI (Alpine.js)                                          │
│  presets[], effects, waveforms, spectrum, bands, ...        │
└──────────────────────────┬─────────────────────────────────┘
                           │ sendCommand('presetLoad', 'Default.json')
                           ▼
┌────────────────────────────────────────────────────────────┐
│ WebSocket Message                                           │
│  {"cmd": "presetLoad", "filename": "Default.json"}         │
└──────────────────────────┬─────────────────────────────────┘
                           │ queue
                           ▼
┌────────────────────────────────────────────────────────────┐
│ WebServerProcessCommands() - main thread                    │
│  1. Parse command                                           │
│  2. Call WebBridgeApplyCommand() → PresetToAppConfigs()    │
│  3. Broadcast FULL CONFIG via WebBridgeSerializeConfig()   │
│  4. Broadcast presetStatus (list + status message)          │
└──────────────────────────┬─────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────┐
│ Visualizer sees changes immediately (reads AppConfigs*)     │
│ Native UI sees changes immediately (shared pointers)        │
│ Web clients receive {"type":"config","preset":{...}}        │
│ Web clients receive {"type":"presetStatus",...}             │
└──────────────────────────┬─────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────┐
│ Web UI updateConfig() applies full preset to store          │
│ All web controls now reflect loaded preset values           │
└────────────────────────────────────────────────────────────┘
```

## File Changes

| File | Change |
|------|--------|
| `src/web/web_bridge.h` | Add `WebBridgeSerializePresetStatus()` declaration |
| `src/web/web_bridge.cpp` | Expand `WebBridgeSerializeConfig()` to use preset serialization, implement preset command handlers + status serialization |
| `src/web/web_server.h` | Add `WebServerBroadcastPresetStatus()` declaration |
| `src/web/web_server.cpp` | Implement broadcast, broadcast full config after preset load, hook into command processing, send list on connect |
| `web/app.js` | Add preset state + config state, expand `updateConfig()` for full preset, add actions, message handler, command formatting |
| `web/index.html` | Add Presets panel section |
| `web/style.css` | Add preset panel styles |

## Build Sequence

### Phase 1: Backend Commands

**Objective:** Preset commands work via WebSocket, full config serialization

- [ ] Add `#include <filesystem>` and `#include "config/preset.h"` to `src/web/web_bridge.cpp`
- [ ] Expand `WebBridgeSerializeConfig()` to use `PresetFromAppConfigs()` + existing `to_json()` (buffer 512 → 8192)
- [ ] Implement `WebBridgeSerializePresetStatus()` in `src/web/web_bridge.cpp`
- [ ] Add declaration to `src/web/web_bridge.h`
- [ ] Add `presetList` handler in `WebBridgeApplyCommand()` at `src/web/web_bridge.cpp:75`
- [ ] Add `presetLoad` handler with `PresetLoad()` + `PresetToAppConfigs()`
- [ ] Add `presetSave` handler with `PresetFromAppConfigs()` + `PresetSave()`
- [ ] Add `presetDelete` handler with `std::filesystem::remove()`
- [ ] Build and verify no compilation errors

### Phase 2: Server Broadcast

**Objective:** Preset status broadcasts after operations, full config after load

- [ ] Declare `WebServerBroadcastPresetStatus()` in `src/web/web_server.h`
- [ ] Implement `WebServerBroadcastPresetStatus()` in `src/web/web_server.cpp`
- [ ] Modify `WebServerProcessCommands()` to detect preset commands and broadcast status
- [ ] Add full config broadcast after successful `presetLoad` via `WebBridgeSerializeConfig()`
- [ ] Add initial preset list broadcast in WebSocket Open handler
- [ ] Test via WebSocket client: send `{"cmd":"presetLoad","filename":"Default.json"}`, verify full config received

### Phase 3: Frontend Store

**Objective:** Alpine.js state management for presets and full config

- [ ] Add preset state properties to Alpine store in `web/app.js`
- [ ] Add config state properties (effects, waveforms, spectrum, bands)
- [ ] Expand `updateConfig()` to apply full preset data from `config.preset`
- [ ] Add `loadPreset()`, `savePreset()`, `deletePreset()`, `refreshPresets()` actions
- [ ] Add `updatePresets()` handler
- [ ] Extend `handleMessage()` for `presetStatus` type
- [ ] Modify `window.sendCommand()` for preset command format
- [ ] Add preset list request in `ws.onopen`
- [ ] Test: load preset, verify `Alpine.store('app').effects` populated

### Phase 4: Frontend UI

**Objective:** Preset panel displays and functions

- [ ] Add Presets panel section to `web/index.html` after Analysis panel
- [ ] Add preset panel CSS to `web/style.css`
- [ ] Test save: enter name, click Save, verify file created
- [ ] Test load: click preset name, verify visualizer changes
- [ ] Test delete: click ×, confirm dialog, verify file removed
- [ ] Test status: verify message appears and fades after 3 seconds

## Validation

- [ ] Save preset from web UI creates `.json` file in `presets/` directory
- [ ] Load preset from web UI applies config (visible in native UI controls)
- [ ] Load preset from web UI updates web store state (verify `Alpine.store('app').channelMode` matches loaded preset)
- [ ] Delete preset from web UI removes file from disk
- [ ] Preset list refreshes after save/delete operations
- [ ] Status message displays for 3 seconds with success/failure feedback
- [ ] Native raygui preset panel continues working unchanged
- [ ] Both UIs can operate presets simultaneously
- [ ] Loading preset from native panel does NOT update web UI (no cross-UI sync for native operations)
- [ ] WebSocket reconnect triggers preset list refresh and full config sync
- [ ] Empty preset name cannot be saved (button disabled)
- [ ] Delete confirmation prevents accidental deletion
- [ ] Empty state shows "No presets saved" message
- [ ] Selected preset highlighted in list after load

## References

- Existing command pattern: `src/web/web_bridge.cpp:63-73`
- Alpine store conventions: `web/CLAUDE.md:46-102`
- Panel HTML structure: `web/index.html:28-52`
- Preset backend: `src/config/preset.cpp:81-153`
- Native preset panel: `src/ui/ui_panel_preset.cpp:39-98`
