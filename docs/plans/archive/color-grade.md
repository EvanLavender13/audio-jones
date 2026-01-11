# Color Grade

Full-spectrum color manipulation as a reorderable transform effect. Includes hue rotation, saturation, brightness/exposure, log-space contrast, temperature, and lift/gamma/gain (shadows/midtones/highlights). All 8 parameters support audio modulation.

## Current State

Relevant integration points:
- `src/config/effect_config.h:25-44` - TransformEffectType enum, needs new entry
- `src/config/effect_config.h:69-92` - TransformOrderConfig, needs new entry
- `src/render/post_effect.h:13-208` - PostEffect struct, needs shader + uniform locs
- `src/render/shader_setup.cpp:10-50` - GetTransformEffect switch, needs new case
- `src/ui/imgui_effects_transforms.cpp:293-403` - DrawStyleCategory, add section
- `src/automation/param_registry.cpp:12-91` - PARAM_TABLE, register 8 params

## Technical Implementation

**Source**: [docs/research/color-grade.md](../research/color-grade.md) - GPU Gems, Filmic Worlds, Catlike Coding

### Shader Algorithm

Operations applied in this order (from research doc):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float hueShift;        // 0-1 (normalized from 0-360 degrees)
uniform float saturation;      // 0-2, default 1
uniform float brightness;      // -2 to +2 F-stops, default 0
uniform float contrast;        // 0.5-2, default 1
uniform float temperature;     // -1 to +1, default 0
uniform float shadowsOffset;   // -0.5 to +0.5, default 0
uniform float midtonesOffset;  // -0.5 to +0.5, default 0
uniform float highlightsOffset;// -0.5 to +0.5, default 0

// RGB to HSV (GPU Gems optimized)
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Hue shift in HSV space
vec3 applyHueShift(vec3 color, float shift) {
    vec3 hsv = rgb2hsv(color);
    hsv.x = fract(hsv.x + shift);
    return hsv2rgb(hsv);
}

// Saturation via luminance lerp (Filmic Worlds weights)
vec3 applySaturation(vec3 color, float sat) {
    vec3 lumaWeights = vec3(0.25, 0.50, 0.25);
    float grey = dot(lumaWeights, color);
    return vec3(grey) + sat * (color - vec3(grey));
}

// Exposure in F-stops
vec3 applyBrightness(vec3 color, float exposure) {
    return color * exp2(exposure);
}

// Log-space contrast around 18% grey (Filmic Worlds)
float logContrast(float x, float con) {
    float eps = 1e-6;
    float logMidpoint = log2(0.18);
    float logX = log2(x + eps);
    float adjX = logMidpoint + (logX - logMidpoint) * con;
    return max(0.0, exp2(adjX) - eps);
}

vec3 applyContrast(vec3 color, float con) {
    return vec3(
        logContrast(color.r, con),
        logContrast(color.g, con),
        logContrast(color.b, con)
    );
}

// Temperature: warm/cool RGB scaling
vec3 applyTemperature(vec3 color, float temp) {
    float warmth = temp * 0.3;
    return color * vec3(1.0 + warmth, 1.0, 1.0 - warmth);
}

// Lift/Gamma/Gain per channel (Filmic Worlds)
float liftGammaGain(float x, float lift, float gamma, float gain) {
    float lerpV = clamp(pow(x, gamma), 0.0, 1.0);
    return gain * lerpV + lift * (1.0 - lerpV);
}

vec3 applyShadowsMidtonesHighlights(vec3 color, float shadows, float midtones, float highlights) {
    float lift = shadows;
    float gain = 1.0 + highlights;
    float gamma = 1.0 / max(0.01, 1.0 + midtones);
    return vec3(
        liftGammaGain(color.r, lift, gamma, gain),
        liftGammaGain(color.g, lift, gamma, gain),
        liftGammaGain(color.b, lift, gamma, gain)
    );
}

void main() {
    vec3 color = texture(texture0, fragTexCoord).rgb;

    // 1. Brightness/Exposure (linear operation first)
    color = applyBrightness(color, brightness);

    // 2. Temperature shift
    color = applyTemperature(color, temperature);

    // 3. Contrast (log space preserves shadows)
    color = applyContrast(color, contrast);

    // 4. Saturation
    color = applySaturation(color, saturation);

    // 5. Lift/Gamma/Gain
    color = applyShadowsMidtonesHighlights(color, shadowsOffset, midtonesOffset, highlightsOffset);

    // 6. Hue shift (last - operates on final color relationships)
    color = applyHueShift(color, hueShift);

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
```

### Parameters

| Param | Range | Default | UI Display |
|-------|-------|---------|------------|
| hueShift | 0-1 | 0 | 0-360 degrees |
| saturation | 0-2 | 1 | direct |
| brightness | -2 to +2 | 0 | F-stops |
| contrast | 0.5-2 | 1 | direct |
| temperature | -1 to +1 | 0 | cool/warm |
| shadowsOffset | -0.5 to +0.5 | 0 | lift |
| midtonesOffset | -0.5 to +0.5 | 0 | gamma |
| highlightsOffset | -0.5 to +0.5 | 0 | gain |

---

## Phase 1: Config and Shader

**Goal**: Create config struct and shader file.

**Build**:
- Create `src/config/color_grade_config.h` with ColorGradeConfig struct (8 float params + enabled bool)
- Create `shaders/color_grade.fs` with full algorithm from above

**Done when**: Files compile standalone (header guards correct, shader syntax valid).

---

## Phase 2: Effect Integration

**Goal**: Wire Color Grade into the transform pipeline.

**Build**:
- `src/config/effect_config.h`: Add `#include`, `TRANSFORM_COLOR_GRADE` enum entry, name in `TransformEffectName()`, entry in `TransformOrderConfig::order`, `ColorGradeConfig colorGrade` member in EffectConfig
- `src/render/post_effect.h`: Add `Shader colorGradeShader` and 8 uniform location ints (`colorGradeHueShiftLoc`, etc.)
- `src/render/post_effect.cpp`: Load shader in `LoadPostEffectShaders()`, get uniform locations in `GetShaderUniformLocations()`, unload in `PostEffectUninit()`

**Done when**: App compiles and loads shader without error (verify in console log).

---

## Phase 3: Shader Setup and Pipeline

**Goal**: Connect shader uniforms and add to transform execution.

**Build**:
- `src/render/shader_setup.h`: Declare `void SetupColorGrade(PostEffect* pe)`
- `src/render/shader_setup.cpp`: Implement `SetupColorGrade()` to set all 8 uniforms, add `TRANSFORM_COLOR_GRADE` case in `GetTransformEffect()`

**Done when**: Enabling Color Grade in code applies the effect (test by hardcoding `enabled = true`).

---

## Phase 4: UI and Modulation

**Goal**: Add user controls and audio modulation.

**Build**:
- `src/ui/imgui_effects_transforms.cpp`: Add static `sectionColorGrade` bool, add section in `DrawStyleCategory()` with checkbox and 8 sliders (use `ModulatableSlider` for all)
- `src/automation/param_registry.cpp`: Add 8 entries to `PARAM_TABLE` and `targets[]` array

**Done when**: UI controls appear and modulation routes can target Color Grade parameters.

---

## Phase 5: Serialization

**Goal**: Save/load Color Grade in presets.

**Build**:
- `src/config/preset.cpp`: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for ColorGradeConfig, add to_json/from_json entries

**Done when**: Preset save/load round-trips Color Grade settings correctly.
