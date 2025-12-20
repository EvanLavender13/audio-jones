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
```
