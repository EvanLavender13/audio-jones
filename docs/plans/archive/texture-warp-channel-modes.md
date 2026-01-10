# Texture Warp Channel Modes

Add a parameter to select which color channel combination drives UV displacement in the Texture Warp effect. Provides 7 modes (RG, RB, GB, Luminance, LuminanceSplit, Chrominance, Polar) that produce different emergent patterns from the same input.

## Current State

- `src/config/texture_warp_config.h:4-8` - Config struct with enabled, strength, iterations
- `shaders/texture_warp.fs:16` - Hardcoded `sample.rg` for offset
- `src/render/post_effect.h:105-106` - Uniform locations for strength/iterations
- `src/render/post_effect.cpp:135-136` - Uniform location fetching
- `src/render/shader_setup.cpp:218-225` - `SetupTextureWarp()` passes uniforms
- `src/ui/imgui_effects.cpp:429-437` - UI section with strength slider and iterations
- `src/config/preset.cpp:116-117` - JSON serialization macro

## Technical Implementation

**Source**: `docs/research/texture-warp-channel-modes.md`

**Channel Mode Formulas** (all operate on `vec3 sample`):

| Mode | ID | Offset Computation |
|------|----|--------------------|
| RG | 0 | `(sample.rg - 0.5) * 2.0` |
| RB | 1 | `(sample.rb - 0.5) * 2.0` |
| GB | 2 | `(sample.gb - 0.5) * 2.0` |
| Luminance | 3 | `l = dot(sample, vec3(0.299, 0.587, 0.114)); vec2(l - 0.5) * 2.0` |
| LuminanceSplit | 4 | `l = ...; vec2(l - 0.5, 0.5 - l) * 2.0` |
| Chrominance | 5 | `vec2(sample.r - sample.b, sample.g - sample.b) * 2.0` |
| Polar | 6 | Hue → angle, saturation → magnitude (see below) |

**Polar Mode HSV Conversion**:

```glsl
// RGB to HSV (optimized inline)
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Polar offset
vec3 hsv = rgb2hsv(sample);
float angle = hsv.x * 6.28318530718;  // Hue [0,1] → radians
vec2 offset = vec2(cos(angle), sin(angle)) * hsv.y;  // Saturation = magnitude
```

---

## Phase 1: Config and Enum

**Goal**: Define the channel mode enum and extend config struct.

**Build**:
- Add `TextureWarpChannelMode` enum to `texture_warp_config.h` with 7 values
- Add `TextureWarpChannelMode channelMode = TextureWarpChannelMode::RG;` field to struct

**Done when**: Config compiles with new field defaulting to RG.

---

## Phase 2: Shader

**Goal**: Implement channel mode selection in the fragment shader.

**Build**:
- Add `uniform int channelMode;` to `texture_warp.fs`
- Add `rgb2hsv()` function for Polar mode
- Replace the hardcoded offset line with if-chain selecting mode 0-6
- Each mode computes `vec2 offset` per the formulas above

**Done when**: Shader compiles. Mode 0 (RG) produces identical output to current behavior.

---

## Phase 3: Uniform Plumbing

**Goal**: Pass channelMode from config to shader.

**Build**:
- Add `int textureWarpChannelModeLoc;` to `PostEffect` struct in `post_effect.h`
- Fetch location in `GetShaderUniformLocations()` in `post_effect.cpp`
- Pass value in `SetupTextureWarp()` in `shader_setup.cpp` (cast enum to int)

**Done when**: Uniform value reaches shader.

---

## Phase 4: UI and Serialization

**Goal**: Expose channel mode in UI and persist in presets.

**Build**:
- Add combo box in `imgui_effects.cpp` Texture Warp section with mode names
- Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `TextureWarpConfig` in `preset.cpp` to include `channelMode`

**Done when**: Mode persists across preset save/load and displays correctly in UI.

---

## Phase 5: Verification

**Goal**: Test all modes produce distinct visual output.

**Build**:
- Enable Texture Warp with moderate strength (0.05) and 3-4 iterations
- Cycle through all 7 modes, verify each produces different displacement pattern
- Verify RG mode matches original behavior exactly

**Done when**: All modes work, no visual regressions.
