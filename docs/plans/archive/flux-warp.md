# Flux Warp

Evolving cellular distortion where sharp-edged cells split, merge, and restructure over time. The core trick is `mod()` on coupled oscillators with an animated divisor: smooth trig inputs create sharp cell boundaries, and the divisor morphing between product-cells and radial-cells causes the cell topology itself to continuously restructure. Converts the scalar field to 2D displacement via two phase-offset evaluations.

**Research**: `docs/research/flux-warp.md`

## Design

### Types

**FluxWarpConfig** (user-facing parameters):
```
bool enabled = false
float warpStrength = 0.15f    // 0.0-0.5 — displacement amplitude
float cellScale = 6.0f        // 1.0-20.0 — UV multiplier (more/smaller cells)
float coupling = 0.7f         // 0.0-1.0 — x→y wave dependency
float waveFreq = 15.0f        // 1.0-50.0 — trig oscillation frequency
float animSpeed = 1.0f        // 0.0-2.0 — overall time multiplier
float divisorSpeed = 0.3f     // 0.0-1.0 — cell geometry morph rate
float divisorBias = 0.5f      // 0.0-1.0 — product-cells (0) vs radial-cells (1)
float gateSpeed = 0.15f       // 0.0-0.5 — amplitude modulation rate
```

**FluxWarpEffect** (runtime state):
```
Shader shader
int warpStrengthLoc, cellScaleLoc, couplingLoc, waveFreqLoc
int timeLoc, divisorSpeedLoc, divisorBiasLoc, gateSpeedLoc
float time    // accumulated animation time
```

### Algorithm

The shader evaluates a scalar flux field twice (with different phase offsets) to produce independent x and y displacement channels.

**Core flux field function** (`fluxField`):
```glsl
float fluxField(vec2 uv, float time, float phaseOffset) {
    float t = time + phaseOffset;
    float x = cos(waveFreq * abs(uv.x) + 0.2 * t);
    float y = sin(abs(uv.y) + coupling * x - 0.5 * t);

    // Animated divisor blend: product-cells ↔ radial-cells
    float b = 0.5 * (1.0 + cos(divisorSpeed * t));
    b = mix(b, divisorBias, 0.5);  // blend animated with static bias

    // Amplitude modulation gate
    float b2 = 0.5 * (1.0 + cos(gateSpeed * t));

    float divisor = (1.0 - b) * x * y + b * 0.4 * length(uv);
    return mod(x + y, divisor) * (b2 + (1.0 - b2) * 0.5 * (0.2 + sin(2.0 * uv.x * uv.y + t)));
}
```

**Displacement in main()**:
```glsl
vec2 uv = fragTexCoord;
vec2 centered = cellScale * (uv - 0.5);  // center-origin UV

float fx = fluxField(centered, time, 0.0);    // phaseX = 0
float fy = fluxField(centered, time, 1.7);    // phaseY = 1.7 (decorrelated)

vec2 displaced = uv + vec2(fx, fy) * warpStrength;
finalColor = texture(texture0, clamp(displaced, 0.0, 1.0));
```

Time is accumulated on CPU: `time += animSpeed * deltaTime`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| warpStrength | float | 0.0-0.5 | 0.15 | Yes | Strength |
| cellScale | float | 1.0-20.0 | 6.0 | Yes | Cell Scale |
| coupling | float | 0.0-1.0 | 0.7 | Yes | Coupling |
| waveFreq | float | 1.0-50.0 | 15.0 | Yes | Wave Freq |
| animSpeed | float | 0.0-2.0 | 1.0 | Yes | Anim Speed |
| divisorSpeed | float | 0.0-1.0 | 0.3 | No | Divisor Speed |
| divisorBias | float | 0.0-1.0 | 0.5 | Yes | Divisor Bias |
| gateSpeed | float | 0.0-0.5 | 0.15 | No | Gate Speed |

### Constants

- Enum: `TRANSFORM_FLUX_WARP`
- Display name: `"Flux Warp"`
- Category badge: `"WARP"`
- Section index: `1`
- Flags: `EFFECT_FLAG_NONE`
- Field name: `fluxWarp`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create FluxWarpConfig and FluxWarpEffect

**Files**: `src/effects/flux_warp.h` (create)
**Creates**: Config struct, Effect struct, function declarations — needed by all other tasks.

**Do**: Follow `domain_warp.h` pattern. Declare `FluxWarpConfig` with all 8 parameters (defaults from Parameter table), `FLUX_WARP_CONFIG_FIELDS` macro, `FluxWarpEffect` typedef with shader + 8 uniform locations + float time accumulator. Declare Init/Setup/Uninit/ConfigDefault/RegisterParams.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect Module Implementation

**Files**: `src/effects/flux_warp.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `domain_warp.cpp` pattern exactly. Init loads `shaders/flux_warp.fs`, caches 8 uniform locations. Setup accumulates `time += cfg->animSpeed * deltaTime`, sets all 8 uniforms. Uninit unloads shader. RegisterParams registers the 6 modulatable parameters (warpStrength, cellScale, coupling, waveFreq, animSpeed, divisorBias) with ranges from Parameter table. Include the `SetupFluxWarp` bridge function and `REGISTER_EFFECT` macro at bottom:
```
REGISTER_EFFECT(TRANSFORM_FLUX_WARP, FluxWarp, fluxWarp, "Flux Warp", "WARP", 1, EFFECT_FLAG_NONE, SetupFluxWarp, NULL)
```

**Verify**: Compiles (with stub shader).

#### Task 2.2: Fragment Shader

**Files**: `shaders/flux_warp.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section above. Uniforms: `texture0`, `resolution`, `warpStrength`, `cellScale`, `coupling`, `waveFreq`, `time`, `divisorSpeed`, `divisorBias`, `gateSpeed`. Define `fluxField()` function, evaluate twice with phase offsets 0.0 and 1.7, displace UVs, clamp, sample texture.

**Verify**: Valid GLSL 330.

#### Task 2.3: Config Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/flux_warp.h"`. Add `TRANSFORM_FLUX_WARP` to `TransformEffectType` enum (before `TRANSFORM_EFFECT_COUNT`). Add to `TransformOrderConfig::order` array. Add `FluxWarpConfig fluxWarp;` to `EffectConfig` struct.

**Verify**: Compiles.

#### Task 2.4: PostEffect Struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/flux_warp.h"`. Add `FluxWarpEffect fluxWarp;` member to `PostEffect` struct (near other warp effects).

**Verify**: Compiles.

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/flux_warp.cpp` to `EFFECTS_SOURCES`.

**Verify**: CMake configures.

#### Task 2.6: UI Panel

**Files**: `src/ui/imgui_effects_warp.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `static bool sectionFluxWarp = false;` with other section bools. Add `DrawWarpFluxWarp()` helper following `DrawWarpDomainWarp` pattern. 8 sliders: `ModulatableSlider` for the 6 modulatable params, `ImGui::SliderFloat` for divisorSpeed and gateSpeed. Use `"##fluxwarp"` suffix for all widget IDs. Add call in `DrawWarpCategory()` with `ImGui::Spacing()` — place after Tone Warp (last current entry).

**Verify**: Compiles.

#### Task 2.7: Preset Serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `#include "effects/flux_warp.h"`. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FluxWarpConfig, FLUX_WARP_CONFIG_FIELDS)` with other per-config macros. Add `X(fluxWarp)` to `EFFECT_CONFIG_FIELDS` X-macro table.

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in transform reorder UI with "WARP" badge
- [ ] Enabling shows cellular distortion pattern
- [ ] All 8 sliders modify the effect in real-time
- [ ] Coupling at 0 = grid-like, at 1 = organic tangles
- [ ] DivisorBias morphs between diagonal ridges and radial rings
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes work for the 6 registered parameters
