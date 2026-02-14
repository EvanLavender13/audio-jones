# Nebula Enhancement

Adds a noise type selector (kaliset vs domain-warped FBM), FBM-based dust lanes, and diffraction cross-spikes on bright stars to the existing nebula generator. The 3-layer parallax system, FFT-reactive stars, and gradient LUT coloring remain unchanged.

**Research**: `docs/research/nebula_enhancement.md`

## Design

### Types

**NebulaConfig additions** (appended to existing struct):

```
int noiseType = 0;            // 0 = kaliset, 1 = domain-warped FBM
float dustScale = 3.5f;       // FBM frequency for dust lanes (1.0-8.0)
float dustStrength = 0.4f;    // Opacity of dark absorption (0.0-1.0) — 0 disables
float dustEdge = 0.1f;        // Smoothstep width for lane boundaries (0.05-0.3)
float spikeIntensity = 0.5f;  // Diffraction cross brightness (0.0-2.0)
float spikeSharpness = 20.0f; // Exponent for spike thinness (5.0-40.0)
```

**NebulaEffect additions** (uniform location cache):

```
int noiseTypeLoc;
int dustScaleLoc;
int dustStrengthLoc;
int dustEdgeLoc;
int spikeIntensityLoc;
int spikeSharpnessLoc;
```

### Algorithm

#### FBM functions

Always defined in the shader (needed for dust lanes in both modes):

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1, 0)), f.x),
        mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x),
        f.y
    );
}

float fbm(vec2 p, int octaves) {
    float v = 0.0, a = 0.5;
    mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    for (int i = 0; i < octaves; i++) {
        v += a * noise(p);
        p = rot * p * 2.0;
        a *= 0.5;
    }
    return v;
}
```

#### Noise type selector

`uniform int noiseType;` branches per-layer evaluation. Uniform-based branch — no divergence.

**Kaliset mode (noiseType == 0):** Existing `field()` function, unchanged.

**FBM mode (noiseType == 1):** 3-layer nested domain warp per layer:

```glsl
// layerUV is the per-layer scaled UV (same as kaliset's uvs/scale)
// octaves is the per-layer iter value clamped to 2-8
vec2 q = vec2(fbm(layerUV * 2.0 + time * 0.08, octaves),
              fbm(layerUV * 2.0 + vec2(5.2, 1.3), octaves));
vec2 rr = vec2(fbm(layerUV * 2.0 + 4.0 * q + vec2(1.7, 9.2) + 0.12 * time, octaves),
               fbm(layerUV * 2.0 + 4.0 * q + vec2(8.3, 2.8) + 0.1 * time, octaves));
float t = fbm(layerUV * 2.0 + 4.0 * rr, octaves);
```

The `fbm()` takes an octave count from the layer's iter uniform (clamped 2-8 in shader). The resulting `t` feeds the same LUT color mapping as kaliset mode.

Extract per-layer evaluation into a helper to avoid duplicating the branch 3 times. Kaliset `field()` takes `vec3 p` and `float s = length(p)`, FBM only needs 2D UV. Pass both — the unused one costs nothing.

#### Dust lanes

Single global dust mask applied after gas color computation, before stars are added. Not per-layer.

```glsl
float dust = fbm(uvs * dustScale + vec2(time * 0.05, -time * 0.03), 6);
float dustLanes = smoothstep(0.45 - dustEdge, 0.45 + dustEdge, dust) * dustStrength;
vec3 darkTint = texture(gradientLUT, vec2(0.02, 0.5)).rgb * 0.1;
gasColor = mix(gasColor, darkTint, dustLanes * 0.7);
```

The `fbm()` function here always uses 6 octaves (hardcoded, not tied to layer iters). When `dustStrength == 0.0`, the mix is a no-op.

In kaliset mode (noiseType == 0), the `hash()`, `noise()`, `fbm()` functions are still needed for dust lanes. They're always defined in the shader regardless of mode.

#### Diffraction spikes

Added inside the `starLayer()` function, after the existing glow calculation, for stars above a brightness threshold:

```glsl
// After glow is computed, for bright stars:
float h = rnd.y;  // already available — the hash used for star density threshold
if (h > 0.99) {   // top ~1% brightest stars get spikes
    float angle = atan(sp.y, sp.x);
    float spike = pow(max(cos(angle * 2.0), 0.0), spikeSharpness)
                + pow(max(cos(angle * 2.0 + 1.5707963), 0.0), spikeSharpness);
    spike *= exp(-d2 * 8.0) * spikeIntensity;
    // Add spike with same tint as star core
    color += spike * mix(tint, vec3(1.0), 0.5);
}
```

The threshold `0.99` means only ~1% of grid cells that already passed the star density check get spikes. The `PI/2` offset is hardcoded as `1.5707963`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| noiseType | int | 0-1 | 0 | No | "Noise Type" (Combo: "Kaliset", "FBM") |
| dustScale | float | 1.0-8.0 | 3.5 | Yes | "Dust Scale" |
| dustStrength | float | 0.0-1.0 | 0.4 | Yes | "Dust Strength" |
| dustEdge | float | 0.05-0.3 | 0.1 | Yes | "Dust Edge" |
| spikeIntensity | float | 0.0-2.0 | 0.5 | Yes | "Spike Intensity" |
| spikeSharpness | float | 5.0-40.0 | 20.0 | Yes | "Spike Sharpness" |

### UI Layout

New sections inserted into the existing nebula UI panel:

- After "Layers" section: **"Dust"** section with dustScale, dustStrength, dustEdge
- Inside existing "Layers" section (before iteration sliders): noiseType combo
- After "Stars" section: **"Spikes"** section with spikeIntensity, spikeSharpness

---

## Tasks

### Wave 1: Config & Shader

#### Task 1.1: Add config fields and uniform locations

**Files**: `src/effects/nebula.h`, `src/effects/nebula.cpp`, `src/config/preset.cpp`

**Do**:
- Add the 6 new fields to `NebulaConfig` (see Design > Types)
- Add the 6 new `*Loc` fields to `NebulaEffect`
- In `NebulaEffectInit()`: add 6 `GetShaderLocation()` calls for the new uniforms
- In `NebulaEffectSetup()`: add 6 `SetShaderValue()` calls binding the new config fields
- In `NebulaRegisterParams()`: register dustScale, dustStrength, dustEdge, spikeIntensity, spikeSharpness (5 new `ModEngineRegisterParam` calls). noiseType is NOT registered (non-modulatable).
- In `preset.cpp`: add the 6 new field names to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `NebulaConfig`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Enhance nebula shader

**Files**: `shaders/nebula.fs`

**Do**:
- Add 6 new uniforms: `noiseType` (int), `dustScale`, `dustStrength`, `dustEdge`, `spikeIntensity`, `spikeSharpness` (float)
- Add `hash()`, `noise()`, `fbm(vec2 p, int octaves)` functions from the Algorithm section (always defined, needed for dust in both modes)
- Refactor per-layer gas evaluation: extract a helper that branches on `noiseType` — kaliset calls existing `field()`, FBM calls the nested domain warp pattern. Apply to all 3 layers.
- In FBM mode, clamp the per-layer iter uniform to 2-8 range: `int oct = clamp(frontIter, 2, 8);`
- Add dust lanes after gas Reinhard tone-map, before star computation (see Algorithm > Dust lanes)
- Add diffraction spikes inside `starLayer()` for stars with `rnd.y > 0.99` (see Algorithm > Diffraction spikes)

**Verify**: `cmake.exe --build build` compiles. Run the app and toggle nebula — kaliset mode should look identical to before. Switch to FBM mode and verify different gas appearance. Increase dustStrength to see dark lanes. Increase spikeIntensity to see cross flares on bright stars.

### Wave 2: UI

#### Task 2.1: Add UI controls

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp`

**Depends on**: Wave 1 complete (needs new config fields)

**Do**:
- Inside the existing `if (e->nebula.enabled)` block in `DrawGeneratorsNebula()`:
- In the "Layers" section, before the iteration sliders: add `ImGui::Combo("Noise Type##nebula", &n->noiseType, "Kaliset\0FBM\0")`
- After the "Layers" section: add new `ImGui::SeparatorText("Dust")` section with:
  - `ModulatableSlider("Dust Scale##nebula", &n->dustScale, "nebula.dustScale", "%.1f", modSources)`
  - `ModulatableSlider("Dust Strength##nebula", &n->dustStrength, "nebula.dustStrength", "%.2f", modSources)`
  - `ModulatableSlider("Dust Edge##nebula", &n->dustEdge, "nebula.dustEdge", "%.2f", modSources)`
- After the "Stars" section: add new `ImGui::SeparatorText("Spikes")` section with:
  - `ModulatableSlider("Spike Intensity##nebula", &n->spikeIntensity, "nebula.spikeIntensity", "%.2f", modSources)`
  - `ModulatableSlider("Spike Sharpness##nebula", &n->spikeSharpness, "nebula.spikeSharpness", "%.1f", modSources)`

**Verify**: `cmake.exe --build build` compiles. Open UI, verify new controls appear in correct sections.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Kaliset mode (noiseType=0) looks identical to the current nebula
- [ ] FBM mode (noiseType=1) shows domain-warped cloud gas
- [ ] Dust lanes darken gas when dustStrength > 0
- [ ] Diffraction spikes appear on brightest stars when spikeIntensity > 0
- [ ] All new sliders respond to modulation
- [ ] Presets save/load new fields correctly

## Implementation Notes

**Diffraction spikes (deviations from original plan):**
- Spikes use `pow(abs(cos(angle)), sharpness) + pow(abs(sin(angle)), sharpness)` for a `+` cross pattern instead of the plan's `cos(angle * 2.0)` which produced an X
- Added per-star flicker: `0.5 + 0.5 * sin(time * rate + offset)` so spikes shimmer instead of being static
- Multiplied by `react` so spikes are audio-reactive (only visible on stars with FFT energy)
- Radial falloff reduced from `exp(-d2 * 8.0)` to `exp(-d2 * 2.0)` so spikes extend past the glow halo
- Threshold lowered from `0.99` to `0.90` (top 10% of stars get spikes instead of 1%)

**Star color mapping:**
- Changed from `fract(semi / 12.0)` to `semi / float(totalSemitones)` so each semitone gets a unique gradient color (48 colors with 4 octaves) instead of collapsing octaves to 12 note classes
