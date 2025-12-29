# Shape Drawables

Add `DRAWABLE_SHAPE` type to the unified drawable system. Shapes render regular polygons (3-32 sides) with optional texture sampling from the feedback buffer. This enables MilkDrop-style fractal zoom effects and decorative geometry.

## Current State

Unified drawable system complete (Phases 1-3):
- `src/config/drawable_config.h:6-63` - Drawable struct with type-discriminated union
- `src/render/drawable.cpp:103-149` - Split-pass rendering dispatch loop
- `src/render/drawable.cpp:170-192` - Validation (max 1 spectrum, max 8 waveforms)
- `src/config/preset.cpp:93-114` - JSON serialization

Shape rendering patterns to follow:
- `src/render/spectrum_bars.cpp:168-171` - DrawTriangle pairs for quads
- `src/render/waveform.cpp:137` - Rotation via `rotationSpeed * tick + rotationOffset`
- `src/render/post_effect.cpp:22-32` - Fragment shader loading pattern

---

## Phase 1: Data Structures

**Goal**: Add DRAWABLE_SHAPE type and ShapeData struct to existing union.

**Modify**:
- `src/config/drawable_config.h:6` - Add `DRAWABLE_SHAPE` to DrawableType enum
- `src/config/drawable_config.h` (after line 33) - Add ShapeData struct:
  ```cpp
  struct ShapeData {
      int sides = 6;
      float size = 0.2f;
      bool textured = false;
      float texZoom = 1.0f;
      float texAngle = 0.0f;
  };
  ```
- `src/config/drawable_config.h:39-42` - Add `ShapeData shape;` to union
- `src/config/drawable_config.h:45-62` - Update copy constructor/assignment for DRAWABLE_SHAPE case

**Done when**: Project compiles with no errors.

---

## Phase 2: Shape Rendering Module

**Goal**: Create module that generates polygon vertices and renders shapes.

**Create**:
- `src/render/shape.h` - Declare ShapeDrawTextured() and ShapeDrawSolid()
- `src/render/shape.cpp` - Implement polygon vertex generation and rendering:
  - Generate regular polygon vertices: `angle = 2π * i / sides + rotation`
  - Convert normalized coords to screen: `centerX = base.x * ctx->screenW`
  - ShapeDrawSolid: DrawTriangle loop from center vertex to edge pairs
  - ShapeDrawTextured: stub that returns immediately (Phase 3 implements)

**Modify**:
- `CMakeLists.txt` - Add shape.cpp to source list

**Done when**: ShapeDrawSolid renders visible polygon when called directly from main loop.

---

## Phase 3: Drawable Integration

**Goal**: Wire shapes into the dispatch loop with validation.

**Modify**:
- `src/render/drawable.h:6` - Add `#include "render/shape.h"`
- `src/render/drawable.cpp:139-147` - Add case to switch:
  ```cpp
  case DRAWABLE_SHAPE:
      if (drawables[i].shape.textured) {
          ShapeDrawTextured(ctx, &drawables[i], tick, opacity);
      } else {
          ShapeDrawSolid(ctx, &drawables[i], tick, opacity);
      }
      break;
  ```
- `src/render/drawable.cpp:170-192` - Add shape count validation (max 4)

**Done when**: Adding DRAWABLE_SHAPE to drawables array renders solid polygon.

---

## Phase 4: Texture Shader

**Goal**: Create fragment shader for sampling accumTexture with zoom/rotation.

**Create**:
- `shaders/shape_texture.fs`:
  ```glsl
  #version 330
  in vec2 fragTexCoord;
  in vec4 fragColor;
  uniform sampler2D texture0;
  uniform float texZoom;
  uniform float texAngle;
  out vec4 finalColor;

  void main() {
      vec2 uv = fragTexCoord - 0.5;
      float c = cos(texAngle);
      float s = sin(texAngle);
      uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
      uv /= texZoom;
      uv = clamp(uv + 0.5, 0.0, 1.0);
      finalColor = texture(texture0, uv) * fragColor;
  }
  ```

**Modify**:
- `src/render/post_effect.h` (after line 21) - Add shader fields:
  ```cpp
  Shader shapeTextureShader;
  int shapeTexZoomLoc;
  int shapeTexAngleLoc;
  ```
- `src/render/post_effect.cpp:22-32` - Load shader in LoadPostEffectShaders()
- `src/render/post_effect.cpp` (after line 73) - Cache uniform locations
- `src/render/post_effect.cpp` (cleanup section) - Unload shader in uninit

**Done when**: Shader loads without errors (check console output).

---

## Phase 5: Textured Shape Rendering

**Goal**: Implement ShapeDrawTextured using shader and DrawTexturePoly.

**Modify**:
- `src/render/render_context.h` - Add `Texture2D accumTexture;` field
- `src/main.cpp` (render section) - Set `ctx.accumTexture = pe->accumTexture.texture` before drawable render calls
- `src/render/shape.cpp` - Implement ShapeDrawTextured:
  - Generate polygon vertices (same as solid)
  - Generate texture coords: map vertices to [0,1] UV range centered at 0.5
  - BeginShaderMode(pe->shapeTextureShader)
  - SetShaderValue for texZoom, texAngle (convert degrees to radians)
  - DrawTexturePoly(ctx->accumTexture, center, vertices, texCoords, count, tint)
  - EndShaderMode()

**Note**: ShapeDrawTextured needs PostEffect* or shader access. Options:
1. Add PostEffect* to RenderContext (simplest)
2. Pass shaders as function parameters

**Done when**: Textured shape samples and displays feedback buffer content with zoom/rotation.

---

## Phase 6: Serialization

**Goal**: Enable preset save/load for shapes.

**Modify**:
- `src/config/preset.cpp` (after line 91) - Add JSON macro:
  ```cpp
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShapeData,
      sides, size, textured, texZoom, texAngle)
  ```
- `src/config/preset.cpp:93-102` - Update to_json with shape case
- `src/config/preset.cpp:104-114` - Update from_json with shape case

**Done when**: Preset with shapes saves to JSON and reloads with all fields intact.

---

## Phase 7: UI Controls

**Goal**: Add shape controls to drawable panel.

**Modify**:
- `src/ui/imgui_drawables.cpp` - Add helper: `CountShapes(drawables, count)`
- `src/ui/imgui_drawables.cpp` (add buttons section) - Add "+ Shape" button:
  - Disable when count >= MAX_DRAWABLES or shapeCount >= 4
  - Create Drawable with type=DRAWABLE_SHAPE, default ShapeData
- `src/ui/imgui_drawables.cpp` (list section) - Add "[P] Shape" label
- `src/ui/imgui_drawables.cpp` (controls section) - Add shape controls:
  - SliderInt: Sides (3-32)
  - SliderFloat: Size (0.05-0.5)
  - Checkbox: Textured
  - If textured: SliderFloat texZoom (0.1-5.0), SliderFloat texAngle (-180 to 180)
- `src/ui/imgui_drawables.cpp` (delete section) - Allow shape deletion

**Done when**: Full UI workflow: add shape, configure, delete.

---

## Testing Checklist

- [ ] Solid triangle renders at center
- [ ] Solid hexagon with rotationSpeed rotates smoothly
- [ ] Textured shape displays accumTexture content
- [ ] texZoom=2.0 shows zoomed-in feedback
- [ ] texAngle=90 rotates texture independently of shape
- [ ] feedbackPhase=0.0 integrates shape into feedback (dreamy)
- [ ] feedbackPhase=1.0 renders shape crisp on top
- [ ] Max 4 shapes enforced in validation
- [ ] Preset save/load preserves all shape fields
- [ ] UI add/configure/delete workflow works

---

## Files Summary

**Create (3 files)**:
- `src/render/shape.h`
- `src/render/shape.cpp`
- `shaders/shape_texture.fs`

**Modify (8 files)**:
- `src/config/drawable_config.h`
- `src/render/drawable.h`
- `src/render/drawable.cpp`
- `src/render/post_effect.h`
- `src/render/post_effect.cpp`
- `src/render/render_context.h`
- `src/config/preset.cpp`
- `src/ui/imgui_drawables.cpp`
