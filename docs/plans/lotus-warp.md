# Lotus Warp

Log-polar coordinate warp creating infinitely spiraling diamond cells. The input texture tiles inward along logarithmic spirals, with each cell showing a warped copy of the underlying image. Shares all 9 CellMode sub-effects with Voronoi and Phyllotaxis. Independent zoom and spin speeds create bidirectional spiral flow.

**Research**: `docs/research/lotus_warp.md`

## Design

### Types

**LotusWarpConfig** (`src/effects/lotus_warp.h`):

```
struct LotusWarpConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 3.0f;         // Cell density (0.5-10.0)
  float zoomSpeed = 0.3f;     // Radial zoom rate, rad/s (-PI_F to PI_F)
  float spinSpeed = 0.0f;     // Angular rotation rate, rad/s (-PI_F to PI_F)
  float edgeFalloff = 0.3f;   // Edge softness (0.01-1.0)
  float isoFrequency = 10.0f; // Ring density for iso effects (1.0-30.0)
  int mode = 0;               // CellMode index (0-8)
  float intensity = 0.5f;     // Sub-effect strength (0.0-1.0)
};
```

**LotusWarpEffect** (`src/effects/lotus_warp.h`):

```
typedef struct LotusWarpEffect {
  Shader shader;
  int resolutionLoc;
  int scaleLoc;
  int zoomPhaseLoc;
  int spinPhaseLoc;
  int edgeFalloffLoc;
  int isoFrequencyLoc;
  int smoothModeLoc;
  int modeLoc;
  int intensityLoc;
  float zoomPhase;  // CPU-accumulated zoom offset
  float spinPhase;  // CPU-accumulated spin offset
} LotusWarpEffect;
```

Fields macro: `enabled, smoothMode, scale, zoomSpeed, spinSpeed, edgeFalloff, isoFrequency, mode, intensity`

### Algorithm

The shader implements a log-polar coordinate transform followed by the shared CellMode sub-effect dispatch. Built by mechanical substitution from the reference (FabriceNeyret2's logpolar mapping - 2).

**Forward transform chain:**

```glsl
const float TWO_OVER_PI = 0.6366;
const float TWO_PI = 6.28318530718;

vec2 pos = fragTexCoord * resolution - resolution * 0.5;

// Clamp to avoid log(0) singularity at center
float r = max(length(pos), 1.0);

// Log-polar transform
vec2 lp = vec2(log(r / resolution.y) - zoomPhase,
               atan(pos.y, pos.x) - spinPhase);

// Conformal 45-degree rotation (diamond cells)
vec2 conformal = TWO_OVER_PI * lp * mat2(1, -1, 1, 1);

// Scale to cell grid
vec2 cellCoord = conformal * scale;
vec2 ip = floor(cellCoord);   // cell ID
vec2 fp = fract(cellCoord);   // position within cell [0,1)
```

**Cell geometry from fract coordinates:**

```glsl
vec2 toCenter = vec2(0.5) - fp;
float centerDist = length(toCenter);

float edgeDistX = min(fp.x, 1.0 - fp.x);
float edgeDistY = min(fp.y, 1.0 - fp.y);
float edgeDist = min(edgeDistX, edgeDistY);

// Direction toward nearest cell edge
vec2 borderVec;
if (edgeDistX < edgeDistY) {
    borderVec = vec2(sign(0.5 - fp.x) * edgeDistX, 0.0);
} else {
    borderVec = vec2(0.0, sign(0.5 - fp.y) * edgeDistY);
}
```

**Cell center UV (inverse transform for cellColor sampling):**

```glsl
vec2 cellCenterLP = (ip + 0.5) / scale;
// Inverse conformal: inverse of mat2(1,-1,1,1) is mat2(1,1,-1,1) * 0.5
cellCenterLP = (cellCenterLP * mat2(1, 1, -1, 1)) * 0.5 / TWO_OVER_PI;
// Add back animation phases
float rho = cellCenterLP.x + zoomPhase;
float theta = cellCenterLP.y + spinPhase;
// Inverse log-polar
float cellR = exp(rho) * resolution.y;
vec2 cartesian = cellR * vec2(cos(theta), sin(theta));
// To UV
vec2 cellCenterUV = (cartesian + resolution * 0.5) / resolution;
vec3 cellColor = texture(texture0, cellCenterUV).rgb;
```

**Smooth mode (grid-specific):**

Unlike Voronoi's inverse-distance neighbor weighting, grid cells use smoothstep to soften boundaries. The grid cell is a unit square after fract(), so edge distances max at 0.5 (cell center to edge). Use that as the geometric reference for smoothstep ranges:

```glsl
if (smoothMode) {
    // Soften edge distances: ramp from hard 0 at edge to 1 at 0.5 (cell half-width)
    edgeDist = smoothstep(0.0, 0.5, edgeDist);
    // Circular falloff for center distance: clamp square grid to circle
    // toCenter length is 0 at center, sqrt(0.5) at corners
    // Use circular distance directly instead of axis-aligned min
    centerDist = length(toCenter);
}
```

This preserves `centerDist` as a raw distance (0 at center, larger away) so downstream sub-effects (mode 3 Center Iso, mode 6 Ratio, etc.) work correctly. The smoothstep on `edgeDist` rounds cell corners by fading the hard 0-to-0.5 ramp.

**UV displacement Jacobian correction (modes 0 and 1):**

The displacement vector in cell space must be scaled by the inverse Jacobian to produce correct screen-space UV offsets. The Jacobian scale varies with radius:

```glsl
float jacobianScale = max(length(pos), 1.0) * TWO_OVER_PI * scale;
vec2 displacement = toCenter * intensity * 0.5;
displacement /= jacobianScale;
// Convert from centered coords back to UV
finalUV = fragTexCoord + displacement / resolution * resolution.y;
```

The exact UV displacement math follows from the chain rule of the log-polar + conformal transform. `length(pos)` is the distance from screen center, which determines pixel scale in log-polar space. Dividing by `jacobianScale` converts cell-space displacement to screen-space pixels, then dividing by resolution converts to UV.

**Sub-effect dispatch:**

All 9 CellMode sub-effects (modes 0-8) use the same structure as the Voronoi shader. The grid-derived `toCenter`, `centerDist`, `edgeDist`, and `borderVec` values substitute for Voronoi's `mr`, `length(mr)`, `length(borderVec)`, and `borderVec`. Copy the mode dispatch from `shaders/voronoi.fs` lines 116-184, replacing:

| Voronoi variable | Lotus Warp replacement |
|------------------|----------------------|
| `smoothMr` | `toCenter` (or smooth variant) |
| `mr` | `toCenter` |
| `length(mr)` / `centerDist` | `centerDist` |
| `length(borderVec)` / `edgeDist` | `edgeDist` |
| `borderVec` | `borderVec` |
| `voronoiData.xy` | `borderVec` |
| `voronoiData.zw` | `toCenter` |
| `cellCenterUV` | `cellCenterUV` (from inverse transform above) |
| `vec2(scale * aspect, scale)` (UV denormalization) | `jacobianScale` (Jacobian-corrected) |

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| scale | float | 0.5-10.0 | 3.0 | Yes | `Scale` |
| zoomSpeed | float | -PI_F to PI_F | 0.3 | Yes | `Zoom Speed` (speed slider) |
| spinSpeed | float | -PI_F to PI_F | 0.0 | Yes | `Spin Speed` (speed slider) |
| edgeFalloff | float | 0.01-1.0 | 0.3 | Yes | `Edge Falloff` |
| isoFrequency | float | 1.0-30.0 | 10.0 | Yes | `Iso Frequency` |
| mode | int | 0-8 | 0 | No | `Mode` (Combo) |
| intensity | float | 0.0-1.0 | 0.5 | Yes | `Intensity` |
| smoothMode | bool | - | false | No | `Smooth` (Checkbox) |

### Constants

- Enum name: `TRANSFORM_LOTUS_WARP`
- Display name: `"Lotus Warp"`
- Category badge: `"CELL"` (section index 2, alongside Voronoi and Phyllotaxis)
- Flags: `EFFECT_FLAG_NONE`
- Field name: `lotusWarp`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create LotusWarpConfig and LotusWarpEffect

**Files**: `src/effects/lotus_warp.h` (create)
**Creates**: Config/effect struct types, lifecycle declarations, fields macro

**Do**: Create header following `phyllotaxis.h` structure. Define `LotusWarpConfig`, `LotusWarpEffect`, and `LOTUS_WARP_CONFIG_FIELDS` macro per Design section. Two CPU accumulators: `zoomPhase`, `spinPhase`. Top-of-file comment: `// Lotus Warp - log-polar conformal coordinate warp with cellular sub-effects`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Shader

**Files**: `shaders/lotus_warp.fs` (create)
**Depends on**: Wave 1 complete (needs to know uniform names from header)

**Do**: Create fragment shader. Begin with attribution block:
```
// Based on "logpolar mapping - 2" by FabriceNeyret2
// https://www.shadertoy.com/view/3sSSDR
// License: CC BY-NC-SA 3.0 Unported
// Modified: cellular sub-effects, zoom/spin phase, smooth mode
```

Implement the Algorithm section: forward transform chain, cell geometry from fract, cell center UV inverse transform, smooth mode, Jacobian-corrected UV displacement, and all 9 sub-effect modes. Copy sub-effect dispatch structure from `shaders/voronoi.fs` lines 116-184, applying the variable substitution table from Design. Uniforms: `resolution`, `scale`, `zoomPhase`, `spinPhase`, `edgeFalloff`, `isoFrequency`, `smoothMode`, `mode`, `intensity`.

Note: the `atan()` has a discontinuity at angle +/-PI (left edge of screen). The 2/PI normalization maps the angular period to 4 units, so `4 * scale` being an integer guarantees seamless tiling. This is inherent to the mapping and does not need code to handle -- it is a constraint on scale values. Add a comment in the shader near the atan call noting this.

**Verify**: File exists with correct uniform names matching header loc fields.

---

#### Task 2.2: Effect module implementation

**Files**: `src/effects/lotus_warp.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create implementation following `phyllotaxis.cpp` structure. Include standard effect includes per conventions.

- `LotusWarpEffectInit`: Load `shaders/lotus_warp.fs`, cache all uniform locations, init `zoomPhase = 0`, `spinPhase = 0`.
- `LotusWarpEffectSetup`: Accumulate `zoomPhase += cfg->zoomSpeed * deltaTime`, `spinPhase += cfg->spinSpeed * deltaTime`. Bind all uniforms. Pass `zoomPhase` and `spinPhase` as the shader uniforms (not speed).
- `LotusWarpEffectUninit`: Unload shader.
- `LotusWarpRegisterParams`: Register all modulatable params per Design table. Use `ROTATION_SPEED_MAX` for zoomSpeed/spinSpeed bounds.
- UI section: `DrawLotusWarpParams` with `##lw` suffix. Scale slider, Smooth checkbox, Zoom Speed (speed slider), Spin Speed (speed slider), Mode combo, Intensity slider. Show Iso Frequency conditionally for modes 2,3. Show Edge Falloff conditionally for modes 1,4,5,8 (matching Voronoi pattern).
- Bridge: `SetupLotusWarp(PostEffect* pe)` (non-static).
- Registration: `REGISTER_EFFECT(TRANSFORM_LOTUS_WARP, LotusWarp, lotusWarp, "Lotus Warp", "CELL", 2, EFFECT_FLAG_NONE, SetupLotusWarp, NULL, DrawLotusWarpParams)`

**Verify**: Compiles standalone (no linker errors for this file).

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lotus_warp.h"` with other effect includes (alphabetical position near `lattice_fold.h`)
2. Add `TRANSFORM_LOTUS_WARP,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` (after `TRANSFORM_PHYLLOTAXIS` to keep cellular effects grouped)
3. Add `LotusWarpConfig lotusWarp;` to `EffectConfig` struct with comment `// Lotus Warp (log-polar conformal cellular transform)`, near the phyllotaxis entry

No manual order array edit needed - `TransformOrderConfig` constructor auto-fills from enum indices.

**Verify**: Compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lotus_warp.h"` with other effect includes
2. Add `LotusWarpEffect lotusWarp;` member to `PostEffect` struct, near `phyllotaxis`

**Verify**: Compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/lotus_warp.cpp` to `EFFECTS_SOURCES` (alphabetical position near `lattice_fold.cpp`).

**Verify**: cmake configure succeeds.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lotus_warp.h"` with other includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LotusWarpConfig, LOTUS_WARP_CONFIG_FIELDS)` macro
3. Add `X(lotusWarp) \` to `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe --build build`
- [ ] No warnings
- [ ] Effect appears in Cellular section (section 2) with "CELL" badge
- [ ] Zoom speed creates inward/outward spiral flow
- [ ] Spin speed creates angular rotation
- [ ] All 9 CellMode sub-effects work (cycle through mode combo)
- [ ] Preset save/load round-trips all parameters

## Implementation Notes

- **Jacobian correction was inverted in the plan**: The plan specified `displacement /= (r * TWO_OVER_PI * scale)` for modes 0/1, but the forward Jacobian scales as 1/r, so the inverse (cell-space to pixels) scales as r. Fixed to `displacement *= r / (TWO_OVER_PI * scale)` then `/ resolution` for UV conversion.
- **smoothMode removed**: Voronoi's smooth mode relies on inverse-distance neighbor blending across cell boundaries, which has no grid equivalent without neighbor sampling. Removed entirely rather than shipping a broken approximation.
- **Mode names must match Voronoi**: The 9 CellMode sub-effects share names with Voronoi (Distort, Organic Flow, Edge Iso, etc.), not independent names.
