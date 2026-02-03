# Trail Drawable Simplification

Replace the ThickLine-based parametric trail with simple raylib shape drawing. The trail effect emerges from gate timing, motion speed, and feedback decay—not from connecting line segments. This removes unnecessary complexity while enabling shape variety.

## Specification

### Type Changes

**New enum** in `drawable_config.h`:
```cpp
typedef enum {
  TRAIL_SHAPE_CIRCLE = 0,    // sides=32 (smooth circle)
  TRAIL_SHAPE_TRIANGLE = 1,  // sides=3
  TRAIL_SHAPE_SQUARE = 2,    // sides=4
  TRAIL_SHAPE_PENTAGON = 3,  // sides=5
  TRAIL_SHAPE_HEXAGON = 4,   // sides=6
} TrailShapeType;
```

**Modified `ParametricTrailData`**:
```cpp
struct ParametricTrailData {
  // Lissajous motion parameters (unchanged)
  DualLissajousConfig lissajous = { ... };

  // Shape parameters (replaces stroke)
  TrailShapeType shapeType = TRAIL_SHAPE_CIRCLE;
  float size = 8.0f;      // Shape diameter in pixels (renamed from thickness)
  bool filled = true;     // true=filled, false=outline (repurposed roundedCaps)

  // Draw gate (unchanged)
  float gateFreq = 0.0f;

  // Runtime state (unchanged, but prevX/prevY/prevT/hasPrevPos removed)
};
```

**Removed fields**:
- `thickness` → renamed to `size`
- `roundedCaps` → renamed to `filled`
- `prevX`, `prevY`, `prevT`, `hasPrevPos` → removed (no line segments)

### Algorithm

**DrawableRenderParametricTrail** (simplified):
```cpp
static void DrawableRenderParametricTrail(RenderContext *ctx, Drawable *d,
                                          uint64_t tick, float opacity) {
  (void)tick;
  ParametricTrailData &trail = d->parametricTrail;

  // Compute cursor position via dual-harmonic Lissajous
  float deltaTime = GetFrameTime();
  float offsetX, offsetY;
  DualLissajousUpdate(&trail.lissajous, deltaTime, 0.0f, &offsetX, &offsetY);
  float x = d->base.x + offsetX;
  float y = d->base.y + offsetY;

  // Draw gate check
  bool shouldDraw = true;
  if (trail.gateFreq > 0.0f) {
    float gatePhase = fmodf((float)GetTime() * trail.gateFreq, 1.0f);
    shouldDraw = gatePhase < 0.5f;
  }

  if (!shouldDraw) {
    return;
  }

  // Convert to screen coordinates
  Vector2 pos = {x * ctx->screenW, y * ctx->screenH};
  float t = fmodf(trail.lissajous.phase, 1.0f);
  Color color = ColorFromConfig(&d->base.color, t, opacity);

  // Map shape type to polygon sides
  int sides;
  switch (trail.shapeType) {
  case TRAIL_SHAPE_TRIANGLE: sides = 3; break;
  case TRAIL_SHAPE_SQUARE:   sides = 4; break;
  case TRAIL_SHAPE_PENTAGON: sides = 5; break;
  case TRAIL_SHAPE_HEXAGON:  sides = 6; break;
  case TRAIL_SHAPE_CIRCLE:
  default:                   sides = 32; break;
  }

  float radius = trail.size * 0.5f;
  float rotation = d->rotationAccum * RAD2DEG;

  if (trail.filled) {
    DrawPoly(pos, sides, radius, rotation, color);
  } else {
    DrawPolyLinesEx(pos, sides, radius, rotation, 1.0f, color);
  }
}
```

### Parameters

| Parameter | Type | Range | Default | UI Label |
|-----------|------|-------|---------|----------|
| `shapeType` | `TrailShapeType` | enum | `TRAIL_SHAPE_CIRCLE` | "Shape" |
| `size` | `float` | 1.0–100.0 | 8.0 | "Size" |
| `filled` | `bool` | - | `true` | "Filled" |

### Removed Code

- Remove `#include "thick_line.h"` from `drawable.cpp`
- Remove `ThickLineBegin/Vertex/End` calls
- Remove `DrawCircleV` cap calls
- Remove runtime state fields from struct

---

## Tasks

### Wave 1: Config Update

#### Task 1.1: Update ParametricTrailData struct

**Files**: `src/config/drawable_config.h`
**Creates**: `TrailShapeType` enum, updated `ParametricTrailData` struct

**Build**:
1. Add `TrailShapeType` enum before `DrawableBase` struct
2. Replace `ParametricTrailData` struct fields:
   - Add `shapeType` field (default `TRAIL_SHAPE_CIRCLE`)
   - Rename `thickness` to `size` (default 8.0f)
   - Rename `roundedCaps` to `filled` (default true)
   - Remove `prevX`, `prevY`, `prevT`, `hasPrevPos` fields

**Verify**: `cmake.exe --build build` compiles (will fail in later files, expected).

---

### Wave 2: Implementation Updates

#### Task 2.1: Simplify trail rendering

**Files**: `src/render/drawable.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Remove `#include "thick_line.h"`
2. Replace `DrawableRenderParametricTrail` function body with algorithm from spec
3. Keep existing function signature

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Update preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `ParametricTrailData`:
   - Change `thickness` to `size`
   - Change `roundedCaps` to `filled`
   - Add `shapeType`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Update UI controls

**Files**: `src/ui/drawable_type_controls.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `DrawParametricTrailControls`, update "Stroke" section (rename to "Shape"):
   - Add combo box for `shapeType` with labels: "Circle", "Triangle", "Square", "Pentagon", "Hexagon"
   - Change `thickness` slider to `size` slider (keep same range 1-100, format "%.0f px")
   - Change `roundedCaps` checkbox to `filled` checkbox with label "Filled"

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] Run application, create parametric trail drawable
- [ ] Verify shape appears at cursor position (no line segments)
- [ ] Test shape type combo (circle/triangle/square/pentagon/hexagon)
- [ ] Test filled vs outline toggle
- [ ] Test with feedback enabled—trail persists correctly
- [ ] Test gate frequency—gaps appear in trail
- [ ] Load existing preset with old field names (should use defaults gracefully)
