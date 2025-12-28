# Unified Drawable Architecture

Research on refactoring waveforms, spectrum bars, and shapes into a unified drawable system with configurable draw order.

---

## Current Architecture

### Render Order (main.cpp:157-168)

```
1. RenderPipelineApplyFeedback()
   └─ Voronoi → Feedback warp → H-blur → V-blur+decay → accumTexture

2. RenderWaveformsToPhysarum()
   └─ Draw waveforms to physarum trail map

3. RenderWaveformsToAccum()
   └─ Draw waveforms + spectrum bars onto accumTexture

4. RenderPipelineApplyOutput()
   └─ Trail boost → Kaleidoscope → Chromatic → FXAA → Gamma → screen
```

**Result:** Waveforms appear ON TOP of feedback. Crisp lines over blurry trails.

### Current Drawing (No Abstraction)

Each drawable type uses direct raylib calls with hardcoded positioning:

| Type | File | Draw Calls | Position |
|------|------|------------|----------|
| Linear waveform | waveform.cpp:197 | `DrawLineEx()` + `DrawCircleV()` | `centerY + verticalOffset * screenH` |
| Circular waveform | waveform.cpp:234 | `DrawLineEx()` + `DrawCircleV()` | `centerX, centerY` fixed |
| Spectrum circular | spectrum_bars.cpp:134 | `DrawTriangle()` x2 per bar | `centerX, centerY` fixed |
| Spectrum linear | spectrum_bars.cpp:190 | `DrawRectangle()` | Bottom of screen |

### Current Config Structs

```cpp
// waveform_config.h
struct WaveformConfig {
    float amplitudeScale;   // Height scaling
    float thickness;        // Line width
    float smoothness;       // Smoothing window
    float radius;           // Circular base radius (fraction of minDim)
    float verticalOffset;   // Linear Y offset (-0.5 to 0.5)
    float rotationSpeed;    // Radians per tick
    float rotationOffset;   // Base rotation
    ColorConfig color;
};

// spectrum_bars_config.h
struct SpectrumBarsConfig {
    bool enabled;
    float innerRadius;      // Circular base radius
    float barHeight;        // Max height fraction
    float barWidth;         // Arc/slot fraction
    float smoothing;        // Decay rate
    // ... color, rotation
};
```

**Gap:** No `x, y` position fields. Center is hardcoded via `RenderContext.centerX/Y`.

---

## Proposed Unified Architecture

### Drawable Base Properties

All drawables share spatial and appearance properties:

```cpp
struct DrawableBase {
    bool enabled = true;

    // Spatial (all in normalized 0..1 coordinates)
    float x = 0.5f;           // Center X
    float y = 0.5f;           // Center Y
    float scale = 1.0f;       // Size multiplier
    float rotation = 0.0f;    // Base rotation (radians)
    float rotationSpeed = 0.0f;

    // Appearance
    ColorConfig color;
    float alpha = 1.0f;
    bool additive = false;    // Blend mode

    // Draw order (0 = fully integrated, 1 = fully crisp)
    float feedbackPhase = 1.0f;
};
```

### Drawable Types

```cpp
enum DrawableType {
    DRAWABLE_WAVEFORM,
    DRAWABLE_SPECTRUM,
    DRAWABLE_SHAPE,           // Solid polygon
    DRAWABLE_TEXTURED_SHAPE,  // Samples previous frame
};

enum DrawablePath {
    PATH_LINEAR,
    PATH_CIRCULAR,
};

struct Drawable {
    DrawableType type;
    DrawableBase base;
    DrawablePath path;        // LINEAR or CIRCULAR (waveform/spectrum only)

    union {
        struct {
            float amplitudeScale;
            float thickness;
            float smoothness;
        } waveform;

        struct {
            float barHeight;
            float barWidth;
            float smoothing;
        } spectrum;

        struct {
            int sides;            // 3 = triangle, 4 = quad, etc.
            float texZoom;        // Textured only: >1 = zoom in
            float texAngle;       // Textured only: texture rotation
        } shape;
    };
};
```

## Draw Order: feedbackPhase Slider

A 0..1 slider controls how "integrated" vs "crisp" each drawable appears:

| Value | Pre-Alpha | Post-Alpha | Visual |
|-------|-----------|------------|--------|
| 0.0 | 100% | 0% | Fully integrated into feedback, dreamy |
| 0.3 | 70% | 30% | Mostly integrated, subtle crisp edge |
| 0.5 | 50% | 50% | Balanced trails + definition |
| 1.0 | 0% | 100% | Fully crisp on top (current behavior) |

### Render Loop

```cpp
void RenderPipelineApplyFeedback(PostEffect* pe, Drawable* drawables, int count) {
    // 1. Pre-feedback draws (integrated into warp)
    for (int i = 0; i < count; i++) {
        float preAlpha = drawables[i].base.alpha * (1.0f - drawables[i].base.feedbackPhase);
        if (preAlpha > 0.001f) {
            DrawDrawable(&drawables[i], preAlpha, pe->accumTexture);
        }
    }

    // 2. Feedback passes (voronoi, warp, blur, decay)
    // ... existing ping-pong logic ...

    // 3. Post-feedback draws (crisp on top)
    for (int i = 0; i < count; i++) {
        float postAlpha = drawables[i].base.alpha * drawables[i].base.feedbackPhase;
        if (postAlpha > 0.001f) {
            DrawDrawable(&drawables[i], postAlpha, pe->accumTexture);
        }
    }
}
```

### Per-Drawable Control

Each drawable can have a different phase:

```cpp
drawables[0] = { DRAWABLE_WAVEFORM, { .feedbackPhase = 0.8f }, PATH_CIRCULAR };  // Mostly crisp
drawables[1] = { DRAWABLE_TEXTURED_SHAPE, { .feedbackPhase = 0.0f } };           // Fully integrated
drawables[2] = { DRAWABLE_SPECTRUM, { .feedbackPhase = 1.0f }, PATH_CIRCULAR };  // Fully crisp
```

### Recommended Defaults

| Drawable Type | Default | Reason |
|---------------|---------|--------|
| Waveform | 0.7 - 1.0 | Audio clarity matters |
| Spectrum bars | 1.0 | Clean bar edges |
| Textured shape | 0.0 | Must be pre-feedback for fractal effect |
| Solid shape | 0.0 - 0.5 | Usually decorative, can integrate |

---

## Implementation Changes

### render_pipeline.cpp

Current:
```cpp
void RenderPipelineApplyFeedback(PostEffect* pe, ...) { ... }
// gap
void RenderWaveformsToAccum(...) { ... }
```

Proposed:
```cpp
void RenderPipelineApplyFeedback(PostEffect* pe, Drawable* drawables, int count, ...) {
    // 1. Pre-feedback draws
    for (int i = 0; i < count; i++) {
        float preAlpha = drawables[i].base.alpha * (1.0f - drawables[i].base.feedbackPhase);
        if (preAlpha > 0.001f) {
            DrawDrawable(&drawables[i], preAlpha, pe->accumTexture, ctx);
        }
    }

    // 2. Feedback passes (voronoi, warp, blur, decay)
    // ... existing ping-pong logic ...

    // 3. Post-feedback draws
    for (int i = 0; i < count; i++) {
        float postAlpha = drawables[i].base.alpha * drawables[i].base.feedbackPhase;
        if (postAlpha > 0.001f) {
            DrawDrawable(&drawables[i], postAlpha, pe->accumTexture, ctx);
        }
    }
}
```

### New drawable.cpp

```cpp
void DrawDrawable(Drawable* d, RenderTexture2D target, RenderContext* ctx) {
    if (!d->base.enabled) return;

    // Convert normalized coords to screen coords
    float screenX = d->base.x * ctx->screenW;
    float screenY = d->base.y * ctx->screenH;
    float size = d->base.scale * ctx->minDim;

    BeginTextureMode(target);

    switch (d->type) {
        case DRAWABLE_WAVEFORM_CIRCULAR:
            DrawWaveformCircular(screenX, screenY, size, d->waveform, ...);
            break;
        case DRAWABLE_TEXTURED_SHAPE:
            DrawTexturedShape(screenX, screenY, size, d->shape, prevFrame);
            break;
        // ...
    }

    EndTextureMode();
}
```

### Modulation Integration

```cpp
// In modulation_engine.cpp
ModTarget targets[] = {
    { "drawable[0].x",        &drawables[0].base.x },
    { "drawable[0].y",        &drawables[0].base.y },
    { "drawable[0].scale",    &drawables[0].base.scale },
    { "drawable[0].rotation", &drawables[0].base.rotation },
    { "drawable[1].texZoom",  &drawables[1].shape.texZoom },
    // ...
};
```

---

## Migration Path

### Phase 1: Extract Position

1. Add `x, y` to `WaveformConfig` and `SpectrumBarsConfig`
2. Replace `ctx->centerX` with `config.x * ctx->screenW`
3. Keep defaults at 0.5 (centered) — behavior unchanged

### Phase 2: Add feedbackPhase

1. Add `float feedbackPhase` to config structs
2. Split `RenderWaveformsToAccum()` into pre/post loops with alpha scaling
3. Default to `1.0` — behavior unchanged (fully crisp)

### Phase 3: Unify Config

1. Create `DrawableBase` with shared fields
2. Refactor `WaveformConfig` and `SpectrumBarsConfig` to embed `DrawableBase`
3. Create single `Drawable` array replacing separate waveform/spectrum arrays
4. Move "circular" from effect toggle to per-waveform `path` property (`PATH_LINEAR` / `PATH_CIRCULAR`)

### Phase 4: Add Shapes

1. Implement `DRAWABLE_SHAPE` type (solid polygon, any number of sides)
2. Add `textured` flag + texZoom/texAngle for previous-frame sampling
3. Shader for texture sampling with zoom/rotation

---

## Textured Shape Shader

```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D prevFrame;
uniform float texZoom;
uniform float texAngle;
uniform vec4 tintColor;

out vec4 finalColor;

void main() {
    // Apply zoom and rotation to texture coords
    vec2 uv = fragTexCoord - 0.5;

    // Zoom
    uv *= texZoom;

    // Rotate
    float c = cos(texAngle);
    float s = sin(texAngle);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    uv += 0.5;

    // Sample with edge handling
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0);
    } else {
        finalColor = texture(prevFrame, uv) * tintColor;
    }
}
```

---

## References

- Current waveform: `src/render/waveform.cpp:197-287`
- Current spectrum: `src/render/spectrum_bars.cpp:134-225`
- Render pipeline: `src/render/render_pipeline.cpp:161-188`
- Post-effect textures: `src/render/post_effect.h:10-64`
