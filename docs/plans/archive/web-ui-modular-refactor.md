# Web UI Modular Refactor

Rewrite web UI with modular Alpine.js stores, modular CSS, and updated documentation.

## Goal

Transform monolithic web UI (1 HTML, 1 JS, 1 CSS) into a modular system with domain-specific stores, feature-based CSS, and pattern-focused documentation. Desktop-only, same visual design, no build step.

## Current State

### Monolithic Structure

- `web/app.js:1-386` - Single Alpine store with all state/methods mixed together
- `web/style.css:1-1232` - All styles in one file, dead code accumulating
- `web/CLAUDE.md` - Bloated with code examples that weren't followed

### Problems

1. **JS monolith** - 290-line Alpine store mixes connection, analysis, presets, waveforms, color helpers
2. **CSS accumulation** - New styles added instead of reusing; dead styles never removed
3. **Hue slider** - DOM event listeners mixed with Alpine store (`initHueSlider`)
4. **Documentation** - Tutorial-style examples instead of enforceable patterns

### What Works

- Backend WebSocket API is solid
- Visual design (hardware rack aesthetic) is good
- Current functionality is complete for Audio + Waveforms sections

## Architecture Decision

**Pragmatic modular approach** - 13 files total.

### Why This Approach

- **4 domain stores** map to clear concerns (like C++ module organization)
- **1 component extraction** - Only hue slider is complex enough to warrant `Alpine.data()`
- **6 CSS files** - Feature zones, not per-component (avoids 20+ tiny files)
- **Pattern-focused docs** - Rules to follow, not examples to copy

### Key Patterns

**Alpine.js (from web research):**
- ES modules work without build step
- Plugin pattern for stores: `export default function(Alpine) { Alpine.store(...) }`
- `Alpine.data()` only for truly reusable/complex components
- Order: register stores → register components → `Alpine.start()` once

**Store access:**
```html
$store.connection.connected
$store.analysis.beatPercent
$store.presets.list
$store.waveforms.current
```

## Component Design

### File Structure

```
web/
├── index.html                    # Shell - loads Alpine CDN, imports modules
│
├── js/
│   ├── stores/
│   │   ├── connection.js         # WebSocket lifecycle + connected/statusText
│   │   ├── analysis.js           # beat/bass/mid/treb + computed percentages
│   │   ├── presets.js            # Preset CRUD operations
│   │   └── waveforms.js          # Waveform state + all waveform methods
│   │
│   ├── components/
│   │   └── hue-slider.js         # Dual-handle hue range (Alpine.data)
│   │
│   └── init.js                   # Store registration + initWebSocket()
│
├── css/
│   ├── base.css                  # Variables, reset, body, fonts
│   ├── layout.css                # Grid, sidebar, main-area, sections
│   ├── controls.css              # Sliders, selects, buttons, inputs
│   ├── meters.css                # LED meter fills and animations
│   ├── panels.css                # Sidebar panels, headers, nav
│   └── presets.css               # Preset list, items, save row
│
└── CLAUDE.md                     # Pattern reference (rewritten)
```

### Store Responsibilities

**`connection.js`** (~40 lines):
- `connected`, `statusText` state
- `setStatus()` method
- WebSocket connect/reconnect logic
- `sendCommand()` global function
- Message dispatch to other stores

**`analysis.js`** (~30 lines):
- `beat`, `bass`, `mid`, `treb` values
- Computed percentages: `beatPercent`, etc.
- `update(data)` method

**`presets.js`** (~50 lines):
- `list`, `selected`, `name`, `status` state
- `load()`, `save()`, `delete()`, `refresh()` methods
- Status message auto-clear (3s timeout)

**`waveforms.js`** (~150 lines):
- `count`, `items`, `selectedIndex` state
- `current` getter
- CRUD: `add()`, `remove()`, `select()`
- Param changes: `setParam()`, `setColorMode()`, `setSolidColor()`, `setRainbow()`
- Color helpers: `getSolidHex()`, `setSolidHex()`, `setAlpha()`
- Hue range computed styles: `spectrumGradient`, `hueSelectionStyle`, etc.

### Component: Hue Slider

**`hue-slider.js`** (~80 lines):

Extracted as `Alpine.data('hueSlider')` because:
- Complex interaction (dual-handle dragging)
- Local state (`dragging: null`)
- DOM event listeners (mousedown, mousemove, mouseup)
- Self-contained logic

```javascript
Alpine.data('hueSlider', () => ({
    dragging: null,

    get spectrumGradient() { ... },
    get selectionStyle() { ... },
    get startStyle() { ... },
    get endStyle() { ... },
    get valueText() { ... },

    startDrag(e) { ... },
    onDrag(e) { ... },
    endDrag() { ... },

    init() {
        document.addEventListener('mousemove', (e) => this.onDrag(e));
        document.addEventListener('mouseup', () => this.endDrag());
    },

    destroy() {
        // Cleanup listeners if needed
    }
}));
```

### CSS Organization

| File | ~Lines | Contents |
|------|--------|----------|
| `base.css` | 80 | `:root` variables, reset, body, font imports |
| `layout.css` | 200 | `.app-container` grid, `.sidebar`, `.main-area`, `.section` |
| `controls.css` | 200 | `.control-row`, `.control-slider`, `select`, `.btn-*`, inputs |
| `meters.css` | 150 | `.meter`, `.meter-fill`, `.meter-beat/bass/mid/treb`, animations |
| `panels.css` | 250 | `.sidebar-panel`, `.panel-header`, `.nav-item`, `.waveform-tab` |
| `presets.css` | 180 | `.preset-list`, `.preset-item`, `.preset-input`, `.btn-save` |

### CLAUDE.md Rewrite

Replace tutorial with pattern rules:

```markdown
# AudioJones Web UI

## Stack
Alpine.js 3.x (CDN), ES modules, no build step. Desktop-only (1440px min).

## Store Access
- Connection: `$store.connection.connected`, `$store.connection.statusText`
- Analysis: `$store.analysis.beat`, `$store.analysis.beatPercent`
- Presets: `$store.presets.list`, `$store.presets.save()`
- Waveforms: `$store.waveforms.current`, `$store.waveforms.setParam()`

## Adding Controls
1. Bind to store: `x-model="$store.waveforms.current.radius"`
2. Debounce input: `@input.debounce.100ms="$store.waveforms.setParam('radius', $event.target.value)"`
3. Disable when disconnected: `:disabled="!$store.connection.connected"`

## CSS Rules
- New control style? Add to `controls.css`
- New panel component? Add to `panels.css`
- Reuse existing classes before creating new ones
- Delete unused styles immediately

## File Ownership
| Change | File |
|--------|------|
| WebSocket behavior | `js/stores/connection.js` |
| Meter calculations | `js/stores/analysis.js` |
| Preset operations | `js/stores/presets.js` |
| Waveform params | `js/stores/waveforms.js` |
| Hue picker logic | `js/components/hue-slider.js` |
```

## File Changes

| File | Action | Description |
|------|--------|-------------|
| `web/index.html` | Modify | Update CSS links, add module imports |
| `web/app.js` | Delete | Split into stores/components |
| `web/style.css` | Delete | Split into CSS modules |
| `web/CLAUDE.md` | Rewrite | Pattern reference, not tutorial |
| `web/js/stores/connection.js` | Create | WebSocket + connection state |
| `web/js/stores/analysis.js` | Create | Analysis values + percentages |
| `web/js/stores/presets.js` | Create | Preset CRUD |
| `web/js/stores/waveforms.js` | Create | Waveform state + methods |
| `web/js/components/hue-slider.js` | Create | Dual-handle component |
| `web/js/init.js` | Create | Store registration |
| `web/css/base.css` | Create | Variables, reset |
| `web/css/layout.css` | Create | Grid, sidebar, main |
| `web/css/controls.css` | Create | Form controls |
| `web/css/meters.css` | Create | LED meters |
| `web/css/panels.css` | Create | Panels, nav, tabs |
| `web/css/presets.css` | Create | Preset list |

## Build Sequence

### Phase 1: Directory Structure

Create directories, verify site still loads with old files.

- [ ] Create `web/js/stores/`
- [ ] Create `web/js/components/`
- [ ] Create `web/css/`

### Phase 2: CSS Split

Extract CSS into feature files. No functional changes.

- [ ] Create `web/css/base.css` - lines 1-81 from style.css (variables, reset, body)
- [ ] Create `web/css/layout.css` - lines 84-254 (grid, sidebar, main-area, sections)
- [ ] Create `web/css/controls.css` - lines 294-372, 756-830 (sliders, selects, buttons)
- [ ] Create `web/css/meters.css` - lines 832-985 (meter components)
- [ ] Create `web/css/panels.css` - lines 148-210, 398-520, 632-660, 722-754 (panels, nav, tabs)
- [ ] Create `web/css/presets.css` - lines 1010-1176 (preset list)
- [ ] Update `web/index.html` to load 6 CSS files
- [ ] Delete `web/style.css`
- [ ] Verify: Visual appearance unchanged

### Phase 3: JS Store Split

Split monolithic store into domain stores.

- [ ] Create `web/js/stores/connection.js` - WebSocket lifecycle, sendCommand
- [ ] Create `web/js/stores/analysis.js` - beat/bass/mid/treb + percentages
- [ ] Create `web/js/stores/presets.js` - list, CRUD methods
- [ ] Create `web/js/stores/waveforms.js` - waveforms array, all waveform methods
- [ ] Create `web/js/init.js` - register stores, call initWebSocket
- [ ] Update `web/index.html` - add module imports, remove old script
- [ ] Update HTML bindings - `$store.app.*` → `$store.{domain}.*`
- [ ] Verify: WebSocket connects, meters update, presets work

### Phase 4: Hue Slider Component

Extract complex widget to Alpine.data().

- [ ] Create `web/js/components/hue-slider.js`
- [ ] Remove `initHueSlider` from waveforms store
- [ ] Update HTML to use `x-data="hueSlider()"`
- [ ] Verify: Dual-handle drag works, rainbow colors update

### Phase 5: Cleanup + Documentation

Delete old files, rewrite docs.

- [ ] Delete `web/app.js`
- [ ] Rewrite `web/CLAUDE.md` with pattern rules
- [ ] Final verification of all functionality

## Validation

### Structure
- [ ] 6 CSS files load without 404
- [ ] 5 JS modules load without errors
- [ ] No references to deleted files

### Functionality
- [ ] WebSocket connects and reconnects
- [ ] Analysis meters update at 20Hz
- [ ] Preset save/load/delete works
- [ ] Waveform add/remove/select works
- [ ] All sliders send debounced commands
- [ ] Hue slider dual-handle drag works
- [ ] Color picker hex/alpha works
- [ ] All sections navigate correctly

### Code Quality
- [ ] Each store has single domain responsibility
- [ ] CSS files group related styles
- [ ] No duplicate styles across files
- [ ] No dead code remaining
- [ ] CLAUDE.md documents patterns, not tutorials

## References

- Current web files: `web/app.js`, `web/style.css`, `web/index.html`
- Backend API: `src/web/web_bridge.cpp`
- Alpine.js docs: https://alpinejs.dev/
- ES modules: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Modules
