# Hue Remap Rotation Speeds

Add rotation speed parameters to hue remap's angular and linear spatial fields, so the masking/shifting patterns spin around the image over time instead of remaining static.

## Classification

- **Category**: TRANSFORMS > Color (enhancement to existing `hue_remap` effect)
- **Pipeline Position**: Same as existing hue remap (color section, index 8)

## Algorithm

Pure rotation: accumulate angle offsets on the CPU (`accum += speed * deltaTime`), pass as uniforms, add to the spatial angle computations in the shader.

**Angular fields**: The shader currently computes `ang = atan(uv.y, uv.x)` and uses it directly in both `computeSpatialField` and `computeShiftField`. Pass accumulated offsets (`blendAngularOffset`, `shiftAngularOffset`) and add them to `ang` before the `sin(ang * freq)` calculation.

**Linear fields**: The shader currently uses `blendLinearAngle` and `shiftLinearAngle` as fixed direction angles. Pass accumulated offsets (`blendLinearOffset`, `shiftLinearOffset`) and add them to the respective angle uniforms before computing the gradient direction `vec2(cos(angle), sin(angle))`.

### Changes per layer

**Config struct** (`hue_remap.h`): 4 new `float` fields with `*Speed` suffix.

**Effect struct** (`hue_remap.h`): 4 new `float` accumulators for the accumulated offsets, 4 new `int` uniform locations.

**Setup** (`hue_remap.cpp`): Accumulate each offset (`accum += speed * deltaTime`), bind 4 new uniforms.

**Shader** (`hue_remap.fs`): 4 new `uniform float` declarations. In `main()`, compute offset-adjusted angles and pass them to the spatial field functions instead of raw `ang` / raw linear angles.

**Serialization** (`effect_serialization.cpp`): Covered automatically by `HUE_REMAP_CONFIG_FIELDS` macro update.

**Param registration** (`hue_remap.cpp`): 4 new `ModEngineRegisterParam` calls with `ROTATION_SPEED_MAX` bounds.

**UI** (`hue_remap.cpp`): 4 new `ModulatableSliderSpeedDeg()` calls, one below each corresponding Angular/Linear slider in both Blend Spatial and Shift Spatial sections.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| blendAngularSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Rotation rate of blend angular sector pattern (rad/s) |
| shiftAngularSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Rotation rate of shift angular sector pattern (rad/s) |
| blendLinearSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Rotation rate of blend linear gradient direction (rad/s) |
| shiftLinearSpeed | float | -ROTATION_SPEED_MAX to +ROTATION_SPEED_MAX | 0.0 | Rotation rate of shift linear gradient direction (rad/s) |

## Modulation Candidates

- **blendAngularSpeed**: Spinning blend mask — audio pulses spin the "where" of the remap
- **shiftAngularSpeed**: Spinning shift offset — audio pulses spin the "what hue" of the remap
- **blendLinearSpeed**: Sweeping linear blend mask across the image
- **shiftLinearSpeed**: Sweeping linear shift offset across the image

### Interaction Patterns

**Competing forces (blend vs shift angular speeds)**: When `blendAngularSpeed` and `shiftAngularSpeed` run at different rates, the "where the remap applies" and "what hue offset it applies" drift in and out of alignment. Aligned moments produce sharp color bands; misaligned moments blur the effect. Modulating speeds with different audio sources creates song-structure-reactive dynamics — verse and chorus produce distinct visual states.

**Resonance (angular speed + angular coefficient)**: The angular speed only produces visible motion when the corresponding angular coefficient (`blendAngular` or `shiftAngular`) is nonzero. Modulating both creates a gating effect where the pattern only spins when the coefficient is active.

## Notes

- Follows the `*Speed` suffix convention: radians/second, accumulated on CPU with `accum += speed * deltaTime`
- Accumulated offsets are passed as shader uniforms (not the speeds themselves), ensuring smooth transitions when speed changes via modulation
- The `angularFreq` parameter interacts multiplicatively with rotation — higher freq creates more sectors that appear to spin faster visually (same angular velocity, more pattern repeats)
- Zero default for all speeds preserves existing static behavior
