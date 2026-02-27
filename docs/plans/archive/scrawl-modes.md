# Scrawl Fractal Fold Modes

Add a `mode` selector to the existing Scrawl generator that swaps the IFS fold formula in the shader's iteration loop. Seven modes (0–6) produce fundamentally different curve topologies while sharing the same rendering framework (min-distance tracking, sinusoidal warp, smoothstep stroke, gradient LUT coloring, per-cell rotation).

**Research**: `docs/research/scrawl-modes.md`

## Design

### Types

**ScrawlConfig** — add one field before `iterations`:

```
int mode = 0;  // Fold formula selector (0-6)
```

Add `mode` to `SCRAWL_CONFIG_FIELDS` (insert before `iterations`).

**ScrawlEffect** — add one cached location:

```
int modeLoc;
```

### Algorithm

The shader's `fractal()` function currently has a single fold line at line 56:

```glsl
uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
```

Replace with a switch on `uniform int mode`:

```glsl
switch (mode) {
case 0: // IFS Fold (default)
    uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
    break;
case 1: // Kali Set
    uv = abs(uv) / dot(uv, uv) - foldOffset;
    break;
case 2: // Burning Ship
    uv = vec2(uv.x * uv.x - uv.y * uv.y, 2.0 * abs(uv.x * uv.y)) - foldOffset;
    break;
case 3: // Menger Fold
    uv = abs(uv) - foldOffset; if (uv.x < uv.y) uv = uv.yx;
    break;
case 4: // Box Fold
    uv = clamp(uv, -foldOffset, foldOffset) * 2.0 - uv;
    uv /= clamp(dot(uv, uv), 0.25, 1.0);
    break;
case 5: // Spiral IFS
    uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
    float ca = 0.714, sa = 0.700;
    uv *= mat2(ca, -sa, sa, ca);
    break;
case 6: // Power Fold
    uv = pow(abs(uv), vec2(1.5)) * sign(uv) - foldOffset;
    break;
}
```

Everything else in the fractal function (rotation, zoom, scroll, tiling, distance warp, min tracking, stroke, LUT coloring, scanline) stays identical.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode | int | 0–6 | 0 | No | "Mode" |

### Constants

Mode label array for the combo dropdown:

```
"IFS Fold\0Kali Set\0Burning Ship\0Menger Fold\0Box Fold\0Spiral IFS\0Power Fold\0"
```

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add mode field to ScrawlConfig

**Files**: `src/effects/scrawl.h`
**Creates**: `mode` field and `modeLoc` that Wave 2 tasks depend on

**Do**:
- Add `int mode = 0; // Fold formula selector (0-6)` to `ScrawlConfig` between `enabled` and `iterations`
- Add `mode` to `SCRAWL_CONFIG_FIELDS` macro (insert before `iterations`)
- Add `int modeLoc;` to `ScrawlEffect` struct after `gradientLUTLoc`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader + C++ (parallel)

#### Task 2.1: Add mode switch to shader

**Files**: `shaders/scrawl.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add `uniform int mode;` after the existing uniform declarations
- Replace the single fold line (line 56: `uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;`) with the switch block from the Algorithm section above
- Note: Mode 5 declares local variables (`ca`, `sa`) inside the case — wrap the case body in braces to scope them

**Verify**: Shader compiles (build succeeds).

---

#### Task 2.2: Wire up mode uniform and UI combo

**Files**: `src/effects/scrawl.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `ScrawlEffectInit()`: cache `e->modeLoc = GetShaderLocation(e->shader, "mode");` after the existing `GetShaderLocation` calls
- In `ScrawlEffectSetup()`: bind `SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);` after the `iterations` bind
- In `DrawScrawlParams()`: add an `ImGui::Combo` at the top of the Geometry section (before the `Iterations` slider):
  ```cpp
  ImGui::Combo("Mode##scrawl", &cfg->mode,
               "IFS Fold\0Kali Set\0Burning Ship\0Menger Fold\0Box Fold\0Spiral IFS\0Power Fold\0");
  ```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Mode 0 produces identical output to current Scrawl (no regression)
- [ ] All 7 modes produce distinct visuals when switching the combo
- [ ] Mode persists through preset save/load
