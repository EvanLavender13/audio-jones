# Web Waveform UI

Browser-based waveform controls matching desktop functionality with hardware rack aesthetic.

## Goal

Implement the waveform UI section in the web app with full waveform management (add, remove, select) and all parameter controls (6 sliders + color picker). Create reusable slider and color picker widgets. Extend `ColorConfig` to support future gradient modes.

**Sync model**: Unidirectional - web sends commands, desktop receives and broadcasts updated config.

## Current State

### Desktop Waveform Panel

`src/ui/ui_panel_waveform.cpp:33-85` implements master-detail pattern:

- List view with "New" button (max 8 waveforms) - line 42-48
- Scrollable list showing "Waveform N" labels - line 51-62
- Settings group for selected waveform - line 65-85
  - 6 sliders: Radius (0.05-0.45), Height (0.05-0.5), Thickness (1-25), Smooth (0-100), Rotation (-0.05-0.05), Offset (0-2π)
  - Color controls via `UIDrawColorControls()`: mode dropdown, solid picker + alpha, or rainbow hue range + sat + val

### ColorConfig Structure

`src/render/color_config.h:6-18`:

```cpp
typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW
} ColorMode;

struct ColorConfig {
    ColorMode mode = COLOR_MODE_SOLID;
    Color solid = WHITE;
    float rainbowHue = 0.0f;
    float rainbowRange = 360.0f;
    float rainbowSat = 1.0f;
    float rainbowVal = 1.0f;
};
```

### Web App Patterns

`web/index.html:203-239` contains waveform placeholder with static tabs.

`web/app.js:28-31` already stores waveform state:
```javascript
waveformCount: 1,
waveforms: [],
```

`web/style.css:398-432` defines `.waveform-tab` styling with LED glow.

### WebSocket Command Pattern

`src/web/web_bridge.cpp:76-161` shows command dispatch with `{cmd, value}` format.

## Architecture Decision

**Pragmatic Balance**: Inline sliders (simple), reusable color picker component (needed for spectrum too), tab-based navigation, per-parameter WebSocket commands.

**Key decisions:**
1. HTML5 `<input type="range">` for sliders (6 sliders don't justify abstraction)
2. `colorPicker()` Alpine component factory (reused by spectrum panel)
3. ColorConfig extended with gradient fields now (data only, no UI)
4. Debounced input (100ms) for slider changes
5. Three color commands: mode, solid, rainbow (match desktop granularity)

## Component Design

### Alpine Store Extension

**File**: `web/app.js`

Add after line 31:
```javascript
selectedWaveform: 0,

get currentWaveform() {
    return this.waveforms[this.selectedWaveform] || null;
},
```

Add before `onChannelChange()`:
```javascript
selectWaveform(index) {
    this.selectedWaveform = index;
},

addWaveform() {
    if (!this.connected || this.waveformCount >= 8) return;
    window.sendCommand('waveformAdd', null);
},

removeWaveform(index) {
    if (!this.connected || this.waveformCount <= 1) return;
    window.sendCommand('waveformRemove', index);
    if (this.selectedWaveform >= this.waveformCount - 1) {
        this.selectedWaveform = Math.max(0, this.waveformCount - 2);
    }
},

onWaveformParamChange(param, value) {
    window.sendCommand('setWaveformParam', {
        index: this.selectedWaveform,
        param: param,
        value: value
    });
},

onWaveformColorModeChange(mode) {
    window.sendCommand('setWaveformColorMode', {
        index: this.selectedWaveform,
        mode: mode
    });
},

onWaveformSolidChange(color) {
    window.sendCommand('setWaveformColor', {
        index: this.selectedWaveform,
        color: color
    });
},

onWaveformRainbowChange(hue, range, sat, val) {
    window.sendCommand('setWaveformRainbow', {
        index: this.selectedWaveform,
        hue: hue,
        range: range,
        sat: sat,
        val: val
    });
},
```

### Color Picker Component

**File**: `web/app.js` (add at end of file)

```javascript
function colorPicker(getColor, onModeChange, onSolidChange, onRainbowChange) {
    return {
        get color() { return getColor(); },

        get solidHex() {
            const c = this.color?.solid || { r: 255, g: 255, b: 255 };
            return '#' + [c.r, c.g, c.b].map(x => x.toString(16).padStart(2, '0')).join('');
        },

        set solidHex(hex) {
            const r = parseInt(hex.slice(1, 3), 16);
            const g = parseInt(hex.slice(3, 5), 16);
            const b = parseInt(hex.slice(5, 7), 16);
            const a = this.color?.solid?.a || 255;
            onSolidChange({ r, g, b, a });
        },

        onModeChange(mode) {
            onModeChange(mode);
        },

        onAlphaChange(alpha) {
            const c = this.color?.solid || { r: 255, g: 255, b: 255 };
            onSolidChange({ r: c.r, g: c.g, b: c.b, a: alpha });
        },

        onRainbowChange() {
            const rb = this.color?.rainbow || {};
            onRainbowChange(
                rb.hue || 0,
                rb.range || 360,
                rb.sat || 1.0,
                rb.val || 1.0
            );
        },

        get rainbowGradient() {
            const h1 = this.color?.rainbow?.hue || 0;
            const range = this.color?.rainbow?.range || 360;
            return `linear-gradient(90deg, hsl(${h1}, 100%, 50%), hsl(${h1 + range}, 100%, 50%))`;
        }
    };
}
```

### Waveform Section HTML

**File**: `web/index.html` - Replace lines 203-239

```html
<!-- Waveforms Section -->
<div class="section" x-show="activeSection === 'waveforms'">
    <div class="section-header">
        <h2 class="section-title">Waveforms</h2>
        <span class="section-count" x-text="$store.app.waveformCount + '/8'"></span>
    </div>
    <div class="section-content">
        <!-- Tab bar -->
        <div class="waveform-tabs">
            <template x-for="(wf, i) in $store.app.waveforms.slice(0, $store.app.waveformCount)" :key="i">
                <button class="waveform-tab"
                        :class="{ active: $store.app.selectedWaveform === i }"
                        @click="$store.app.selectWaveform(i)">
                    <span x-text="'W' + (i + 1)"></span>
                </button>
            </template>
            <button class="waveform-tab waveform-tab-add"
                    @click="$store.app.addWaveform()"
                    :disabled="!$store.app.connected || $store.app.waveformCount >= 8">+</button>
        </div>

        <!-- Settings for selected waveform -->
        <template x-if="$store.app.currentWaveform">
            <div class="waveform-settings">
                <!-- Geometry Group -->
                <div class="control-group">
                    <h3 class="control-group-title">Geometry</h3>

                    <div class="control-row">
                        <label class="control-label">Radius</label>
                        <input type="range" class="control-slider"
                               min="0.05" max="0.45" step="0.01"
                               :value="$store.app.currentWaveform.radius"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('radius', parseFloat($event.target.value))">
                        <span class="control-value" x-text="$store.app.currentWaveform.radius.toFixed(2)"></span>
                    </div>

                    <div class="control-row">
                        <label class="control-label">Height</label>
                        <input type="range" class="control-slider"
                               min="0.05" max="0.5" step="0.01"
                               :value="$store.app.currentWaveform.amplitudeScale"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('amplitudeScale', parseFloat($event.target.value))">
                        <span class="control-value" x-text="$store.app.currentWaveform.amplitudeScale.toFixed(2)"></span>
                    </div>

                    <div class="control-row">
                        <label class="control-label">Thickness</label>
                        <input type="range" class="control-slider"
                               min="1" max="25" step="1"
                               :value="$store.app.currentWaveform.thickness"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('thickness', parseInt($event.target.value))">
                        <span class="control-value" x-text="$store.app.currentWaveform.thickness + 'px'"></span>
                    </div>

                    <div class="control-row">
                        <label class="control-label">Smooth</label>
                        <input type="range" class="control-slider"
                               min="0" max="100" step="1"
                               :value="$store.app.currentWaveform.smoothness"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('smoothness', parseFloat($event.target.value))">
                        <span class="control-value" x-text="Math.round($store.app.currentWaveform.smoothness)"></span>
                    </div>
                </div>

                <!-- Rotation Group -->
                <div class="control-group">
                    <h3 class="control-group-title">Rotation</h3>

                    <div class="control-row">
                        <label class="control-label">Speed</label>
                        <input type="range" class="control-slider"
                               min="-0.05" max="0.05" step="0.001"
                               :value="$store.app.currentWaveform.rotationSpeed"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('rotationSpeed', parseFloat($event.target.value))">
                        <span class="control-value" x-text="$store.app.currentWaveform.rotationSpeed.toFixed(3)"></span>
                    </div>

                    <div class="control-row">
                        <label class="control-label">Offset</label>
                        <input type="range" class="control-slider"
                               min="0" max="6.28" step="0.01"
                               :value="$store.app.currentWaveform.rotationOffset"
                               @input.debounce.100ms="$store.app.onWaveformParamChange('rotationOffset', parseFloat($event.target.value))">
                        <span class="control-value" x-text="$store.app.currentWaveform.rotationOffset.toFixed(2)"></span>
                    </div>
                </div>

                <!-- Color Group -->
                <div class="control-group"
                     x-data="colorPicker(
                         () => $store.app.currentWaveform?.color,
                         (m) => $store.app.onWaveformColorModeChange(m),
                         (c) => $store.app.onWaveformSolidChange(c),
                         (h, r, s, v) => $store.app.onWaveformRainbowChange(h, r, s, v)
                     )">
                    <h3 class="control-group-title">Color</h3>

                    <div class="control-row">
                        <label class="control-label">Mode</label>
                        <div class="select-wrapper">
                            <select :value="color?.mode || 0"
                                    @change="onModeChange(parseInt($event.target.value))"
                                    :disabled="!$store.app.connected">
                                <option value="0">Solid</option>
                                <option value="1">Rainbow</option>
                            </select>
                        </div>
                    </div>

                    <!-- Solid Mode -->
                    <template x-if="(color?.mode || 0) === 0">
                        <div class="color-picker-group">
                            <div class="control-row">
                                <label class="control-label">Color</label>
                                <input type="color" class="color-input"
                                       :value="solidHex"
                                       @input.debounce.100ms="solidHex = $event.target.value">
                            </div>
                            <div class="control-row">
                                <label class="control-label">Alpha</label>
                                <input type="range" class="control-slider"
                                       min="0" max="255" step="1"
                                       :value="color?.solid?.a || 255"
                                       @input.debounce.100ms="onAlphaChange(parseInt($event.target.value))">
                                <span class="control-value" x-text="color?.solid?.a || 255"></span>
                            </div>
                        </div>
                    </template>

                    <!-- Rainbow Mode -->
                    <template x-if="(color?.mode || 0) === 1">
                        <div class="rainbow-picker-group">
                            <div class="rainbow-preview" :style="{ background: rainbowGradient }"></div>

                            <div class="control-row">
                                <label class="control-label">Hue</label>
                                <input type="range" class="control-slider"
                                       min="0" max="360" step="1"
                                       :value="color?.rainbow?.hue || 0"
                                       @input.debounce.100ms="$store.app.currentWaveform.color.rainbow.hue = parseFloat($event.target.value); onRainbowChange()">
                                <span class="control-value" x-text="(color?.rainbow?.hue || 0).toFixed(0) + '\u00B0'"></span>
                            </div>

                            <div class="control-row">
                                <label class="control-label">Range</label>
                                <input type="range" class="control-slider"
                                       min="0" max="360" step="1"
                                       :value="color?.rainbow?.range || 360"
                                       @input.debounce.100ms="$store.app.currentWaveform.color.rainbow.range = parseFloat($event.target.value); onRainbowChange()">
                                <span class="control-value" x-text="(color?.rainbow?.range || 360).toFixed(0) + '\u00B0'"></span>
                            </div>

                            <div class="control-row">
                                <label class="control-label">Sat</label>
                                <input type="range" class="control-slider"
                                       min="0" max="1" step="0.01"
                                       :value="color?.rainbow?.sat || 1"
                                       @input.debounce.100ms="$store.app.currentWaveform.color.rainbow.sat = parseFloat($event.target.value); onRainbowChange()">
                                <span class="control-value" x-text="(color?.rainbow?.sat || 1).toFixed(2)"></span>
                            </div>

                            <div class="control-row">
                                <label class="control-label">Bright</label>
                                <input type="range" class="control-slider"
                                       min="0" max="1" step="0.01"
                                       :value="color?.rainbow?.val || 1"
                                       @input.debounce.100ms="$store.app.currentWaveform.color.rainbow.val = parseFloat($event.target.value); onRainbowChange()">
                                <span class="control-value" x-text="(color?.rainbow?.val || 1).toFixed(2)"></span>
                            </div>
                        </div>
                    </template>
                </div>

                <!-- Delete Button -->
                <button class="btn-delete-waveform"
                        @click="$store.app.removeWaveform($store.app.selectedWaveform)"
                        :disabled="!$store.app.connected || $store.app.waveformCount <= 1">
                    Delete Waveform
                </button>
            </div>
        </template>
    </div>
</div>
```

### CSS Additions

**File**: `web/style.css` - Add after line 432

```css
/* ══════════════════════════════════════════════════════════════════════════
   Waveform Settings
   ══════════════════════════════════════════════════════════════════════════ */

.waveform-settings {
    display: flex;
    flex-direction: column;
    gap: 24px;
}

.control-group {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.control-group-title {
    font-family: 'Oxanium', sans-serif;
    font-size: 0.625rem;
    font-weight: 600;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-dim);
    margin-bottom: 4px;
}

.section-count {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.6875rem;
    color: var(--text-secondary);
    background: var(--metal-dark);
    padding: 4px 8px;
    border-radius: 4px;
    border: 1px solid var(--metal-edge);
}

/* Waveform Tab Enhancements */
.waveform-tab-add {
    background: var(--metal-mid);
    color: var(--led-mid);
    font-size: 1rem;
    font-weight: 700;
}

.waveform-tab-add:hover:not(:disabled) {
    background: var(--metal-highlight);
    border-color: var(--led-mid);
    box-shadow: 0 0 8px var(--led-mid-glow);
}

.waveform-tab-add:disabled {
    opacity: 0.3;
    cursor: not-allowed;
}

/* Color Picker Group */
.color-picker-group,
.rainbow-picker-group {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 16px;
    background: var(--metal-mid);
    border: 1px solid var(--metal-edge);
    border-radius: 4px;
}

.color-input {
    width: 80px;
    height: 32px;
    padding: 2px;
    border: 1px solid var(--metal-edge);
    border-radius: 4px;
    background: var(--metal-dark);
    cursor: pointer;
}

.color-input::-webkit-color-swatch-wrapper {
    padding: 2px;
}

.color-input::-webkit-color-swatch {
    border: 1px solid var(--metal-highlight);
    border-radius: 2px;
}

/* Rainbow Preview */
.rainbow-preview {
    height: 24px;
    border: 1px solid var(--metal-edge);
    border-radius: 4px;
    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.3);
}

/* Delete Button */
.btn-delete-waveform {
    width: 100%;
    padding: 12px;
    margin-top: 8px;
    background: var(--metal-dark);
    border: 1px solid var(--led-bass-dim);
    border-radius: 4px;
    color: var(--led-bass);
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.75rem;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.15s ease;
}

.btn-delete-waveform:hover:not(:disabled) {
    background: rgba(255, 45, 85, 0.1);
    border-color: var(--led-bass);
    box-shadow: 0 0 8px var(--led-bass-glow);
}

.btn-delete-waveform:disabled {
    opacity: 0.3;
    cursor: not-allowed;
}
```

### Backend Command Handlers

**File**: `src/web/web_bridge.cpp` - Add after line 99 (after `setAudioChannel`)

```cpp
if (cmd == "waveformAdd") {
    if (*configs->waveformCount >= MAX_WAVEFORMS) {
        return false;
    }
    static const Color presetColors[] = {
        {255, 255, 255, 255}, {230, 41, 55, 255}, {0, 228, 48, 255},
        {0, 121, 241, 255}, {253, 249, 0, 255}, {255, 0, 255, 255},
        {255, 161, 0, 255}, {102, 191, 255, 255}
    };
    configs->waveforms[*configs->waveformCount] = WaveformConfig{};
    configs->waveforms[*configs->waveformCount].color.solid =
        presetColors[*configs->waveformCount % 8];
    (*configs->waveformCount)++;
    return true;
}

if (cmd == "waveformRemove") {
    if (!msg.contains("value")) return false;
    const int index = msg["value"].get<int>();
    if (index < 0 || index >= *configs->waveformCount || *configs->waveformCount <= 1) {
        return false;
    }
    for (int i = index; i < *configs->waveformCount - 1; i++) {
        configs->waveforms[i] = configs->waveforms[i + 1];
    }
    (*configs->waveformCount)--;
    return true;
}

if (cmd == "setWaveformParam") {
    if (!msg.contains("value")) return false;
    const json& val = msg["value"];
    if (!val.contains("index") || !val.contains("param") || !val.contains("value")) {
        return false;
    }
    const int index = val["index"].get<int>();
    if (index < 0 || index >= *configs->waveformCount) return false;

    const std::string param = val["param"].get<std::string>();
    WaveformConfig* wf = &configs->waveforms[index];

    if (param == "radius") {
        wf->radius = val["value"].get<float>();
    } else if (param == "amplitudeScale") {
        wf->amplitudeScale = val["value"].get<float>();
    } else if (param == "thickness") {
        wf->thickness = val["value"].get<int>();
    } else if (param == "smoothness") {
        wf->smoothness = val["value"].get<float>();
    } else if (param == "rotationSpeed") {
        wf->rotationSpeed = val["value"].get<float>();
    } else if (param == "rotationOffset") {
        wf->rotationOffset = val["value"].get<float>();
    } else {
        return false;
    }
    return true;
}

if (cmd == "setWaveformColorMode") {
    if (!msg.contains("value")) return false;
    const json& val = msg["value"];
    if (!val.contains("index") || !val.contains("mode")) return false;
    const int index = val["index"].get<int>();
    if (index < 0 || index >= *configs->waveformCount) return false;
    configs->waveforms[index].color.mode = (ColorMode)val["mode"].get<int>();
    return true;
}

if (cmd == "setWaveformColor") {
    if (!msg.contains("value")) return false;
    const json& val = msg["value"];
    if (!val.contains("index") || !val.contains("color")) return false;
    const int index = val["index"].get<int>();
    if (index < 0 || index >= *configs->waveformCount) return false;
    const json& c = val["color"];
    configs->waveforms[index].color.solid = Color{
        (unsigned char)c["r"].get<int>(),
        (unsigned char)c["g"].get<int>(),
        (unsigned char)c["b"].get<int>(),
        (unsigned char)c["a"].get<int>()
    };
    return true;
}

if (cmd == "setWaveformRainbow") {
    if (!msg.contains("value")) return false;
    const json& val = msg["value"];
    if (!val.contains("index")) return false;
    const int index = val["index"].get<int>();
    if (index < 0 || index >= *configs->waveformCount) return false;
    ColorConfig* cc = &configs->waveforms[index].color;
    if (val.contains("hue")) cc->rainbowHue = val["hue"].get<float>();
    if (val.contains("range")) cc->rainbowRange = val["range"].get<float>();
    if (val.contains("sat")) cc->rainbowSat = val["sat"].get<float>();
    if (val.contains("val")) cc->rainbowVal = val["val"].get<float>();
    return true;
}
```

### ColorConfig Extension

**File**: `src/render/color_config.h` - Replace entire file

```cpp
#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#include "raylib.h"

#define MAX_GRADIENT_STOPS 8

typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW,
    COLOR_MODE_GRADIENT  // Future: multi-stop gradients
} ColorMode;

// Future: gradient stop for multi-color gradients
struct GradientStop {
    float position = 0.0f;  // 0.0-1.0 along waveform
    Color color = WHITE;
};

struct ColorConfig {
    ColorMode mode = COLOR_MODE_SOLID;
    Color solid = WHITE;
    float rainbowHue = 0.0f;       // Starting hue offset (0-360)
    float rainbowRange = 360.0f;   // Hue degrees to span (0-360)
    float rainbowSat = 1.0f;       // Saturation (0-1)
    float rainbowVal = 1.0f;       // Value/brightness (0-1)

    // Gradient mode (future - not serialized yet)
    GradientStop gradientStops[MAX_GRADIENT_STOPS] = {};
    int gradientStopCount = 0;
};

#endif // COLOR_CONFIG_H
```

## File Changes

| File | Change | Lines |
|------|--------|-------|
| `src/render/color_config.h` | Replace - Add gradient fields and enum | ~35 |
| `web/app.js` | Modify - Add store methods and colorPicker component | ~80 |
| `web/index.html` | Modify - Replace waveform section (lines 203-239) | ~180 |
| `web/style.css` | Add - Waveform settings, color picker styles | ~90 |
| `src/web/web_bridge.cpp` | Add - 6 command handlers after line 99 | ~85 |

**Total**: ~470 lines across 5 files

## Build Sequence

### Phase 1: ColorConfig Extension

**Objective**: Extend data structure for future gradients

- [ ] Replace `src/render/color_config.h` with extended version
- [ ] Build desktop app to verify compilation
- [ ] Load existing preset, verify waveforms render unchanged
- [ ] Save preset, verify JSON contains new fields with defaults

**Validation**: Desktop app compiles and runs, presets roundtrip correctly

### Phase 2: Backend Commands

**Objective**: Implement WebSocket command handlers

- [ ] Add 6 command handlers to `src/web/web_bridge.cpp`
- [ ] Build desktop app
- [ ] Test via browser console:
  - `window.sendCommand('waveformAdd', null)`
  - `window.sendCommand('waveformRemove', 0)`
  - `window.sendCommand('setWaveformParam', {index: 0, param: 'radius', value: 0.3})`

**Validation**: Commands execute, desktop waveforms respond

### Phase 3: Alpine Store Extension

**Objective**: Add waveform state management to web

- [ ] Add `selectedWaveform` and computed getter to store
- [ ] Add waveform management methods
- [ ] Add `colorPicker()` component factory function
- [ ] Test store via browser console:
  - `Alpine.store('app').selectWaveform(1)`
  - `Alpine.store('app').addWaveform()`

**Validation**: Store state updates, methods dispatch commands

### Phase 4: Waveform UI Section

**Objective**: Implement complete waveform controls

- [ ] Replace waveform placeholder section in HTML
- [ ] Add all 6 geometry/rotation sliders
- [ ] Add color mode dropdown
- [ ] Add solid color picker (input + alpha)
- [ ] Add rainbow controls (4 sliders + preview)
- [ ] Add delete button

**Validation**: All controls render and interact correctly

### Phase 5: CSS Styling

**Objective**: Apply hardware rack aesthetic

- [ ] Add control-group styling
- [ ] Add color picker group styling
- [ ] Add rainbow preview styling
- [ ] Add delete button styling
- [ ] Add waveform tab add button styling

**Validation**: UI matches hardware aesthetic, LED glows on active states

## Validation

### Waveform Management
- [ ] Add waveform via web, appears in desktop immediately
- [ ] Remove waveform via web, desktop list shrinks
- [ ] Cannot add 9th waveform (button disables at 8)
- [ ] Cannot remove last waveform (button disables at 1)
- [ ] Tab selection highlights correct waveform

### Parameter Controls
- [ ] Radius slider updates desktop waveform size
- [ ] Height slider updates wave amplitude
- [ ] Thickness slider updates line width
- [ ] Smooth slider updates interpolation
- [ ] Rotation speed slider updates spin rate
- [ ] Rotation offset slider updates starting angle

### Color Controls
- [ ] Mode dropdown switches between solid/rainbow
- [ ] Solid color picker updates waveform color
- [ ] Alpha slider updates transparency
- [ ] Rainbow hue slider updates gradient start
- [ ] Rainbow range slider updates gradient span
- [ ] Rainbow sat/val sliders update saturation/brightness
- [ ] Rainbow preview shows correct gradient

### Sync Behavior
- [ ] Web slider change reflects on desktop within 100ms
- [ ] Preset load syncs correct waveform state to web
- [ ] Disconnect disables all controls
- [ ] Reconnect syncs current state

## References

- Desktop waveform panel: `src/ui/ui_panel_waveform.cpp:33-85`
- Color controls: `src/ui/ui_color.cpp:7-61`
- Hue range slider: `src/ui/ui_widgets.cpp:104-176`
- Web control patterns: `web/CLAUDE.md:111-149`
- WebSocket commands: `src/web/web_bridge.cpp:76-161`
- Preset serialization: `src/config/preset.cpp:10-12`
