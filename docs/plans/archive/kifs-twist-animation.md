# KIFS Twist Animation

Add animated per-iteration rotation to the KIFS effect. Replaces the static `twistAngle` parameter with `twistSpeed` (radians/frame) using CPU accumulation.

## Current State

- `src/config/kifs_config.h:13` — Static `twistAngle` field (radians)
- `src/render/post_effect.h:299` — `currentKifsRotation` accumulator exists (for global rotation)
- `src/render/render_pipeline.cpp:298` — CPU accumulation pattern: `pe->currentKifsRotation += pe->effects.kifs.rotationSpeed`
- `src/render/shader_setup.cpp:270-271` — Passes `twistAngle` directly to shader
- `src/ui/imgui_effects_transforms.cpp:102-103` — "Twist" slider displays degrees, stores radians
- `src/automation/param_registry.cpp:60` — `kifs.twistAngle` registered with `ROTATION_OFFSET_MAX`
- `shaders/kifs.fs:77` — Uses `twistAngle` per-iteration: `rotate2d(p, twistAngle + float(i) * 0.1)`

## Phase 1: Config and State

**Goal**: Add twistSpeed field and CPU accumulator.

**Build**:
- `kifs_config.h` — Rename `twistAngle` → `twistSpeed`, update comment to "Per-iteration rotation rate (radians/frame)"
- `post_effect.h` — Add `float currentKifsTwist;` accumulator field after `currentKifsRotation`

**Done when**: Project compiles (with errors in dependent files).

---

## Phase 2: CPU Accumulation

**Goal**: Accumulate twist angle each frame.

**Build**:
- `render_pipeline.cpp` — Add `pe->currentKifsTwist += pe->effects.kifs.twistSpeed;` after the existing KIFS rotation accumulation line
- `shader_setup.cpp` — Change `SetupKifs` to pass `pe->currentKifsTwist` instead of `k->twistAngle`

**Done when**: Shader receives accumulated twist value.

---

## Phase 3: UI and Param Registry

**Goal**: Update UI label and modulation bounds.

**Build**:
- `imgui_effects_transforms.cpp` — Update slider: change param ID from `kifs.twistAngle` to `kifs.twistSpeed`, keep label "Twist", format "%.2f °/f"
- `param_registry.cpp` — Change `kifs.twistAngle` → `kifs.twistSpeed`, change bounds from `ROTATION_OFFSET_MAX` to `ROTATION_SPEED_MAX`

**Done when**: UI displays twist speed slider with correct units.

---

## Phase 4: Preset Serialization

**Goal**: Update serialization field name.

**Build**:
- `preset.cpp` — In `KifsConfig` serialization macro, rename `twistAngle` → `twistSpeed`

**Done when**: Presets save/load with new field name. Old presets get default 0.0f via `_WITH_DEFAULT` macro.

---

## Phase 5: Test and Verify

**Goal**: Confirm animated twist works correctly.

**Build**:
- Enable KIFS effect
- Set `twistSpeed` to non-zero value (~0.01 rad/frame)
- Verify fractal spirals continuously
- Verify modulation source can animate the twist speed

**Done when**: Visual confirmation of spiraling fractal animation.
