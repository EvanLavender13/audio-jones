# Heightfield Relief

Treats texture luminance as a heightfield, computes surface normals via Sobel gradient, and applies directional Lambertian lighting with specular highlights. Creates an embossed 3D appearance that enhances depth in images with structure (gradients, edges).

## Current State

- `shaders/toon.fs:72-89` - Existing Sobel 3x3 sampling pattern to reuse
- `src/config/toon_config.h` - Reference config struct pattern
- `src/config/effect_config.h:20-52` - TransformEffectType enum and name function
- `src/render/post_effect.h:36,147-152` - Shader + uniform location fields
- `src/render/post_effect.cpp:46,62,177-182,197,297` - Shader load/locate/resize/unload
- `src/render/shader_setup.cpp:35-36,366-378` - GetTransformEffect case + SetupToon
- `src/config/preset.cpp:181,208` - JSON serialization pattern
- `src/ui/imgui_effects.cpp:27,719-735` - Section state + Toon UI under "Style"
- `src/automation/param_registry.cpp:12-83,98-176` - Parameter registration

## Technical Implementation

### Source
- [docs/research/heightfield-relief.md](../research/heightfield-relief.md)
- Daniel Ilett Edge Detection Tutorial (Sobel)
- Chalmers University Heightfield Tutorial (normal from gradient)

### Core Algorithm

**Step 1: Sobel Gradients** (reuse pattern from `toon.fs`)
```glsl
// Sample 3x3 neighborhood
vec4 n[9];
n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
// ... n[1] through n[8] ...

// Sobel kernels produce horizontal/vertical gradients
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
```

**Step 2: Normal from Gradient**
```glsl
// Convert RGB gradients to luminance
float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));

// reliefScale controls steepness: higher = flatter surface
vec3 normal = normalize(vec3(-gx, -gy, reliefScale));
```

**Step 3: Directional Lighting**
```glsl
// Light direction from angle (radians) and height
vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));

// Lambertian diffuse with bias to avoid pure black
float diffuse = dot(normal, lightDir) * 0.5 + 0.5;
```

**Step 4: Specular Highlight**
```glsl
// Orthographic view assumption
vec3 viewDir = vec3(0.0, 0.0, 1.0);
vec3 halfDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
```

**Step 5: Final Blend**
```glsl
// Mix original with lit result
float lighting = diffuse + spec;
vec3 result = texColor.rgb * mix(1.0, lighting, intensity);
```

### Uniforms

| Uniform | Type | Description |
|---------|------|-------------|
| resolution | vec2 | Screen size for texel calculation |
| intensity | float | Blend 0-1 (0 = original, 1 = full effect) |
| reliefScale | float | Surface flatness (higher = subtler) |
| lightAngle | float | Light direction in radians |
| lightHeight | float | Light elevation (higher = more top-down) |
| shininess | float | Specular exponent (higher = tighter highlight) |

---

## Phase 1: Config and Shader

**Goal**: Create the config struct and fragment shader.

**Build**:
- Create `src/config/heightfield_relief_config.h`:
  - `bool enabled = false`
  - `float intensity = 0.7f` (range 0.0-1.0)
  - `float reliefScale = 1.0f` (range 0.1-5.0)
  - `float lightAngle = 0.785f` (π/4, range 0-2π, stored radians)
  - `float lightHeight = 0.5f` (range 0.1-2.0)
  - `float shininess = 32.0f` (range 1.0-128.0)

- Create `shaders/heightfield_relief.fs`:
  - Standard header: `#version 330`, inputs, uniforms
  - 3x3 Sobel sampling (copy pattern from `toon.fs:72-82`)
  - Normal computation from luminance gradient
  - Lambertian diffuse + specular (Blinn-Phong half-vector)
  - Intensity blend

**Done when**: Shader compiles (syntax check only, not wired yet).

---

## Phase 2: Pipeline Integration

**Goal**: Wire shader into render pipeline.

**Build**:
- Update `src/config/effect_config.h`:
  - Include `heightfield_relief_config.h`
  - Add `TRANSFORM_HEIGHTFIELD_RELIEF` to enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add case to `TransformEffectName()` returning `"Heightfield Relief"`
  - Add to `TransformOrderConfig::order[]` default
  - Add `HeightfieldReliefConfig heightfieldRelief;` to `EffectConfig`

- Update `src/render/post_effect.h`:
  - Add `Shader heightfieldReliefShader;`
  - Add uniform locations: `heightfieldReliefResolutionLoc`, `heightfieldReliefIntensityLoc`, `heightfieldReliefReliefScaleLoc`, `heightfieldReliefLightAngleLoc`, `heightfieldReliefLightHeightLoc`, `heightfieldReliefShininessLoc`

- Update `src/render/post_effect.cpp`:
  - `PostEffectLoadShaders`: Load `heightfield_relief.fs`
  - `PostEffectLoadShaders`: Add to validation chain
  - `PostEffectInitUniformLocations`: Get all uniform locations
  - `PostEffectSetResolution`: Set resolution uniform
  - `PostEffectUninit`: Unload shader

- Update `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_HEIGHTFIELD_RELIEF` case in `GetTransformEffect()`
  - Implement `SetupHeightfieldRelief()` to set all uniforms

**Done when**: Effect appears in transform order list (disabled).

---

## Phase 3: UI and Serialization

**Goal**: Add UI controls and preset persistence.

**Build**:
- Update `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionHeightfieldRelief = false;`
  - Add `TRANSFORM_HEIGHTFIELD_RELIEF` case in transform order enabled-check switch
  - Add UI section under "Style" category (after Toon):
    - Checkbox "Enabled##relief"
    - `SliderFloat` for intensity (0.0-1.0)
    - `SliderFloat` for reliefScale (0.1-5.0)
    - `SliderAngleDeg` for lightAngle (UI shows degrees)
    - `SliderFloat` for lightHeight (0.1-2.0)
    - `SliderFloat` for shininess (1.0-128.0)

- Update `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `HeightfieldReliefConfig`
  - Add to `to_json`: `if (e.heightfieldRelief.enabled) { j["heightfieldRelief"] = e.heightfieldRelief; }`
  - Add to `from_json`: `e.heightfieldRelief = j.value("heightfieldRelief", e.heightfieldRelief);`

**Done when**: Effect toggles on/off from UI, persists across preset save/load.

---

## Phase 4: Modulation

**Goal**: Register modulatable parameters for audio-reactivity.

**Build**:
- Update `src/automation/param_registry.cpp`:
  - Add to `PARAM_TABLE`:
    - `{"heightfieldRelief.lightAngle", {0.0f, 6.28f}}`
    - `{"heightfieldRelief.intensity", {0.0f, 1.0f}}`
  - Add to `targets[]` array in `ParamRegistryInit()`:
    - `&effects->heightfieldRelief.lightAngle`
    - `&effects->heightfieldRelief.intensity`

- Update `src/ui/imgui_effects.cpp`:
  - Replace `SliderFloat` for intensity with `ModulatableSlider`
  - Replace `SliderAngleDeg` for lightAngle with `ModulatableSliderAngleDeg`

**Done when**: lightAngle and intensity show modulation dots, respond to audio routing.

---

## Phase 5: Verification

**Goal**: Confirm effect works correctly.

**Build**:
- Test with Texture Warp enabled first (creates structure for relief to enhance)
- Verify light rotation via lightAngle slider
- Verify specular highlight appears at low shininess values
- Verify transform order respects heightfield relief position
- Test preset save/load with effect enabled

**Done when**: Effect produces visible embossed lighting that responds to parameter changes.
