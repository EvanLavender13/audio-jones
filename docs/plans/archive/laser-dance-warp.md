# Laser Dance Warp

Add position perturbation to the existing Laser Dance generator's raymarch loop, bending the rigid cosine lattice into organic tangled filaments. Three new parameters (`warpAmount`, `warpSpeed`, `warpFreq`) control the distortion. Default `warpAmount = 0.0` preserves current behavior — purely additive enhancement, no new files.

**Research**: `docs/research/laser_dance_warp.md`

## Design

### Types

Additions to `LaserDanceConfig` (new "Warp" group between Geometry and Audio):

```
float warpAmount = 0.0f;  // Perturbation strength (0.0-1.5)
float warpSpeed = 1.0f;   // Warp evolution speed (0.1-3.0)
float warpFreq = 0.4f;    // Spatial tightness (0.1-2.0)
```

Additions to `LaserDanceEffect`:

```
int warpAmountLoc;
int warpSpeedLoc;
int warpFreqLoc;
```

### Algorithm

Insert one line into the raymarch loop in `laser_dance.fs`, between the sample point calculation and the distance field evaluation:

```glsl
// Existing line 52:
vec3 p = z * rayDir + vec3(cameraOffset);

// NEW — position warp (insert here):
p += cos(time * warpSpeed + p.y + p.x + p.yzx * warpFreq) * warpAmount;

// Existing line 55:
vec3 q = cos(p + time) + cos(p / freqRatio).yzx;
```

Three new uniform declarations (after `baseBright`):

```glsl
uniform float warpAmount;
uniform float warpSpeed;
uniform float warpFreq;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| warpAmount | float | 0.0-1.5 | 0.0 | Yes | `"Amount##laserDanceWarp"` |
| warpSpeed | float | 0.1-3.0 | 1.0 | Yes | `"Speed##laserDanceWarp"` |
| warpFreq | float | 0.1-2.0 | 0.4 | Yes | `"Freq##laserDanceWarp"` |

### UI Layout

Add `ImGui::SeparatorText("Warp")` in `DrawLaserDanceParams()` between the Brightness slider and the Audio separator. Three `ModulatableSlider` calls with `"%.2f"` format.

---

## Tasks

### Wave 1: Header + Shader

#### Task 1.1: Config and Effect Struct

**Files**: `src/effects/laser_dance.h`
**Creates**: New config fields and uniform loc fields that `.cpp` references

**Do**:
1. Add three fields after `brightness` and before the `// Audio` comment, under a new `// Warp` comment:
   - `float warpAmount = 0.0f; // Perturbation strength (0.0-1.5)`
   - `float warpSpeed = 1.0f;  // Warp evolution speed (0.1-3.0)`
   - `float warpFreq = 0.4f;   // Spatial tightness (0.1-2.0)`
2. Add `warpAmount, warpSpeed, warpFreq` to `LASER_DANCE_CONFIG_FIELDS` macro after `brightness`
3. Add three `int` loc fields to `LaserDanceEffect` after `brightnessLoc`:
   - `int warpAmountLoc;`
   - `int warpSpeedLoc;`
   - `int warpFreqLoc;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Shader Warp Line

**Files**: `shaders/laser_dance.fs`

**Do**:
1. Update attribution: add a line after the existing `// Modified:` comment:
   `// Warp technique from "Star Field Flight [351]" by diatribes (https://www.shadertoy.com/view/3ft3DS)`
2. Add three uniform declarations after `uniform float baseBright;`:
   ```glsl
   uniform float warpAmount;
   uniform float warpSpeed;
   uniform float warpFreq;
   ```
3. Insert one line between `vec3 p = z * rayDir + vec3(cameraOffset);` (line 52) and `vec3 q = cos(p + time) + cos(p / freqRatio).yzx;` (line 55). Implement the Algorithm section exactly:
   ```glsl
   p += cos(time * warpSpeed + p.y + p.x + p.yzx * warpFreq) * warpAmount;
   ```

**Verify**: Shader file has correct GLSL syntax.

---

### Wave 2: C++ Integration

#### Task 2.1: Init, Setup, RegisterParams, UI

**Files**: `src/effects/laser_dance.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. **Init**: In `LaserDanceEffectInit()`, add three `GetShaderLocation` calls after the `baseBrightLoc` line:
   - `e->warpAmountLoc = GetShaderLocation(e->shader, "warpAmount");`
   - `e->warpSpeedLoc = GetShaderLocation(e->shader, "warpSpeed");`
   - `e->warpFreqLoc = GetShaderLocation(e->shader, "warpFreq");`
2. **Setup**: In `LaserDanceEffectSetup()`, add three `SetShaderValue` calls after the `baseBright` SetShaderValue (line 83):
   - `SetShaderValue(e->shader, e->warpAmountLoc, &cfg->warpAmount, SHADER_UNIFORM_FLOAT);`
   - `SetShaderValue(e->shader, e->warpSpeedLoc, &cfg->warpSpeed, SHADER_UNIFORM_FLOAT);`
   - `SetShaderValue(e->shader, e->warpFreqLoc, &cfg->warpFreq, SHADER_UNIFORM_FLOAT);`
3. **RegisterParams**: Add three `ModEngineRegisterParam` calls after `baseBright` registration:
   - `"laserDance.warpAmount"`, `&cfg->warpAmount`, 0.0f, 1.5f
   - `"laserDance.warpSpeed"`, `&cfg->warpSpeed`, 0.1f, 3.0f
   - `"laserDance.warpFreq"`, `&cfg->warpFreq`, 0.1f, 2.0f
4. **UI**: In `DrawLaserDanceParams()`, add a Warp section between the Brightness slider (line 139) and the Audio separator (line 141):
   ```
   ImGui::SeparatorText("Warp");
   ModulatableSlider("Amount##laserDanceWarp", &c->warpAmount, "laserDance.warpAmount", "%.2f", modSources);
   ModulatableSlider("Speed##laserDanceWarp", &c->warpSpeed, "laserDance.warpSpeed", "%.2f", modSources);
   ModulatableSlider("Freq##laserDanceWarp", &c->warpFreq, "laserDance.warpFreq", "%.2f", modSources);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Default warpAmount=0.0 produces identical output to current Laser Dance
- [x] Increasing warpAmount visibly distorts the laser beams into organic filaments
- [x] warpSpeed changes the rate of warp animation
- [x] warpFreq changes spatial density of the distortion
- [x] Warp section appears in UI between Geometry and Audio
- [x] Preset save/load preserves warp settings
- [x] Modulation routes to all 3 warp parameters

## Implementation Notes

- **Blowout fix**: Per-step attenuation clamped to 50.0 to prevent white/saturated blowout when beams pass close to the camera. Pre-existing issue amplified by warp.
- **Accumulated warpTime**: `warpSpeed` is accumulated on the CPU (`warpTime += warpSpeed * deltaTime`) and sent as a `warpTime` uniform, avoiding phase jumps when speed is modulated. Uses per-component time multipliers `vec3(1.0, 1.3, 0.7)` in the shader.
- **Non-directional warp**: Original formula `cos(time * warpSpeed + p.y + p.x + p.yzx * warpFreq)` drifted bottom-left due to the `p.y + p.x` linear coupling. Replaced with `cos(p.yzx * warpFreq + warpTime * vec3(1.0, 1.3, 0.7))` which churns in place.
- **Removed time fmod**: The main `time` accumulator's `fmodf(time, 2π)` wrap caused visible snaps when warp was active (chaotic raymarcher amplifies numerical discontinuity at wrap boundary). Removed — float precision is sufficient for hours of runtime.
- **Removed cameraOffset**: Translated camera along (1,1,1) diagonal, but cosine lattice periodicity made the effect imperceptible. Removed from shader, config, UI, and param registration.
- **warpTime fmod period**: Wraps at 20π (≈62.83) — the LCM of the three per-component time multipliers (1.0, 1.3, 0.7), ensuring seamless wrap for all three cosine arguments.
