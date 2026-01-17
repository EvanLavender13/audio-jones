# Shape Width/Height

Replace the uniform `size` field in ShapeData with independent `width` and `height` controls, enabling non-uniform shapes like rectangles and ellipses.

## Current State

- `src/config/drawable_config.h:39` - `ShapeData.size` (single float, default 0.2)
- `src/render/shape.cpp:30` - Computes `radius = size * ctx->minDim` (uniform scaling)
- `src/render/shape.cpp:50-51` - Vertices: `cos(angle) * radius`, `sin(angle) * radius`
- `src/ui/drawable_type_controls.cpp:97` - Single "Size" slider (0.05-0.5)
- `src/config/preset.cpp:338-339` - Serializes `size` field
- `src/automation/param_registry.cpp:173-179` - `size` not registered for modulation

## Phase 1: Config Update

**Goal**: Replace `size` with `width`, `height`, and `aspectLocked` in ShapeData.

**Build**:
- Modify `ShapeData` in `drawable_config.h`:
  - Remove `float size = 0.2f`
  - Add `float width = 0.4f` (normalized 0-1, fraction of screen width)
  - Add `float height = 0.4f` (normalized 0-1, fraction of screen height)
  - Add `bool aspectLocked = true` (UI-only state for linked editing)

**Done when**: Project compiles with updated struct (render/UI will show errors until fixed in later phases).

---

## Phase 2: Rendering Update

**Goal**: Update shape rendering to use separate X/Y scaling.

**Build**:
- Modify `ShapeGeometry` struct in `shape.cpp`:
  - Replace `float radius` with `float radiusX` and `float radiusY`
- Update `ShapeCalcGeometry`:
  - `radiusX = d->shape.width * ctx->screenW * 0.5f`
  - `radiusY = d->shape.height * ctx->screenH * 0.5f`
- Update `ShapeDrawSolid` vertex calculation:
  - `v1.x = centerX + cosf(angle1) * radiusX`
  - `v1.y = centerY + sinf(angle1) * radiusY`
  - Same pattern for v2
- Update `ShapeDrawTextured` vertex calculation:
  - Same changes as ShapeDrawSolid
  - UV mapping: keep unit-circle based (unchanged)

**Done when**: Shapes render with independent X/Y scaling. A 4-sided shape with width=0.8, height=0.2 displays as a horizontal rectangle.

---

## Phase 3: UI Controls

**Goal**: Add width/height sliders with aspect-lock toggle.

**Build**:
- Modify `DrawShapeControls` in `drawable_type_controls.cpp`:
  - Remove `ImGui::SliderFloat("Size", &d->shape.size, 0.05f, 0.5f)`
  - Add `ImGui::Checkbox("Lock", &d->shape.aspectLocked)` on same line as first slider
  - Add `ImGui::SliderFloat("Width", &d->shape.width, 0.01f, 2.0f)` (allow >1 for oversized shapes)
  - Add `ImGui::SliderFloat("Height", &d->shape.height, 0.01f, 2.0f)`
  - When `aspectLocked` is true and one slider changes, update the other proportionally:
    - Store previous values before sliders
    - If width changed: `height *= (width / prevWidth)`
    - If height changed: `width *= (height / prevHeight)`

**Done when**: UI shows width/height sliders with working lock toggle. Locked mode maintains aspect ratio when either slider moves.

---

## Phase 4: Preset Serialization

**Goal**: Serialize width/height and migrate old presets.

**Build**:
- Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for ShapeData in `preset.cpp`:
  - Replace `size` with `width, height, aspectLocked`
- Add migration in `from_json` for Drawable (near line 372):
  - If `j["shape"]` contains `"size"` but not `"width"`:
    - Read old `size` value
    - Set `width = size * 2.0f` and `height = size * 2.0f`
    - This converts the old minDim-based scale to screen-fraction scale
    - On a square window, `size=0.2` → `width=0.4, height=0.4` gives roughly equivalent visual size

**Done when**: Old presets load with equivalent visual appearance. New presets save width/height.

---

## Phase 5: Modulation Support

**Goal**: Register width and height for audio-reactive modulation.

**Build**:
- Add entries to `DRAWABLE_FIELD_TABLE` in `param_registry.cpp`:
  - `{"width", {0.01f, 2.0f}}`
  - `{"height", {0.01f, 2.0f}}`
- Update `ModulatableDrawableSlider` calls in `drawable_type_controls.cpp`:
  - Change `ImGui::SliderFloat("Width", ...)` to `ModulatableDrawableSlider("Width", &d->shape.width, d->id, "width", "%.2f", sources)`
  - Same for height

**Done when**: Width and height can be modulated via bass/treble/beat sources. Shape pulses on beat when modulation is applied.
