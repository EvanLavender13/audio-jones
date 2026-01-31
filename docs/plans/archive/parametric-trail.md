# Parametric Trail

A cursor traces Lissajous curves, depositing strokes that accumulate via feedback. Time-gating toggles between smooth continuous lines and scattered chaotic fragments. Primary and secondary harmonic frequencies create complex multi-harmonic paths.

<!-- Intentional deviation: Spiral path type omitted per user choice (Lissajous only for initial implementation) -->
<!-- Intentional deviation: HSV color cycling omitted per user choice (uses existing ColorConfig system instead) -->

## Specification

### Types

```cpp
// Add to DrawableType enum in drawable_config.h
DRAWABLE_PARAMETRIC_TRAIL

// New struct in drawable_config.h
struct ParametricTrailData {
  // Lissajous parameters
  float speed = 1.0f;           // Phase accumulation rate
  float amplitude = 0.25f;      // Path size (fraction of screen)
  float freqX1 = 3.14159f;      // Primary X frequency
  float freqY1 = 1.0f;          // Primary Y frequency
  float freqX2 = 0.72834f;      // Secondary X frequency (0 = disabled)
  float freqY2 = 2.781374f;     // Secondary Y frequency (0 = disabled)
  float offsetX = 0.3f;         // Phase offset for secondary X (radians)
  float offsetY = 3.47912f;     // Phase offset for secondary Y (radians)

  // Stroke parameters
  float thickness = 4.0f;       // Stroke width in pixels
  bool roundedCaps = true;      // Circle caps at segment endpoints

  // Draw gate parameters
  bool gateEnabled = false;     // Enable time-gating
  float gateFreq = 5.0f;        // Gate oscillation frequency (Hz)
  float gateDuty = 0.5f;        // Duty cycle (fraction of time drawing)

  // Runtime state (not serialized)
  float phase = 0.0f;           // Accumulated phase (like rotationAccum)
  float prevX = 0.0f;           // Previous cursor X (normalized 0-1)
  float prevY = 0.0f;           // Previous cursor Y (normalized 0-1)
  bool hasPrevPos = false;      // Valid previous position exists
};
```

### Algorithm

**Phase Accumulation** (called from DrawableTickRotations, NOT in render):
```cpp
// In DrawableTickRotations, add case for DRAWABLE_PARAMETRIC_TRAIL:
if (drawables[i].type == DRAWABLE_PARAMETRIC_TRAIL) {
  drawables[i].parametricTrail.phase +=
      drawables[i].parametricTrail.speed * deltaTime;
}
```

**Cursor Position** (Lissajous with dual harmonics):
```cpp
ParametricTrailData& trail = d->parametricTrail;
float x = d->base.x + trail.amplitude *
    (sinf(trail.freqX1 * trail.phase) +
     sinf(trail.freqX2 * trail.phase + trail.offsetX));
float y = d->base.y + trail.amplitude *
    (sinf(trail.freqY1 * trail.phase) +
     sinf(trail.freqY2 * trail.phase + trail.offsetY));
```

**Draw Gate Check** (uses GetTime() directly like other raylib timing):
```cpp
bool shouldDraw = true;
if (trail.gateEnabled && trail.gateFreq > 0.0f) {
  float gatePhase = fmodf((float)GetTime() * trail.gateFreq, 1.0f);
  shouldDraw = gatePhase < trail.gateDuty;
}
```

**Stroke Rendering** (when gate allows and hasPrevPos):
```cpp
// Convert normalized coords to screen pixels
Vector2 prevScreen = {trail.prevX * ctx->screenW, trail.prevY * ctx->screenH};
Vector2 currScreen = {x * ctx->screenW, y * ctx->screenH};

// Color: use phase for t value (creates cycling color as cursor moves)
float t = fmodf(trail.phase, 1.0f);
Color color = ColorFromConfig(&d->base.color, t, opacity);

// Draw thick line segment
ThickLineBegin(trail.thickness);
ThickLineVertex(prevScreen, color);
ThickLineVertex(currScreen, color);
ThickLineEnd(false);

// Rounded caps via circles at endpoints
if (trail.roundedCaps) {
  DrawCircleV(prevScreen, trail.thickness * 0.5f, color);
  DrawCircleV(currScreen, trail.thickness * 0.5f, color);
}
```

**State Update** (always, regardless of gate):
```cpp
trail.prevX = x;
trail.prevY = y;
trail.hasPrevPos = true;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| speed | float | 0.1-10.0 | 1.0 | Yes | Speed |
| amplitude | float | 0.05-0.5 | 0.25 | Yes | Size |
| freqX1 | float | 0.1-5.0 | 3.14159 | Yes | Freq X1 |
| freqY1 | float | 0.1-5.0 | 1.0 | Yes | Freq Y1 |
| freqX2 | float | 0.0-5.0 | 0.72834 | Yes | Freq X2 |
| freqY2 | float | 0.0-5.0 | 2.781374 | Yes | Freq Y2 |
| offsetX | float | 0-2π | 0.3 | No | Offset X |
| offsetY | float | 0-2π | 3.47912 | No | Offset Y |
| thickness | float | 1.0-50.0 | 4.0 | Yes | Thickness |
| roundedCaps | bool | - | true | No | Rounded |
| gateEnabled | bool | - | false | No | Gate |
| gateFreq | float | 0.1-20.0 | 5.0 | Yes | Gate Freq |
| gateDuty | float | 0.0-1.0 | 0.5 | Yes | Gate Duty |

### Constants

- Enum value: `DRAWABLE_PARAMETRIC_TRAIL`
- Display name: `"Parametric Trail"`
- List prefix: `[T]` (for Trail)

---

## Tasks

### Wave 1: Config Struct

#### Task 1.1: Add ParametricTrailData and enum value

**Files**: `src/config/drawable_config.h`
**Creates**: `ParametricTrailData` struct, `DRAWABLE_PARAMETRIC_TRAIL` enum value

**Build**:
1. Add `DRAWABLE_PARAMETRIC_TRAIL` to `DrawableType` enum after `DRAWABLE_SHAPE`
2. Add `ParametricTrailData` struct before `Drawable` struct, with all fields from Specification > Types
3. Add `ParametricTrailData parametricTrail;` to union in `Drawable` struct after `ShapeData shape;`
4. Add case to copy constructor switch for `DRAWABLE_PARAMETRIC_TRAIL`: copy `parametricTrail = other.parametricTrail;`
5. Add case to assignment operator switch for `DRAWABLE_PARAMETRIC_TRAIL`: copy `parametricTrail = other.parametricTrail;`

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Phase Accumulation

**Files**: `src/render/drawable.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `DrawableTickRotations` function (lines 236-240), add after the rotation accumulation:
```cpp
if (drawables[i].type == DRAWABLE_PARAMETRIC_TRAIL) {
  drawables[i].parametricTrail.phase +=
      drawables[i].parametricTrail.speed * deltaTime;
}
```

**Verify**: Compiles.

---

#### Task 2.2: Rendering

**Files**: `src/render/drawable.cpp`
**Depends on**: Wave 1 complete, Task 2.1 complete

**Build**:
1. Add includes at top if not present:
   - `#include <math.h>`
   - `#include "draw_utils.h"`
   - `#include "thick_line.h"`
2. Add static helper function before `DrawableRenderFull`:
```cpp
static void DrawableRenderParametricTrail(RenderContext *ctx, Drawable *d,
                                          uint64_t tick, float opacity) {
  ParametricTrailData &trail = d->parametricTrail;

  // Compute cursor position (Lissajous with dual harmonics)
  float x = d->base.x + trail.amplitude *
      (sinf(trail.freqX1 * trail.phase) +
       sinf(trail.freqX2 * trail.phase + trail.offsetX));
  float y = d->base.y + trail.amplitude *
      (sinf(trail.freqY1 * trail.phase) +
       sinf(trail.freqY2 * trail.phase + trail.offsetY));

  // Draw gate check
  bool shouldDraw = true;
  if (trail.gateEnabled && trail.gateFreq > 0.0f) {
    float gatePhase = fmodf((float)GetTime() * trail.gateFreq, 1.0f);
    shouldDraw = gatePhase < trail.gateDuty;
  }

  // Render stroke if gate allows and have previous position
  if (shouldDraw && trail.hasPrevPos) {
    Vector2 prevScreen = {trail.prevX * ctx->screenW, trail.prevY * ctx->screenH};
    Vector2 currScreen = {x * ctx->screenW, y * ctx->screenH};

    float t = fmodf(trail.phase, 1.0f);
    Color color = ColorFromConfig(&d->base.color, t, opacity);

    ThickLineBegin(trail.thickness);
    ThickLineVertex(prevScreen, color);
    ThickLineVertex(currScreen, color);
    ThickLineEnd(false);

    if (trail.roundedCaps) {
      DrawCircleV(prevScreen, trail.thickness * 0.5f, color);
      DrawCircleV(currScreen, trail.thickness * 0.5f, color);
    }
  }

  // Always update state
  trail.prevX = x;
  trail.prevY = y;
  trail.hasPrevPos = true;
}
```
3. In `DrawableRenderFull` Pass 2 loop (around line 191), add case for `DRAWABLE_PARAMETRIC_TRAIL`:
```cpp
case DRAWABLE_PARAMETRIC_TRAIL:
  DrawableRenderParametricTrail(ctx, &drawables[i], tick, opacity);
  break;
```
Note: Need non-const `Drawable*` for state update. Change loop variable or cast.

**Verify**: Compiles.

---

#### Task 2.3: UI Controls

**Files**: `src/ui/drawable_type_controls.cpp`, `src/ui/drawable_type_controls.h`
**Depends on**: Wave 1 complete

**Build**:
1. In `drawable_type_controls.h`: Add declaration:
   ```cpp
   void DrawParametricTrailControls(Drawable *d, const ModSources *sources);
   ```
2. In `drawable_type_controls.cpp`:
   - Add section state variables at file scope:
     ```cpp
     static bool sectionPath = true;
     static bool sectionStroke = true;
     static bool sectionGate = true;
     ```
   - Implement function:
```cpp
void DrawParametricTrailControls(Drawable *d, const ModSources *sources) {
  if (DrawSectionBegin("Path", Theme::GLOW_CYAN, &sectionPath)) {
    ModulatableDrawableSlider("X", &d->base.x, d->id, "x", "%.2f", sources);
    ModulatableDrawableSlider("Y", &d->base.y, d->id, "y", "%.2f", sources);
    ModulatableDrawableSlider("Speed", &d->parametricTrail.speed, d->id,
                              "speed", "%.2f", sources);
    ModulatableDrawableSlider("Size", &d->parametricTrail.amplitude, d->id,
                              "amplitude", "%.2f", sources);
    ModulatableDrawableSlider("Freq X1", &d->parametricTrail.freqX1, d->id,
                              "freqX1", "%.2f", sources);
    ModulatableDrawableSlider("Freq Y1", &d->parametricTrail.freqY1, d->id,
                              "freqY1", "%.2f", sources);
    ModulatableDrawableSlider("Freq X2", &d->parametricTrail.freqX2, d->id,
                              "freqX2", "%.2f", sources);
    ModulatableDrawableSlider("Freq Y2", &d->parametricTrail.freqY2, d->id,
                              "freqY2", "%.2f", sources);
    SliderAngleDeg("Offset X", &d->parametricTrail.offsetX);
    SliderAngleDeg("Offset Y", &d->parametricTrail.offsetY);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Stroke", Theme::GLOW_MAGENTA, &sectionStroke)) {
    ModulatableDrawableSlider("Thickness", &d->parametricTrail.thickness, d->id,
                              "thickness", "%.0f px", sources);
    ImGui::Checkbox("Rounded", &d->parametricTrail.roundedCaps);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Gate", Theme::GLOW_ORANGE, &sectionGate)) {
    ImGui::Checkbox("Enabled", &d->parametricTrail.gateEnabled);
    if (d->parametricTrail.gateEnabled) {
      ModulatableDrawableSlider("Frequency", &d->parametricTrail.gateFreq, d->id,
                                "gateFreq", "%.1f Hz", sources);
      ModulatableDrawableSlider("Duty", &d->parametricTrail.gateDuty, d->id,
                                "gateDuty", "%.0f%%", sources);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Animation", Theme::GLOW_MAGENTA, &sectionAnimation)) {
    DrawBaseAnimationControls(&d->base, d->id, sources);
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Color", Theme::GLOW_ORANGE, &sectionColor)) {
    DrawBaseColorControls(&d->base);
    DrawSectionEnd();
  }
}
```

**Verify**: Compiles.

---

#### Task 2.4: Drawables Panel Integration

**Files**: `src/ui/imgui_drawables.cpp`
**Depends on**: Wave 1 complete, Task 2.3 complete

**Build**:
1. Add `#include "ui/drawable_type_controls.h"` if not present
2. Add "+ Trail" button after "+ Shape" button (around line 84):
```cpp
ImGui::SameLine();

ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
if (ImGui::Button("+ Trail")) {
  Drawable d = {};
  d.id = sNextDrawableId++;
  d.type = DRAWABLE_PARAMETRIC_TRAIL;
  d.path = PATH_CIRCULAR;  // unused but initialize
  d.base.color.solid = ThemeColor::NEON_CYAN;
  d.parametricTrail = ParametricTrailData{};
  drawables[*count] = d;
  DrawableParamsRegister(&drawables[*count]);
  *selected = *count;
  (*count)++;
}
ImGui::EndDisabled();
```
3. Add trail index counter with others (around line 139):
```cpp
int trailIdx = 0;
```
4. Add case in list label generation (around line 148):
```cpp
} else if (drawables[i].type == DRAWABLE_PARAMETRIC_TRAIL) {
  trailIdx++;
  (void)snprintf(label, sizeof(label), "[T] Trail %d", trailIdx);
}
```
5. Add case for type indicator header (around line 183):
```cpp
} else if (sel->type == DRAWABLE_PARAMETRIC_TRAIL) {
  ImGui::TextColored(Theme::ACCENT_CYAN, "Parametric Trail Settings");
}
```
6. Add case to call controls (around line 207):
```cpp
} else if (sel->type == DRAWABLE_PARAMETRIC_TRAIL) {
  DrawParametricTrailControls(sel, sources);
}
```

**Verify**: Compiles.

---

#### Task 2.5: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `ParametricTrailData` after `ShapeData` (around line 636):
```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParametricTrailData,
    speed, amplitude, freqX1, freqY1, freqX2, freqY2, offsetX, offsetY,
    thickness, roundedCaps, gateEnabled, gateFreq, gateDuty)
```
Note: Excludes runtime fields (phase, prevX, prevY, hasPrevPos).

2. In `to_json(Drawable)` switch (around line 652), add case:
```cpp
case DRAWABLE_PARAMETRIC_TRAIL:
  j["parametricTrail"] = d.parametricTrail;
  break;
```

3. In `from_json(Drawable)` switch (around line 676), add case:
```cpp
case DRAWABLE_PARAMETRIC_TRAIL:
  if (j.contains("parametricTrail")) {
    d.parametricTrail = j["parametricTrail"].get<ParametricTrailData>();
  }
  break;
```

**Verify**: Compiles.

---

#### Task 2.6: Parameter Registration

**Files**: `src/automation/drawable_params.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add case for `DRAWABLE_PARAMETRIC_TRAIL` in `DrawableParamsRegister` (after shape params, around line 58):
```cpp
if (d->type == DRAWABLE_PARAMETRIC_TRAIL) {
  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.speed", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.speed, 0.1f, 10.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.amplitude", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.amplitude, 0.05f, 0.5f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.freqX1", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.freqX1, 0.1f, 5.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.freqY1", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.freqY1, 0.1f, 5.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.freqX2", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.freqX2, 0.0f, 5.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.freqY2", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.freqY2, 0.0f, 5.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.thickness", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.thickness, 1.0f, 50.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.gateFreq", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.gateFreq, 0.1f, 20.0f);

  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.gateDuty", d->id);
  ModEngineRegisterParam(paramId, &d->parametricTrail.gateDuty, 0.0f, 1.0f);
}
```

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] "+ Trail" button appears in Drawables panel
- [ ] Adding a trail shows "[T] Trail 1" in list
- [ ] Selecting trail shows "Parametric Trail Settings" header
- [ ] Path/Stroke/Gate/Animation/Color sections render with sliders
- [ ] Cursor traces Lissajous figure on screen (visible strokes)
- [ ] Gate toggle produces fragmented strokes when enabled
- [ ] Preset save/load preserves trail configuration
- [ ] Modulation routes work for speed/amplitude/freq/thickness/gate params
