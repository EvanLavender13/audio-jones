# Fracture Grid Enhancements

Add three new parameters to the fracture grid effect: `waveShape` (smooth sine vs snappy hold-and-release animation), `borderBlend` (hard tile edges vs soft neighbor-blended edges), and `spatialBias` (random hash-based variation vs structured position-derived variation). Techniques adapted from Kali's Fractasia and Fiesta shaders.

## Design

### Types

Add to `FractureGridConfig`:

```cpp
float waveShape = 0.0f;    // 0.0-1.0 — smooth sine (0) to snappy hold-and-release (1)
float borderBlend = 0.0f;  // 0.0-1.0 — hard tile edges (0) to soft neighbor-blended (1)
float spatialBias = 0.0f;  // 0.0-1.0 — random hash per tile (0) to position-derived (1)
```

Add to `FractureGridEffect`:

```cpp
int waveShapeLoc;
int borderBlendLoc;
int spatialBiasLoc;
```

Update `FRACTURE_GRID_CONFIG_FIELDS` to include all three new fields.

### Algorithm

#### shapedWave — smooth-to-snappy wave shaping

Replaces all three `sin()` calls in the per-tile animation. Controlled by `waveShape` uniform.

```glsl
float shapedWave(float t, float shape) {
    float s = sin(t);
    float k = mix(1.0, 0.1, shape);
    return smoothstep(-k, k, s) * 2.0 - 1.0;
}
```

At `shape=0`: `k=1.0`, `smoothstep(-1,1,sin(t))*2-1` is approximately linear — close to sine. At `shape=1`: `k=0.1`, the wave snaps to ±1 quickly and holds. Output range stays [-1, 1].

Replace all three `sin(waveTime + phase)`, `sin(waveTime * 1.3 + phase)`, `sin(waveTime * 0.7 + phase)` with `shapedWave(...)`.

#### sabs — smooth absolute value (used by spatialBias)

```glsl
float sabs(float x, float c) {
    return sqrt(x * x + c);
}
```

#### computeSpatial — position-derived tile variation

Replaces `hash3(id)` when `spatialBias > 0`. Produces smooth structured values in [0,1] that vary by tile position.

```glsl
vec3 computeSpatial(vec2 center) {
    return vec3(
        fract(sabs(center.x * center.y, 0.1) * 3.7),
        fract(sabs(center.x + center.y, 0.2) * 2.3),
        fract(sabs(center.x - center.y, 0.15) * 3.1)
    );
}
```

Blend with hash in main logic:

```glsl
vec3 h = mix(hash3(id), computeSpatial(cellCenter), spatialBias);
```

#### computeTileWarp — extracted per-tile transform

Extract the current per-tile warp computation (hash → offset/angle/zoom → sample point) into a reusable function so it can be called for both the current tile and the neighbor tile:

```glsl
vec2 computeTileWarp(vec2 tileId, vec2 tileCellUV, vec2 tileCellCenter, float sub) {
    vec3 h = mix(hash3(tileId), computeSpatial(tileCellCenter), spatialBias);
    float phase = h.x * 6.283;
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale
                * shapedWave(waveTime + phase, waveShape);
    float angle = (h.z - 0.5) * stagger * rotationScale
                * shapedWave(waveTime * 1.3 + phase, waveShape);
    float zoom = max(0.2, 1.0 + (h.y - 0.5) * stagger * zoomScale
                * shapedWave(waveTime * 0.7 + phase, waveShape));
    return tileCellCenter + rot2(angle) * (tileCellUV / (sub * zoom)) + offset;
}
```

#### borderBlend — neighbor-sampled edge blending

After computing the current tile's warped sample, compute edge distance and blend with the neighbor tile's warped sample when `borderBlend > 0`.

Edge distance per tessellation:

```glsl
float edgeDist;
vec2 edgeDir;
if (tessellation == 0) {
    vec2 ac = abs(cellUV);
    edgeDist = 0.5 - max(ac.x, ac.y);
    edgeDir = (ac.x > ac.y) ? vec2(sign(cellUV.x), 0.0) : vec2(0.0, sign(cellUV.y));
} else {
    edgeDist = 0.5 - length(cellUV);
    edgeDir = (length(cellUV) > 0.001) ? normalize(cellUV) : vec2(1.0, 0.0);
}
```

Neighbor sampling via re-tessellation (universal for all 3 modes):

```glsl
float blendWidth = borderBlend * 0.4;
float blendFactor = smoothstep(0.0, blendWidth + 0.001, edgeDist);

vec4 currentSample = texture(texture0, currentMirrorUV);

if (borderBlend > 0.001 && blendFactor < 0.999) {
    vec2 neighborP = p + edgeDir / subdivision;

    vec2 id2, cellUV2, cellCenter2;
    if (tessellation == 1) tessHex(neighborP, subdivision, id2, cellUV2, cellCenter2);
    else if (tessellation == 2) tessTri(neighborP, subdivision, id2, cellUV2, cellCenter2);
    else tessRect(neighborP, subdivision, id2, cellUV2, cellCenter2);

    vec2 neighborWarpedP = computeTileWarp(id2, cellUV2, cellCenter2, subdivision);
    vec2 neighborUV = vec2(neighborWarpedP.x / aspect + 0.5, neighborWarpedP.y + 0.5);
    vec2 neighborMirror = 1.0 - abs(mod(neighborUV, 2.0) - 1.0);
    vec4 neighborSample = texture(texture0, neighborMirror);

    finalColor = mix(neighborSample, currentSample, blendFactor);
} else {
    finalColor = currentSample;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveShape | float | 0.0-1.0 | 0.0 | Yes | Wave Shape |
| borderBlend | float | 0.0-1.0 | 0.0 | Yes | Border Blend |
| spatialBias | float | 0.0-1.0 | 0.0 | Yes | Spatial Bias |

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add config fields and uniform locations

**Files**: `src/effects/fracture_grid.h`

**Do**:
- Add `waveShape`, `borderBlend`, `spatialBias` fields to `FractureGridConfig` with defaults and range comments (see Design > Types)
- Add `waveShapeLoc`, `borderBlendLoc`, `spatialBiasLoc` to `FractureGridEffect`
- Update `FRACTURE_GRID_CONFIG_FIELDS` macro to include all three new fields

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: C++ lifecycle and UI

**Files**: `src/effects/fracture_grid.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `FractureGridEffectInit`: cache uniform locations for `waveShape`, `borderBlend`, `spatialBias`
- In `FractureGridEffectSetup`: set all three uniforms via `SetShaderValue`
- In `FractureGridRegisterParams`: register all three params with ranges matching the config header comments
- In `DrawFractureGridParams`: add three `ModulatableSlider` calls with `"%.2f"` format, IDs `"fractureGrid.waveShape"` etc.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Shader implementation

**Files**: `shaders/fracture_grid.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add `uniform float waveShape`, `uniform float borderBlend`, `uniform float spatialBias`
- Add helper functions: `sabs()`, `shapedWave()`, `computeSpatial()` (see Algorithm section)
- Extract per-tile warp into `computeTileWarp()` (see Algorithm section)
- Replace the three `sin()` calls in `main()` with `computeTileWarp()` which internally uses `shapedWave()`
- Replace `vec3 h = hash3(id)` with `mix(hash3(id), computeSpatial(cellCenter), spatialBias)` inside `computeTileWarp`
- Add borderBlend neighbor sampling after the current tile's sample (see Algorithm > borderBlend)
- Preserve the existing pass-through guard (`if (subdivision < 0.01)`) and mirror-wrap logic

**Verify**: `cmake.exe --build build` compiles. Shader loads without errors at runtime.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] waveShape=0 looks identical to current behavior
- [x] waveShape=1 produces snappy hold-and-release animation
- [x] spatialBias=0 looks identical to current behavior (random per tile)
- [x] spatialBias=1 produces cohesive position-varying tile transforms
- [x] Both params respond to modulation
- [x] Preset save/load round-trips new fields
- [x] Works correctly with all 3 tessellation modes (rect, hex, tri)

## Implementation Notes

**borderBlend dropped.** Cross-fading neighboring tile samples (both color-space and warp-space) produced ghosting/smearing artifacts. Supersampling (à la Kali's Fractasia 5×5 loop) just blurred. No clean approach exists for blending tiles with independently randomized warps — removed entirely.

**computeSpatial reworked to time-varying value noise.** The original `sabs`-based algebraic functions (`x*y`, `x+y`, `x-y`) had axis/diagonal symmetry, causing visible repeating oscillation. Replacing with iterative Kali folds added complexity but created random-looking holes. The fix: 2D value noise (bilinear interpolation of `hash3` at grid corners) with the sampling position drifting by `waveTime * (0.07, 0.05)`. This preserves spatial continuity (nearby tiles move together) while the slowly shifting noise landscape prevents the same pattern from repeating.

**rotationScale range fixed.** Was `0.0–PI` with a raw float slider — violated angular conventions. Changed to `±ROTATION_OFFSET_MAX` with `ModulatableSliderAngleDeg` displaying degrees.
