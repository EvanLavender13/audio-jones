# Chromatic Aberration Rework

Radial color fringing that spreads RGB channels outward from center — hard neon bands at low sample counts, smooth prismatic rainbow at high counts. Moves from the fixed Output pipeline slot into the Optical transform category so users can reorder it alongside Bloom, Bokeh, etc.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: User-reorderable in transform chain (currently hard-coded before transforms)
- **Migration**: Remove from Output pipeline, add as standard transform effect with descriptor table entry

## References

- [GM Shaders Mini: Chromatic Aberration](https://mini.gmshaders.com/p/gm-shaders-mini-chromatic-aberration) - Spectral multi-sample loop with weighted RGB averaging
- [Godot Shaders: Radial Chromatic Aberration](https://godotshaders.com/shader/radial-chromatic-aberration-2/) - Exponential distance falloff for radial intensity control
- [3D Game Shaders For Beginners: Chromatic Aberration](https://lettier.github.io/3d-game-shaders-for-beginners/chromatic-aberration.html) - Basic radial 3-sample approach (similar to current implementation)

## Reference Code

Spectral multi-sample approach from GM Shaders (XorDev). This is the core loop — at 3 samples it reproduces hard RGB bands, at higher counts it produces smooth prismatic spreading:

```glsl
// From GM Shaders Mini: Chromatic Aberration
#define SAMPLES 10.0;
#define OFFSET vec2(0.01, 0.01)

vec4 color_sum = vec4(0);
vec4 weight_sum = vec4(0);

for (float i = 0.0; i <= 1.0; i += 1.0 / SAMPLES)
{
    vec2 coord = v_vTexcoord + (i - 0.5) * OFFSET;
    vec4 color = texture2D(gm_BaseTexture, coord);
    // R: 0→1, G: 0→1→0 (peak at center), B: 1→0
    vec4 weight = vec4(i, 1.0 - abs(i * 2.0 - 1.0), 1.0 - i, 0.5);

    color_sum += color * color * weight;  // squared for gamma
    weight_sum += weight;
}
gl_FragColor = sqrt(color_sum / weight_sum);  // sqrt for gamma decode
```

Exponential distance falloff from Godot Shaders:

```glsl
// From Godot Shaders: Radial Chromatic Aberration
vec2 center = vec2(0.5);
vec2 dir = SCREEN_UV - center;
float dist = 2.0 * length(dir);
float effect = exp(intensity * (dist - threshold));
```

## Algorithm

Adapt the spectral loop to our radial setup. Key changes from the reference:

- Replace linear `OFFSET` direction with radial direction from center (keep from current `chromatic.fs`)
- Replace `#define SAMPLES` with `int samples` uniform for runtime control
- Replace fixed offset with `chromaticOffset / min(resolution.x, resolution.y)` pixel-based scaling (keep from current)
- Add `pow(dist, falloff)` distance curve instead of the Godot exponential (simpler, more intuitive range)
- Keep the center fade `smoothstep(0.0, 0.15, dist)` from the current shader to avoid direction instability at center
- Drop the gamma square/sqrt from the reference — our pipeline handles gamma separately in the output stage

The shader loop body:

1. Compute radial direction and distance from center (same as current)
2. Apply distance curve: `intensity = smoothstep(0.0, 0.15, dist) * pow(dist, falloff)`
3. Loop `i` from `0` to `1` in `1/samples` steps
4. At each step: offset UV by `radialDir * chromaticOffset * intensity * (i - 0.5) / minRes`
5. Sample texture, apply spectral weight `(i, 1 - abs(2i - 1), 1 - i, 0.5)`
6. Accumulate weighted color sum
7. Output `color_sum / weight_sum`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `offset` | float | 0–50 | 0.0 | Channel separation strength in pixels (0 = disabled) |
| `samples` | int | 3–24 | 3 | Spectral sample count — 3 = hard RGB bands, higher = smooth rainbow |
| `falloff` | float | 0.5–3.0 | 1.0 | Distance curve exponent — 1 = linear, 2+ = edge-heavy, <1 = uniform |

## Modulation Candidates

- **offset**: Primary animation target — maps to beats, energy, transients. The main "kick" parameter.
- **samples**: Shift between hard/smooth character across song sections. Low in verses (gritty), high in chorus (lush).
- **falloff**: Reshape the spatial distribution. High falloff concentrates fringing at edges; low spreads it everywhere.

### Interaction Patterns

**offset × falloff (competing forces):** High offset wants to push color everywhere, high falloff wants to concentrate it at edges. With both modulated, the effect breathes between "tight edge fringe" and "full-screen prismatic wash" depending on which parameter leads at any moment.

**offset × samples (cascading threshold):** At low offset values, sample count barely matters — there's not enough separation to see individual bands vs smooth gradient. As offset climbs, the difference between 3 and 16 samples becomes dramatic. Offset gates whether the samples parameter is visually relevant.

## Notes

- Default of 3 samples exactly reproduces the current hard RGB look — no visual regression on existing presets
- The `chromaticOffset` config field stays at 0.0 default, meaning the effect is visually disabled until the user dials it up (same as current behavior)
- At 24 samples this is 24 texture fetches per pixel — moderate cost, but Bloom already does more. Keep max reasonable.
- Existing presets that use `chromaticOffset` will need the field migrated from the top-level `EffectConfig` to the new `ChromaticAberrationConfig` struct. Preset deserialization should handle the old field name gracefully.
- Remove: `chromaticShader`, `chromaticResolutionLoc`, `chromaticOffsetLoc` from `PostEffect` struct; `SetupChromatic()` from `shader_setup.cpp`; the fixed `RenderPass` call in `render_pipeline.cpp`; the "Chroma" slider from `imgui_effects.cpp` OUTPUT section
