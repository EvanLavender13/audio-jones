# Physarum Visual Quality Exploration

Systematic experiments to identify which implementation differences cause AudioJones physarum to appear "smudgey" compared to ComputeShader's vibrant output.

## Background

`docs/research/physarum-audiojones-vs-computeshader.md` identifies six key differences between implementations. This plan tests each difference in isolation to measure visual impact.

## Current State

AudioJones physarum produces low-contrast, desaturated trails. ComputeShader produces dense worm textures and high-contrast networks (`ComputeShader/images/README.md:1`).

Key implementation points:

- Agent compute shader: `shaders/physarum_agents.glsl:131`
- Deposit blending: `shaders/physarum_agents.glsl:186-194`
- Diffuse/decay: `shaders/blur_v.fs:17-44`
- Pipeline order: `src/render/post_effect.cpp:269-272`

## Experiments

Run experiments sequentially. Capture screenshots at 30s intervals. Compare against ComputeShader reference images.

### Experiment 1: Additive Deposit Blending

**Hypothesis**: Lerp blending caps brightness too early, preventing trail accumulation.

**Change** (`shaders/physarum_agents.glsl:186-194`):

```glsl
// Current (lerp toward deposit color)
float blendFactor = clamp(depositAmount * 0.5, 0.0, 1.0);
vec3 blended = mix(current.rgb, depositColor, blendFactor);
blended = min(blended + depositColor * 0.1, vec3(1.0));

// Test A: Pure additive (matches ComputeShader)
vec3 blended = current.rgb + depositColor * depositAmount;

// Test B: Additive with soft clamp
vec3 blended = current.rgb + depositColor * depositAmount;
blended = blended / (1.0 + blended);  // Reinhard tonemap
```

**Observe**: Trail brightness accumulation, color saturation in overlap regions, white blowout frequency.

### Experiment 2: Box Blur Diffusion

**Hypothesis**: Gaussian blur concentrates diffusion too tightly for organic spreading.

**Change** (`shaders/blur_v.fs:27-36`):

```glsl
// Current: 5-tap Gaussian [1,4,6,4,1]/16
// Test: 5-tap box [1,1,1,1,1]/5

result += texture(texture0, clamp(fragTexCoord + vec2(0.0, -2.0 * texelSize.y), 0.0, 1.0)).rgb * 0.2;
result += texture(texture0, clamp(fragTexCoord + vec2(0.0, -1.0 * texelSize.y), 0.0, 1.0)).rgb * 0.2;
result += texture(texture0, fragTexCoord).rgb * 0.2;
result += texture(texture0, clamp(fragTexCoord + vec2(0.0,  1.0 * texelSize.y), 0.0, 1.0)).rgb * 0.2;
result += texture(texture0, clamp(fragTexCoord + vec2(0.0,  2.0 * texelSize.y), 0.0, 1.0)).rgb * 0.2;
```

**Observe**: Trail edge sharpness, diffusion radius, filament continuity.

### Experiment 3: Bypass Feedback Pass

**Hypothesis**: Feedback desaturation transforms physarum trails before diffusion, washing out colors.

**Change** (`src/render/post_effect.cpp:269-272`):

```cpp
// Current order
ApplyPhysarumPass(pe, deltaTime);
ApplyVoronoiPass(pe);
ApplyFeedbackPass(pe, effectiveRotation);  // Includes desaturation
ApplyBlurPass(pe, blurScale, deltaTime);

// Test: Skip feedback when physarum enabled
ApplyPhysarumPass(pe, deltaTime);
ApplyVoronoiPass(pe);
if (!pe->effects.physarum.enabled) {
    ApplyFeedbackPass(pe, effectiveRotation);
}
ApplyBlurPass(pe, blurScale, deltaTime);
```

**Observe**: Color saturation preservation, trail contrast, interaction with other effects.

### Experiment 4: Increase Agent Count

**Hypothesis**: 100K agents produce sparse trails; 1M+ agents saturate the trail map faster.

**Change** (`src/config/effect_config.h` default or UI):

| Test | Agent Count | Notes |
|------|-------------|-------|
| Baseline | 100,000 | Current default |
| Test A | 500,000 | 5x increase |
| Test B | 1,000,000 | 10x increase |
| Test C | 8,000,000 | Match ComputeShader |

**Observe**: Trail density, filament continuity, GPU frame time impact.

### Experiment 5: Per-Frame Decay vs Half-Life

**Hypothesis**: Half-life decay behaves differently from ComputeShader's `(1 - decayAmount)` multiplier.

**Change** (`shaders/blur_v.fs:38-42`):

```glsl
// Current: framerate-independent half-life
float decayMultiplier = exp(-0.693147 * deltaTime / safeHalfLife);

// Test: fixed per-frame multiplier (ComputeShader style)
// With decayAmount = 0.1, multiplier = 0.9 per frame
uniform float decayAmount;
float decayMultiplier = 1.0 - decayAmount;
```

**Observe**: Trail persistence stability, visual difference at 30fps vs 60fps.

### Experiment 6: Wrap-Around Diffusion

**Hypothesis**: Edge clamping creates visible border absorption; wrap produces seamless continuity.

**Change** (`shaders/blur_v.fs:31-35`):

```glsl
// Current: clamp UVs at edges
result += texture(texture0, clamp(fragTexCoord + offset, 0.0, 1.0)).rgb * weight;

// Test: wrap UVs (requires TEXTURE_WRAP_REPEAT)
result += texture(texture0, fract(fragTexCoord + offset)).rgb * weight;
```

Also change `src/render/post_effect.cpp:41`:

```cpp
SetTextureWrap(tex->texture, TEXTURE_WRAP_REPEAT);  // Was TEXTURE_WRAP_CLAMP
```

**Observe**: Edge behavior, seamless tiling, interaction with feedback zoom.

### Experiment 7: Double-Buffered Trail Map

**Hypothesis**: In-place `imageLoad`/`imageStore` causes execution-order artifacts.

**Change**: Add dedicated physarum trail textures to `PostEffect` struct.

1. Add `physarumTrailA` and `physarumTrailB` render textures (`src/render/post_effect.h`)
2. Agents read from A, write to B (`shaders/physarum_agents.glsl`)
3. Swap A/B each frame (`src/render/post_effect.cpp`)
4. Copy result into `accumTexture` after swap

**Observe**: Trail stability, visual artifacts from concurrent writes.

## Combination Tests

After isolating individual effects, test promising combinations:

| Combo | Components |
|-------|------------|
| A | Additive deposit + box blur |
| B | Additive deposit + bypass feedback |
| C | Additive deposit + 1M agents + box blur |
| D | Full ComputeShader match (all changes) |

## Validation

For each experiment:

- [ ] Capture screenshot at t=0s, t=30s, t=60s, t=120s
- [ ] Note qualitative observations (contrast, saturation, filament shape)
- [ ] Record frame time impact (ms)
- [ ] Compare against `ComputeShader/images/` reference

## File Changes Summary

| File | Experiments |
|------|-------------|
| `shaders/physarum_agents.glsl` | 1, 7 |
| `shaders/blur_v.fs` | 2, 5, 6 |
| `shaders/blur_h.fs` | 2, 6 |
| `src/render/post_effect.cpp` | 3, 6, 7 |
| `src/render/post_effect.h` | 7 |

## Decision Criteria

Adopt changes that:

1. Produce visible improvement in trail contrast and saturation
2. Add <2ms to frame time at 1080p
3. Maintain compatibility with existing feedback/voronoi effects
4. Require minimal code complexity increase

## References

- `docs/research/physarum-audiojones-vs-computeshader.md` - Detailed implementation comparison
- `docs/research/physarum-simulation.md` - Jeff Jones algorithm background
- `ComputeShader/images/README.md` - Visual reference targets
