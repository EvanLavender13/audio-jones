# Muons Distance Function Modes

Add a `mode` dropdown to Muons that selects which distance function the raymarching shader evaluates. One new config field, one new uniform, one switch in the shader, one UI combo. The FBM warping, trail persistence, FFT mapping, and color system are mode-independent — only the single line computing `d` changes.

**Research**: `docs/research/muons-modes.md`

## Design

### Types

Add to `MuonsConfig` (before the Raymarching section):

```
int mode = 0;  // Distance function mode (0-6)
```

Add to `MuonsEffect`:

```
int modeLoc;
```

### Algorithm

In the shader, replace the two lines:

```glsl
s = length(a);
d = ringThickness * abs(sin(s));
```

with a switch on `uniform int mode`:

```glsl
s = length(a);
switch (mode) {
case 1:  // L1 Norm
    d = ringThickness * (abs(a.x - a.y) + abs(a.y - a.z) + abs(a.z - a.x));
    break;
case 2:  // Axis Distance
    d = ringThickness * min(length(a.xy), min(length(a.yz), length(a.xz)));
    break;
case 3:  // Dot Product
    d = ringThickness * abs(a.x*a.y + a.y*a.z);
    break;
case 4:  // Chebyshev Spread
    {
        vec3 aa = abs(a);
        d = ringThickness * abs(max(aa.x, max(aa.y, aa.z)) - min(aa.x, min(aa.y, aa.z)));
    }
    break;
case 5:  // Cone Metric
    d = ringThickness * abs(length(a.xy) - abs(a.z));
    break;
case 6:  // Triple Product
    d = ringThickness * abs(a.x * a.y * a.z);
    break;
default: // 0: Sine Shells (original)
    d = ringThickness * abs(sin(s));
    break;
}
```

Note: `s = length(a)` stays unconditional — it's used later in the color accumulation denominator (`d * s`), not just by mode 0.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| mode      | int  | 0-6   | 0       | No          | "Mode" (Combo dropdown) |

Mode labels in order: `Sine Shells`, `L1 Norm`, `Axis Distance`, `Dot Product`, `Chebyshev Spread`, `Cone Metric`, `Triple Product`

### Serialization

Add `mode` to `MUONS_CONFIG_FIELDS` macro.

---

## Tasks

### Wave 1: Config, Shader, C++, UI

All changes touch different sections of the same files, so this is a single wave with one task.

#### Task 1.1: Add mode field, uniform, shader switch, and UI combo

**Files**: `src/effects/muons.h`, `src/effects/muons.cpp`, `shaders/muons.fs`

**Do**:

1. **`src/effects/muons.h`**:
   - Add `int mode = 0;` field to `MuonsConfig`, before `marchSteps` (first in the Raymarching section)
   - Add `mode` to the front of `MUONS_CONFIG_FIELDS` macro
   - Add `int modeLoc;` to `MuonsEffect` struct, after `readIdx`

2. **`shaders/muons.fs`**:
   - Add `uniform int mode;` after the existing uniform block
   - Replace lines 61-62 (`s = length(a); d = ringThickness * abs(sin(s));`) with the switch from the Algorithm section above. Keep `s = length(a)` before the switch.

3. **`src/effects/muons.cpp`**:
   - In `MuonsEffectInit`: add `e->modeLoc = GetShaderLocation(e->shader, "mode");` after the existing location caching
   - In `MuonsEffectSetup`: add `SetShaderValue(e->shader, e->modeLoc, &cfg->mode, SHADER_UNIFORM_INT);` after the existing uniform bindings (alongside the other int uniforms like marchSteps)
   - In `DrawMuonsParams`: add an `ImGui::Combo` dropdown as the first widget in the Raymarching section (after `ImGui::SeparatorText("Raymarching")`, before the March Steps slider). Use:
     ```cpp
     const char *modeLabels[] = {"Sine Shells",     "L1 Norm",
                                 "Axis Distance",   "Dot Product",
                                 "Chebyshev Spread", "Cone Metric",
                                 "Triple Product"};
     ImGui::Combo("Mode##muons", &m->mode, modeLabels, IM_ARRAYSIZE(modeLabels));
     ```

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Mode 0 looks identical to current Muons (no regression)
- [ ] All 7 modes produce visually distinct results
- [ ] Mode persists through preset save/load (serialization via `MUONS_CONFIG_FIELDS`)
- [ ] Dropdown appears in UI before March Steps slider
