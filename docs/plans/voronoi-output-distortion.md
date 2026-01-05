# Voronoi Output Distortion

Move voronoi from feedback phase (brightness boost) to output phase (UV distortion). Transform effect warps the image through voronoi cell geometry, creating glass-block or organic lens effects.

## Current State

- `shaders/voronoi.fs` - Edge-based brightness boost shader
- `src/config/voronoi_config.h` - Config with enabled, intensity, scale, speed, edgeWidth
- `src/render/render_pipeline.cpp:459-463` - Applied in feedback phase before feedback/blur
- `src/render/post_effect.h:22,56-60` - Shader and uniform locations

## Technical Implementation

**Source**: [Voronoi distances by Inigo Quilez](https://www.shadertoy.com/view/ldl3W8)

### Core Algorithm

```glsl
// First pass: find nearest cell center
vec2 mr;  // Vector from fragment to nearest cell point
float md = 8.0;
for (j = -1 to 1) {
    for (i = -1 to 1) {
        vec2 g = vec2(i, j);
        vec2 o = hash2(ip + g);
        o = 0.5 + 0.5 * sin(time + TWO_PI * o);  // Animate
        vec2 r = g + o - fp;
        float d = dot(r, r);
        if (d < md) { md = d; mr = r; }
    }
}

// Second pass: distance to nearest edge
float edgeDist = 8.0;
for (j = -2 to 2) {
    for (i = -2 to 2) {
        vec2 g = mg + vec2(i, j);
        vec2 o = hash2(ip + g);
        o = 0.5 + 0.5 * sin(time + TWO_PI * o);
        vec2 r = g + o - fp;
        if (dot(mr - r, mr - r) > 0.00001) {
            edgeDist = min(edgeDist, dot(0.5 * (mr + r), normalize(r - mr)));
        }
    }
}
```

### Distortion Modes

| Mode | UV Offset Formula | Visual Effect |
|------|-------------------|---------------|
| Glass Blocks | `mr * strength` | Hard refraction, lens per cell |
| Organic Flow | `mr * strength * smoothstep(0.0, edgeFalloff, length(mr))` | Smooth flowing displacement |
| Edge Warp | `mr * strength * (1.0 - smoothstep(0.0, edgeFalloff, edgeDist))` | Distortion at boundaries only |

### Parameters

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| enabled | bool | - | Toggle effect |
| mode | int | 0-2 | Visual style selector |
| scale | float | 5-50 | Cell count across screen |
| strength | float | 0.0-0.5 | UV displacement magnitude |
| speed | float | 0.1-2.0 | Animation rate |
| edgeFalloff | float | 0.1-1.0 | Distortion gradient sharpness |

---

## Phase 1: Shader and Config

**Goal**: Rewrite voronoi shader for UV distortion with mode selection.

**Build**:
- Update `src/config/voronoi_config.h`:
  - Add `VoronoiMode` enum (VORONOI_GLASS_BLOCKS, VORONOI_ORGANIC_FLOW, VORONOI_EDGE_WARP)
  - Replace `intensity` with `strength` (0.0-0.5 range)
  - Replace `edgeWidth` with `edgeFalloff` (0.1-1.0 range)
  - Add `mode` field
- Rewrite `shaders/voronoi.fs`:
  - Two-pass voronoi (Quilez algorithm) for `mr` and `edgeDist`
  - Mode switch for displacement formula
  - Sample texture at `fragTexCoord + displacement`

**Done when**: Shader compiles, config struct updated.

---

## Phase 2: Pipeline Integration

**Goal**: Move voronoi from feedback to output as a transform effect.

**Build**:
- Update `src/config/effect_config.h`:
  - Add `TRANSFORM_VORONOI` to `TransformEffectType` enum
  - Update `TransformEffectName()` switch
  - Increase `TRANSFORM_EFFECT_COUNT`
  - Update default `transformOrder` array
- Update `src/render/render_pipeline.cpp`:
  - Add `SetupVoronoi` to output phase pattern
  - Add voronoi entry to `GetTransformEffect()` switch
  - Remove voronoi pass from `RenderPipelineApplyFeedback()`
  - Keep `voronoiTime` accumulation in feedback (drives animation)
- Update `src/render/post_effect.h` and `post_effect.cpp`:
  - Add uniform locations for `mode` and `edgeFalloff`
  - Update `SetShaderValue` calls in setup function

**Done when**: Voronoi renders in output phase, respects transform order.

---

## Phase 3: UI and Serialization

**Goal**: Update UI panel and preset save/load for new parameters.

**Build**:
- Update `src/ui/imgui_effects.cpp`:
  - Replace intensity slider with strength slider (0.0-0.5)
  - Replace edgeWidth slider with edgeFalloff slider (0.1-1.0)
  - Add mode combo box (Glass Blocks, Organic Flow, Edge Warp)
- Update `src/config/preset.cpp`:
  - Rename JSON keys: `intensity` → `strength`, `edgeWidth` → `edgeFalloff`
  - Add `mode` serialization
  - Handle legacy presets (map old keys to new)

**Done when**: UI controls work, presets save/load correctly, old presets migrate.
