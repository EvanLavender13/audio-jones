# Fracture Grid

A warp-category transform that shatters the image into a grid of tiles, each showing the scene from a different offset, rotation, and zoom. Subdivision controls grid density, stagger controls per-tile variation intensity, and tessellation mode switches between rectangular, hexagonal, and triangular grids.

**Research**: `docs/research/fracture-grid.md`

## Design

### Types

**FractureGridConfig** (config struct):

```cpp
struct FractureGridConfig {
  bool enabled = false;
  float subdivision = 4.0f;     // 0.0-20.0 — grid density
  float stagger = 0.5f;         // 0.0-1.0 — per-tile variation intensity
  float offsetScale = 0.3f;     // 0.0-1.0 — max UV offset per tile
  float rotationScale = 0.5f;   // 0.0-3.14159 — max rotation per tile (radians)
  float zoomScale = 0.3f;       // 0.0-1.0 — max zoom deviation per tile
  int tessellation = 0;         // 0=rect, 1=hex, 2=triangular
  float seed = 0.0f;            // 0.0-100.0 — hash seed for reshuffling
};
```

**FractureGridEffect** (runtime state):

```cpp
typedef struct FractureGridEffect {
  Shader shader;
  int resolutionLoc;
  int subdivisionLoc;
  int staggerLoc;
  int offsetScaleLoc;
  int rotationScaleLoc;
  int zoomScaleLoc;
  int tessellationLoc;
  int seedLoc;
} FractureGridEffect;
```

No time accumulator needed — the effect is static unless parameters are modulated externally.

### Algorithm

The shader divides UV space into a grid of cells based on the tessellation mode, then remaps each cell's UV coordinates with per-cell offset, rotation, and zoom derived from a hash of the cell ID.

**Coordinate pipeline**: `fragTexCoord` is centered and aspect-corrected into grid space for tessellation. After per-cell remapping, coordinates convert back to UV space for texture sampling.

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float subdivision;
uniform float stagger;
uniform float offsetScale;
uniform float rotationScale;
uniform float zoomScale;
uniform int tessellation;
uniform float seed;

// 3 pseudo-random values from cell ID + seed
vec3 hash3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33 + seed);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

mat2 rot2(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// --- Tessellation functions ---
// All produce: id (cell identifier for hashing), cellUV (local coord in grid-scaled space),
// cellCenter (cell center in grid space for reconstruction)
// Invariant: cellCenter + cellUV / subdivision == p (original grid-space point)

void tessRect(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    id = floor(p * sub);
    cellUV = fract(p * sub) - 0.5;
    cellCenter = (id + 0.5) / sub;
}

// Dual offset grid, pick nearest hex center
void tessHex(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    vec2 s = p * sub;
    vec2 r = vec2(1.0, 1.732);
    vec2 h = r * 0.5;
    vec2 a = mod(s, r) - h;
    vec2 b = mod(s - h, r) - h;

    vec2 gv = dot(a, a) < dot(b, b) ? a : b;
    id = s - gv;
    cellUV = gv;
    cellCenter = id / sub;
}

// Row-staggered grid with up/down triangle detection
void tessTri(vec2 p, float sub, out vec2 id, out vec2 cellUV, out vec2 cellCenter) {
    vec2 s = p * sub;
    float sq3 = 1.732;
    float rowF = s.y / (sq3 * 0.5);
    float row = floor(rowF);
    float oddRow = mod(row, 2.0);
    float col = floor(s.x + oddRow * 0.5);

    vec2 center = vec2(col - oddRow * 0.5 + 0.5, (row + 0.5) * sq3 * 0.5);
    vec2 local = s - center;

    float fy = fract(rowF);
    float fx = fract(s.x + oddRow * 0.5);
    float triId = (fx + fy > 1.0) ? 1.0 : 0.0;

    id = vec2(col * 2.0 + triId, row);
    cellUV = local;
    cellCenter = center / sub;
}

void main() {
    vec2 uv = fragTexCoord;
    float aspect = resolution.x / resolution.y;

    // Pass through at zero subdivision
    if (subdivision < 0.01) {
        finalColor = texture(texture0, uv);
        return;
    }

    // Center and aspect-correct into grid space
    vec2 p = vec2((uv.x - 0.5) * aspect, uv.y - 0.5);

    vec2 id, cellUV, cellCenter;
    if (tessellation == 1) {
        tessHex(p, subdivision, id, cellUV, cellCenter);
    } else if (tessellation == 2) {
        tessTri(p, subdivision, id, cellUV, cellCenter);
    } else {
        tessRect(p, subdivision, id, cellUV, cellCenter);
    }

    // Per-cell variation from hash
    vec3 h = hash3(id);
    vec2 offset = (h.xy - 0.5) * stagger * offsetScale;
    float angle = (h.z - 0.5) * stagger * rotationScale;
    float zoom = 1.0 + (h.x - 0.5) * stagger * zoomScale;

    // Remap: rotate and zoom cell content, offset sampling position
    vec2 sampleP = cellCenter + rot2(angle) * (cellUV / (subdivision * zoom)) + offset;

    // Back to UV space (undo aspect correction)
    vec2 sampleUV = vec2(sampleP.x / aspect + 0.5, sampleP.y + 0.5);

    finalColor = texture(texture0, clamp(sampleUV, 0.0, 1.0));
}
```

Key decisions:
- Aspect correction makes tiles appear square on screen regardless of resolution
- `cellCenter + cellUV / subdivision` reconstructs the original point (identity transform)
- Rotation and zoom act on the cell-local coordinate before adding to cell center
- Uniform-controlled tessellation branching has no divergence (all fragments take same path)
- Clamped sampling prevents out-of-bounds artifacts at tile edges

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| subdivision | float | 0.0-20.0 | 4.0 | Yes | Subdivision |
| stagger | float | 0.0-1.0 | 0.5 | Yes | Stagger |
| offsetScale | float | 0.0-1.0 | 0.3 | Yes | Offset Scale |
| rotationScale | float | 0.0-3.14159 | 0.5 | Yes | Rotation Scale |
| zoomScale | float | 0.0-1.0 | 0.3 | Yes | Zoom Scale |
| tessellation | int | 0-2 | 0 | No | Tessellation |
| seed | float | 0.0-100.0 | 0.0 | Yes | Seed |

### Constants

- Enum: `TRANSFORM_FRACTURE_GRID`
- Display name: `"Fracture Grid"`
- Category badge: `"WARP"`, section index `1`
- Flags: `EFFECT_FLAG_NONE`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create FractureGridConfig header

**Files**: `src/effects/fracture_grid.h` (create)
**Creates**: Config and Effect structs that all other tasks depend on

**Do**: Create the header following the Types section. Same structure as `flux_warp.h`. Declare 5 public functions: Init, Setup, Uninit, ConfigDefault, RegisterParams. Setup signature: `(FractureGridEffect*, const FractureGridConfig*, int screenWidth, int screenHeight)` — no deltaTime (no time accumulation). Define `FRACTURE_GRID_CONFIG_FIELDS` macro listing all serializable fields.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/fracture_grid.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement all lifecycle functions following `flux_warp.cpp` as pattern. Init loads shader, caches all 8 uniform locations. Setup sends all uniforms (resolution as vec2, tessellation as int via `SHADER_UNIFORM_INT`, rest as float). No time accumulator. RegisterParams registers all float params except `enabled` — tessellation is int so skip it. Add `SetupFractureGrid(PostEffect* pe)` bridge that calls `FractureGridEffectSetup(&pe->fractureGrid, &pe->effects.fractureGrid, pe->screenWidth, pe->screenHeight)`. Add `REGISTER_EFFECT` macro at bottom.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/fracture_grid.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the shader from the Algorithm section. Copy the GLSL verbatim.

**Verify**: File exists with correct uniform names matching the .cpp.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/fracture_grid.h"` with other effect includes
2. Add `TRANSFORM_FRACTURE_GRID,` before `TRANSFORM_EFFECT_COUNT` in enum
3. Add `FractureGridConfig fractureGrid;` to `EffectConfig` struct near other warp configs

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/fracture_grid.h"` and `FractureGridEffect fractureGrid;` member to PostEffect struct, near other warp effects (e.g., after `fluxWarp`).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/fracture_grid.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release` configures.

---

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_warp.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/fracture_grid.h"` at top with other effect includes. Add `static bool sectionFractureGrid = false;` with other section bools. Create `DrawWarpFractureGrid()` function following `DrawWarpFluxWarp` pattern:
- Section name: `"Fracture Grid"`
- Enable checkbox with `MoveTransformToEnd(&e->transformOrder, TRANSFORM_FRACTURE_GRID)` on fresh enable
- When enabled:
  - `ModulatableSlider("Subdivision##fracgrid", &e->fractureGrid.subdivision, "fractureGrid.subdivision", "%.1f", modSources)`
  - `ModulatableSlider("Stagger##fracgrid", &e->fractureGrid.stagger, "fractureGrid.stagger", "%.2f", modSources)`
  - `ModulatableSlider("Offset Scale##fracgrid", &e->fractureGrid.offsetScale, "fractureGrid.offsetScale", "%.2f", modSources)`
  - `ModulatableSlider("Rotation Scale##fracgrid", &e->fractureGrid.rotationScale, "fractureGrid.rotationScale", "%.2f", modSources)` — raw float slider, not angle deg (this is a scale, not an angle)
  - `ModulatableSlider("Zoom Scale##fracgrid", &e->fractureGrid.zoomScale, "fractureGrid.zoomScale", "%.2f", modSources)`
  - `ImGui::Combo("Tessellation##fracgrid", &e->fractureGrid.tessellation, tessNames, 3)` with `const char* tessNames[] = {"Rectangular", "Hexagonal", "Triangular"}`
  - `ModulatableSlider("Seed##fracgrid", &e->fractureGrid.seed, "fractureGrid.seed", "%.1f", modSources)`
- Add `DrawWarpFractureGrid` call at end of `DrawWarpCategory()` with `ImGui::Spacing()` before it.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.7: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/fracture_grid.h"`
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FractureGridConfig, FRACTURE_GRID_CONFIG_FIELDS)` with other per-config macros
3. Add `X(fractureGrid) \` to `EFFECT_CONFIG_FIELDS` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Cellular category with "CELL" badge
- [x] Rectangular tessellation splits image into a grid of tiles
- [x] Hexagonal tessellation produces honeycomb layout
- [x] Triangular tessellation produces triangle grid layout
- [x] Subdivision at 0 passes through unchanged
- [x] Stagger controls per-tile variation intensity (0 = no variation)
- [x] Wave Speed animates per-cell distortion over time
- [x] Preset save/load preserves all settings
- [x] Modulation routes to registered parameters

## Implementation Notes

Changes from the original plan made during implementation:

- **Moved to Cellular category**: Badge changed from `"WARP"` to `"CELL"`, section index from 1 to 2. UI moved from `imgui_effects_warp.cpp` to `imgui_effects_cellular.cpp`. Better fit alongside Voronoi, Lattice Fold, and Phyllotaxis.
- **Removed `seed` parameter**: User found it unnecessary. Hash is now static per cell ID.
- **Added `waveSpeed` parameter** (0.0–5.0, default 1.0): Replaces seed. Accumulates `waveTime` on CPU, sent as uniform. Shader uses per-cell sinusoidal animation — each cell gets a unique phase from `hash.x * TAU`, and offset/rotation/zoom animate at different frequency multipliers (1.0, 1.3, 0.7) so they go in and out of sync for organic motion.
- **Increased `zoomScale` range**: Changed from 0.0–1.0 (default 0.3) to 0.0–4.0 (default 1.0). Original range produced imperceptible variation (~7.5% at defaults). Zoom also uses `h.y` instead of `h.x` to decorrelate from offset.
- **Setup signature gained `deltaTime`**: Required for `waveTime` accumulation. Bridge function passes `pe->currentDeltaTime`.
- **Hex ID stabilization**: Hex tessellation `id` (used for hashing) is snapped to the nearest hex center grid via rounding. Raw floating-point `id = s - gv` caused hash discontinuities at `mod()` tile boundaries, visible as cross-shaped line artifacts aligned with screen center. `cellCenter` still uses the raw value to preserve the reconstruction invariant.
- **Mirror-wrap sampling**: `clamp(sampleUV, 0.0, 1.0)` replaced with `1.0 - abs(mod(sampleUV, 2.0) - 1.0)` to avoid flat-colored stripe artifacts when remapped UVs leave the [0,1] range.
