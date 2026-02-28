# Moiré Rework

Bring the Moiré Interference transform up to feature parity with the Moiré Generator: per-layer controls, grating shape modes (stripes/circles/grid), and a shared 4-profile wave function (sine/square/triangle/sawtooth). Also upgrade the Generator's bool `sharpMode` toggle to the same 4-profile `profileMode` int. The interference shader is replaced entirely — the old multi-sample rotation/scale blending approach becomes a wave-displacement approach matching the generator's vocabulary.

**Research**: `docs/research/moire-rework.md`

## Design

### Types

**MoireInterferenceLayerConfig** (new struct, in `moire_interference.h`):

```c
struct MoireInterferenceLayerConfig {
  float frequency = 10.0f;      // Wave spatial frequency (1.0-30.0)
  float angle = 0.0f;           // Wave direction in radians
  float rotationSpeed = 0.3f;   // Drift rate in radians/second
  float amplitude = 0.04f;      // Displacement strength (0.0-0.15)
  float phase = 0.0f;           // Phase offset in radians
};
```

Fields macro: `MOIRE_INTERFERENCE_LAYER_CONFIG_FIELDS` = `frequency, angle, rotationSpeed, amplitude, phase`

**MoireInterferenceConfig** (reworked):

```c
struct MoireInterferenceConfig {
  bool enabled = false;
  int patternMode = 0;          // 0=stripes, 1=circles, 2=grid
  int profileMode = 0;          // 0=sine, 1=square, 2=triangle, 3=sawtooth
  int layerCount = 2;           // Active layers (2-4)
  float centerX = 0.5f;
  float centerY = 0.5f;

  MoireInterferenceLayerConfig layer0;
  MoireInterferenceLayerConfig layer1 = {.frequency = 10.0f, .angle = 1.047f, .rotationSpeed = 0.3f, .amplitude = 0.04f, .phase = 0.0f};
  MoireInterferenceLayerConfig layer2 = {.frequency = 10.0f, .angle = 2.094f, .rotationSpeed = 0.3f, .amplitude = 0.04f, .phase = 0.0f};
  MoireInterferenceLayerConfig layer3 = {.frequency = 10.0f, .angle = 3.14159f, .rotationSpeed = 0.3f, .amplitude = 0.04f, .phase = 0.0f};
};
```

Removed fields: `rotationAngle`, `scaleDiff`, `blendMode`, `animationSpeed`
Fields macro: `MOIRE_INTERFERENCE_CONFIG_FIELDS` = `enabled, patternMode, profileMode, layerCount, centerX, centerY, layer0, layer1, layer2, layer3`

Default layer angles spread: 0, PI/3 (1.047), 2PI/3 (2.094), PI (3.14159).

**MoireInterferenceEffect** (reworked):

```c
typedef struct MoireInterferenceEffect {
  Shader shader;

  // Global uniform locations
  int patternModeLoc;
  int profileModeLoc;
  int layerCountLoc;
  int centerXLoc;
  int centerYLoc;
  int resolutionLoc;

  // Per-layer uniform locations (4 each)
  int frequencyLoc[4];
  int angleLoc[4];    // receives angle + accumulated drift
  int amplitudeLoc[4];
  int phaseLoc[4];

  // Runtime state
  float layerAngles[4]; // Per-layer rotation accumulators
} MoireInterferenceEffect;
```

**MoireGeneratorConfig** (profile mode change):

Replace `bool sharpMode = false;` with `int profileMode = 0;`

Fields macro: replace `sharpMode` with `profileMode` in `MOIRE_GENERATOR_CONFIG_FIELDS`.

**MoireGeneratorEffect** (location rename):

Replace `int sharpModeLoc;` with `int profileModeLoc;`

### Algorithm

#### Shared profile() function (used in both shaders)

```glsl
#define TAU 6.28318530718
#define PI  3.14159265359

// 0=sine, 1=square, 2=triangle, 3=sawtooth
float profile(float t, int mode) {
    if (mode == 1) return sign(sin(t));                       // square
    if (mode == 2) return asin(sin(t)) * (2.0 / PI);         // triangle
    if (mode == 3) return 2.0 * fract(t / TAU + 0.5) - 1.0;  // sawtooth
    return sin(t);                                             // sine (default)
}
```

#### Moiré Interference shader (replacement)

The shader computes per-layer wave displacement and sums them:

```glsl
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

// Global
uniform int patternMode;    // 0=stripes, 1=circles, 2=grid
uniform int profileMode;    // 0=sine, 1=square, 2=triangle, 3=sawtooth
uniform int layerCount;
uniform float centerX;
uniform float centerY;
uniform vec2 resolution;

// Per-layer: "layer0.frequency", "layer0.angle", etc.
struct WaveLayer {
    float frequency;
    float angle;      // static angle + accumulated drift (combined on CPU)
    float amplitude;
    float phase;
};

uniform WaveLayer layer0;
uniform WaveLayer layer1;
uniform WaveLayer layer2;
uniform WaveLayer layer3;

out vec4 finalColor;

#define TAU 6.28318530718
#define PI  3.14159265359

float profile(float t, int mode) {
    if (mode == 1) return sign(sin(t));
    if (mode == 2) return asin(sin(t)) * (2.0 / PI);
    if (mode == 3) return 2.0 * fract(t / TAU + 0.5) - 1.0;
    return sin(t);
}

// Compute displacement for one layer
vec2 layerDisplacement(WaveLayer l, vec2 centered, float aspect) {
    vec2 pos = centered;
    pos.x *= aspect;

    if (patternMode == 1) {
        // Circles: radial displacement
        float dist = length(pos);
        float wave = profile(dist * l.frequency * TAU + l.phase, profileMode);
        vec2 radDir = (dist > 0.001) ? normalize(pos) : vec2(0.0);
        radDir.x /= aspect;
        return radDir * wave * l.amplitude;
    }

    if (patternMode == 2) {
        // Grid: sum of two perpendicular planar waves
        vec2 dir = vec2(cos(l.angle), sin(l.angle));
        vec2 perp = vec2(-dir.y, dir.x);

        float waveA = profile(dot(pos, dir) * l.frequency * TAU + l.phase, profileMode);
        float waveB = profile(dot(pos, perp) * l.frequency * TAU + l.phase, profileMode);

        vec2 perpA = vec2(-dir.y, dir.x);
        perpA.x /= aspect;
        vec2 perpB = vec2(dir.x, dir.y);  // direction itself as perpendicular for second wave
        perpB.x /= aspect;

        return (perpA * waveA + perpB * waveB) * l.amplitude * 0.5;
    }

    // Stripes (default): planar wave along direction, displace perpendicular
    vec2 dir = vec2(cos(l.angle), sin(l.angle));
    float wave = profile(dot(pos, dir) * l.frequency * TAU + l.phase, profileMode);
    vec2 perp = vec2(-dir.y, dir.x);
    perp.x /= aspect;
    return perp * wave * l.amplitude;
}

void main() {
    vec2 center = vec2(centerX, centerY);
    vec2 centered = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;

    vec2 displacement = vec2(0.0);
    int count = clamp(layerCount, 1, 4);

    displacement += layerDisplacement(layer0, centered, aspect);
    if (count >= 2) displacement += layerDisplacement(layer1, centered, aspect);
    if (count >= 3) displacement += layerDisplacement(layer2, centered, aspect);
    if (count >= 4) displacement += layerDisplacement(layer3, centered, aspect);

    vec2 uv = fragTexCoord + displacement;
    uv = 1.0 - abs(mod(uv, 2.0) - 1.0);  // mirror repeat

    finalColor = texture(texture0, uv);
}
```

#### Moiré Generator shader (profile mode upgrade)

Replace `gratingSmooth`/`gratingSharp`/`grating()` routing with `profile()`. The `grating()` function becomes:

```glsl
float grating(float coord, float freq, float phase) {
    return 0.5 + 0.5 * profile(2.0 * PI * coord * freq + phase, profileMode);
}
```

Remove `gratingSmooth()` and `gratingSharp()`. The `uniform int sharpMode` becomes `uniform int profileMode`.

In `main()`: remove the `if (sharpMode != 0)` branching for additive vs multiplicative combination. Use a single combination strategy. Since `profile()` returns [-1,1] values for square/triangle/sawtooth, and `grating()` remaps to [0,1], the existing multiplicative+pow path works for all profiles. Use:

```glsl
void main() {
    int count = clamp(layerCount, 1, 4);

    float result = computeLayer(layer0);
    if (count >= 2) result *= computeLayer(layer1);
    if (count >= 3) result *= computeLayer(layer2);
    if (count >= 4) result *= computeLayer(layer3);
    result = pow(result, 1.0 / float(count));

    vec3 gray = vec3(result);
    vec3 lutColor = textureLod(gradientLUT, vec2(result, 0.5), 0.0).rgb;
    vec3 color = mix(gray, lutColor, colorIntensity);

    finalColor = vec4(color * globalBrightness, 1.0);
}
```

### Parameters

#### Moiré Interference (after rework)

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| patternMode | int | 0-2 | 0 | No | `"Pattern##moireint"` (Combo: Stripes/Circles/Grid) |
| profileMode | int | 0-3 | 0 | No | `"Profile##moireint"` (Combo: Sine/Square/Triangle/Sawtooth) |
| layerCount | int | 2-4 | 2 | No | `"Layers##moireint"` (SliderInt) |
| centerX | float | 0-1 | 0.5 | No | `"X##moireintcenter"` |
| centerY | float | 0-1 | 0.5 | No | `"Y##moireintcenter"` |
| layer[i].frequency | float | 1-30 | 10.0 | Yes | `"Frequency##moireint_l%d"` |
| layer[i].angle | float | -PI..PI | varies | Yes | `"Angle##moireint_l%d"` (AngleDeg) |
| layer[i].rotationSpeed | float | -PI..PI | 0.3 | Yes | `"Rotation Speed##moireint_l%d"` (SpeedDeg) |
| layer[i].amplitude | float | 0-0.15 | 0.04 | Yes | `"Amplitude##moireint_l%d"` |
| layer[i].phase | float | -PI..PI | 0.0 | Yes | `"Phase##moireint_l%d"` (AngleDeg) |

#### Moiré Generator (profile change)

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| profileMode | int | 0-3 | 0 | No | `"Profile##moiregen"` (Combo: Sine/Square/Triangle/Sawtooth) |

### Constants

- Interference enum: `TRANSFORM_MOIRE_INTERFERENCE` (unchanged)
- Interference display name: `"Moire Interference"` (unchanged)
- Interference category: **`"WARP"`, section 1** (changed from `"SYM"` section 0)
- Generator enum: `TRANSFORM_MOIRE_GENERATOR_BLEND` (unchanged)

---

## Tasks

### Wave 1: Headers

#### Task 1.1: Rework Moiré Interference header

**Files**: `src/effects/moire_interference.h`
**Creates**: `MoireInterferenceLayerConfig` struct, reworked `MoireInterferenceConfig`, reworked `MoireInterferenceEffect`

**Do**:
- Add `MoireInterferenceLayerConfig` struct with fields from Design section above (frequency, angle, rotationSpeed, amplitude, phase). Add `MOIRE_INTERFERENCE_LAYER_CONFIG_FIELDS` macro.
- Replace `MoireInterferenceConfig`: remove `rotationAngle`, `scaleDiff`, `blendMode`, `animationSpeed`. Add `patternMode`, `profileMode`, `layerCount`, keep `enabled`, `centerX`, `centerY`. Add `layer0` through `layer3` with defaults from Design section. Update `MOIRE_INTERFERENCE_CONFIG_FIELDS` macro.
- Replace `MoireInterferenceEffect`: remove old uniform locations (`rotationAngleLoc`, `scaleDiffLoc`, `blendModeLoc`, `rotationAccumLoc`). Add `patternModeLoc`, `profileModeLoc`, `layerCountLoc`, per-layer arrays (`frequencyLoc[4]`, `angleLoc[4]`, `amplitudeLoc[4]`, `phaseLoc[4]`), and `float layerAngles[4]` accumulator. Keep `resolutionLoc`, `centerXLoc`, `centerYLoc`.
- Update function signatures: `MoireInterferenceEffectSetup` keeps `(Effect*, const Config*, float deltaTime)`.

**Verify**: `cmake.exe --build build` compiles (will have linker errors until Wave 2, but header parses).

---

#### Task 1.2: Update Moiré Generator header

**Files**: `src/effects/moire_generator.h`
**Creates**: Updated config and effect structs with `profileMode`

**Do**:
- Replace `bool sharpMode = false;` with `int profileMode = 0;` in `MoireGeneratorConfig`. Update the comment: `// 0=sine, 1=square, 2=triangle, 3=sawtooth`
- In `MOIRE_GENERATOR_CONFIG_FIELDS`, replace `sharpMode` with `profileMode`.
- In `MoireGeneratorEffect`, replace `int sharpModeLoc;` with `int profileModeLoc;`.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (all parallel, no file overlap)

#### Task 2.1: Rework Moiré Interference source

**Files**: `src/effects/moire_interference.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Follow `moire_generator.cpp` as the pattern reference for per-layer uniform binding and UI.
- Add `static const MoireInterferenceLayerConfig *GetLayer()` and `static MoireInterferenceLayerConfig *GetMutableLayer()` switch helpers (same pattern as generator's `GetLayer`/`GetMutableLayer`).
- Rewrite `MoireInterferenceEffectInit()`:
  - Cache locations for `patternMode`, `profileMode`, `layerCount`, `centerX`, `centerY`, `resolution`.
  - Loop `i=0..3`: cache `layer%d.frequency`, `layer%d.angle`, `layer%d.amplitude`, `layer%d.phase` using `TextFormat`.
  - Initialize `layerAngles[4]` to 0.
- Rewrite `MoireInterferenceEffectSetup()`:
  - Accumulate per-layer rotation: `e->layerAngles[i] += GetLayer(cfg, i)->rotationSpeed * deltaTime`.
  - Bind globals: `patternMode`, `profileMode`, `layerCount`, `centerX`, `centerY`, `resolution`.
  - Bind per-layer: for each layer, bind `frequency`, combined `angle` (static + accumulated), `amplitude`, `phase`.
- Rewrite `MoireInterferenceRegisterParams()`:
  - Register per-layer: `moireInterference.layer%d.frequency` (1-30), `.angle` (±ROTATION_OFFSET_MAX), `.rotationSpeed` (±ROTATION_SPEED_MAX), `.amplitude` (0-0.15), `.phase` (±ROTATION_OFFSET_MAX).
- Rewrite UI (`DrawMoireInterferenceParams`):
  - Combo for Pattern (Stripes/Circles/Grid), Combo for Profile (Sine/Square/Triangle/Sawtooth), SliderInt for Layers (2-4).
  - Per-layer controls in a helper function `DrawMoireInterferenceLayerControls()` matching the generator's `DrawMoireLayerControls()` pattern: Frequency (ModulatableSlider, "%.1f"), Angle (ModulatableSliderAngleDeg), Rotation Speed (ModulatableSliderSpeedDeg), Amplitude (ModulatableSlider, "%.3f"), Phase (ModulatableSliderAngleDeg).
  - Center controls in a TreeNodeAccented.
- Update registration macro: change category from `"SYM", 0` to `"WARP", 1`.

**Verify**: Compiles with Wave 1 headers.

---

#### Task 2.2: Rewrite Moiré Interference shader

**Files**: `shaders/moire_interference.fs`
**Depends on**: Wave 1 complete

**Do**: Replace the entire shader with the Algorithm section's "Moiré Interference shader (replacement)" GLSL. This includes:
- `profile()` function (4 modes)
- `WaveLayer` struct with `layer0`–`layer3` uniforms
- `layerDisplacement()` function with 3 pattern modes (stripes, circles, grid)
- `main()` summing displacements and applying mirror repeat
- Header comment: `// Moiré Interference: Wave displacement with multi-layer interference patterns`

**Verify**: Shader file is valid GLSL 330.

---

#### Task 2.3: Update Moiré Generator source

**Files**: `src/effects/moire_generator.cpp`
**Depends on**: Wave 1 complete

**Do**:
- In `MoireGeneratorEffectInit()`: replace `e->sharpModeLoc = GetShaderLocation(e->shader, "sharpMode")` with `e->profileModeLoc = GetShaderLocation(e->shader, "profileMode")`.
- In `MoireGeneratorEffectSetup()`: replace the `int sharpInt = cfg->sharpMode ? 1 : 0;` / `SetShaderValue(... sharpModeLoc ...)` block with direct `SetShaderValue(e->shader, e->profileModeLoc, &cfg->profileMode, SHADER_UNIFORM_INT)`.
- In UI: replace `ImGui::Checkbox("Sharp##moiregen", &mg->sharpMode)` with `ImGui::Combo("Profile##moiregen", &mg->profileMode, "Sine\0Square\0Triangle\0Sawtooth\0")`.

**Verify**: Compiles.

---

#### Task 2.4: Update Moiré Generator shader

**Files**: `shaders/moire_generator.fs`
**Depends on**: Wave 1 complete

**Do**:
- Replace `uniform int sharpMode;` with `uniform int profileMode;`
- Add the `profile()` function from the Algorithm section.
- Replace `gratingSmooth()`, `gratingSharp()`, and the routing `grating()` function with a single `grating()` that uses `profile()`:
  ```glsl
  float grating(float coord, float freq, float phase) {
      return 0.5 + 0.5 * profile(2.0 * PI * coord * freq + phase, profileMode);
  }
  ```
- In `main()`: remove the `if (sharpMode != 0)` additive/multiplicative branching. Use the single multiplicative+pow path for all profiles (as shown in Algorithm section).

**Verify**: Shader file is valid GLSL 330.

---

#### Task 2.5: Update serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireInterferenceLayerConfig, MOIRE_INTERFERENCE_LAYER_CONFIG_FIELDS)` next to the existing `MoireLayerConfig` registration.
- The existing `MoireInterferenceConfig` and `MoireGeneratorConfig` macros already reference the `*_CONFIG_FIELDS` macros, which Wave 1 updates. No manual changes needed for those lines — they auto-pick up the new fields.
- Add `sharpMode` → `profileMode` migration for generator: write a custom `from_json` for `MoireGeneratorConfig` that:
  1. First calls the default macro-generated deserialization (start with `c = MoireGeneratorConfig{};` then use `j.value()` for each field).
  2. If `j` contains `"sharpMode"` and does NOT contain `"profileMode"`: read `sharpMode` as bool, set `profileMode = sharpMode ? 1 : 0`.
  - This means replacing the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireGeneratorConfig, ...)` with a manual `to_json`/`from_json` pair. `to_json` serializes all fields from `MOIRE_GENERATOR_CONFIG_FIELDS` (can use a helper macro or write explicitly). `from_json` deserializes with default fallback plus the migration check.

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Moiré Interference appears in WARP section (section 1) in UI
- [ ] Interference shows Pattern combo (Stripes/Circles/Grid), Profile combo (Sine/Square/Triangle/Sawtooth), Layers slider, per-layer controls
- [ ] Generator shows Profile combo instead of Sharp checkbox
- [ ] Old presets with `sharpMode: true` load as `profileMode: 1` (square) in generator
- [ ] Old presets with interference fields load with defaults for new layer params
