# Lichen Color Advection

Replace the static screen-space noise overlay in the Lichen effect with a hue field that diffuses alongside the reaction-diffusion species (so colonies carry their own colors as they spread), and add growth-front edge brightness from the chemistry gradient (so spreading edges glow brighter than saturated interiors). Removes the `colorScatter` parameter and introduces `hueDrift` + `edgeBoost`.

**Research**: `docs/research/lichen_color.md`

## Design

### Types

Modifications to `LichenConfig` in `src/effects/lichen.h`:

Remove field:
- `float colorScatter`

Add fields:
- `float hueDrift = 0.015f;` (range 0.0-0.1) — random walk step per growth event
- `float edgeBoost = 2.0f;` (range 0.0-8.0) — growth-front brightness multiplier

Update `LICHEN_CONFIG_FIELDS` macro: remove `colorScatter`, add `hueDrift` and `edgeBoost`.

Modifications to `LichenEffect` (uniform location cache fields):

Remove:
- `int colorScatterLoc;`

Add to state shader location group:
- `int stateHueDriftLoc;`

Add to color shader location group:
- `int colorEdgeBoostLoc;`

(The `time` uniform is already passed to the state shader; reuse it for the hash seed.)

### Algorithm

#### State shader (`shaders/lichen_state.fs`)

The existing reaction-diffusion math (Gray-Scott loop, cyclic coupling, warp, asymmetric diffusion) stays verbatim. Three additions:

**1. Add uniforms:**

```glsl
uniform float hueDrift;
```

**2. Hue diffusion (only emitted on `passIndex == 1`, since hue lives in `statePingPong1.b`):**

After computing `c0`, `c1`, `c2` and before the reaction loop, add a hue-advection block that runs only when `passIndex == 1`. The hue read uses the same warped coordinate `p` as the existing diffusion, sampled at the activator radius:

```glsl
// Hue field advection - life-weighted Laplacian over 4 cardinal neighbors.
// Reusing activatorRadius so hue spreads at the same rate as the colony front.
float selfLife = self0.y + self0.w + self1.y;
float selfHue  = self1.b;

float hueSum    = selfHue * (selfLife + 0.001);
float weightSum = (selfLife + 0.001);

vec2 dirs[4] = vec2[4](
    vec2( activatorRadius, 0.0),
    vec2(-activatorRadius, 0.0),
    vec2(0.0,  activatorRadius),
    vec2(0.0, -activatorRadius)
);
for (int i = 0; i < 4; i++) {
    vec4 ns0 = texture(texture0,  (p + dirs[i]) / r);
    vec4 ns1 = texture(stateTex1, (p + dirs[i]) / r);
    float h = ns1.b;
    float w = ns0.y + ns0.w + ns1.y + 0.001;
    hueSum    += h * w;
    weightSum += w;
}
float diffusedHue = hueSum / weightSum;

// Growth detection: difference between new total inhibitor and old total inhibitor.
// Positive when this cell is being newly colonized.
float newLife = c0.y + c1.y + c2.y;
float growth  = max(0.0, newLife - selfLife);

float drift   = (hash(p + time) - 0.5) * hueDrift * growth;
float newHue  = fract(diffusedHue + drift);
```

**3. Hash function (file-local helper in state shader):**

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
```

**4. Output write:**

Replace the `passIndex == 1` write block:

```glsl
if (passIndex == 0) {
    finalColor = vec4(c0.x, c0.y, c1.x, c1.y);
} else {
    finalColor = vec4(c2.x, c2.y, newHue, 0.0);
}
```

The `passIndex == 0` branch is unchanged. The hue computation is gated to only emit useful values into `statePingPong1.b`; under `passIndex == 0` the `.ba` of the species 0+1 texture is `c1.x, c1.y` (unchanged).

Note: the hue computation block costs 8 texture reads per fragment (4 neighbors x 2 textures). It only contributes to the output on `passIndex == 1`, so wrap the entire computation inside an `if (passIndex == 1)` to skip the work on pass 0.

#### Color shader (`shaders/lichen.fs`)

**1. Remove:**
- `uniform float colorScatter;`
- `valueNoise()` function
- `hash()` function (no longer needed in color shader; only state shader uses it now)

**2. Add uniforms:**
```glsl
uniform float edgeBoost;
```

**3. Replace `speciesColor()` function:**

```glsl
vec3 speciesColor(float v, float sliceOffset, float hue, float edgeMag) {
    float t = sliceOffset + hue * (1.0 / 3.0);
    vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;
    float mag = fftAt(t);
    float bright = baseBright + mag + edgeBoost * edgeMag;
    return col * v * bright;
}
```

**4. Update `main()`:**

```glsl
void main() {
    vec2 uv = fragTexCoord;
    vec4 s0 = texture(stateTex0, uv);
    vec4 s1 = texture(stateTex1, uv);
    float v0 = s0.y;
    float v1 = s0.w;
    float v2 = s1.y;
    float hue = s1.b;

    // Pixel-space gradient magnitude per species, normalized so radius is consistent across resolutions.
    float scale = min(resolution.x, resolution.y);
    float edge0 = length(vec2(dFdx(v0), dFdy(v0))) * scale;
    float edge1 = length(vec2(dFdx(v1), dFdy(v1))) * scale;
    float edge2 = length(vec2(dFdx(v2), dFdy(v2))) * scale;

    const float SLICE = 1.0 / 3.0;
    vec3 col0 = speciesColor(v0, 0.0,         hue, edge0);
    vec3 col1 = speciesColor(v1, SLICE,       hue, edge1);
    vec3 col2 = speciesColor(v2, 2.0 * SLICE, hue, edge2);

    vec3 col = brightness * (col0 + col1 + col2);
    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
```

The same `hue` value is used for all three species, so each species' within-slice offset is identical at any given pixel. Boundary discontinuity between species comes from the `sliceOffset` step (which is unchanged); smooth drift inside a colony comes from the diffused hue field.

#### CPU seeding (`InitializeSeed()` in `src/effects/lichen.cpp`)

The existing seed function plants species at `seedPos = {0.16, 0.5, 0.84}` with a noise-perturbed circle. The hue field needs distinct initial values per seed so colonies start with distinguishable identities:

- `pixels1[idx + 2]` (the `.b` channel) currently writes `0.0f`. Change to write a per-seed hue offset:
  - For pixels inside seed[0]'s radius: `0.15f`
  - For pixels inside seed[1]'s radius: `0.50f`
  - For pixels inside seed[2]'s radius: `0.85f`
  - For pixels outside all seeds: a low-magnitude value-noise field in `[0.0, 1.0]` so dead regions have a soft random gradient that gets adopted by whichever colony reaches them. Implement a small CPU value-noise helper (4-corner hash + smoothstep interpolation, same shape as the GLSL `valueNoise` being removed from the color shader) sampled at low frequency (e.g., grid period ~64-128 px) so the fallback varies smoothly across the screen rather than at per-pixel grain.

The seed loop already iterates each species — extend the per-pixel logic so when `dist + noise < seedRadius` for species `k`, also set the hue to the corresponding seed offset. If a pixel is inside multiple seed radii (rare with chosen positions but possible at high resolutions), the last-touching species wins (consistent with the existing v[k] = 1.0 overwrite behavior).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| hueDrift | float | 0.0-0.1 | 0.015 | yes | `Hue Drift` |
| edgeBoost | float | 0.0-8.0 | 2.0 | yes | `Edge Boost` |

`colorScatter` (range 1-120, default 40) is removed entirely.

### Constants

No new enum values. This is an enhancement to the existing `TRANSFORM_LICHEN` effect; its descriptor row in `EFFECT_DESCRIPTORS[]` does not need to change.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Update LichenConfig and LichenEffect

**Files**: `src/effects/lichen.h`
**Creates**: New struct fields and uniform location members that all other tasks depend on.

**Do**:
- In `LichenConfig`:
  - Remove `float colorScatter` field and its inline comment.
  - Add `float hueDrift = 0.015f;` with comment `// Hue random-walk step per growth event (0.0-0.1)`.
  - Add `float edgeBoost = 2.0f;` with comment `// Growth-front brightness multiplier (0.0-8.0)`.
- Update `LICHEN_CONFIG_FIELDS` macro: remove `colorScatter`, add `hueDrift` and `edgeBoost` (placement order is irrelevant for JSON, group them with the other reaction/output fields for readability).
- In `LichenEffect`:
  - Remove `int colorScatterLoc;`.
  - Add `int stateHueDriftLoc;` to the state shader uniform location group.
  - Add `int colorEdgeBoostLoc;` to the color shader uniform location group.

**Verify**: `cmake.exe --build build` fails at this stage with errors in `lichen.cpp` referencing `colorScatter` and missing locations. That is expected — Wave 2 fixes them.

---

### Wave 2: Implementation (parallel — no file overlap)

#### Task 2.1: Update Lichen C++ implementation

**Files**: `src/effects/lichen.cpp`
**Depends on**: Wave 1 complete.

**Do**:
- In `CacheStateLocations()`: add `e->stateHueDriftLoc = GetShaderLocation(e->stateShader, "hueDrift");`.
- In `CacheColorLocations()`: remove the `colorScatterLoc` lookup; add `e->colorEdgeBoostLoc = GetShaderLocation(e->shader, "edgeBoost");`.
- In `BindStateUniforms()`: add `SetShaderValue(e->stateShader, e->stateHueDriftLoc, &cfg->hueDrift, SHADER_UNIFORM_FLOAT);`.
- In `BindColorUniforms()`: remove the `colorScatter` SetShaderValue call; add `SetShaderValue(e->shader, e->colorEdgeBoostLoc, &cfg->edgeBoost, SHADER_UNIFORM_FLOAT);`.
- In `InitializeSeed()`: extend the existing per-pixel seed loop to also write a hue value into `pixels1[idx + 2]` (the `.b` channel). For pixels inside seed[k]'s radius, set hue to `{0.15f, 0.50f, 0.85f}[k]`. For pixels outside all seeds, write a smooth fallback hue using a low-frequency value-noise helper (4-corner hash + smoothstep interpolation, grid period ~64-128 px) — port the same shape as the GLSL `valueNoise` being removed from the color shader. Track which species (if any) most recently colonized this pixel and assign its seed hue; ties resolved by species index order.
- In `LichenRegisterParams()`:
  - Remove the `lichen.colorScatter` registration.
  - Add `ModEngineRegisterParam("lichen.hueDrift", &cfg->hueDrift, 0.0f, 0.1f);`.
  - Add `ModEngineRegisterParam("lichen.edgeBoost", &cfg->edgeBoost, 0.0f, 8.0f);`.
- In `DrawLichenParams()`:
  - Remove the `Color Scatter` slider.
  - In the `Output` section, add `ModulatableSlider("Hue Drift##lichen", &cfg->hueDrift, "lichen.hueDrift", "%.3f", modSources);`.
  - In the `Output` section, add `ModulatableSlider("Edge Boost##lichen", &cfg->edgeBoost, "lichen.edgeBoost", "%.2f", modSources);`.

**Verify**: `cmake.exe --build build` compiles without warnings.

#### Task 2.2: Update state shader for hue advection

**Files**: `shaders/lichen_state.fs`
**Depends on**: Wave 1 complete (uniform name locked in by header).

**Do**:
- Add `uniform float hueDrift;` alongside the other uniforms.
- Add the file-local `hash(vec2 p)` helper (formula in the Algorithm section).
- After the existing diffusion block (after `c2 /= 5.0;`) and before the reaction loop, declare `float newHue = 0.0;` then insert the hue-advection block from the Algorithm section wrapped in `if (passIndex == 1) { ... }` so pass 0 skips the 8 extra texture reads. The `newHue` variable is only consumed by the pass-1 output write, so its pass-0 value is irrelevant.
- Update the output write to use `newHue` in the `passIndex == 1` branch:
  ```glsl
  if (passIndex == 0) {
      finalColor = vec4(c0.x, c0.y, c1.x, c1.y);
  } else {
      finalColor = vec4(c2.x, c2.y, newHue, 0.0);
  }
  ```
- Keep all existing reaction-diffusion math (warp, asymmetric diffusion, reaction loop, clamp) verbatim.

**Verify**: After Task 2.1 + 2.3 are also done, run the app, enable Lichen, confirm the simulation still progresses normally and colonies have visible color variation that moves with the spread (not pinned to screen).

#### Task 2.3: Update color shader

**Files**: `shaders/lichen.fs`
**Depends on**: Wave 1 complete (uniform name locked in by header).

**Do**:
- Remove `uniform float colorScatter;`.
- Add `uniform float edgeBoost;`.
- Delete the `valueNoise()` function and the `hash()` function (no longer used in this shader).
- Replace `speciesColor()` and `main()` with the versions in the Algorithm section.

**Verify**: After Tasks 2.1 + 2.2 are also done, run the app, enable Lichen, observe:
- Each colony has a distinguishable hue at frame 1 (seed hues 0.15 / 0.50 / 0.85 within their species' LUT slice).
- As colonies spread, the hue inside each patch drifts smoothly rather than being pinned to a screen-space pattern.
- Growth fronts (the spreading edges) are visibly brighter than saturated interiors when `Edge Boost > 0`.
- Setting `Edge Boost = 0` recovers uniform colony brightness (just the FFT mag + baseBright).
- Setting `Hue Drift = 0` makes colonies preserve their seed hue indefinitely (no within-patch drift).

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings.
- [ ] `colorScatter` is gone from header, cpp, both shaders, and serialization compiles cleanly.
- [ ] `hueDrift` and `edgeBoost` appear in the UI under the `Output` section with `Hue Drift` and `Edge Boost` labels.
- [ ] Both new params appear as modulation targets (registered via `ModEngineRegisterParam`).
- [ ] On reset, the three colonies are visibly distinct colors from their seed pixels.
- [ ] As colonies spread across the screen, color travels with them rather than the colony picking up a fixed screen-space pattern.
- [ ] Growth fronts glow brighter than interiors when `Edge Boost > 0`.
- [ ] Saving and reloading a preset preserves the new `hueDrift` and `edgeBoost` values.

---

## Implementation Notes

Deviations from the original plan, captured during testing:

### `edgeBoost` removed entirely

The plan added `edgeBoost` to multiply growth-front brightness by `dFdx/dFdy` gradient magnitude. In practice this failed: at default Gray-Scott parameters the visible structure is pixel-scale spots, not large smooth colonies. Every spot's edge is its own gradient, so `length(vec2(dFdx(v), dFdy(v)))` is high almost everywhere — the "edge boost" became a global brightness multiplier, and the additional `* min(resolution.x, resolution.y)` scaling drove brightness into saturation. Removed the uniform, location, config field, registration, UI slider, and per-species edge math. Color shader now restores the simpler `bright = baseBright + mag` form.

### Hue diffusion changed: life-weighted Laplacian → max-life inheritance

The plan specified a life-weighted Laplacian average over 4 cardinal neighbors. During testing this homogenized hue rapidly — averaging blends, and within a few sim steps all colonies converged to a similar hue value, collapsing the gradient spread. Replaced with: each growing cell adopts the hue of its *most-alive* neighbor (single pick, not average), gated by `growth > 0.001`. Stable cells preserve their hue indefinitely. This mimics colonization (one parent inherits) instead of blending, and keeps colonies distinct.

### Seed noise function rewritten to fix aliasing

The plan reused the original `sinf(px * py + dateOffset)` boundary noise. At the default 1920x1080 window, `px * py` reaches ~1.46M at seed[2] = (1613, 907), where `sinf` precision degrades (float mantissa ~24 bits resolves to ~0.7 at that magnitude). Two of three seed disks end up carved into disconnected speckles too small to survive Gray-Scott startup. Replaced with `(HueHash(px + dateOffset, py + dateOffset) - 0.5) * 2 * noiseAmp` — same noise amplitude, no aliasing, all three seeds reliably establish.

### Pre-existing bugs uncovered and fixed

Two unrelated issues surfaced during testing and were fixed in the same branch:

- **`reactionSteps` default was 10 in code but research/reference spec says 25.** Restored default to 25; without this, the Gray-Scott parameter regime sat in spots mode rather than the intended coral mode (visible as fine-grained noisy patterns rather than smooth growths).
- **`warpSpeed` was multiplied by `time` directly in the state shader.** Bumping `warpSpeed` at runtime caused an instant phase jump of `time * Δspeed` radians, snapping the warp pattern. Refactored to CPU-accumulated `warpPhase = sum(warpSpeed * deltaTime)` passed as its own uniform. Per the speed-accumulation rule (memory: `feedback_speed_accumulation.md`), `time * speed` belongs on CPU, never in the shader. Reference shadertoy violates this rule because Shadertoy has no runtime parameter changes; we must always extract speed to CPU regardless of how the reference looks.

### Final parameter set

After deviations:

| Parameter | Type | Range | Default | Notes |
|-----------|------|-------|---------|-------|
| hueDrift | float | 0.0-0.1 | 0.015 | Random-walk step on growth events |

`edgeBoost` removed. `colorScatter` removed (per plan).

### Category move

Lichen relocated from section 13 (Field) to section 12 (Texture) in the effect descriptor and `docs/effects.md` inventory. The pixel-scale spot patterns sit better with the Texture grouping than the Field grouping.
