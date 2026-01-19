# Rotation Speed: Frame-Based to Time-Based

Convert all `*Speed` rotation fields from radians/frame to radians/second, fixing the unintuitive slider behavior where aesthetic values require tiny numbers like 0.002.

## Current State

- `src/ui/ui_units.h:17` - `ROTATION_SPEED_MAX = 0.0872665f` (5° in radians)
- `src/render/render_pipeline.cpp:297-331` - Per-frame accumulation: `accum += speed`
- `src/render/drawable.cpp:203` - Drawable rotation: `rotationAccum += base.rotationSpeed`
- `src/simulation/attractor_flow.cpp:212-214` - Attractor rotation accumulation
- Presets contain tiny values like 0.01 to achieve gentle rotation

**Problem**: At 60fps, the current max of 5°/frame = 300°/s. Aesthetic speeds (1-10°/s) require values like 0.002-0.02, which are hard to type and adjust.

## Technical Implementation

**Unit conversion**:
- Old: radians/frame, accumulated as `accum += speed` (implicit ×60 at 60fps)
- New: radians/second, accumulated as `accum += speed * deltaTime`

**Bounds change**:
- Old: `ROTATION_SPEED_MAX = 0.0872665f` (5°)
- New: `ROTATION_SPEED_MAX = 3.14159265f` (π = 180°/s)

**Migration factor**: Old values × 60 (assuming 60fps tuning). Example: 0.01 rad/frame → 0.6 rad/s → 34°/s

**UI display**: Show °/s with 1 decimal place. A value of 1 rad/s displays as "57.3 °/s".

---

## Phase 1: Core Constants and Documentation

**Goal**: Update constants and documentation to reflect new semantics.

**Build**:
- `src/ui/ui_units.h` - Change `ROTATION_SPEED_MAX` to `3.14159265f` (π). Update comment to "180°/s in radians".
- `CLAUDE.md` - Update Angular Field Naming section:
  - `*Speed` → rate (radians/second), time-scaled accumulation (`accum += speed * deltaTime`), UI shows "°/s", bounded by `ROTATION_SPEED_MAX`
  - Remove the `*Rate` definition (consolidate into `*Speed`)
- `.claude/skills/add-effect/SKILL.md` - Update speed field documentation if present.

**Done when**: Constants and documentation reflect radians/second semantics.

---

## Phase 2: Accumulation Pattern Changes

**Goal**: Change all speed accumulation sites to use deltaTime.

**Build**:

**`src/render/render_pipeline.cpp`** - In `RenderPipelineApplyOutput()` (~line 297):
- This function already has access to deltaTime via `pe->currentDeltaTime`
- Change all frame-based accumulations to time-based:
  ```
  pe->currentKaleidoRotation += pe->effects.kaleidoscope.rotationSpeed * dt;
  pe->currentKifsRotation += pe->effects.kifs.rotationSpeed * dt;
  pe->currentKifsTwist += pe->effects.kifs.twistSpeed * dt;
  pe->currentLatticeFoldRotation += pe->effects.latticeFold.rotationSpeed * dt;
  pe->currentMandelboxRotation += pe->effects.mandelbox.rotationSpeed * dt;
  pe->currentMandelboxTwist += pe->effects.mandelbox.twistSpeed * dt;
  pe->currentTriangleFoldRotation += pe->effects.triangleFold.rotationSpeed * dt;
  pe->currentTriangleFoldTwist += pe->effects.triangleFold.twistSpeed * dt;
  pe->currentPoincareRotation += pe->effects.poincareDisk.rotationSpeed * dt;
  pe->poincareTime += pe->effects.poincareDisk.translationSpeed * dt;
  pe->currentHalftoneRotation += pe->effects.halftone.rotationSpeed * dt;
  pe->domainWarpDrift += pe->effects.domainWarp.driftSpeed * dt;
  ```

**`src/render/shader_setup.cpp`** - Two locations:
- `SetupFeedback()` (~line 132): FlowField rotations are passed directly to shader per-frame. Multiply by deltaTime before SetShaderValue:
  ```
  float rotBase = pe->effects.flowField.rotationSpeed * pe->currentDeltaTime;
  float rotRadial = pe->effects.flowField.rotationSpeedRadial * pe->currentDeltaTime;
  SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc, &rotBase, ...);
  SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc, &rotRadial, ...);
  ```
- `SetupPhyllotaxis()` (~line 841): Change to time-based:
  ```
  pe->phyllotaxisAngleTime += ph->angleSpeed * pe->currentDeltaTime;
  pe->phyllotaxisPhaseTime += ph->phaseSpeed * pe->currentDeltaTime;
  ```

**`src/render/drawable.cpp`** - In `DrawablesUpdateRotation()` (~line 203):
- Add deltaTime parameter to function signature
- Change to `drawables[i].rotationAccum += drawables[i].base.rotationSpeed * deltaTime;`
- Update caller in `src/main.cpp` to pass deltaTime

**`src/simulation/attractor_flow.cpp`** - In `AttractorFlowUpdate()` (~line 212):
- Already receives deltaTime parameter
- Change `af->rotationAccumX/Y/Z += af->config.rotationSpeedX/Y/Z;` to `* deltaTime`

**Done when**: All speed fields use time-scaled accumulation. Build succeeds.

---

## Phase 3: UI Format Updates

**Goal**: Update all speed sliders to display °/s instead of °/f.

**Build**:
- `src/ui/imgui_effects.cpp` - Find all `ModulatableSliderAngleDeg` calls for speed fields:
  - Change format from `"%.3f °/f"` or `"%.2f °/f"` to `"%.1f °/s"`
  - Affected: flowField spin/spin radial, attractorFlow spin X/Y/Z, kaleidoscope, kifs, latticeFold, mandelbox, triangleFold, poincareDisk, halftone, phyllotaxis, domainWarp
- `src/ui/imgui_effects_transforms.cpp` - Check for any speed sliders here
- `src/ui/drawable_type_controls.cpp` - Update drawable rotation speed format if present

**Done when**: All speed sliders show °/s unit.

---

## Phase 4: Migrate Presets via Script

**Goal**: Update all preset JSON files with a one-time Python script.

**Build**:
- Create `scripts/migrate_speed_values.py` (temporary, delete after use)
- Script reads each `presets/*.json`, multiplies speed fields by 60, writes back
- Run script once, verify presets look correct, commit updated presets, delete script

**Speed fields to multiply by 60**:
```
effects.flowField.rotationSpeed
effects.flowField.rotationSpeedRadial
effects.kaleidoscope.rotationSpeed
effects.kifs.rotationSpeed
effects.kifs.twistSpeed
effects.latticeFold.rotationSpeed
effects.attractorFlow.rotationSpeedX
effects.attractorFlow.rotationSpeedY
effects.attractorFlow.rotationSpeedZ
effects.poincareDisk.rotationSpeed
effects.poincareDisk.translationSpeed
effects.halftone.rotationSpeed
effects.mandelbox.rotationSpeed
effects.mandelbox.twistSpeed
effects.triangleFold.rotationSpeed
effects.triangleFold.twistSpeed
effects.domainWarp.driftSpeed
effects.phyllotaxis.angleSpeed
effects.phyllotaxis.phaseSpeed
drawables[*].base.rotationSpeed
```

**Already time-based (do NOT migrate)**:
- `effects.sineWarp.animRate` - radians/second
- `lfos[*].rate` - Hz

**Done when**: All presets updated, script deleted.

---

## Phase 5: Config Default Updates

**Goal**: Update default values in config structs to reasonable time-based defaults.

**Build**:
- `src/config/kaleidoscope_config.h` - `rotationSpeed = 0.0f` (was 0.002)
- `src/config/kifs_config.h` - `rotationSpeed = 0.0f`, `twistSpeed = 0.0f`
- `src/config/lattice_fold_config.h` - `rotationSpeed = 0.0f` (was 0.002)
- `src/config/mandelbox_config.h` - `rotationSpeed = 0.0f`, `twistSpeed = 0.0f`
- `src/config/triangle_fold_config.h` - `rotationSpeed = 0.0f`, `twistSpeed = 0.0f`

Note: Many configs already default to 0.0f; only update those with non-zero defaults that would become too fast.

**Done when**: Default config values produce reasonable initial behavior.

---

## Phase 6: Verification

**Goal**: Confirm all changes work correctly.

**Build**:
- Build project, fix any compile errors
- Load each preset, verify visual behavior matches pre-change appearance
- Test modulation on speed params (should still work)
- Test changing speed mid-animation (no jumps, smooth transition)
- Verify UI shows °/s and slider range feels usable (0-180°/s max)

**Done when**: All effects function correctly with new time-based speeds.
