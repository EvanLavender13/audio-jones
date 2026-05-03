# Lichen Color Advection

Enhancement to the existing Lichen effect that replaces the static screen-space
noise overlay with a hue field that diffuses alongside the reaction-diffusion
species, so colonies carry distinctive territorial colors as they spread, and
adds growth-front edge brightness derived from the chemistry gradient so the
spreading edges glow brighter than saturated interiors.

## Classification

- **Category**: GENERATORS > Field (enhancement to existing Lichen effect)
- **Pipeline Position**: 13 (Field) - unchanged
- **Compute Model**: Existing dual ping-pong texture pair + color output pass; reuses `statePingPong1.b` channel (currently unused) for hue field

## Attribution

- Builds on existing Lichen effect, which is based on "rps cell nomming :3" by frisk256 (CC BY-NC-SA 3.0)
- Hue advection technique adapts passive scalar transport from GPU Gems

## References

- [GPU Gems Ch 38: Fast Fluid Dynamics Simulation on the GPU](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu) - Passive scalar advection on the GPU. The dye-color RGB textures concept maps directly to a single-channel hue field carried by the same diffusion process.
- [Karl Sims - Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html) - 4-tap Laplacian kernel; same kernel used for the passive hue channel.
- [Inigo Quilez - dFdx/dFdy gradient](https://iquilezles.org/articles/gradientnoise/) - Screen-space derivatives for gradient magnitude.

## Reference Code

### Existing Lichen state shader (relevant excerpt from `shaders/lichen_state.fs`)

The diffusion pattern that hue will reuse:

```glsl
vec2 c0 = self0.xy;
c0.x += texture(texture0, (p + vec2(activatorRadius, 0.0)) / r).x;
c0.x += texture(texture0, (p + vec2(0.0, activatorRadius)) / r).x;
c0.x += texture(texture0, (p - vec2(activatorRadius, 0.0)) / r).x;
c0.x += texture(texture0, (p - vec2(0.0, activatorRadius)) / r).x;
c0.y += texture(texture0, (p + vec2(inhibitorRadius, 0.0)) / r).y;
c0.y += texture(texture0, (p + vec2(0.0, inhibitorRadius)) / r).y;
c0.y += texture(texture0, (p - vec2(inhibitorRadius, 0.0)) / r).y;
c0.y += texture(texture0, (p - vec2(0.0, inhibitorRadius)) / r).y;
c0 /= 5.0;
```

### Hue advection kernel (new — same 5-tap pattern, no reaction)

```glsl
// Read hue channel (.b of statePingPong1) at center and 4 cardinal neighbors at activatorRadius.
// Weight each neighbor's contribution by that neighbor's life (sum of all 3 species inhibitors)
// so dead pixels do not dilute living hue.

float hueAt(vec2 ofs) {
    vec4 s1 = texture(stateTex1, (p + ofs) / r);
    return s1.b;
}

float lifeAt(vec2 ofs) {
    vec4 s0 = texture(texture0,  (p + ofs) / r);
    vec4 s1 = texture(stateTex1, (p + ofs) / r);
    return s0.y + s0.w + s1.y;  // sum of 3 inhibitors
}

float selfHue   = hueAt(vec2(0.0));
float selfLife  = lifeAt(vec2(0.0));
vec2 dirs[4] = vec2[4](
    vec2( activatorRadius, 0.0),
    vec2(-activatorRadius, 0.0),
    vec2(0.0,  activatorRadius),
    vec2(0.0, -activatorRadius)
);

float hueSum    = selfHue * (selfLife + 0.001);
float weightSum = (selfLife + 0.001);
for (int i = 0; i < 4; i++) {
    float h = hueAt(dirs[i]);
    float w = lifeAt(dirs[i]) + 0.001;
    hueSum    += h * w;
    weightSum += w;
}
float diffusedHue = hueSum / weightSum;

// Drift: small random walk where new growth occurs (dv > 0 means cell is being colonized).
// Use a deterministic hash so the drift is reproducible per pixel/frame.
float growth = max(0.0, (c0.y + c1.y + c2.y) - selfLife);
float drift  = (hash(p + time) - 0.5) * hueDrift * growth;
float newHue = fract(diffusedHue + drift);
```

### Edge brightness in color shader (new)

```glsl
float edgeMag(float v) {
    // Pixel-space gradient magnitude; resolution-normalize so radius is consistent.
    return length(vec2(dFdx(v), dFdy(v))) * min(resolution.x, resolution.y);
}
```

## Algorithm

### Adaptation Notes

This is a modification of an existing effect. Two changes:

1. **State shader (`shaders/lichen_state.fs`)** — extend output to write a hue channel into `statePingPong1.b` alongside species 2.
2. **Color shader (`shaders/lichen.fs`)** — replace `valueNoise(uv * colorScatter)` with the diffused hue, and multiply species brightness by `(1 + edgeBoost * gradMag)`.

### What to keep verbatim

- Entire existing reaction-diffusion math (Gray-Scott loop, cyclic coupling, warp, asymmetric diffusion radii).
- `passIndex` dispatch (pass 0 = species 0+1, pass 1 = species 2).
- Color shader's per-species LUT slice (`sliceOffset = 0.0, 1/3, 2/3`).
- FFT brightness mapping.

### What to add

**State shader (only on `passIndex == 1`, since hue lives in `statePingPong1.b`):**

- Read 4 cardinal neighbors at `activatorRadius` from `stateTex1.b`.
- For each neighbor, also read its life (sum of 3 inhibitors from both stateTex0 and stateTex1) to use as a weight.
- Compute life-weighted average hue. This stops dead-region hues from leaking inward; only "live" colonies contribute to neighbor color.
- Detect growth: `growth = max(0.0, newInhibitorSum - oldInhibitorSum)`. When a cell is being newly colonized this is positive.
- Drift: `drift = (hash(p + time) - 0.5) * hueDrift * growth`. Frozen for stable cells, walks slowly during growth.
- Output: `finalColor = vec4(c2.x, c2.y, fract(diffusedHue + drift), 0.0)`.

**Color shader:**

- Read hue from `stateTex1.b`. This is one float in `[0,1]`.
- Replace per-species color sampling:
  ```
  // OLD
  float spatial = valueNoise(uv * colorScatter);
  float t = sliceOffset + spatial * (1.0 / 3.0);

  // NEW
  float t = sliceOffset + texture(stateTex1, uv).b * (1.0 / 3.0);
  ```
  This gives option 3 from the brainstorm: each species still occupies its own 1/3 LUT slice (boundary discontinuity at species fronts), but the offset within that slice drifts smoothly across each colony patch (smooth gradient inside).
- Compute edge boost per species:
  ```
  float edge0 = length(vec2(dFdx(v0), dFdy(v0))) * min(resolution.x, resolution.y);
  float bright0 = baseBright + fftAt(t) + edge0 * edgeBoost;
  ```
- Multiply species color by `bright0` instead of `(baseBright + mag)`.

### Seeding

Initial hue field needs to be planted with three distinct colors at the seed positions so colonies have distinguishable identities from frame 1.

In `InitializeSeed()` (`src/effects/lichen.cpp`):

- Currently `pixels1` writes `(1.0, v[2], 0.0, 0.0)` for stateTex1.
- Change `.b` to a per-seed hue: pixels closest to seed[0] get hue ~0.15, seed[1] ~0.5, seed[2] ~0.85 (these are within-slice offsets, so they spread the LUT slice for each species).
- Outside seed radii, fill with low-magnitude valueNoise so dead regions have a soft gradient that gets adopted by whichever colony reaches them.

### Removing colorScatter

The `colorScatter` config field becomes irrelevant since we no longer use `valueNoise(uv * colorScatter)`. Remove from header, registration, UI, and `LICHEN_CONFIG_FIELDS`. Replace with `hueDrift` (drift step size during growth) and `edgeBoost` (growth-front brightness multiplier).

## Parameters

New / changed:

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| hueDrift | float | 0.0-0.1 | 0.015 | Random-walk step per growth event; 0 = colonies stay perfectly inherited, high = each new pixel rerolls hue |
| edgeBoost | float | 0.0-8.0 | 2.0 | Growth-front brightness multiplier from chemistry gradient; 0 = uniform brightness, high = dark interiors with bright edges |

Removed: `colorScatter`.

All other existing Lichen parameters (feedRate, killRateBase, etc.) are unchanged.

## Modulation Candidates

- **hueDrift**: Modulate to make colonies stable (low) versus chaotic-shifting (high). Audio-energy on this makes loud passages produce kaleidoscopic color churn while quiet passages settle into stable territorial colors.
- **edgeBoost**: Modulate to control "where the action is." Low = whole colony glows uniformly. High = only the spreading edge is visible, interiors go dark — turns the visualization into a moving frontline.

### Interaction Patterns

**Cascading threshold - feedRate vs edgeBoost:**
Edge brightness only exists where chemistry is changing. If feedRate drops to a quasi-static state, `dFdx(v)` collapses and edge highlights vanish regardless of edgeBoost setting. So edgeBoost is gated by whether the system is actively growing. Audio on feedRate creates moments where the screen comes alive with bright moving edges (high feed) versus moments where the colonies just sit there dimly (low feed).

**Competing forces - hueDrift vs warpIntensity:**
Drift introduces hue variation at growth events (small, local). Warp shears the entire coordinate field (large, structural). High drift + low warp = colonies with smooth internal gradients but stable shapes. Low drift + high warp = monochromatic colonies with chaotic shapes. Both high = the colonies look like they are dissolving into multicolored smoke.

## Notes

- The hue diffusion happens once per sim step, same loop as the existing reaction-diffusion. With `simSteps = 3` (the recently added param), hue diffuses 3 steps per frame, which is plenty to keep up with the spreading front.
- `dFdx`/`dFdy` are pixel-quad finite differences (free in fragment shaders). Edge detection cost is essentially zero.
- The life-weighted hue average is critical: a naive Laplacian average would let zero-hue dead regions drag living colony hues toward 0 (red-magenta) and slowly bleed all colors out. Weighting by inhibitor sum keeps hue confined to live regions.
- Hash function for drift: reuse the existing `hash(vec2 p)` from the color shader, move it to the state shader. Seed it with `p + time` so drift is per-pixel-per-frame deterministic but not aligned to a static screen pattern.
- Boundary case: when `growth = 0` (cell is stable or dying), `drift = 0`, so stable colonies keep their hue forever. Only fresh growth diverges. This is what creates option 3's "smooth drift inside patches, sharp gaps at boundaries" behavior — the boundary gap comes from the species changing (different LUT slice), not from hue drift.
- The reference frisk256 shader maps each species' inhibitor `.y` to one RGB channel directly. Our gradient-LUT-slice approach is a deliberate divergence to support presets/themes via the shared ColorConfig. This research preserves that, just replaces the noise source for the within-slice offset.
