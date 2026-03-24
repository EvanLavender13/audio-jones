# Dream Fractal Enhancements

Four additive enhancements to Dream Fractal: swappable carve modes (SDF primitive selection), space-folding before carving, orbit trap coloring mapped to gradient LUT, and a Julia offset for structural morphing. All features are additive - the existing look is carve mode 0 (sphere), fold disabled, coloring mode 0 (turbulence), Julia offset (0,0,0).

**Research**: `docs/research/dream_fractal_enhancements.md`

## Design

### Types

**`CarveMode`** (`src/config/carve_mode.h` - new file):

```c
typedef enum {
    CARVE_SPHERE = 0,    // Round tunnels (original)
    CARVE_BOX,           // Angular corridors
    CARVE_CROSS,         // Crosshair voids (Menger-like)
    CARVE_CYLINDER,      // Tubular channels
    CARVE_OCTAHEDRON,    // Diamond cavities
    CARVE_COUNT
} CarveMode;
```

**`DreamFractalConfig`** additions (in `src/effects/dream_fractal.h`):

Rename `sphereRadius` to `carveRadius` (field, CONFIG_FIELDS macro, all references). Add:

```c
// Carve
int carveMode = 0;           // SDF primitive (0-4, CarveMode)

// Fold
bool foldEnabled = false;    // Enable space-folding before carving
int foldMode = 0;            // Fold operation (0-5, FoldMode)

// Orbit trap
int trapMode = 0;            // Trap shape: 0=off, 1=point, 2=plane, 3=shell, 4=cross
float trapRadius = 1.0f;     // Shell trap radius (0.1-3.0)
float trapColorScale = 4.0f; // Log scale for trap-to-LUT mapping (1.0-16.0)
int colorMode = 0;           // 0=turbulence, 1=orbit trap, 2=hybrid

// Julia
float juliaX = 0.0f;         // Julia offset X (-1.0 to 1.0)
float juliaY = 0.0f;         // Julia offset Y (-1.0 to 1.0)
float juliaZ = 0.0f;         // Julia offset Z (-1.0 to 1.0)
```

Updated CONFIG_FIELDS macro:

```c
#define DREAM_FRACTAL_CONFIG_FIELDS                                            \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, orbitSpeed, driftSpeed, \
      marchSteps, fractalIters, carveRadius, scaleFactor, carveMode,           \
      foldEnabled, foldMode, trapMode, trapRadius, trapColorScale, colorMode,  \
      juliaX, juliaY, juliaZ, colorScale, turbulenceIntensity, gradient,       \
      blendMode, blendIntensity
```

**`DreamFractalEffect`** additions (in `src/effects/dream_fractal.h`):

Rename `sphereRadiusLoc` to `carveRadiusLoc`. Add:

```c
int carveModeLoc;
int foldEnabledLoc;
int foldModeLoc;
int trapModeLoc;
int trapRadiusLoc;
int trapColorScaleLoc;
int colorModeLoc;
int juliaOffsetLoc;   // vec3 composed from juliaX/Y/Z
```

New includes needed in `dream_fractal.h`:

```c
#include "config/carve_mode.h"
#include "config/fold_mode.h"
```

### Algorithm

**Shader structure** - the IFS loop gains fold, carve mode selection, orbit trap accumulation, and julia offset. Three helper functions are added above `main()`.

#### Carve SDF helper

```glsl
float carveSDF(vec3 q, int mode) {
    if (mode == 0) return length(q);
    if (mode == 1) return max(max(q.x, q.y), q.z);
    if (mode == 2) return min(max(q.y, q.z), min(max(q.x, q.z), max(q.x, q.y)));
    if (mode == 3) return length(q.xz);
    return (q.x + q.y + q.z) * 0.577;
}
```

Modes: 0=sphere (`length`), 1=box (Chebyshev), 2=cross (Menger cross from research), 3=cylinder (`length(xz)`), 4=octahedron (L1 normalized by 1/sqrt(3)).

#### Fold helper

```glsl
void applyFold(inout vec3 p, int mode) {
    if (mode == 0) {
        // Box fold
        p = clamp(p, -1.0, 1.0) * 2.0 - p;
    } else if (mode == 1) {
        // Mandelbox: box clamp + sphere inversion
        p = clamp(p, -1.0, 1.0) * 2.0 - p;
        float r2 = dot(p, p);
        if (r2 < 0.25) p *= 4.0;
        else if (r2 < 1.0) p /= r2;
    } else if (mode == 2) {
        // Sierpinski: tetrahedral plane reflections
        if (p.x + p.y < 0.0) p.xy = -p.yx;
        if (p.x + p.z < 0.0) p.xz = -p.zx;
        if (p.y + p.z < 0.0) p.zy = -p.yz;
    } else if (mode == 3) {
        // Menger: abs + descending 3-axis sort
        p = abs(p);
        if (p.x < p.y) p.xy = p.yx;
        if (p.x < p.z) p.xz = p.zx;
        if (p.y < p.z) p.yz = p.zy;
    } else if (mode == 4) {
        // Kali: abs + sphere inversion
        p = abs(p);
        float r2 = max(dot(p, p), 1e-6);
        p /= r2;
    } else {
        // Burning Ship: abs reflection
        p = abs(p);
    }
}
```

#### Trap distance helper

```glsl
float trapDist(vec3 q, int mode, float radius) {
    if (mode == 1) return length(q);
    if (mode == 2) return abs(q.y);
    if (mode == 3) return abs(length(q) - radius);
    return min(length(q.xy), min(length(q.yz), length(q.xz)));
}
```

Modes: 1=point (cell center), 2=plane (Y=0), 3=shell (sphere at `trapRadius`), 4=cross (distance to axes). Mode 0 = off (no trap tracking).

#### Modified IFS loop

```glsl
float trap = 1e10;

d = 0.0;
s = 1.0;
float ct = 0.8;
float st = 0.6;
for (int j = 0; j < fractalIters; j++) {
    // Fold (start of iteration, before scaling)
    if (foldEnabled != 0) {
        applyFold(p, foldMode);
    }

    p *= scaleFactor;
    s *= scaleFactor;

    // Carve with mode selection
    vec3 q = abs(mod(p - 1.0, 2.0) - 1.0);
    d = max(d, (carveRadius - carveSDF(q, carveMode)) / s);

    // Orbit trap accumulation (skip when trapMode == 0)
    if (trapMode > 0) {
        trap = min(trap, trapDist(q, trapMode, trapRadius));
    }

    // Twist rotation
    p.xz *= mat2(ct, st, -st, ct);

    // Julia offset (after twist)
    p += juliaOffset;
}
```

#### Coloring modes

Replace the existing turbulence + LUT section with a 3-way branch:

```glsl
const float LUT_FREQ = 0.1;
vec3 color = vec3(0.0);
float tColorAvg = 0.0;

if (colorMode == 0) {
    // Mode 0: Turbulence (original, unchanged)
    for (int pass = 0; pass < TURBULENCE_PASSES; pass++) {
        float n = -0.2;
        for (int k = 0; k < 9; k++) {
            n += 1.0;
            q += turbulenceIntensity * sin(q.zxy * n) / n;
        }
        float comp = pass == 0 ? q.x : (pass == 1 ? q.y : q.z);
        float tc = 0.5 + 0.5 * sin(comp * LUT_FREQ);
        color += texture(gradientLUT, vec2(tc, 0.5)).rgb;
        tColorAvg += tc;
    }
} else if (colorMode == 1) {
    // Mode 1: Orbit trap only
    float tc = clamp(1.0 + log2(max(trap, 1e-6)) / trapColorScale, 0.0, 1.0);
    color = texture(gradientLUT, vec2(tc, 0.5)).rgb * 3.0;
    tColorAvg = tc * 3.0;
} else {
    // Mode 2: Hybrid - turbulence for FFT band selection, trap for color
    for (int pass = 0; pass < TURBULENCE_PASSES; pass++) {
        float n = -0.2;
        for (int k = 0; k < 9; k++) {
            n += 1.0;
            q += turbulenceIntensity * sin(q.zxy * n) / n;
        }
        float comp = pass == 0 ? q.x : (pass == 1 ? q.y : q.z);
        tColorAvg += 0.5 + 0.5 * sin(comp * LUT_FREQ);
    }
    float tc = clamp(1.0 + log2(max(trap, 1e-6)) / trapColorScale, 0.0, 1.0);
    color = texture(gradientLUT, vec2(tc, 0.5)).rgb * 3.0;
}
```

In mode 1 (pure trap): `tColorAvg` is trap-derived, so FFT band selection follows trap geometry. In mode 2 (hybrid): turbulence still drives `tColorAvg` for FFT spatial variation, while trap drives gradient color. The `* 3.0` compensates for the 3-pass accumulation in mode 0.

The rest of the shader (FFT band lookup, brightness computation, final output) remains unchanged - it uses `tColorAvg / 3.0` regardless of coloring mode.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| carveMode | int | 0-4 | 0 | No | Carve Mode |
| foldEnabled | bool | - | false | No | Fold |
| foldMode | int | 0-5 | 0 | No | Fold Mode |
| trapMode | int | 0-4 | 0 | No | Trap Shape |
| trapRadius | float | 0.1-3.0 | 1.0 | Yes | Trap Radius |
| trapColorScale | float | 1.0-16.0 | 4.0 | Yes | Trap Scale |
| colorMode | int | 0-2 | 0 | No | Color Mode |
| juliaX | float | -1.0-1.0 | 0.0 | Yes | Julia X |
| juliaY | float | -1.0-1.0 | 0.0 | Yes | Julia Y |
| juliaZ | float | -1.0-1.0 | 0.0 | Yes | Julia Z |

Existing `sphereRadius` renamed to `carveRadius` (same range 0.3-1.5, same default 0.9, param ID becomes `"dreamFractal.carveRadius"`).

### UI Layout

Geometry section additions (after "Iterations", before existing Carve Radius):
- `DrawCarveCombo("Carve Mode##dreamFractal", &d->carveMode)`
- Rename "Sphere Radius" label to "Carve Radius"

New "Fold" section (after Geometry, before Animation):
- `ImGui::Checkbox("Fold##dreamFractal", &d->foldEnabled)`
- If foldEnabled: `DrawFoldCombo("Fold Mode##dreamFractal", &d->foldMode)`

New "Julia" section (after Animation, before Color):
- `ModulatableSlider("Julia X##dreamFractal", ...)` for juliaX
- `ModulatableSlider("Julia Y##dreamFractal", ...)` for juliaY
- `ModulatableSlider("Julia Z##dreamFractal", ...)` for juliaZ

Color section additions (before existing Color Scale):
- `const char *colorModes[] = {"Turbulence", "Orbit Trap", "Hybrid"};`
- `ImGui::Combo("Color Mode##dreamFractal", &d->colorMode, colorModes, 3)`
- If colorMode > 0:
  - `const char *trapModes[] = {"Off", "Point", "Plane", "Shell", "Cross"};`
  - `ImGui::Combo("Trap Shape##dreamFractal", &d->trapMode, trapModes, 5)`
  - If trapMode == 3: `ModulatableSlider("Trap Radius##dreamFractal", ...)`
  - `ModulatableSlider("Trap Scale##dreamFractal", ...)`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Create CarveMode enum

**Files**: `src/config/carve_mode.h` (create)
**Creates**: CarveMode enum included by dream_fractal.h

**Do**: Create `src/config/carve_mode.h` following the pattern of `src/config/fold_mode.h`. Define `CarveMode` typedef enum with values from the Design Types section. Include guard `CARVE_MODE_H`. Top-of-file comment: `// SDF carving primitives for iterative fractal effects`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: Add DrawCarveCombo widget

**Files**: `src/ui/ui_units.h` (modify)
**Creates**: DrawCarveCombo widget used by dream_fractal.cpp UI

**Do**: Add `DrawCarveCombo` inline function after the existing `DrawFoldCombo` in `src/ui/ui_units.h`. Same pattern as `DrawFoldCombo`:
```cpp
inline bool DrawCarveCombo(const char *label, int *mode) {
  return ImGui::Combo(label, mode,
                      "Sphere\0Box\0Cross\0Cylinder\0Octahedron\0");
}
```

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation

#### Task 2.1: Update DreamFractalConfig and DreamFractalEffect structs

**Files**: `src/effects/dream_fractal.h` (modify)
**Depends on**: Wave 1 complete (carve_mode.h exists)

**Do**:
1. Add includes for `"config/carve_mode.h"` and `"config/fold_mode.h"`
2. Rename `sphereRadius` to `carveRadius` in the config struct (keep same default 0.9, same range comment 0.3-1.5)
3. Add all new config fields from the Design Types section, in the order shown. Place `carveMode` in the "Fractal geometry" group after `carveRadius`. Place fold fields, trap fields, and julia fields as new groups with comments matching the Design section
4. Update the `DREAM_FRACTAL_CONFIG_FIELDS` macro to match the Design section exactly
5. Rename `sphereRadiusLoc` to `carveRadiusLoc` in the Effect struct
6. Add new loc fields from the Design Types section: `carveModeLoc`, `foldEnabledLoc`, `foldModeLoc`, `trapModeLoc`, `trapRadiusLoc`, `trapColorScaleLoc`, `colorModeLoc`, `juliaOffsetLoc`
7. Update the top-of-file comment to mention the new features

**Verify**: `cmake.exe --build build` compiles (will have warnings about unused locs until Task 2.2).

---

#### Task 2.2: Update DreamFractal C++ implementation

**Files**: `src/effects/dream_fractal.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. **Init**: Rename `sphereRadiusLoc` cache to `carveRadiusLoc` with uniform name `"carveRadius"`. Add `GetShaderLocation` calls for all new uniforms: `"carveMode"`, `"foldEnabled"`, `"foldMode"`, `"trapMode"`, `"trapRadius"`, `"trapColorScale"`, `"colorMode"`, `"juliaOffset"`.

2. **Setup**: Rename `sphereRadius` binding to `carveRadius`. Add `SetShaderValue` calls for all new uniforms. For `foldEnabled`, convert bool to int: `int foldEn = cfg->foldEnabled ? 1 : 0;` then bind as `SHADER_UNIFORM_INT`. For `juliaOffset`, compose vec3 from 3 floats: `float juliaOffset[3] = {cfg->juliaX, cfg->juliaY, cfg->juliaZ};` then bind as `SHADER_UNIFORM_VEC3`. All int fields (`carveMode`, `foldMode`, `trapMode`, `colorMode`) bind as `SHADER_UNIFORM_INT`.

3. **RegisterParams**: Rename `"dreamFractal.sphereRadius"` to `"dreamFractal.carveRadius"` (same bounds 0.3-1.5). Add registrations for the 5 modulatable params:
   - `"dreamFractal.trapRadius"` (0.1, 3.0)
   - `"dreamFractal.trapColorScale"` (1.0, 16.0)
   - `"dreamFractal.juliaX"` (-1.0, 1.0)
   - `"dreamFractal.juliaY"` (-1.0, 1.0)
   - `"dreamFractal.juliaZ"` (-1.0, 1.0)

4. **UI** (`DrawDreamFractalParams`): Follow the UI Layout section in the Design. Add `#include "ui/ui_units.h"` if not present (it is not currently included). In the Geometry section, add `DrawCarveCombo` after "Iterations" and rename "Sphere Radius" label to "Carve Radius". Add new Fold section, Julia section, and Color section controls as specified. Use `DrawFoldCombo` from `ui_units.h` for fold mode. Conditional visibility: fold mode combo only when `foldEnabled`, trap controls only when `colorMode > 0`, trap radius only when `trapMode == 3`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Update Dream Fractal shader

**Files**: `shaders/dream_fractal.fs` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. **Attribution**: Add new attribution lines after the existing header comment block, before `#version 330`:
   ```
   // Orbit trap coloring adapted from "Mandelbulb Cathedral" by erezrob
   // https://www.shadertoy.com/view/33tfDN
   // Fold operations from Syntopia (http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/)
   // and cglearn.eu (https://cglearn.eu/pub/advanced-computer-graphics/fractal-rendering)
   ```

2. **New uniforms**: Add after existing uniforms:
   ```glsl
   uniform int carveMode;
   uniform int foldEnabled;
   uniform int foldMode;
   uniform int trapMode;
   uniform float trapRadius;
   uniform float trapColorScale;
   uniform int colorMode;
   uniform vec3 juliaOffset;
   ```
   Rename `uniform float sphereRadius` to `uniform float carveRadius`.

3. **Helper functions**: Add the three helper functions from the Algorithm section (`carveSDF`, `applyFold`, `trapDist`) above `main()`.

4. **IFS loop**: Replace the existing IFS loop body with the modified version from the Algorithm section. Key changes:
   - Add `float trap = 1e10;` before the loop
   - Add `applyFold` call at start of loop body (guarded by `foldEnabled != 0`)
   - Replace `(sphereRadius - length(...))` with `(carveRadius - carveSDF(q, carveMode))`
   - Split the carve line: compute `vec3 q = abs(mod(p - 1.0, 2.0) - 1.0);` separately, then `d = max(d, (carveRadius - carveSDF(q, carveMode)) / s);`
   - Add orbit trap accumulation after carve (guarded by `trapMode > 0`)
   - Add `p += juliaOffset;` after twist rotation

5. **Coloring**: Replace the turbulence coloring block with the 3-way `colorMode` branch from the Algorithm section. The existing turbulence code becomes the `colorMode == 0` branch (unchanged). Add `colorMode == 1` (pure orbit trap) and `colorMode == 2` (hybrid) branches. Everything after the coloring block (FFT band lookup, brightness, final output) stays unchanged.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Dream Fractal with all defaults looks identical to before (carve sphere, no fold, turbulence coloring, zero julia)
- [ ] Each carve mode produces visually distinct geometry
- [ ] Fold toggle creates visible symmetry changes
- [ ] Julia X/Y/Z sliders deform the fractal structure
- [ ] All new modulatable params respond to LFO routing
- [ ] Preset save/load round-trips all new fields

## Implementation Notes

- **Orbit trap coloring removed**: All three coloring modes (turbulence, orbit trap, hybrid) and associated params (`trapMode`, `trapRadius`, `trapColorScale`, `colorMode`) were cut during implementation. The trap-to-LUT mapping produced poor results.
- **Fold mode is local, not shared**: The shared `FoldMode` enum (`src/config/fold_mode.h`) was deleted. Mandelbox and Kali folds use sphere inversion (`p /= r2`) which is incompatible with Dream Fractal's grid-based IFS - the inversion blows `p` out of the range that `mod(p - 1.0, 2.0)` expects, producing broken distance estimates. The 4 remaining folds (Box, Sierpinski, Menger, Burning Ship) are all bounded/isometric and work correctly. Dream Fractal uses a local combo with its own 0-3 mapping.
- **`sphereRadius` renamed to `carveRadius`**: Field, uniform, param ID, and UI label all renamed since the radius now controls non-sphere carve primitives too.
