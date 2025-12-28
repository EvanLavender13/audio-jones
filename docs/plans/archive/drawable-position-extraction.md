# Drawable Position Extraction

Add configurable X/Y center position to waveforms and spectrum bars. First step toward unified drawable architecture (see `docs/research/unified-drawable-architecture.md`).

## Current State

Position handling today:

| Location | Current Behavior |
|----------|------------------|
| `waveform.cpp:281-282` | Circular: `ctx->centerX + cos(angle) * radius` |
| `waveform.cpp:219-220` | Linear: `ctx->centerY - samples[i] * amplitude - yOffset` |
| `spectrum_bars.cpp:165-167` | Circular: `ctx->centerX + cos(angle) * r` |
| `spectrum_bars.cpp:221` | Linear: `ctx->centerY - barHeight * 0.5f` |

Key files:
- `src/config/waveform_config.h` - WaveformConfig struct
- `src/config/spectrum_bars_config.h` - SpectrumConfig struct
- `src/render/waveform.cpp` - Waveform drawing
- `src/render/spectrum_bars.cpp` - Spectrum bar drawing
- `src/ui/imgui_waveforms.cpp` - Waveform UI panel
- `src/ui/imgui_spectrum.cpp` - Spectrum UI panel
- `src/config/preset.cpp` - JSON serialization

---

## Phase 1: Add Position Fields to Configs

**Goal**: Add `x`, `y` fields to WaveformConfig and SpectrumConfig.

**Modify**:
- `src/config/waveform_config.h` - Add `float x = 0.5f` and `float y = 0.5f`
- `src/config/spectrum_bars_config.h` - Add `float x = 0.5f` and `float y = 0.5f`

**Done when**: Configs compile with new fields, defaults at center (0.5, 0.5).

---

## Phase 2: Update Circular Waveform Drawing

**Goal**: Replace hardcoded `ctx->centerX/Y` with config-driven position.

**Modify**:
- `src/render/waveform.cpp:234-287` (`DrawWaveformCircular`)
  - Compute `centerX = cfg->x * ctx->screenW`
  - Compute `centerY = cfg->y * ctx->screenH`
  - Replace `ctx->centerX` → `centerX` and `ctx->centerY` → `centerY`

**Done when**: Circular waveform renders at config position. Default 0.5, 0.5 produces identical output to before.

---

## Phase 3: Update Linear Waveform Drawing

**Goal**: Replace `verticalOffset` usage with `y` field.

**Modify**:
- `src/render/waveform.cpp:197-232` (`DrawWaveformLinear`)
  - Compute `centerY = cfg->y * ctx->screenH`
  - Remove `yOffset` calculation from `verticalOffset`
  - Use `centerY` directly: `centerY - samples[i] * amplitude`

**Note**: `x` doesn't affect linear waveform since it spans full width. The `y` field replaces `verticalOffset` semantically (0.5 = center, 0.0 = top, 1.0 = bottom).

**Done when**: Linear waveform renders at `y` position. Setting `y = 0.5` matches previous `verticalOffset = 0.0` behavior.

---

## Phase 4: Update Circular Spectrum Drawing

**Goal**: Replace hardcoded `ctx->centerX/Y` with config-driven position.

**Modify**:
- `src/render/spectrum_bars.cpp:134-188` (`SpectrumBarsDrawCircular`)
  - Compute `centerX = config->x * ctx->screenW`
  - Compute `centerY = config->y * ctx->screenH`
  - Replace all `ctx->centerX` → `centerX` and `ctx->centerY` → `centerY`

**Done when**: Circular spectrum renders at config position.

---

## Phase 5: Update Linear Spectrum Drawing

**Goal**: Use `y` field for vertical centering.

**Modify**:
- `src/render/spectrum_bars.cpp:190-225` (`SpectrumBarsDrawLinear`)
  - Compute `centerY = config->y * ctx->screenH`
  - Replace `ctx->centerY` → `centerY` in bar Y calculation

**Note**: `x` doesn't affect linear spectrum since it spans full width.

**Done when**: Linear spectrum renders at `y` position.

---

## Phase 6: Update UI Panels

**Goal**: Add X/Y sliders to both panels.

**Modify**:
- `src/ui/imgui_waveforms.cpp:85-92` (Geometry section)
  - Add `ImGui::SliderFloat("X", &sel->x, 0.0f, 1.0f)`
  - Add `ImGui::SliderFloat("Y", &sel->y, 0.0f, 1.0f)`
  - Remove Y-Offset slider (replaced by Y)

- `src/ui/imgui_spectrum.cpp:29-34` (Geometry section)
  - Add `ImGui::SliderFloat("X", &cfg->x, 0.0f, 1.0f)`
  - Add `ImGui::SliderFloat("Y", &cfg->y, 0.0f, 1.0f)`

**Done when**: UI shows X/Y sliders, dragging them moves drawables.

---

## Phase 7: Update Preset Serialization

**Goal**: Persist `x`, `y` fields in presets.

**Modify**:
- `src/config/preset.cpp:84-85` - Add `x, y` to WaveformConfig serialization macro
- `src/config/preset.cpp:86-88` - Add `x, y` to SpectrumConfig serialization macro

**Done when**: Save/load preset preserves X/Y positions.

---

## Phase 8: Remove Deprecated Field

**Goal**: Clean up `verticalOffset` from WaveformConfig.

**Modify**:
- `src/config/waveform_config.h` - Remove `verticalOffset` field
- `src/ui/imgui_waveforms.cpp` - Verify Y-Offset slider already removed

**Note**: Existing presets with `verticalOffset` will simply ignore it (nlohmann handles unknown fields gracefully).

**Done when**: `verticalOffset` fully removed, no compiler errors.

---

## Verification

After all phases:
1. Default preset renders identically to before (x=0.5, y=0.5)
2. Circular waveform/spectrum can be positioned anywhere on screen
3. Linear waveform/spectrum vertical position controlled by Y slider
4. Presets save/load position correctly
5. Build passes with no warnings
