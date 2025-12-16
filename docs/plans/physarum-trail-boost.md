# Physarum Trail Boost

Transform the physarum trail map from a visible grayscale overlay into an invisible intensity multiplier that boosts colors passing over it.

## Goal

Make the trail map act as a post-process color enhancer rather than a direct visual element. Colors in the main texture get brightened proportionally to trail intensity, revealing the trail pattern only where colored content exists. This creates an organic, reactive glow effect without the trails themselves being directly visible.

Key constraint: **prevent white overflow**. Previous attempts failed because multiplicative boosting in the HDR feedback loop caused exponential brightness growth.

## Current State

### Trail Map Rendering

The trail map currently renders as a grayscale debug overlay early in the pipeline:

- `src/render/post_effect.cpp:127-140` - `ApplyPhysarumPass()` calls `PhysarumDrawDebug()`
- `shaders/physarum_debug.fs:11-15` - converts R32F intensity to RGB grayscale
- `src/render/physarum.h:69-70` - `PhysarumDrawDebug()` declaration

### Pipeline Order

From `src/render/post_effect.cpp:272-279`:

```
1. ApplyPhysarumPass()    ← draws trail as grayscale overlay
2. ApplyVoronoiPass()     ← multiplicative boost on edges
3. ApplyFeedbackPass()    ← zoom + rotation
4. ApplyBlurPass()        ← gaussian blur + decay
5. [Waveforms drawn]
6. Kaleidoscope
7. Chromatic → Screen
```

### Voronoi Boost Pattern

The voronoi shader at `shaders/voronoi.fs:71-75` provides the reference implementation:

```glsl
vec3 original = min(texture(texture0, fragTexCoord).rgb, vec3(1.0));  // Pre-clamp
vec3 boosted = original * (1.0 + edgeMask * intensity);
```

Problem: This works for thin voronoi edges but causes white overflow for large trail areas because:
1. Trails cover more pixels than thin edges
2. Boosted pixels feed back into next frame's input
3. Pre-clamp alone doesn't prevent saturation—it just prevents runaway growth

## Algorithm

### Headroom-Limited Boost (Primary Approach)

Only boost as much as there's room before hitting 1.0:

```glsl
uniform sampler2D texture0;      // Main accumulation texture
uniform sampler2D trailMap;      // Physarum R32F trail intensity
uniform float boostStrength;     // User-controlled 0.0-2.0

void main() {
    // Pre-clamp to prevent HDR feedback runaway
    vec3 original = min(texture(texture0, fragTexCoord).rgb, vec3(1.0));
    float trail = texture(trailMap, fragTexCoord).r;

    // Calculate headroom (distance to white)
    float maxChan = max(original.r, max(original.g, original.b));
    float headroom = 1.0 - maxChan;

    // Boost proportional to available headroom
    // Dark pixels boost more, bright pixels boost less
    float safeBoost = trail * boostStrength * headroom;
    vec3 boosted = original + original * safeBoost;

    finalColor = vec4(boosted, 1.0);
}
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| boostStrength | 0.0-2.0 | 0 = off, 1 = moderate glow, 2 = strong enhancement |
| headroom | 1.0 - maxChannel | Prevents ANY output from exceeding 1.0 |

### Fallback: Soft Saturation Curve

If headroom-limiting looks wrong (e.g., too little effect on bright content), use Reinhard tone mapping:

```glsl
vec3 boosted = original * (1.0 + trail * boostStrength);
vec3 result = boosted / (boosted + vec3(1.0));  // Asymptotic clamp
result *= 2.0;  // Rescale since Reinhard caps at 0.5
```

This allows stronger visible boost but with smooth rolloff instead of hard clipping.

## Integration

### New Files

1. **`shaders/physarum_boost.fs`** - Fragment shader implementing headroom-limited boost
2. No new C++ files—integration uses existing patterns

### Modified Files

#### `src/render/post_effect.cpp`

Add `ApplyTrailBoostPass()` following the voronoi pattern:

```cpp
static void ApplyTrailBoostPass(PostEffect* pe)
{
    if (pe->physarum == NULL || pe->effects.physarum.boostIntensity <= 0.0f) {
        return;
    }

    BeginTextureMode(pe->tempTexture);
    BeginShaderMode(pe->trailBoostShader);
        // Bind trail map as second texture
        SetShaderValueTexture(pe->trailBoostShader, pe->trailMapLoc,
                              pe->physarum->trailMap.texture);
        SetShaderValue(pe->trailBoostShader, pe->trailBoostIntensityLoc,
                       &pe->effects.physarum.boostIntensity, SHADER_UNIFORM_FLOAT);
        DrawFullscreenQuad(pe, &pe->accumTexture);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(pe->accumTexture);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    EndTextureMode();
}
```

Call from `PostEffectBeginAccum()` at line ~272, either before or after `ApplyVoronoiPass()`.

#### `src/render/post_effect.h`

Add to `PostEffect` struct (~line 32):

```cpp
Shader trailBoostShader;
int trailMapLoc;
int trailBoostIntensityLoc;
```

#### `src/render/physarum.h`

Add to `PhysarumConfig` struct (line 14-24):

```cpp
float boostIntensity = 0.0f;    // Trail boost strength (0.0-2.0)
bool debugOverlay = false;       // Show grayscale debug visualization
```

#### `src/render/post_effect.cpp` - ApplyPhysarumPass

Modify to respect `debugOverlay` toggle:

```cpp
static void ApplyPhysarumPass(PostEffect* pe, float deltaTime)
{
    if (pe->physarum == NULL || !pe->effects.physarum.enabled) {
        return;
    }

    PhysarumApplyConfig(pe->physarum, &pe->effects.physarum);
    PhysarumUpdate(pe->physarum, deltaTime);
    PhysarumProcessTrails(pe->physarum, deltaTime);

    // Only draw debug overlay if explicitly enabled
    if (pe->effects.physarum.debugOverlay) {
        BeginTextureMode(pe->accumTexture);
        PhysarumDrawDebug(pe->physarum);
        EndTextureMode();
    }
}
```

#### `src/ui/ui_panel_effects.cpp`

Add slider for boost intensity in physarum section:

```cpp
DrawLabeledSlider(l, "Trail Boost", &effects->physarum.boostIntensity, 0.0f, 2.0f);
DrawLabeledCheckbox(l, "Debug Overlay", &effects->physarum.debugOverlay);
```

## File Changes

| File | Change |
|------|--------|
| `shaders/physarum_boost.fs` | Create - headroom-limited boost shader |
| `src/render/post_effect.h` | Add shader handle and uniform locations |
| `src/render/post_effect.cpp` | Add `ApplyTrailBoostPass()`, modify `ApplyPhysarumPass()` for debug toggle, load new shader |
| `src/render/physarum.h` | Add `boostIntensity` and `debugOverlay` to config |
| `src/ui/ui_panel_effects.cpp` | Add boost slider and debug checkbox |
| `src/config/preset.cpp` | Add serialization for new config fields |

## Build Sequence

### Phase 1: Shader and Basic Integration

1. Create `shaders/physarum_boost.fs` with headroom-limited algorithm
2. Add shader loading in `PostEffectInit()`
3. Add `ApplyTrailBoostPass()` function
4. Call from `PostEffectBeginAccum()` after physarum update
5. Test with hardcoded `boostIntensity = 1.0`

### Phase 2: Config and UI

1. Add `boostIntensity` and `debugOverlay` to `PhysarumConfig`
2. Modify `ApplyPhysarumPass()` to respect `debugOverlay`
3. Add UI controls in effects panel
4. Add preset serialization

### Phase 3: Tuning (if needed)

If headroom-limiting doesn't look right:
1. Try the soft saturation fallback
2. Adjust boost range (maybe 0-3 instead of 0-2)
3. Consider combining both approaches

## Validation

- [ ] Trail map is invisible when `boostIntensity = 0` and `debugOverlay = false`
- [ ] Colors brighten proportionally to trail intensity when boost is enabled
- [ ] No white overflow even at maximum boost with high trail activity
- [ ] Debug overlay toggle works independently of boost
- [ ] Effect is interchangeable with voronoi (similar pipeline position)
- [ ] UI slider controls boost strength smoothly
- [ ] Presets save and restore boost settings

## Overflow Prevention Summary

| Technique | How It Works | Trade-off |
|-----------|--------------|-----------|
| Pre-clamp input | `min(color, 1.0)` before boost | Prevents runaway but not saturation |
| Headroom-limited | Boost only up to available headroom | Dark content boosts more than bright |
| Soft saturation | Reinhard tone mapping | Smooth rolloff, affects overall contrast |
| Hard output clamp | `min(result, 1.0)` | Can look flat/washed |

This plan uses **headroom-limited** as primary, with **soft saturation** as fallback if visual results are unsatisfactory.
