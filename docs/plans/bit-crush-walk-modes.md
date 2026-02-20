# Bit Crush Walk Modes

Add a `walkMode` parameter (0-5) to both the Bit Crush generator and Lattice Crush transform, exposing six lattice walk variants that produce different cell pattern vocabularies while preserving the hard-pixel aesthetic.

**Research**: `docs/research/bit-crush-walk-modes.md`

## Design

### Types

Add `int walkMode = 0;` to both `BitCrushConfig` and `LatticeCrushConfig`. Range 0-5, not modulatable (int combo dropdown).

Mode labels for UI combo:

```
"Fixed Dir", "Rotating Dir", "Offset Neighbor", "Alternating Snap", "Cross-Coupled", "Asymmetric Hash"
```

### Algorithm

Both shaders share the same `r()` hash and lattice walk loop. The `walkMode` uniform selects one of six step rules inside the loop body. The hash function `r()` is unchanged across modes 0-4; mode 5 modifies it.

**Mode 0 — Original** (current behavior):
```glsl
vec2 ceilCell = ceil(p / cellSize);
vec2 floorCell = floor(p / cellSize);
p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
```

**Mode 1 — Rotating Direction Vector**:
Replace the fixed `vec2(-1.0, 1.0)` with a per-iteration rotation:
```glsl
vec2 ceilCell = ceil(p / cellSize);
vec2 floorCell = floor(p / cellSize);
vec2 dir = vec2(-cos(float(i)), sin(float(i)));
p = ceil(p + r(ceilCell, time) + r(floorCell, time) * dir);
```

**Mode 2 — Offset Neighborhood Query**:
Query non-adjacent cells instead of `ceil`/`floor` of the same position:
```glsl
vec2 cellA = floor(p / cellSize) + vec2(-1.0, 0.0);
vec2 cellB = floor(p / cellSize) + vec2(0.0, 1.0);
p = ceil(p + r(cellA, time) + r(cellB, time) * vec2(-1.0, 1.0));
```

**Mode 3 — Alternating Snap Function**:
Cycle between `ceil`, `floor`, and `round` per iteration:
```glsl
vec2 ceilCell = ceil(p / cellSize);
vec2 floorCell = floor(p / cellSize);
vec2 stepped = p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0);
int mode = i % 3;
if (mode == 0) p = ceil(stepped);
else if (mode == 1) p = floor(stepped);
else p = round(stepped);
```

**Mode 4 — Cross-Coupled Axes**:
X movement depends on Y position and vice versa:
```glsl
vec2 ceilCell = ceil(p / cellSize);
float rx = r(ceilCell, time);
float ry = r(ceilCell.yx, time);
p = ceil(vec2(p.x + rx + ry * 0.5, p.y + ry - rx * 0.5));
```

**Mode 5 — Asymmetric Hash**:
Same walk rule as Mode 0, but replace `p.x * p.y` with `dot(p, vec2(0.7, 1.3))` in the hash function to break the symmetry where `(3,5)` and `(5,3)` produce the same hash. Implement as a second hash function `rAsym()`:
```glsl
float rAsym(vec2 p, float t) {
    return cos(t * cos(dot(p, vec2(0.7, 1.3))));
}
```
Then mode 5 uses `rAsym` in place of `r`:
```glsl
vec2 ceilCell = ceil(p / cellSize);
vec2 floorCell = floor(p / cellSize);
p = ceil(p + rAsym(ceilCell, time) + rAsym(floorCell, time) * vec2(-1.0, 1.0));
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| walkMode  | int  | 0-5   | 0       | No          | "Walk Mode" |

### Constants

Mode name array (file-local in each UI file):
```cpp
static const char *WALK_MODE_NAMES[] = {"Fixed Dir", "Rotating Dir",
                                        "Offset Neighbor", "Alternating Snap",
                                        "Cross-Coupled", "Asymmetric Hash"};
static const int WALK_MODE_COUNT = 6;
```

---

## Tasks

### Wave 1: Config headers

#### Task 1.1: Add walkMode to BitCrushConfig

**Files**: `src/effects/bit_crush.h`
**Creates**: `walkMode` field that Wave 2 tasks consume

**Do**:
- Add `int walkMode = 0; // Walk variant (0-5)` to `BitCrushConfig` after the `speed` field
- Add `walkMode` to `BIT_CRUSH_CONFIG_FIELDS` macro
- Add `int walkModeLoc;` to `BitCrushEffect` struct

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Add walkMode to LatticeCrushConfig

**Files**: `src/effects/lattice_crush.h`
**Creates**: `walkMode` field that Wave 2 tasks consume

**Do**:
- Add `int walkMode = 0; // Walk variant (0-5)` to `LatticeCrushConfig` after the `speed` field
- Add `walkMode` to `LATTICE_CRUSH_CONFIG_FIELDS` macro
- Add `int walkModeLoc;` to `LatticeCrushEffect` struct

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader + C++ + UI (parallel, no file overlap)

#### Task 2.1: Update bit_crush.fs shader

**Files**: `shaders/bit_crush.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add `uniform int walkMode;`
- Replace the current walk loop body with a `walkMode` switch implementing all 6 modes from the Algorithm section above
- Add the `rAsym()` hash function for mode 5

**Verify**: No build needed (runtime-compiled shader).

#### Task 2.2: Update lattice_crush.fs shader

**Files**: `shaders/lattice_crush.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add `uniform int walkMode;`
- Replace the current walk loop body with a `walkMode` switch implementing all 6 modes from the Algorithm section above
- Add the `rAsym()` hash function for mode 5

**Verify**: No build needed (runtime-compiled shader).

#### Task 2.3: Update bit_crush.cpp — uniform binding

**Files**: `src/effects/bit_crush.cpp`
**Depends on**: Task 1.1

**Do**:
- In `BitCrushEffectInit`: add `e->walkModeLoc = GetShaderLocation(e->shader, "walkMode");`
- In `BitCrushEffectSetup`: add `SetShaderValue(e->shader, e->walkModeLoc, &cfg->walkMode, SHADER_UNIFORM_INT);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Update lattice_crush.cpp — uniform binding

**Files**: `src/effects/lattice_crush.cpp`
**Depends on**: Task 1.2

**Do**:
- In `LatticeCrushEffectInit`: add `e->walkModeLoc = GetShaderLocation(e->shader, "walkMode");`
- In `LatticeCrushEffectSetup`: add `SetShaderValue(e->shader, e->walkModeLoc, &cfg->walkMode, SHADER_UNIFORM_INT);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Update bit crush UI — combo dropdown

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Task 1.1

**Do**:
- Add file-local `WALK_MODE_NAMES` array and `WALK_MODE_COUNT` (see Constants section)
- In `DrawGeneratorsBitCrush`, add a combo dropdown in the Lattice section (after Iterations slider):
  ```
  ImGui::Combo("Walk Mode##bitcrush", &cfg->walkMode, WALK_MODE_NAMES, WALK_MODE_COUNT);
  ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: Update lattice crush UI — combo dropdown

**Files**: `src/ui/imgui_effects_retro.cpp`
**Depends on**: Task 1.2

**Do**:
- Add file-local `WALK_MODE_NAMES` array and `WALK_MODE_COUNT` (see Constants section)
- In `DrawRetroLatticeCrush`, add a combo dropdown after the Iterations slider:
  ```
  ImGui::Combo("Walk Mode##latticecrush", &lc->walkMode, WALK_MODE_NAMES, WALK_MODE_COUNT);
  ```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Bit Crush generator renders correctly in all 6 walk modes
- [ ] Lattice Crush transform renders correctly in all 6 walk modes
- [ ] Walk mode persists across preset save/load (via CONFIG_FIELDS macro)
- [ ] Combo dropdown shows all 6 mode names
