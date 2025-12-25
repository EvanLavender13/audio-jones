# Trail Blend Modes

Add selectable blend modes for how the physarum trail map composites with the main visualization. Currently only luminance-based brightness boost exists; this adds options to show trail color through via screen, mix, and soft light blending.

## Current State

- `shaders/physarum_boost.fs:18-33` - Boost shader converts trail to luminance, applies multiplicative boost
- `src/config/effect_config.h:18-34` - PhysarumConfig with `boostIntensity` field
- `src/render/post_effect.cpp:369-375` - Applies boost shader when intensity > 0
- `src/ui/imgui_effects.cpp:75` - Boost intensity slider
- `src/config/preset.cpp` - Preset serialization

## Phase 1: Add Blend Mode Selection

**Goal**: Implement 5 selectable trail blend modes sharing the existing intensity slider.

**Config changes** (`src/config/effect_config.h`):
- Add `TrailBlendMode` enum before PhysarumConfig: `{ Boost, TintedBoost, Screen, Mix, SoftLight }`
- Add `TrailBlendMode trailBlendMode = TrailBlendMode::Boost` field to PhysarumConfig

**Shader changes** (`shaders/physarum_boost.fs`):
- Add `uniform int blendMode`
- Implement all 5 modes with if/else chain:
  - **Boost (0)**: Current behavior - `original * (1 + luminance * intensity * headroom)`
  - **Tinted Boost (1)**: `original * (1 + trailColor * intensity * headroom)`
  - **Screen (2)**: `1 - (1 - original) * (1 - trailColor * intensity)`
  - **Mix (3)**: `mix(original, trailColor, luminance * intensity)`
  - **Soft Light (4)**: Pegtop formula - `(1 - 2*b)*a² + 2*b*a` where b = trail * intensity
- Keep Reinhard tone mapping for all modes

**Post-effect changes** (`src/render/post_effect.h`, `src/render/post_effect.cpp`):
- Add `blendModeLoc` to PostEffect struct
- Get uniform location in init
- Set uniform value before draw

**UI changes** (`src/ui/imgui_effects.cpp`):
- Add combo box above or below Boost slider: `ImGui::Combo("Blend Mode", ...)`
- Mode names: "Boost", "Tinted Boost", "Screen", "Mix", "Soft Light"

**Preset changes** (`src/config/preset.cpp`):
- Serialize `trailBlendMode` as integer in physarum section
- Deserialize with default fallback to 0 (Boost) for existing presets

**Fix comment** (`src/render/physarum.h:87`):
- Change "grayscale" to "color" in debug visualization comment

**Done when**: Can switch between all 5 blend modes via UI dropdown, each produces visually distinct result, existing presets load without error (defaulting to Boost mode).
