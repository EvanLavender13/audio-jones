# Cymatics: Symmetric Source Arrangements

Replace the hardcoded `baseSources` array with dynamic circle distribution. Sources evenly spaced around a circle at configurable radius and rotation angle.

## Current State

- `src/simulation/cymatics.cpp:128-137` — hardcoded `baseSources[8][2]` array (Center, Left, Right, Bottom, Top, etc.)
- `src/simulation/cymatics.cpp:145-149` — loop applies Lissajous animation to base positions
- `src/simulation/cymatics.h:21-24` — existing config: `sourceCount`, `sourceAmplitude`, `sourceFreqX`, `sourceFreqY`

## Phase 1: Config and Serialization

**Goal**: Add `baseRadius` and `patternAngle` fields to `CymaticsConfig`.

**Build**:
- `cymatics.h` — Add to `CymaticsConfig`:
  - `float baseRadius = 0.4f` (0.0-0.5, distance from center)
  - `float patternAngle = 0.0f` (radians, rotation offset)
- `preset.cpp:111-114` — Add `baseRadius`, `patternAngle` to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro

**Done when**: Project compiles with new fields.

---

## Phase 2: Dynamic Source Positioning

**Goal**: Replace hardcoded array with circular distribution.

**Build**:
- `cymatics.cpp` — In `CymaticsUpdate`:
  - Remove the `static const float baseSources[8][2]` array (lines 128-137)
  - Replace the source position loop with:

```cpp
for (int i = 0; i < count; i++) {
    const float angle = TWO_PI * (float)i / (float)count + cym->config.patternAngle;
    const float baseX = cym->config.baseRadius * cosf(angle);
    const float baseY = cym->config.baseRadius * sinf(angle);
    const float offset = (float)i / (float)count * TWO_PI;
    sources[i * 2 + 0] = baseX + amp * sinf(phaseX + offset);
    sources[i * 2 + 1] = baseY + amp * cosf(phaseY + offset);
}
```

**Done when**: Running with 2 sources produces symmetric left/right pattern (not center+left).

---

## Phase 3: UI Controls

**Goal**: Add sliders for `baseRadius` and `patternAngle`.

**Build**:
- `imgui_effects.cpp` — After line 377 (`Sources##cym` slider), add:
  - `ModulatableSlider("Base Radius##cym", &e->cymatics.baseRadius, "cymatics.baseRadius", "%.2f", modSources)` (0.0-0.5)
  - `ModulatableSliderAngleDeg("Pattern Angle##cym", &e->cymatics.patternAngle, "cymatics.patternAngle", modSources)` (uses `ROTATION_OFFSET_MAX`)

**Done when**: UI sliders appear and adjust pattern in real-time.

---

## Phase 4: Modulation Registration

**Goal**: Register new fields for audio modulation.

**Build**:
- `param_registry.cpp` — Add to `PARAM_TABLE` after existing cymatics entries:
  - `{"cymatics.baseRadius", {0.0f, 0.5f}}`
  - `{"cymatics.patternAngle", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}}`
- `param_registry.cpp` — Add to the param pointer array:
  - `&effects->cymatics.baseRadius`
  - `&effects->cymatics.patternAngle`

**Done when**: Parameters appear in modulation dropdown and respond to audio.
