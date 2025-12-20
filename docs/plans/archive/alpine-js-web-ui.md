# Alpine.js Web UI Migration

Reactive state management for AudioJones web control interface using Alpine.js CDN.

## Goal

Eliminate imperative DOM manipulation in the web UI by adopting Alpine.js reactive bindings. Replace 9 `getElementById` queries and manual DOM updates with declarative templates. Establish conventions in `web/CLAUDE.md` for future panel development.

**Scope:** Foundation only - no new panels or features. Refactor existing controls (status, channel select, meters) to use Alpine.js patterns.

## Current State

### Vanilla JS Implementation

**File:** `web/app.js` (~115 lines)

- **Lines 15-24:** 9 `getElementById` queries cached at module init
- **Lines 26-37:** `setStatus()` and `updateMeter()` - imperative DOM updates
- **Lines 39-59:** `handleMessage()` - routes WebSocket messages to DOM updates
- **Lines 108-110:** `addEventListener` for channel select change

```javascript
// Current pattern (imperative)
const beatMeter = document.getElementById('beat-meter');
function updateMeter(meterEl, valueEl, value, maxValue) {
    const clamped = Math.min(Math.max(value, 0), maxValue);
    const percent = (clamped / maxValue) * 100;
    meterEl.style.width = percent + '%';
    valueEl.textContent = value.toFixed(2);
}
```

### HTML Structure

**File:** `web/index.html` (~110 lines)

- **Lines 54-95:** 4 identical meter blocks (beat/bass/mid/treb) - each 10 lines
- **Line 20:** Status indicator with `id="status"`
- **Lines 33-40:** Channel mode dropdown with `id="channel-mode"`

### Pain Points

1. **State scattered** across closure variables and DOM properties
2. **Manual synchronization** - every state change requires explicit DOM update
3. **Repetitive HTML** - 4 identical meter structures, only IDs differ
4. **Tight coupling** - `updateMeter()` knows about both fill bar and value text

### What Works Well

- WebSocket lifecycle and reconnection logic
- CSS-driven styling (hardware rack aesthetic)
- Message type-based routing in `handleMessage()`

## Algorithm

### Alpine.js Integration Strategy

**CDN Approach** - No build step:
```html
<script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
```

Use `defer` to ensure Alpine initializes after DOM parse. Version `@3.x.x` provides stability.

### State Architecture

**Single `Alpine.store('app')`** - Centralized reactive state:

```javascript
Alpine.store('app', {
    // Connection state
    connected: false,
    statusText: 'Connecting...',

    // Analysis values (updated at 20Hz)
    beat: 0, bass: 0, mid: 0, treb: 0,

    // Config state
    channelMode: 2,  // Default: MAX

    // Computed: meter percentages (clamped)
    get beatPercent() { return Math.min((this.beat / 1.0) * 100, 100) },
    get bassPercent() { return Math.min((this.bass / 3.0) * 100, 100) },
    get midPercent()  { return Math.min((this.mid / 3.0) * 100, 100) },
    get trebPercent() { return Math.min((this.treb / 3.0) * 100, 100) }
})
```

**Rationale:**
- Store for shared state (accessible from multiple elements)
- Computed getters centralize max values and clamping logic
- Plain numbers - CSS concatenates `%` via `:style` binding

### Reactive Bindings Map

| Element | Current Code | Alpine Binding |
|---------|--------------|----------------|
| Status text | `statusEl.textContent = text` | `x-text="$store.app.statusText"` |
| Status class | `statusEl.className = 'status ' + ...` | `:class="{'connected': $store.app.connected}"` |
| Channel select | `channelModeSelect.value = ...` | `x-model.number="$store.app.channelMode"` |
| Meter fill | `meterEl.style.width = percent + '%'` | `:style="{width: $store.app.beatPercent + '%'}"` |
| Meter value | `valueEl.textContent = value.toFixed(2)` | `x-text="$store.app.beat.toFixed(2)"` |

### Meter Loop Pattern

Replace 4 duplicated blocks (40 lines) with single `x-for` loop (12 lines):

```html
<template x-for="meter in ['beat', 'bass', 'mid', 'treb']" :key="meter">
    <div class="meter-container">
        <div class="meter-header">
            <span class="meter-label" x-text="meter"></span>
            <span class="meter-value" x-text="$store.app[meter].toFixed(2)"></span>
        </div>
        <div class="meter">
            <div class="meter-fill"
                 :class="'meter-' + meter"
                 :style="{width: $store.app[meter + 'Percent'] + '%'}"></div>
        </div>
    </div>
</template>
```

**Trade-off:** Labels become lowercase ("beat" not "Beat"). Mitigated with CSS:
```css
.meter-label { text-transform: capitalize; }
```

## Component Design

### Alpine Store Module

**File:** `web/app.js` (refactored)

```javascript
// Alpine store initialization
document.addEventListener('alpine:init', () => {
    Alpine.store('app', {
        connected: false,
        statusText: 'Connecting...',
        beat: 0, bass: 0, mid: 0, treb: 0,
        channelMode: 2,

        get beatPercent() { return Math.min((this.beat / 1.0) * 100, 100) },
        get bassPercent() { return Math.min((this.bass / 3.0) * 100, 100) },
        get midPercent()  { return Math.min((this.mid / 3.0) * 100, 100) },
        get trebPercent() { return Math.min((this.treb / 3.0) * 100, 100) },

        setStatus(connected, text) {
            this.connected = connected;
            this.statusText = text;
        },

        updateAnalysis(data) {
            this.beat = data.beat;
            this.bass = data.bass;
            this.mid = data.mid;
            this.treb = data.treb;
        },

        updateConfig(config) {
            if (config.audio?.channelMode != null) {
                this.channelMode = config.audio.channelMode;
            }
        },

        onChannelChange() {
            sendCommand('setAudioChannel', this.channelMode);
        }
    });
});
```

**Responsibilities:**
- Reactive state for connection, analysis, and config
- Computed getters for derived values (percentages)
- Action methods called from WebSocket handlers and Alpine bindings

### WebSocket Client

**File:** `web/app.js` (kept as vanilla IIFE, simplified)

```javascript
(function() {
    'use strict';
    const WS_PORT = 8081;
    const RECONNECT_DELAY = 2000;
    let ws = null;
    let reconnectTimeout = null;

    function handleMessage(event) {
        const msg = JSON.parse(event.data);
        const store = Alpine.store('app');

        if (msg.type === 'analysis') {
            store.updateAnalysis(msg);
        } else if (msg.type === 'config') {
            store.updateConfig(msg);
        }
    }

    function connect() {
        if (reconnectTimeout) {
            clearTimeout(reconnectTimeout);
            reconnectTimeout = null;
        }

        const wsUrl = 'ws://' + window.location.hostname + ':' + WS_PORT;
        Alpine.store('app').setStatus(false, 'Connecting...');

        ws = new WebSocket(wsUrl);
        ws.onopen = () => Alpine.store('app').setStatus(true, 'Connected');
        ws.onclose = () => {
            Alpine.store('app').setStatus(false, 'Disconnected - reconnecting...');
            scheduleReconnect();
        };
        ws.onerror = (error) => console.error('WebSocket error:', error);
        ws.onmessage = handleMessage;
    }

    function scheduleReconnect() {
        if (!reconnectTimeout) {
            reconnectTimeout = setTimeout(() => {
                reconnectTimeout = null;
                connect();
            }, RECONNECT_DELAY);
        }
    }

    window.sendCommand = function(cmd, value) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ cmd, value }));
        }
    };

    connect();
})();
```

**Changes from current:**
- Remove all `getElementById` queries
- Remove `setStatus()` and `updateMeter()` functions
- Call `Alpine.store('app')` methods instead of DOM updates
- Expose `sendCommand()` globally for Alpine bindings

### HTML Template Updates

**Status indicator** (`web/index.html` line 20):
```html
<div x-data
     class="status"
     :class="{'connected': $store.app.connected, 'disconnected': !$store.app.connected}"
     x-text="$store.app.statusText">
</div>
```

**Channel select** (`web/index.html` lines 33-40):
```html
<select x-data
        id="channel-mode"
        x-model.number="$store.app.channelMode"
        @change="$store.app.onChannelChange()"
        :disabled="!$store.app.connected">
    <option value="0">Left</option>
    <option value="1">Right</option>
    <option value="2">Max</option>
    <option value="3">Mix</option>
    <option value="4">Side</option>
    <option value="5">Interleaved</option>
</select>
```

**Analysis meters** (replace lines 54-95):
```html
<template x-for="meter in ['beat', 'bass', 'mid', 'treb']" :key="meter">
    <div class="meter-container">
        <div class="meter-header">
            <span class="meter-label" x-text="meter"></span>
            <span class="meter-value" x-text="$store.app[meter].toFixed(2)"></span>
        </div>
        <div class="meter">
            <div class="meter-fill"
                 :class="'meter-' + meter"
                 :style="{width: $store.app[meter + 'Percent'] + '%'}"></div>
        </div>
    </div>
</template>
```

## Integration

### Script Loading Order

**File:** `web/index.html` (head section)

```html
<script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
<script src="app.js"></script>
```

- Alpine.js `defer`: Loads after HTML parse, before Alpine scans DOM
- `app.js`: Registers store via `alpine:init` event, then connects WebSocket

### Data Flow

```
┌──────────────────────────┐
│  WebSocket Server (C++)  │
└───────────┬──────────────┘
            │ JSON messages
            ▼
┌──────────────────────────┐
│  WebSocket IIFE          │  Vanilla JS
│  - connect/reconnect     │  - Routes messages to store
│  - sendCommand()         │  - Exposes global function
└───────────┬──────────────┘
            │ store.update*()
            ▼
┌──────────────────────────┐
│  Alpine.store('app')     │  Reactive state
│  - connected, statusText │  - Computed percentages
│  - beat, bass, mid, treb │  - Action methods
│  - channelMode           │
└───────────┬──────────────┘
            │ Reactive bindings
            ▼
┌──────────────────────────┐
│  HTML Templates          │  Declarative
│  - x-text, :class        │  - Auto-updates on change
│  - x-model, @change      │  - x-for loops
│  - :style                │
└──────────────────────────┘
```

## File Changes

| File | Change |
|------|--------|
| `web/index.html` | Modify - Add Alpine CDN, replace IDs with Alpine directives, `x-for` meter loop |
| `web/app.js` | Modify - Add Alpine store, remove DOM queries, simplify WebSocket handlers |
| `web/style.css` | Modify - Add `.meter-label { text-transform: capitalize; }` |
| `web/CLAUDE.md` | Create - Alpine.js conventions and patterns |

## Build Sequence

### Phase 1: Alpine.js Foundation

- [ ] Add Alpine.js CDN script to `web/index.html` before `</head>`
- [ ] Add `alpine:init` event listener to `web/app.js`
- [ ] Create `Alpine.store('app')` with state properties and computed getters
- [ ] Add store action methods: `setStatus`, `updateAnalysis`, `updateConfig`, `onChannelChange`
- [ ] Verify store accessible in browser console: `Alpine.store('app')`

### Phase 2: Reactive Bindings

- [ ] Add `:class` and `x-text` to status indicator in `index.html`
- [ ] Add `x-model.number`, `@change`, `:disabled` to channel select
- [ ] Refactor `handleMessage()` in `app.js` to call store methods
- [ ] Remove `statusEl`, `channelModeSelect` queries from `app.js`
- [ ] Remove `setStatus()` function from `app.js`
- [ ] Test status and channel select update reactively

### Phase 3: Meter Migration

- [ ] Replace 4 meter blocks (lines 54-95) with single `x-for` template
- [ ] Add `.meter-label { text-transform: capitalize; }` to `style.css`
- [ ] Remove `beatMeter`, `bassMeter`, etc. queries from `app.js`
- [ ] Remove `beatValue`, `bassValue`, etc. queries from `app.js`
- [ ] Remove `updateMeter()` function from `app.js`
- [ ] Test all 4 meters update at 20Hz from WebSocket

### Phase 4: Documentation

- [ ] Create `web/CLAUDE.md` with conventions (see content below)
- [ ] Verify no `getElementById` calls remain in `app.js`
- [ ] Test full cycle: connect → analysis stream → channel change → reconnect

## Validation

- [ ] Alpine.js loads from CDN without errors
- [ ] Store initializes before Alpine scans DOM
- [ ] Status shows "Connecting..." → "Connected" with LED animation
- [ ] All 4 meters animate smoothly at 20Hz
- [ ] Meter values display with `.toFixed(2)` formatting
- [ ] Meter bars clamp at 100% width (computed getters work)
- [ ] Meter labels display with capital first letter (CSS capitalize)
- [ ] Channel select shows current mode from config message
- [ ] Changing channel select sends WebSocket command
- [ ] Select disabled when disconnected
- [ ] WebSocket reconnection still works
- [ ] No console errors related to undefined Alpine references
- [ ] `web/CLAUDE.md` contains all documented patterns
- [ ] Visual appearance identical to pre-migration

## web/CLAUDE.md Content

```markdown
# AudioJones Web UI

Alpine.js reactive interface for WebSocket-based remote control.

## Stack

- **Alpine.js 3.x** (CDN, no build step)
- **Vanilla JS** for WebSocket lifecycle
- **CSS-first** styling (no inline style objects except dynamic values)

## Architecture

### State Management

**Single `Alpine.store('app')`** - All reactive state:

```javascript
Alpine.store('app', {
    // Connection
    connected: false,
    statusText: 'Connecting...',

    // Analysis (updated 20Hz)
    beat: 0, bass: 0, mid: 0, treb: 0,

    // Config
    channelMode: 2,

    // Computed (clamped percentages)
    get beatPercent() { ... }
})
```

### WebSocket Integration

**Vanilla JS** for connection lifecycle. **Alpine store** for reactive state.

```javascript
// WebSocket handler calls store
function handleMessage(event) {
    const msg = JSON.parse(event.data);
    Alpine.store('app').updateAnalysis(msg);
}
```

## Conventions

### Binding Patterns

**Display state:**
```html
<div x-text="$store.app.statusText"></div>
<div :class="{'connected': $store.app.connected}"></div>
```

**Two-way controls:**
```html
<select x-model.number="$store.app.channelMode"
        @change="$store.app.onChannelChange()">
```

**Computed values:**
```html
<div :style="{width: $store.app.beatPercent + '%'}"></div>
```

### Loop Patterns

Use `x-for` for repetitive structures (3+ similar blocks):

```html
<template x-for="item in items" :key="item">
    <div x-text="item"></div>
</template>
```

### Store Method Naming

- `update*()` - Apply server data
- `set*()` - Direct state setter
- `on*()` - Event handler from Alpine binding

### Adding New Controls

1. Add state property to store
2. Add computed getter if derived value needed
3. Add action method if command required
4. Add binding to HTML
5. Handle in `updateConfig()` for server sync

Example (slider):
```javascript
// Store
volumeLevel: 50,
onVolumeChange() {
    window.sendCommand('setVolume', this.volumeLevel);
}

// HTML
<input type="range" x-model.number="$store.app.volumeLevel"
       @input.debounce.100ms="$store.app.onVolumeChange()">
```

### Panel Components (Future)

```html
<section class="panel" x-data="{ expanded: true }">
    <div class="panel-header" @click="expanded = !expanded">
        <span class="panel-title">Panel Name</span>
    </div>
    <div class="panel-content" x-show="expanded">
        <!-- Controls here -->
    </div>
</section>
```

### Meter Components

Data-driven via `x-for`:
```html
<template x-for="meter in ['beat', 'bass', 'mid', 'treb']" :key="meter">
    <div class="meter-container">
        <span class="meter-value" x-text="$store.app[meter].toFixed(2)"></span>
        <div class="meter-fill"
             :class="'meter-' + meter"
             :style="{width: $store.app[meter + 'Percent'] + '%'}"></div>
    </div>
</template>
```

### Dropdown Components

Options in HTML, state in store:
```html
<select x-model.number="$store.app.channelMode"
        @change="$store.app.onChannelChange()"
        :disabled="!$store.app.connected">
    <option value="0">Left</option>
    <!-- ... -->
</select>
```

## Anti-Patterns

**Don't mix Alpine and vanilla DOM updates:**
```javascript
// BAD
Alpine.store('app').statusText = 'Connected';
document.getElementById('status').textContent = 'Connected';
```

**Don't use Alpine for WebSocket lifecycle:**
```html
<!-- BAD -->
<div x-init="connectWebSocket()">
```

**Don't inline complex logic:**
```html
<!-- BAD -->
<div :style="{width: Math.min((beat / 1.0) * 100, 100) + '%'}"></div>

<!-- GOOD - use computed getter -->
<div :style="{width: $store.app.beatPercent + '%'}"></div>
```

## Performance

- Store updates at 20Hz are efficient
- Use `.debounce` modifier for user input (sliders)
- `x-show` for frequent toggles, `x-if` for rare visibility changes

## Debugging

```javascript
// Browser console
Alpine.store('app')                    // Inspect state
Alpine.store('app').beat = 0.5         // Test reactivity
wsService.ws.readyState                // Check connection (1 = OPEN)
```
```

## References

- [Alpine.js Documentation](https://alpinejs.dev/)
- [Alpine.js Store](https://alpinejs.dev/globals/alpine-store)
- Current implementation: `web/app.js`, `web/index.html`
- WebSocket protocol: `docs/plans/web-ui-control.md:58-92`
