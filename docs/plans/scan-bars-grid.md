# Scan Bars Grid Mode

Add mode 3 (Grid) to the existing Scan Bars generator. A hash-randomized 2D lattice where cell boundaries are pseudo-randomly scattered by a hash function, producing an organic irregular grid that morphs over time with palette-driven color per cell and FFT-reactive brightness.

**Research**: `docs/research/scan_bars_grid.md`

## Design

### Types

No new types. Existing `ScanBarsConfig` and `ScanBarsEffect` unchanged - no new fields, uniforms, or loc members.

Update mode comment in header: `int mode = 0; // 0=Linear, 1=Spokes, 2=Rings, 3=Grid`

### Algorithm

**Hash function** (define after `rotate2D`, before `main()`):

```glsl
// Hash grid - KuKo #343 by kukovisuals, hash by @Fabrice
// https://www.shadertoy.com/view/sclSRN (CC BY-NC-SA 3.0)
#define H(n) fract(1e1 * sin((n).x + (n).y / 0.7 + vec2(1, 12.34)))
```

**Hoist mask declaration** next to the existing `float coord = 0.0;` before STEP 1 (mask is currently declared in STEP 2):

```glsl
float coord = 0.0;
float mask = 0.0;  // Hoisted from STEP 2 for grid mode
```

**STEP 1 grid branch** (add `else if (mode == 3)` before the `else` linear fallback):

```glsl
} else if (mode == 3) {
    // Grid mode: hash-randomized 2D lattice
    vec2 gridUV = rotate2D(angle) * uv;
    gridUV.x += convergence * safeTan(abs(gridUV.x - convergenceOffset) * convergenceFreq);
    gridUV.y += convergence * safeTan(abs(gridUV.y - convergenceOffset) * convergenceFreq);
    vec2 h = H(gridUV * barDensity + scroll);
    float gridDist = min(abs(h.x), abs(h.y)) - sharpness * 0.5;
    mask = 1.0 - smoothstep(0.0, sharpness * 0.25, gridDist);
    coord = h.x + h.y;
```

**STEP 2 guard** - wrap existing body in `if (mode != 3)` (grid mode already set mask in STEP 1):

```glsl
if (mode != 3) {
    float barCoord = barDensity * coord + scroll;
    float d = fract(barCoord);
    mask = smoothstep(0.5 - sharpness, 0.5, d)
         * smoothstep(0.5 + sharpness, 0.5, d);
}
```

STEP 3 (color), STEP 4 (FFT), and STEP 5 (output) are unchanged - they consume `coord` and `mask` as before.

### Parameters

No new parameters. All existing scan_bars params reused per the research doc's parameter mapping table.

### Constants

No new constants. Mode index 3 is implicit from combo position.

---

## Tasks

### Wave 1: All Changes (parallel - no file overlap)

#### Task 1.1: Shader - Grid mode branch

**Files**: `shaders/scan_bars.fs`

**Do**:
1. Update header comment to mention grid layout
2. Add `H()` hash macro with attribution comment after `rotate2D`, before `main()`
3. Hoist `float mask` declaration before STEP 1 (init to `0.0`), remove from STEP 2
4. Add `else if (mode == 3)` branch in STEP 1 before the `else` linear fallback - implement the Algorithm section above verbatim
5. Wrap STEP 2 body in `if (mode != 3)` guard
6. Update mode uniform comment: `// 0=Linear, 1=Spokes, 2=Rings, 3=Grid`

Pattern: follow existing mode branches (spokes, rings) for structure.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 1.2: C++ - Mode combo and UI update

**Files**: `src/effects/scan_bars.h`, `src/effects/scan_bars.cpp`

**Do**:
1. In `scan_bars.h`: update mode field comment to `// 0=Linear, 1=Spokes, 2=Rings, 3=Grid`
2. In `scan_bars.cpp` `DrawScanBarsParams`: update combo string from `"Linear\0Spokes\0Rings\0"` to `"Linear\0Spokes\0Rings\0Grid\0"`
3. In `scan_bars.cpp` `DrawScanBarsParams`: update angle slider condition from `if (sb->mode == 0)` to `if (sb->mode == 0 || sb->mode == 3)` (grid mode uses angle for rotation)

Pattern: existing combo/slider pattern in `DrawScanBarsParams`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Mode combo shows 4 entries: Linear, Spokes, Rings, Grid
- [ ] Grid mode produces visible hash-randomized lattice
- [ ] Angle slider appears in Grid mode
- [ ] barDensity, sharpness, scrollSpeed, convergence affect grid
- [ ] FFT audio reactivity works in grid mode
- [ ] Existing modes (Linear, Spokes, Rings) unchanged
