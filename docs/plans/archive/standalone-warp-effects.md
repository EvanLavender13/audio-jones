# Standalone Warp Effects

Implements 5 standalone warp effects from [conformal-warps-and-domain-distortion.md](../research/conformal-warps-and-domain-distortion.md) plus renames existing "Conformal Warp" to "Power Map" for clarity.

## Current State

Existing effect infrastructure to modify:
- `src/config/conformal_warp_config.h` - rename to power_map_config.h
- `src/config/effect_config.h:23` - TRANSFORM_CONFORMAL_WARP enum
- `src/render/post_effect.h:32` - conformalWarpShader field
- `src/render/post_effect.cpp:42` - shader loading
- `src/ui/imgui_effects.cpp:436` - "Conformal Warp" UI section
- `src/automation/param_registry.cpp:60` - conformalWarp params
- `shaders/conformal_warp.fs` - rename to power_map.fs

New files to create (per effect):
- `shaders/<name>.fs`
- `src/config/<name>_config.h`

Integration points (touched once per phase):
- `src/config/effect_config.h` - add to TransformEffectType enum + EffectConfig struct
- `src/render/post_effect.h` - add Shader field + uniform location ints
- `src/render/post_effect.cpp` - LoadShader + GetShaderLocation + UnloadShader
- `src/render/shader_setup.cpp` - SetShaderValue calls
- `src/render/render_pipeline.cpp` - RenderPass callback
- `src/ui/imgui_effects.cpp` - UI section
- `src/automation/param_registry.cpp` - modulatable params
- `src/config/preset.cpp` - JSON serialization

## Technical Implementation

### Complex Number Primitives (shared)

```glsl
vec2 cx_mul(vec2 a, vec2 b) {
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

vec2 cx_div(vec2 a, vec2 b) {
    float denom = dot(b, b) + 0.0001;
    return vec2(a.x*b.x + a.y*b.y, a.y*b.x - a.x*b.y) / denom;
}

vec2 cx_exp(vec2 z) {
    return exp(z.x) * vec2(cos(z.y), sin(z.y));
}

vec2 cx_sin(vec2 z) {
    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}
```

### Poincare Disk

Hyperbolic compression toward unit disk boundary. Creates "infinite detail at edges" aesthetic.

```glsl
// UV transform (center at origin, scale to [-1,1])
vec2 d = uv - center;
float r = length(d);
if (r < 0.001) return uv;  // Avoid division by zero

float tanhK = tanh(curvature);
float newR = tanh(r * curvature) / tanhK;
vec2 newUV = center + normalize(d) * newR;
```

**Parameters:**
| Param | Range | Effect |
|-------|-------|--------|
| `curvature` | -3.0 to 3.0 | Compression intensity (negative = expand from center) |
| `centerX/Y` | -0.5 to 0.5 | Focal point offset |

### Joukowski Transform

Maps circles to airfoil shapes. Creates elongated, asymmetric bulging.

```glsl
// z + c/z where c is strength coefficient
vec2 z = (uv - 0.5) * scale;
vec2 result = z + cx_div(vec2(strength, 0.0), z);
// Remap back to UV with clamping
```

**Parameters:**
| Param | Range | Effect |
|-------|-------|--------|
| `strength` | 0.0 to 2.0 | Elongation intensity (0 = identity) |
| `scale` | 1.0 to 4.0 | Input domain scaling |
| `rotationSpeed` | rad/frame | Pattern rotation |

### Exponential Map

Converts vertical lines to circles, horizontal to radial rays. Creates spiraling/radiating patterns.

```glsl
// exp(z) with input scaling to prevent explosion
vec2 z = (uv - 0.5) * scale;
vec2 result = cx_exp(z);
// Remap: 0.5 + result * outputScale (needs clamping)
```

**Parameters:**
| Param | Range | Effect |
|-------|-------|--------|
| `scale` | 0.5 to 3.0 | Input domain scaling (lower = less extreme) |
| `outputScale` | 0.1 to 0.5 | Output remapping factor |
| `rotationSpeed` | rad/frame | Pattern rotation |

### Sine Map

Creates oscillating bands with hyperbolic growth along imaginary axis.

```glsl
// sin(z) with input scaling
vec2 z = (uv - 0.5) * scale;
vec2 result = cx_sin(z);
// Remap with smooth falloff
```

**Parameters:**
| Param | Range | Effect |
|-------|-------|--------|
| `scale` | 1.0 to 6.0 | Input domain scaling (higher = more bands) |
| `outputScale` | 0.2 to 0.5 | Output remapping factor |
| `rotationSpeed` | rad/frame | Pattern rotation |

### Texture Warp

Self-referential distortion using texture's own pixel values as UV offsets.

```glsl
vec2 warpedUV = uv;
for (int i = 0; i < iterations; i++) {
    vec3 sample = texture(tex, warpedUV).rgb;
    vec2 offset = (sample.rg - 0.5) * 2.0;  // Remap [0,1] to [-1,1]
    warpedUV += offset * strength;
}
return texture(tex, warpedUV);
```

**Parameters:**
| Param | Range | Effect |
|-------|-------|--------|
| `strength` | 0.0 to 0.3 | Warp magnitude per iteration |
| `iterations` | 1 to 8 | Cascade depth (more = feedback-like) |

---

## Phase 1: Rename Power Map

**Goal**: Rename "Conformal Warp" to "Power Map" for semantic clarity.

**Build**:
- Rename `shaders/conformal_warp.fs` → `shaders/power_map.fs`
- Rename `src/config/conformal_warp_config.h` → `src/config/power_map_config.h`
- Rename struct `ConformalWarpConfig` → `PowerMapConfig`
- Update `effect_config.h`: enum `TRANSFORM_CONFORMAL_WARP` → `TRANSFORM_POWER_MAP`, include, member
- Update `post_effect.h/cpp`: field names `conformalWarp*` → `powerMap*`
- Update `shader_setup.cpp`: uniform setters
- Update `render_pipeline.cpp`: pass references
- Update `imgui_effects.cpp`: UI section title and variable names
- Update `param_registry.cpp`: param IDs `conformalWarp.*` → `powerMap.*`
- Update `preset.cpp`: JSON keys

**Done when**: Effect works identically, all references renamed, no "conformal" strings remain (except comments).

---

## Phase 2: Poincare + Texture Warp

**Goal**: Implement highest-value standalone warps.

**Build**:

*Poincare:*
- Create `shaders/poincare.fs` with tanh compression transform
- Create `src/config/poincare_config.h` with: enabled, curvature, centerX, centerY
- Add `TRANSFORM_POINCARE` to enum, `PoincarConfig poincare` to EffectConfig
- Wire through post_effect, shader_setup, render_pipeline
- Add UI section in imgui_effects
- Register `poincare.curvature` as modulatable param

*Texture Warp:*
- Create `shaders/texture_warp.fs` with iterative RG-offset loop
- Create `src/config/texture_warp_config.h` with: enabled, strength, iterations
- Add `TRANSFORM_TEXTURE_WARP` to enum, `TextureWarpConfig textureWarp` to EffectConfig
- Wire through post_effect, shader_setup, render_pipeline
- Add UI section in imgui_effects
- Register `textureWarp.strength` as modulatable param

**Done when**: Both effects render correctly, UI controls work, modulation functional.

---

## Phase 3: Joukowski + Exponential

**Goal**: Implement conformal complex function pair.

**Build**:

*Joukowski:*
- Create `shaders/joukowski.fs` with z + c/z transform
- Create `src/config/joukowski_config.h` with: enabled, strength, scale, rotationSpeed, focalAmplitude/FreqX/FreqY
- Add to enum, EffectConfig, wire through all integration points
- UI section with strength slider, scale, rotation speed
- Register `joukowski.strength`, `joukowski.rotationSpeed` as modulatable

*Exponential:*
- Create `shaders/exp_map.fs` with cx_exp transform
- Create `src/config/exp_map_config.h` with: enabled, scale, outputScale, rotationSpeed
- Add to enum, EffectConfig, wire through all integration points
- UI section
- Register `expMap.scale`, `expMap.rotationSpeed` as modulatable

**Done when**: Both effects render correctly with proper singularity handling.

---

## Phase 4: Sine Map

**Goal**: Complete the conformal function set.

**Build**:
- Create `shaders/sin_map.fs` with cx_sin transform
- Create `src/config/sin_map_config.h` with: enabled, scale, outputScale, rotationSpeed
- Add `TRANSFORM_SIN_MAP` to enum, `SinMapConfig sinMap` to EffectConfig
- Wire through all integration points
- UI section
- Register `sinMap.scale`, `sinMap.rotationSpeed` as modulatable

**Done when**: Effect renders oscillating bands, integrates with transform order system.
