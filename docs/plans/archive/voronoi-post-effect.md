# Voronoi Post-Process Effect

Adds animated Voronoi edge rendering as a post-processing pass with LFO-modulated cell scale.

Background research: `docs/research/voronoi-shaders.md`

## Goal

Create organic, web-like patterns that overlay the accumulated frame. Voronoi edges provide visual structure without overwhelming waveform content. Animation prevents static appearance; LFO modulation adds slow breathing movement to cell sizes.

Prior art: [The Book of Shaders Ch.12](https://thebookofshaders.com/12/), [Shadertoy Voronoi basic](https://www.shadertoy.com/view/MslGD8)

## Current State

Post-processing pipeline executes 5 passes (`src/render/post_effect.cpp:116-223`):

1. Feedback (zoom/rotate) - line 131-144
2. Blur H - line 146-154
3. Blur V + decay - line 156-166
4. Kaleidoscope (optional) - line 181-202
5. Chromatic aberration - line 205-223

Voronoi inserts between blur and kaleidoscope. This allows Voronoi patterns to feed into kaleidoscope fractalization.

## Algorithm

### Voronoi Distance Field

Partition UV space into tiles. Each tile contains one pseudo-random feature point. For each pixel, find the closest point across a 3x3 neighborhood:

```glsl
vec2 i_st = floor(st);  // Tile index
vec2 f_st = fract(st);  // Position within tile

float m_dist = 100.0;
float m_dist2 = 100.0;  // Second closest for edge detection

for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
        vec2 neighbor = vec2(float(x), float(y));
        vec2 point = hash2(i_st + neighbor);

        // Animate point position
        point = 0.5 + 0.5 * sin(time * speed + 6.2831 * point);

        float dist = length(neighbor + point - f_st);

        if (dist < m_dist) {
            m_dist2 = m_dist;
            m_dist = dist;
        } else if (dist < m_dist2) {
            m_dist2 = dist;
        }
    }
}
```

### Edge Rendering

Edge intensity derives from difference between closest and second-closest distances:

```glsl
float edge = m_dist2 - m_dist;
float edgeMask = 1.0 - smoothstep(0.0, edgeWidth, edge);
```

Values near zero indicate cell boundaries. `edgeMask` is 1.0 on edges, 0.0 in cell interiors.

### Blend Mode

Mix blend preserves original content while overlaying edge pattern:

```glsl
vec3 original = texture(texture0, fragTexCoord).rgb;
vec3 voronoiColor = vec3(edgeMask);  // White edges
finalColor = vec4(mix(original, voronoiColor, intensity), 1.0);
```

| Parameter | Range | Purpose |
|-----------|-------|---------|
| scale | 5-50 | Cell count across screen width |
| intensity | 0.0-1.0 | Blend amount (0 = bypass) |
| speed | 0.1-2.0 | Animation rate |
| edgeWidth | 0.01-0.1 | Edge thickness |

## Integration

### Pipeline Position

Insert after blur/decay, before kaleidoscope in `PostEffectEndAccum()`:

```
PostEffectBeginAccum:
  Feedback → BlurH → BlurV+Decay → [draw waveforms]

PostEffectEndAccum:
  [NEW: Voronoi] → Kaleidoscope

PostEffectToScreen:
  Chromatic
```

Voronoi pass executes at `post_effect.cpp:179` (after `EndTextureMode()`, before kaleidoscope block).

### LFO Modulation

Reuse existing `LFOConfig`/`LFOState` pattern from rotation LFO (`post_effect.cpp:122-129`):

```cpp
float effectiveScale = pe->effects.voronoiScale;
if (pe->effects.voronoiScaleLFO.enabled) {
    float lfoValue = LFOProcess(&pe->voronoiScaleLFOState,
                                 &pe->effects.voronoiScaleLFO,
                                 deltaTime);
    // LFO oscillates -1 to +1, map to scale multiplier 0.5 to 1.5
    effectiveScale *= (1.0f + 0.5f * lfoValue);
}
```

## File Changes

| File | Change |
|------|--------|
| `shaders/voronoi.fs` | Create - fragment shader with edge rendering |
| `src/config/effect_config.h:14` | Add voronoi parameters and LFO config |
| `src/render/post_effect.h:16` | Add `Shader voronoiShader` and uniform locations |
| `src/render/post_effect.h:34` | Add `LFOState voronoiScaleLFOState` |
| `src/render/post_effect.cpp:31` | Load voronoi shader |
| `src/render/post_effect.cpp:50` | Get uniform locations |
| `src/render/post_effect.cpp:179` | Add voronoi pass before kaleidoscope |
| `src/ui/ui_panel_effects.cpp` | Add UI controls (scale, intensity, speed, edge width, LFO) |

### Shader Uniforms

```glsl
uniform sampler2D texture0;
uniform vec2 resolution;
uniform float scale;      // Cell count (LFO-modulated)
uniform float intensity;  // Blend amount
uniform float time;       // Animation driver
uniform float speed;      // Animation rate
uniform float edgeWidth;  // Edge thickness
```

### Config Additions

```cpp
// effect_config.h additions
float voronoiScale = 15.0f;       // Cell count (5-50)
float voronoiIntensity = 0.0f;    // Blend (0 = disabled)
float voronoiSpeed = 0.5f;        // Animation rate
float voronoiEdgeWidth = 0.05f;   // Edge thickness
LFOConfig voronoiScaleLFO;        // Scale modulation
```

### PostEffect Struct Additions

```cpp
// post_effect.h additions
Shader voronoiShader;
int voronoiResolutionLoc;
int voronoiScaleLoc;
int voronoiIntensityLoc;
int voronoiTimeLoc;
int voronoiSpeedLoc;
int voronoiEdgeWidthLoc;
LFOState voronoiScaleLFOState;
```

## Validation

- [ ] Voronoi edges visible when intensity > 0
- [ ] Cells animate smoothly (no flickering or jumping)
- [ ] LFO modulates scale visibly (cells grow/shrink)
- [ ] Effect disabled when intensity = 0 (no performance cost)
- [ ] Voronoi feeds into kaleidoscope when both enabled
- [ ] No edge artifacts at screen boundaries
- [ ] Preset save/load includes voronoi parameters

## References

- [The Book of Shaders - Cellular Noise](https://thebookofshaders.com/12/)
- [libretro voronoi.glsl](https://github.com/libretro/glsl-shaders/blob/master/borders/resources/voronoi.glsl)
- [Nullprogram GPU Voronoi](https://nullprogram.com/blog/2014/06/01/)
