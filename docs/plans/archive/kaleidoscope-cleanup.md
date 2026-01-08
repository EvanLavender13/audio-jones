# Kaleidoscope Cleanup: Extract Droste and Power Map

Extract Droste and Power Map from kaleidoscope shader. Droste merges into Infinite Zoom (both use log-polar depth). Power Map moves to new Conformal Warp effect.

## Current State

Kaleidoscope contains 6 techniques, but only 4 create symmetry:

| Technique | True Kaleidoscope? | Destination |
|-----------|-------------------|-------------|
| Polar, KIFS, Iter Mirror, Hex Fold | Yes | Stay |
| Droste | No (log-polar spiral) | Infinite Zoom |
| Power Map | No (conformal z^n) | New Conformal Warp |

**Key files:**
- `src/config/kaleidoscope_config.h:10,13,33,39` - Droste/PowerMap fields
- `shaders/kaleidoscope.fs:24-27,34-35,42,191-221,292-318` - Shader functions
- `src/render/post_effect.h:53,54,56,60,62` - Uniform locations
- `src/render/shader_setup.cpp:133-134,139-140,169-171,177-179` - SetShaderValue calls
- `src/ui/imgui_effects.cpp:325,332,348-358,373-399` - UI sliders
- `src/config/preset.cpp:102-106` - Serialization
- `src/automation/param_registry.cpp:44,47,48,50,112,115,116,118` - Modulation targets

**Preset compatibility:** Old presets with `kaleidoscope.drosteIntensity` or `kaleidoscope.powerMapIntensity` silently lose those values. Users manually reconfigure.

---

## Phase 1: Remove from Kaleidoscope

**Goal**: Strip Droste and Power Map from kaleidoscope, leaving only true symmetry techniques.

**Modify:**

`src/config/kaleidoscope_config.h` - Remove fields:
- `float drosteIntensity`
- `float drosteScale`
- `float powerMapIntensity`
- `float powerMapN`

`shaders/kaleidoscope.fs` - Remove:
- Uniforms: `drosteIntensity`, `drosteScale`, `powerMapIntensity`, `powerMapN`
- Function `sampleDroste()` (~lines 191-221)
- Function `samplePowerMap()` (~lines 292-318)
- Droste/PowerMap branches in `main()` total intensity calculation and sampling

`src/render/post_effect.h` - Remove uniform location fields:
- `kaleidoDrosteIntensityLoc`, `kaleidoDrosteScaleLoc`
- `kaleidoPowerMapIntensityLoc`, `kaleidoPowerMapNLoc`

`src/render/post_effect.cpp` - Remove GetShaderLocation calls for above uniforms

`src/render/shader_setup.cpp` - Remove from `SetupKaleido()`:
- SetShaderValue for drosteIntensity, drosteScale
- SetShaderValue for powerMapIntensity, powerMapN

`src/ui/imgui_effects.cpp` - Remove from kaleidoscope section:
- Droste toggle button and drosteActive variable
- Power toggle button and powerActive variable
- Droste section (scale slider)
- Power section (exponent slider)
- Update activeCount calculation

`src/config/preset.cpp` - Remove from KaleidoscopeConfig serialization macro:
- `drosteIntensity`, `drosteScale`, `powerMapIntensity`, `powerMapN`

`src/automation/param_registry.cpp` - Remove entries:
- `kaleidoscope.drosteIntensity`, `kaleidoscope.drosteScale`
- `kaleidoscope.powerMapIntensity`, `kaleidoscope.powerMapN`

**Done when**: Kaleidoscope builds, renders only Polar/KIFS/IterMirror/HexFold, no droste/powerMap references remain.

---

## Phase 2: Add Droste to Infinite Zoom

**Goal**: Extend Infinite Zoom with Droste blend mode using log-polar spiral transformation.

**Modify:**

`src/config/infinite_zoom_config.h` - Add fields:
```cpp
float drosteIntensity = 0.0f;  // 0 = standard zoom, 1 = full Droste
float drosteScale = 4.0f;      // Zoom ratio per revolution (2.0-256.0)
```

`src/render/post_effect.h` - Add uniform locations:
- `int infiniteZoomDrosteIntensityLoc`
- `int infiniteZoomDrosteScaleLoc`

`src/render/post_effect.cpp` - Add GetShaderLocation calls in `GetShaderUniformLocations()`

`src/render/shader_setup.cpp` - Add to `SetupInfiniteZoom()`:
```cpp
SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomDrosteIntensityLoc,
               &iz->drosteIntensity, SHADER_UNIFORM_FLOAT);
SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomDrosteScaleLoc,
               &iz->drosteScale, SHADER_UNIFORM_FLOAT);
```

`src/ui/imgui_effects.cpp` - Add to Infinite Zoom section:
- Droste Intensity slider (0.0-1.0)
- Droste Scale slider (2.0-256.0, logarithmic) - show only when intensity > 0

`src/config/preset.cpp` - Add to InfiniteZoomConfig serialization:
- `drosteIntensity`, `drosteScale`

`src/automation/param_registry.cpp` - Add modulation entries:
- `infiniteZoom.drosteIntensity` with range {0.0, 1.0}
- `infiniteZoom.drosteScale` with range {2.0, 256.0}

### Shader: `shaders/infinite_zoom.fs`

Add uniforms:
```glsl
uniform float drosteIntensity;  // Blend: 0=standard, 1=full Droste
uniform float drosteScale;      // Zoom ratio per spiral revolution
```

Add Droste sampling function (extracted from kaleidoscope):
```glsl
// Droste log-polar spiral transformation
// Maps UV to recursive self-similar spiral pattern
vec3 sampleDroste(vec2 uv, vec2 center)
{
    vec2 p = uv - center;
    float r = length(p) + 0.0001;
    float theta = atan(p.y, p.x);

    // Log-polar coordinates
    float logR = log(r);
    float period = log(drosteScale);

    // Spiral shear: rotation causes zoom progression
    // Single spiral arm (no segments parameter)
    logR += theta / TWO_PI * period;

    // Tile in log space for infinite repetition
    logR = mod(logR + period * 100.0, period);

    // Back to linear radius, normalized to [0, 0.5]
    float newR = exp(logR);
    newR = (newR - 1.0) / (drosteScale - 1.0 + 0.0001) * 0.5;

    // Animation via time-based rotation
    theta += time * 0.3;

    // Apply spiral twist if enabled
    if (spiralTwist != 0.0) {
        theta += log(newR + 0.001) * spiralTwist;
    }

    vec2 newUV = vec2(cos(theta), sin(theta)) * newR + center;
    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}
```

Modify `main()` to blend between standard layers and Droste:
```glsl
void main()
{
    vec2 center = vec2(0.5) + focalOffset;

    if (drosteIntensity >= 1.0) {
        // Pure Droste mode
        finalColor = vec4(sampleDroste(fragTexCoord, center), 1.0);
        return;
    }

    // Standard layered zoom (existing code)
    vec3 layeredColor = vec3(0.0);
    float weightAccum = 0.0;
    // ... existing layer loop ...

    if (drosteIntensity <= 0.0) {
        finalColor = vec4(layeredColor, 1.0);
        return;
    }

    // Blend between layered and Droste
    vec3 drosteColor = sampleDroste(fragTexCoord, center);
    finalColor = vec4(mix(layeredColor, drosteColor, drosteIntensity), 1.0);
}
```

**Done when**: Infinite Zoom renders standard layered zoom at drosteIntensity=0, pure Droste spiral at drosteIntensity=1, smooth blend between.

---

## Phase 3: Create Conformal Warp Effect

**Goal**: New transform effect containing Power Map, designed for future conformal techniques.

### Files to Create

`src/config/conformal_warp_config.h`:
```cpp
#ifndef CONFORMAL_WARP_CONFIG_H
#define CONFORMAL_WARP_CONFIG_H

struct ConformalWarpConfig {
    bool enabled = false;
    float powerMapN = 2.0f;       // Power exponent (0.5-8.0, snapped to 0.5 increments)
    float rotationSpeed = 0.0f;   // Pattern rotation (radians/frame)
    float focalAmplitude = 0.0f;  // Lissajous center offset
    float focalFreqX = 1.0f;
    float focalFreqY = 1.5f;
};

#endif // CONFORMAL_WARP_CONFIG_H
```

`shaders/conformal_warp.fs`:
```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float powerMapN;      // Power exponent
uniform float rotation;       // CPU-accumulated rotation (radians)
uniform vec2 focalOffset;     // Lissajous center offset

const float TWO_PI = 6.28318530718;

// 2D rotation helper
vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

void main()
{
    vec2 uv = fragTexCoord - 0.5 - focalOffset;

    // Apply rotation
    uv = rotate2d(uv, rotation);

    // Complex power: z^n in polar form
    // r^n, angle*n
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    float newR = pow(r + 0.001, powerMapN);
    float newAngle = a * powerMapN;

    vec2 newUV = vec2(cos(newAngle), sin(newAngle)) * newR * 0.5 + 0.5;

    finalColor = texture(texture0, clamp(newUV, 0.0, 1.0));
}
```

### Files to Modify

`src/config/effect_config.h`:
- Add include: `#include "conformal_warp_config.h"`
- Add enum entry: `TRANSFORM_CONFORMAL_WARP` (before `TRANSFORM_EFFECT_COUNT`)
- Add to `TransformEffectName()` switch
- Add to `TransformOrderConfig::order[]` default
- Add config member: `ConformalWarpConfig conformalWarp;`

`src/render/post_effect.h` - Add:
- `Shader conformalWarpShader;`
- Uniform locations: `conformalWarpPowerMapNLoc`, `conformalWarpRotationLoc`, `conformalWarpFocalLoc`
- Runtime state: `float conformalWarpRotation;`, `float conformalWarpFocal[2];`

`src/render/post_effect.cpp`:
- Load shader in `LoadPostEffectShaders()`
- Get uniform locations in `GetShaderUniformLocations()`
- Unload shader in `PostEffectUninit()`
- Initialize rotation to 0.0f in `PostEffectInit()`

`src/render/render_pipeline.cpp`:
- Accumulate rotation: `pe->conformalWarpRotation += pe->effects.conformalWarp.rotationSpeed;`
- Compute Lissajous focal offset like kaleidoscope

`src/render/shader_setup.h` - Add:
- `void SetupConformalWarp(PostEffect* pe);`

`src/render/shader_setup.cpp`:
- Add case in `GetTransformEffect()`: `TRANSFORM_CONFORMAL_WARP`
- Implement `SetupConformalWarp()` function

`src/ui/imgui_effects.cpp` - Add Conformal Warp section:
- Enabled checkbox
- Power Map N slider (integer steps snapped to 0.5, displayed as float)
- Rotation speed slider (angle in degrees/frame)
- Focal offset tree node (amplitude, freqX, freqY)

`src/config/preset.cpp`:
- Add serialization macro for ConformalWarpConfig
- Add to `to_json`/`from_json` for EffectConfig

`src/automation/param_registry.cpp`:
- Add entries: `conformalWarp.powerMapN`, `conformalWarp.rotationSpeed`, etc.

**Done when**: Conformal Warp appears in transform order, renders Power Map effect, saves/loads in presets, modulation works.

---

## Verification

1. Kaleidoscope: Only Polar, KIFS, Iterative Mirror, Hex Fold techniques
2. Infinite Zoom: Droste blend mode (intensity 0-1, scale 2-256)
3. Conformal Warp: New effect with Power Map (N 0.5-8.0)
4. All effects render correctly
5. Modulation routes updated
6. Presets save/load without crash

---

## Sources

- [Escher Droste Shadertoy](https://www.shadertoy.com/view/Mdf3zM) - Reference Droste implementation
- [Roy's Droste Blog](http://roy.red/posts/droste/) - Log-polar math explanation
- `docs/research/conformal-warps-and-domain-distortion.md` - Poincare, Joukowski (future)
- `docs/research/conformal-uv-warping.md` - Power map, complex functions
