# Anamorphic Streak Rework

Replace the current 3-tap iterative Kawase blur with a mip-chain horizontal blur matching Bloom's architecture. The current approach produces "string of pearls" artifacts — discrete sample points along the streak instead of continuous lines. The mip-chain approach doubles effective reach at each level, producing smooth streaks.

**Research**: `docs/research/anamorphic-streak.md`

## Design

### Types

**AnamorphicStreakConfig** (replace existing):

```
enabled      bool    false
threshold    float   0.8     Brightness cutoff (0.0-2.0)
knee         float   0.5     Soft threshold falloff (0.0-1.0)
intensity    float   0.5     Streak brightness in composite (0.0-2.0)
stretch      float   0.8     Upsample blend: favors wider blur levels at higher values (0.0-1.0)
tintR        float   0.55    Streak color red channel (0.0-1.0)
tintG        float   0.65    Streak color green channel (0.0-1.0)
tintB        float   1.0     Streak color blue channel (0.0-1.0)
iterations   int     5       Mip chain depth (3-7)
```

Removed fields: `sharpness` (no longer needed — 6-tap kernel inherently smooth), `stretch` range changes from 0.5-8.0 to 0.0-1.0 (now a lerp factor, not a multiplier).

**AnamorphicStreakEffect** (replace existing):

```
prefilterShader     Shader              Kept from current
downsampleShader    Shader              NEW — replaces blurShader
upsampleShader      Shader              NEW
compositeShader     Shader              Kept from current
mips[STREAK_MIP_COUNT]  RenderTexture2D NEW — own mip chain (not shared halfResA/B)

// Prefilter uniform locations
thresholdLoc        int
kneeLoc             int

// Downsample uniform locations
downsampleTexelLoc  int     Horizontal texel size for 6-tap offsets

// Upsample uniform locations
upsampleTexelLoc    int     Horizontal texel size for 3-tap offsets
highResTexLoc       int     Sampler for the narrower mip (lerp target)
stretchLoc          int     Lerp factor per level

// Composite uniform locations
intensityLoc        int
tintLoc             int     NEW
streakTexLoc        int
```

### Constants

- `STREAK_MIP_COUNT = 7` — static max, `iterations` config selects how many to use (3-7)

### Algorithm

**Prefilter**: Identical to current — soft-knee threshold extraction. Renders source at half-resolution into `mips[0]`. Each mip halves width only; height stays at `screenHeight/2`.

**Downsample** (new shader `anamorphic_streak_downsample.fs`): 6 horizontal taps with triangle weights. Offsets in texels: -5, -3, -1, +1, +3, +5. Weights: 1, 2, 3, 3, 2, 1 (sum 12). Each mip level halves width from the previous, so texel size doubles the effective reach. Shader receives `texelSize` uniform = `1.0 / sourceWidth`.

**Upsample** (new shader `anamorphic_streak_upsample.fs`): 3 horizontal taps. Offsets: -1.5, 0, +1.5 texels. Weights: 0.25, 0.5, 0.25. At each level, lerp between the current mip and the upsampled result using `stretch`. Higher stretch favors wider blur. Unlike Bloom's additive blend, this uses `mix()` controlled by the `stretch` uniform.

**Composite** (modify existing `anamorphic_streak_composite.fs`): Add `tint` uniform. Formula: `original + streak * tint * intensity`.

### Mip Allocation

Follow Bloom's `InitMips` pattern. Width starts at `screenWidth/2`, height at `screenHeight/2`. Each successive mip halves width only (height constant). Clamp width minimum to 1.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| threshold | float | 0.0-2.0 | 0.8 | yes | Threshold |
| knee | float | 0.0-1.0 | 0.5 | no | Knee |
| intensity | float | 0.0-2.0 | 0.5 | yes | Intensity |
| stretch | float | 0.0-1.0 | 0.8 | yes | Stretch |
| tintR | float | 0.0-1.0 | 0.55 | yes | Tint (via ColorEdit3) |
| tintG | float | 0.0-1.0 | 0.65 | yes | |
| tintB | float | 0.0-1.0 | 1.0 | yes | |
| iterations | int | 3-7 | 5 | no | Iterations |

### Modulation Registration

- `anamorphicStreak.threshold` — 0.0 to 2.0
- `anamorphicStreak.intensity` — 0.0 to 2.0
- `anamorphicStreak.stretch` — 0.0 to 1.0
- `anamorphicStreak.tint` — register R/G/B as three separate params (0.0 to 1.0 each): `anamorphicStreak.tintR`, `anamorphicStreak.tintG`, `anamorphicStreak.tintB`

Remove: `anamorphicStreak.sharpness` (field deleted)

---

## Tasks

### Wave 1: Header & Shaders

#### Task 1.1: Update effect header

**Files**: `src/effects/anamorphic_streak.h`
**Creates**: Updated struct layouts that Wave 2 depends on

**Do**:
- Add `static const int STREAK_MIP_COUNT = 7;` (same pattern as `BLOOM_MIP_COUNT` in bloom.h)
- Replace `AnamorphicStreakConfig`: remove `sharpness`, change `stretch` range/default (0.8, was 1.5), add `float tintR = 0.55f; float tintG = 0.65f; float tintB = 1.0f;`, change `iterations` default to 5
- Replace `AnamorphicStreakEffect`: remove `blurShader` and its locs (`resolutionLoc`, `offsetLoc`, `sharpnessLoc`), add `downsampleShader`, `upsampleShader`, `RenderTexture2D mips[STREAK_MIP_COUNT]`, `downsampleTexelLoc`, `upsampleTexelLoc`, `highResTexLoc`, `stretchLoc`, `tintLoc`
- Update `AnamorphicStreakEffectInit` signature to take `(AnamorphicStreakEffect *e, int width, int height)` — needs dimensions for mip allocation, same as `BloomEffectInit`
- Remove `Texture2D streakTex` param from `AnamorphicStreakEffectSetup` — now uses `e->mips[0].texture` internally, same as `BloomEffectSetup`
- Add `AnamorphicStreakEffectResize(AnamorphicStreakEffect *e, int width, int height)` declaration

**Verify**: Header parses cleanly.

---

#### Task 1.2: Create downsample shader

**Files**: `shaders/anamorphic_streak_downsample.fs` (create)

**Do**:
- Version 330, standard `in vec2 fragTexCoord; out vec4 finalColor;`
- Uniforms: `sampler2D texture0`, `float texelSize` (1.0 / source width)
- 6 horizontal taps at offsets -5, -3, -1, +1, +3, +5 texels (multiply by texelSize)
- Triangle weights: 1, 2, 3, 3, 2, 1, normalize by dividing sum by 12.0
- Only horizontal sampling — all offsets in x, y stays at `fragTexCoord.y`

**Verify**: Shader file exists.

---

#### Task 1.3: Create upsample shader

**Files**: `shaders/anamorphic_streak_upsample.fs` (create)

**Do**:
- Version 330, standard `in vec2 fragTexCoord; out vec4 finalColor;`
- Uniforms: `sampler2D texture0` (wider mip being upsampled), `sampler2D highResTex` (current narrower mip), `float texelSize`, `float stretch`
- Sample `texture0` with 3 horizontal taps at offsets -1.5, 0, +1.5 texels (weights: 0.25, 0.5, 0.25)
- Sample `highResTex` at `fragTexCoord`
- Output `mix(highRes, upsampled, stretch)` — lerp between narrow and wide per the research doc

**Verify**: Shader file exists.

---

#### Task 1.4: Update composite shader

**Files**: `shaders/anamorphic_streak_composite.fs` (modify)

**Do**:
- Add `uniform vec3 tint;`
- Change formula from `original + streak * intensity` to `original + streak * tint * intensity`

**Verify**: Shader file exists with tint uniform.

---

### Wave 2: C++ Implementation

#### Task 2.1: Update effect module

**Files**: `src/effects/anamorphic_streak.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "render/render_utils.h"` for `RenderUtilsInitTextureHDR`
- Add static `InitMips(AnamorphicStreakEffect *e, int width, int height)` — follow bloom.cpp pattern. Width starts at `width/2`, height at `height/2`. Each successive mip halves width only (not height). Use `RenderUtilsInitTextureHDR` for each mip.
- Add static `UnloadMips(AnamorphicStreakEffect *e)` — unload all `STREAK_MIP_COUNT` mips
- Update `AnamorphicStreakEffectInit`: load 4 shaders (prefilter, downsample, upsample, composite) with cascade cleanup on failure (follow bloom.cpp pattern). Cache all uniform locations. Call `InitMips(e, width, height)`.
- Update `AnamorphicStreakEffectSetup`: bind composite uniforms only (intensity, tint, streakTexLoc pointing to `e->mips[0].texture`). No `Texture2D streakTex` parameter — use own mips.
- Add `AnamorphicStreakEffectResize`: call `UnloadMips` then `InitMips` (same pattern as `BloomEffectResize`)
- Update `AnamorphicStreakEffectUninit`: unload 4 shaders + call `UnloadMips`
- Update `AnamorphicStreakRegisterParams`: remove sharpness, change stretch range to 0.0-1.0, add tintR/tintG/tintB (register `&cfg->tintR`, `&cfg->tintG`, `&cfg->tintB`)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Update PostEffect integration

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Update `AnamorphicStreakEffectInit` call to pass `(screenWidth, screenHeight)` — follow bloom init pattern at line ~440
- Add `AnamorphicStreakEffectResize(&pe->anamorphicStreak, width, height)` in resize handler — add after `BloomEffectResize` call at line ~761
- `SetupAnamorphicStreak` in shader_setup_optical.cpp: remove `pe->halfResA.texture` argument from `AnamorphicStreakEffectSetup` call (no longer takes streakTex)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Update shader setup (render pass)

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Replace `ApplyAnamorphicStreakPasses` with mip-chain version following `ApplyBloomPasses` pattern:
  1. Clamp iterations to 1..STREAK_MIP_COUNT
  2. Prefilter: bind threshold/knee, render source → `mips[0]` using `DrawTexturePro` to scale to mip[0] dimensions
  3. Downsample loop (i=1 to iterations-1): bind `texelSize = 1.0 / mips[i-1].texture.width`, render `mips[i-1]` → `mips[i]`
  4. Upsample loop (i=iterations-1 down to 1): **lerp** between the current mip and the upsampled wider level, per the research doc. `result = mix(mips[i-1], upsample(mips[i]), stretch)`. At stretch=0 the narrow mip survives unchanged; at stretch=1 the wider upsampled blur replaces it entirely.

  **Implementation**: Move the lerp into the upsample shader. Add a `uniform sampler2D highResTex` to the upsample shader (the current mip[i-1]). The shader reads the 3-tap upsampled mips[i] and the high-res mips[i-1], then outputs `mix(highRes, upsampled, stretch)`. Bind mips[i-1] as `highResTex` via `SetShaderValueTexture`, then draw mips[i] into mips[i-1] with the upsample shader. This overwrites mips[i-1] with the lerped result.

  This matches the Kino/Streak reference: each upsample level blends toward the wider blur controlled by `stretch`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Update shader setup (optical)

**Files**: `src/render/shader_setup_optical.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Update `SetupAnamorphicStreak`: remove `pe->halfResA.texture` argument. Call `AnamorphicStreakEffectSetup(&pe->anamorphicStreak, &pe->effects.anamorphicStreak)` — no deltaTime (effect has no animation accumulator), no streakTex (uses own mips).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Update UI panel

**Files**: `src/ui/imgui_effects_optical.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Remove Sharpness slider
- Change Stretch slider range from 0.5-8.0 to 0.0-1.0, format "%.2f"
- Change Iterations slider range from 2-6 to 3-7
- Add tint color editor: `ImGui::ColorEdit3("Tint##anamorphicStreak", &a->tintR)` — tintR/tintG/tintB are contiguous floats, so `&tintR` works as a `float*` for ColorEdit3

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Update preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `AnamorphicStreakConfig`: remove `sharpness`, add `tintR`, `tintG`, `tintB`, keep `stretch`
- Full macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AnamorphicStreakConfig, enabled, threshold, knee, intensity, stretch, tintR, tintG, tintB, iterations)`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.7: Delete old blur shader

**Files**: `shaders/anamorphic_streak_blur.fs` (delete)
**Depends on**: Wave 2 tasks 2.1-2.3 complete (nothing references it anymore)

**Do**:
- Delete `shaders/anamorphic_streak_blur.fs` — replaced by downsample + upsample shaders

**Verify**: File gone, build still succeeds.

---

## Implementation Notes

**Two-array upsample (deviation from plan)**: The plan specified a single `mips[]` array mirroring bloom. The upsample lerp (`mix(highRes, upsampled, stretch)`) requires reading `mips[i-1]` as a shader input while writing to it — undefined behavior in OpenGL. Bloom avoids this because `BLEND_ADDITIVE` never reads the target as a texture. Fix: added a second array `mipsUp[STREAK_MIP_COUNT]` following Kino's actual implementation. Downsample writes to `mips[]` (read-only during upsample). Upsample reads `mips[i]` as `highResTex` and the previous upsampled result as `texture0`, writes to `mipsUp[i]`. Composite reads `mipsUp[0]`.

**5x composite gain**: The lerp-based upsample dilutes energy through the chain — at stretch=1.0 the widest (dimmest) mip dominates. Kino compensates with a hardcoded `* 5` in the composite shader: `original + streak * tint * intensity * 5.0`. Without this, the streak is too dim at any intensity setting.

**Uniform name mismatch**: Initial implementation used `"highResTexture"` in `GetShaderLocation` but the shader declared `uniform sampler2D highResTex`. Fixed to match.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Streak renders as smooth continuous horizontal lines (no "string of pearls")
- [x] Streak width increases with higher `stretch` values
- [x] More `iterations` extends maximum streak reach
- [x] Tint colors the streak (default blue)
- [ ] Preset save/load preserves all parameters including tint
- [ ] Window resize reallocates mip chain correctly
- [x] Modulation routes to threshold, intensity, stretch, tintR/G/B
- [x] Old blur shader deleted
