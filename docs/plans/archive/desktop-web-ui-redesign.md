# Desktop-Focused Web UI Redesign

Transform the web UI from mobile-first (480px) to desktop-focused (1440px+) with a sidebar+main layout optimized for wide monitors.

## Goal

Redesign the web interface for desktop-only use with:
- **Sidebar:** Always-visible analysis meters, presets panel, and section navigation
- **Main area:** Empty by default, shows detailed controls when a nav item is selected
- **Pro audio hardware aesthetic:** Maintain rack-unit styling, scale up for desktop
- **Future-ready architecture:** Support expansion to 75+ parameters across multiple sections

**Constraints:** Layout redesign only. Keep current functionality (channel mode, meters, presets). No new parameter controls beyond placeholder demonstrations.

## Current State

### Mobile-First Layout

**`web/index.html:5-8`** - Mobile meta tags:
```html
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
```

**`web/style.css:87-91`** - 480px container:
```css
.rack-unit {
    max-width: 480px;
    margin: 0 auto;
    padding: 20px;
}
```

**`web/style.css:510-582`** - Mobile breakpoints and touch styles (71 lines to delete)

### Current Panel Structure

**`web/index.html:14-138`**:
- Line 14: `.rack-unit` container
- Lines 16-25: Header (branding + status)
- Lines 28-52: Audio Input panel
- Lines 54-75: Analysis panel (meters)
- Lines 77-130: Presets panel
- Lines 132-137: Footer

## Architecture Decision

**Sidebar + Main Area with Master-Detail Pattern**

```
┌──────────────────────────────────────────────────────────────┐
│                    1440px minimum width                       │
├───────────────────────┬──────────────────────────────────────┤
│  SIDEBAR (320px)      │  MAIN AREA (flexible)                │
│  ─────────────────    │  ─────────────────────────────────   │
│                       │                                       │
│  ┌─────────────────┐  │  (Empty when no section selected)    │
│  │ AudioJones      │  │                                       │
│  │ ● Connected     │  │  ┌─────────────────────────────────┐ │
│  └─────────────────┘  │  │  Audio Input                    │ │
│                       │  │  ─────────────                   │ │
│  ┌─────────────────┐  │  │                                 │ │
│  │ ANALYSIS        │  │  │  Channel Mode: [Max ▼]          │ │
│  │ beat  0.45 ████ │  │  │                                 │ │
│  │ bass  1.23 ████ │  │  │  (shown when Audio nav clicked) │ │
│  │ mid   0.89 ████ │  │  └─────────────────────────────────┘ │
│  │ treb  0.67 ████ │  │                                       │
│  └─────────────────┘  │  ┌─────────────────────────────────┐ │
│                       │  │  Effects (placeholder)          │ │
│  ┌─────────────────┐  │  │  ─────────────                   │ │
│  │ PRESETS         │  │  │  Trail Half-Life   [────●──]    │ │
│  │ ┌─────────────┐ │  │  │  Base Blur Scale   [──●────]    │ │
│  │ │ Save: [___] │ │  │  │  Beat Blur Scale   [────●──]    │ │
│  │ │  [Save]     │ │  │  │                                 │ │
│  │ └─────────────┘ │  │  │  (placeholder structure only)   │ │
│  │ ○ Default       │  │  └─────────────────────────────────┘ │
│  │ ○ Energetic     │  │                                       │
│  │ ○ Chill         │  │                                       │
│  └─────────────────┘  │                                       │
│                       │                                       │
│  ─── NAVIGATION ───   │                                       │
│  [●] Audio            │                                       │
│  [ ] Effects          │                                       │
│  [ ] Waveforms        │                                       │
│  [ ] Spectrum         │                                       │
│                       │                                       │
└───────────────────────┴──────────────────────────────────────┘
```

**Rationale:**
- Analysis + Presets always visible = most-used features accessible
- Main area = detailed parameter editing space
- Nav items = clear path to future 75+ parameter sections
- Master-detail = familiar desktop pattern (file browsers, email clients)

## Component Design

### AppContainer

**Responsibilities:** Desktop grid layout, section state management

**HTML structure:**
```html
<body>
    <div class="app-container" x-data="{ activeSection: null }">
        <aside class="sidebar">
            <!-- Header, Analysis, Presets, Navigation -->
        </aside>
        <main class="main-area">
            <!-- Section content based on activeSection -->
        </main>
    </div>
</body>
```

**CSS:**
```css
.app-container {
    display: grid;
    grid-template-columns: 320px 1fr;
    min-width: 1440px;
    min-height: 100vh;
}
```

### Sidebar

**Responsibilities:** Always-visible panels, section navigation

**Structure:**
```html
<aside class="sidebar">
    <!-- Header with branding + status -->
    <header class="sidebar-header">
        <div class="brand">
            <div class="brand-name">AudioJones</div>
            <div class="brand-model">Web Control Interface</div>
        </div>
        <div class="status" :class="...">...</div>
    </header>

    <!-- Analysis meters (always visible) -->
    <section class="sidebar-panel">
        <div class="panel-header">
            <span class="panel-title">Analysis</span>
        </div>
        <div class="panel-content">
            <!-- 4 meters using existing x-for loop -->
        </div>
    </section>

    <!-- Presets (always visible) -->
    <section class="sidebar-panel">
        <div class="panel-header">
            <span class="panel-title">Presets</span>
        </div>
        <div class="panel-content">
            <!-- Save input + preset list -->
        </div>
    </section>

    <!-- Navigation -->
    <nav class="sidebar-nav">
        <div class="nav-label">Sections</div>
        <button class="nav-item"
                :class="{ active: activeSection === 'audio' }"
                @click="activeSection = activeSection === 'audio' ? null : 'audio'">
            Audio
        </button>
        <button class="nav-item"
                :class="{ active: activeSection === 'effects' }"
                @click="activeSection = activeSection === 'effects' ? null : 'effects'">
            Effects
        </button>
        <button class="nav-item"
                :class="{ active: activeSection === 'waveforms' }"
                @click="activeSection = activeSection === 'waveforms' ? null : 'waveforms'">
            Waveforms
        </button>
        <button class="nav-item"
                :class="{ active: activeSection === 'spectrum' }"
                @click="activeSection = activeSection === 'spectrum' ? null : 'spectrum'">
            Spectrum
        </button>
    </nav>
</aside>
```

### Main Area

**Responsibilities:** Display section content when nav item selected

**Structure:**
```html
<main class="main-area">
    <!-- Empty state -->
    <div class="main-empty" x-show="activeSection === null">
        <div class="empty-icon">◈</div>
        <div class="empty-text">Select a section from the sidebar</div>
    </div>

    <!-- Audio section (demonstrated) -->
    <div class="section" x-show="activeSection === 'audio'">
        <div class="section-header">
            <h2 class="section-title">Audio Input</h2>
        </div>
        <div class="section-content">
            <div class="control-row">
                <label class="control-label">Channel Mode</label>
                <div class="select-wrapper">
                    <select x-model.number="$store.app.channelMode"
                            @change="$store.app.onChannelChange()"
                            :disabled="!$store.app.connected">
                        <option value="0">Left</option>
                        <option value="1">Right</option>
                        <option value="2">Max</option>
                        <option value="3">Mix</option>
                        <option value="4">Side</option>
                        <option value="5">Interleaved</option>
                    </select>
                </div>
            </div>
        </div>
    </div>

    <!-- Effects section (placeholder) -->
    <div class="section" x-show="activeSection === 'effects'">
        <div class="section-header">
            <h2 class="section-title">Effects</h2>
            <span class="section-badge">Coming Soon</span>
        </div>
        <div class="section-content section-placeholder">
            <div class="placeholder-group">
                <h3 class="placeholder-title">Trail / Feedback</h3>
                <div class="control-row placeholder">
                    <label class="control-label">Trail Half-Life</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">0.50s</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Feedback Zoom</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">0.98</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Feedback Rotation</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">0.005</span>
                </div>
            </div>
            <div class="placeholder-group">
                <h3 class="placeholder-title">Blur</h3>
                <div class="control-row placeholder">
                    <label class="control-label">Base Blur Scale</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">1px</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Beat Blur Scale</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">2px</span>
                </div>
            </div>
        </div>
    </div>

    <!-- Waveforms section (placeholder) -->
    <div class="section" x-show="activeSection === 'waveforms'">
        <div class="section-header">
            <h2 class="section-title">Waveforms</h2>
            <span class="section-badge">Coming Soon</span>
        </div>
        <div class="section-content section-placeholder">
            <div class="waveform-tabs placeholder">
                <button class="waveform-tab active">W1</button>
                <button class="waveform-tab">W2</button>
                <button class="waveform-tab">W3</button>
                <button class="waveform-tab">W4</button>
            </div>
            <div class="placeholder-group">
                <div class="control-row placeholder">
                    <label class="control-label">Amplitude Scale</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">0.35</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Line Thickness</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">2px</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Smoothness</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">5.0</span>
                </div>
                <div class="control-row placeholder">
                    <label class="control-label">Radius</label>
                    <input type="range" class="control-slider" disabled>
                    <span class="control-value">0.25</span>
                </div>
            </div>
        </div>
    </div>

    <!-- Spectrum section (placeholder) -->
    <div class="section" x-show="activeSection === 'spectrum'">
        <div class="section-header">
            <h2 class="section-title">Spectrum Bars</h2>
            <span class="section-badge">Coming Soon</span>
        </div>
        <div class="section-content section-placeholder">
            <div class="control-row placeholder">
                <label class="control-label">Enable Spectrum</label>
                <input type="checkbox" class="control-checkbox" disabled>
            </div>
            <div class="control-row placeholder">
                <label class="control-label">Inner Radius</label>
                <input type="range" class="control-slider" disabled>
                <span class="control-value">0.15</span>
            </div>
            <div class="control-row placeholder">
                <label class="control-label">Bar Height</label>
                <input type="range" class="control-slider" disabled>
                <span class="control-value">0.25</span>
            </div>
        </div>
    </div>
</main>
```

### Control Row Component

**Pattern for future parameter controls:**

```html
<!-- Active control -->
<div class="control-row">
    <label class="control-label">Parameter Name</label>
    <input type="range" class="control-slider" min="0" max="1" step="0.01"
           x-model.number="$store.app.effects.paramName"
           @input.debounce.100ms="$store.app.onEffectsChange()">
    <span class="control-value" x-text="$store.app.effects.paramName.toFixed(2)"></span>
</div>

<!-- Placeholder control -->
<div class="control-row placeholder">
    <label class="control-label">Parameter Name</label>
    <input type="range" class="control-slider" disabled>
    <span class="control-value">0.50</span>
</div>
```

**CSS:**
```css
.control-row {
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 8px 0;
}

.control-label {
    min-width: 160px;
    font-size: 0.75rem;
    color: var(--text-secondary);
}

.control-slider {
    flex: 1;
    height: 6px;
    background: var(--metal-dark);
    border-radius: 3px;
    cursor: pointer;
}

.control-value {
    min-width: 60px;
    text-align: right;
    font-family: 'Oxanium', sans-serif;
    font-variant-numeric: tabular-nums;
    color: var(--text-primary);
}

.control-row.placeholder {
    opacity: 0.5;
    pointer-events: none;
}
```

## File Changes

| File | Change |
|------|--------|
| `web/index.html` | Restructure: remove mobile meta, grid layout, sidebar+main, section placeholders |
| `web/style.css` | Remove mobile CSS (~71 lines), add desktop layout (~200 lines) |
| `web/app.js` | No changes (Alpine store unchanged) |
| `web/CLAUDE.md` | Update with desktop patterns and conventions |

## Build Sequence

### Phase 1: Remove Mobile Code

**`web/index.html`:**
- [ ] Delete line 5: `<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">`
- [ ] Delete line 6: `<meta name="apple-mobile-web-app-capable" content="yes">`
- [ ] Delete line 7: `<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">`

**`web/style.css`:**
- [ ] Delete lines 510-549: `@media (max-width: 520px)` block (40 lines)
- [ ] Delete lines 552-582: `@media (hover: none) and (pointer: coarse)` block (31 lines)

### Phase 2: Desktop Layout Structure

**`web/style.css`:**
- [ ] Replace `.rack-unit` (lines 87-91) with `.app-container` grid layout
- [ ] Add `.sidebar` styles (320px width, sticky, overflow-y auto, metal background)
- [ ] Add `.main-area` styles (flex: 1, padding, overflow-y auto)
- [ ] Add `.sidebar-header` styles (branding in sidebar context)
- [ ] Add `.sidebar-panel` styles (analysis/presets panels)
- [ ] Add `.sidebar-nav` styles (navigation section)
- [ ] Add `.nav-item` styles (button states, active highlight)

**`web/index.html`:**
- [ ] Replace `<div class="rack-unit">` with `<div class="app-container" x-data="{ activeSection: null }">`
- [ ] Add `<aside class="sidebar">` wrapper
- [ ] Move header into sidebar as `.sidebar-header`
- [ ] Wrap Analysis panel in `.sidebar-panel`
- [ ] Wrap Presets panel in `.sidebar-panel`
- [ ] Add `<nav class="sidebar-nav">` with navigation buttons
- [ ] Add `<main class="main-area">` wrapper
- [ ] Delete footer (lines 132-137) - not needed in sidebar layout

### Phase 3: Main Area Sections

**`web/style.css`:**
- [ ] Add `.main-empty` styles (centered placeholder when no section selected)
- [ ] Add `.section` styles (max-width, padding)
- [ ] Add `.section-header` styles (title + badge)
- [ ] Add `.section-content` styles (flex column, gap)
- [ ] Add `.section-badge` styles (coming soon indicator)
- [ ] Add `.control-row` styles (label + input + value)
- [ ] Add `.control-slider` styles (range input styling)
- [ ] Add `.placeholder` styles (disabled/dimmed state)
- [ ] Add `.placeholder-group` styles (parameter groups with title)
- [ ] Add `.waveform-tabs` styles (tab buttons for waveform selection)

**`web/index.html`:**
- [ ] Add empty state div with icon and text
- [ ] Move Audio Input panel content into `x-show="activeSection === 'audio'"` section
- [ ] Add Effects placeholder section with Trail/Feedback and Blur groups
- [ ] Add Waveforms placeholder section with tab selector and parameters
- [ ] Add Spectrum placeholder section with enable toggle and parameters

### Phase 4: Navigation Interaction

**`web/index.html`:**
- [ ] Add `x-data="{ activeSection: null }"` to `.app-container`
- [ ] Add `@click="activeSection = activeSection === 'audio' ? null : 'audio'"` to Audio nav
- [ ] Add `:class="{ active: activeSection === 'audio' }"` to Audio nav
- [ ] Repeat click handler and class binding for Effects, Waveforms, Spectrum nav items
- [ ] Add `x-show="activeSection === null"` to empty state
- [ ] Add `x-show="activeSection === 'audio'"` etc. to each section

### Phase 5: Polish and Documentation

**`web/style.css`:**
- [ ] Add entrance animation for sections (fade in on show)
- [ ] Add hover states for nav items
- [ ] Ensure meter LED glows visible in sidebar context
- [ ] Test preset list scrolling in sidebar panel

**`web/CLAUDE.md`:**
- [ ] Update layout documentation (sidebar + main pattern)
- [ ] Add section navigation pattern
- [ ] Add control-row component pattern
- [ ] Add placeholder pattern for future sections
- [ ] Remove mobile-related conventions

## Validation

### Layout
- [ ] Page renders at 1440px minimum width without horizontal scroll
- [ ] Sidebar is 320px fixed width on left
- [ ] Main area fills remaining width on right
- [ ] No mobile meta tags in HTML head
- [ ] No responsive breakpoints in CSS
- [ ] No touch-friendly styles in CSS

### Sidebar
- [ ] Header shows branding and connection status in sidebar
- [ ] Analysis meters always visible with LED colors and glow
- [ ] Meters update at 20Hz (no regression)
- [ ] Presets panel always visible with save input and list
- [ ] Preset load/save/delete operations work unchanged
- [ ] Navigation section shows Audio, Effects, Waveforms, Spectrum items
- [ ] Sidebar scrolls independently when content overflows

### Main Area
- [ ] Empty state shows when no section selected (icon + text)
- [ ] Clicking Audio nav shows Audio Input section in main area
- [ ] Clicking Audio nav again hides section (toggle behavior)
- [ ] Channel mode select works in Audio section
- [ ] Effects placeholder shows Trail/Feedback and Blur parameter groups
- [ ] Waveforms placeholder shows tab selector and parameters
- [ ] Spectrum placeholder shows enable toggle and parameters
- [ ] "Coming Soon" badges visible on placeholder sections
- [ ] Placeholder controls are disabled/dimmed

### Aesthetics
- [ ] Hardware rack aesthetic maintained (metal backgrounds, LED glows)
- [ ] Brushed metal gradient on sidebar
- [ ] Panel headers with indicator LEDs
- [ ] Active nav item has LED accent highlight
- [ ] Typography hierarchy clear (Oxanium headers, JetBrains Mono values)
- [ ] CSS variable color palette unchanged

### Functionality
- [ ] WebSocket connection and reconnection work
- [ ] Status indicator updates correctly
- [ ] No console errors on page load or interaction

## References

**Current implementation:**
- `web/index.html:1-142` - Mobile-first structure
- `web/style.css:1-799` - Hardware aesthetic CSS
- `web/app.js:1-223` - Alpine.js store and WebSocket
- `web/CLAUDE.md` - Current conventions

**Parameter scope (for future sections):**
- `src/config/effect_config.h` - 32+ effect parameters
- `src/config/waveform_config.h` - 7 parameters × 8 waveforms
- `src/config/spectrum_bars_config.h` - 10 spectrum parameters
- `src/config/band_config.h` - 6 band sensitivity parameters
