# Oil Paint — Geometry-Based Brush Strokes

Replace the current single-pass Kuwahara filter with a 2-pass geometry-based brush stroke renderer. Pass 1 places gradient-oriented strokes on a multi-scale grid with bristle texture. Pass 2 applies relief lighting for 3D paint surface appearance.

## Current State

Existing oil paint infrastructure (single-pass Kuwahara, to be replaced):
- `src/config/oil_paint_config.h` — Config with `enabled` and `radius` fields
- `src/config/effect_config.h:73` — `TRANSFORM_OIL_PAINT` enum entry
- `src/render/post_effect.h:48,246-247` — `oilPaintShader`, `oilPaintResolutionLoc`, `oilPaintRadiusLoc`
- `src/render/post_effect.cpp:92,139,341-342,508,647` — Load, check, locations, resolution, unload
- `src/render/shader_setup.cpp:56,630-635` — Dispatch case and `SetupOilPaint`
- `src/ui/imgui_effects_style.cpp:124-136` — UI section with radius slider
- `src/config/preset.cpp:179-180,313,375` — JSON macro, to_json, from_json
- `src/automation/param_registry.cpp:128,355` — `oilPaint.radius` registration
- `shaders/oil_paint.fs` — Kuwahara fragment shader

## Technical Implementation

- **Source**: [Shadertoy: flockaroo Oil Paint (WGGGRh)](https://www.shadertoy.com/view/WGGGRh)
- **Research**: `docs/research/oil-paint.md`

### Pass 1: Stroke Rendering (oil_paint_stroke.fs)

Multi-scale grid distributes brush positions. Each pixel checks 9 neighboring cells at each layer (coarse-to-fine). Strokes orient perpendicular to image gradients.

#### Grid Setup

```glsl
float layerScaleFact = float(quality) / 100.0;
float ls = layerScaleFact * layerScaleFact;
int NumGrid = int(float(0x8000) * min(pow(resolution.x / 1920.0, 0.5), 1.0) * (1.0 - ls));
float aspect = resolution.x / resolution.y;
int NumX = int(sqrt(float(NumGrid) * aspect) + 0.5);
int NumY = int(sqrt(float(NumGrid) / aspect) + 0.5);
int maxLayer = min(layers, int(log2(10.0 / float(NumY)) / log2(layerScaleFact)));
```

#### Per-Pixel Stroke Iteration

```glsl
for (int layer = maxLayer; layer >= 0; layer--) {
    int NumX2 = int(float(NumX) * pow(layerScaleFact, float(layer)) + 0.5);
    int NumY2 = int(float(NumY) * pow(layerScaleFact, float(layer)) + 0.5);
    for (int ni = 0; ni < 9; ni++) {
        int nx = ni % 3 - 1;
        int ny = ni / 3 - 1;
        int n0 = int(dot(floor(pos / resolution * vec2(NumX2, NumY2)), vec2(1, NumX2)));
        int pidx2 = n0 + NumX2 * ny + nx;
        vec2 brushPos = (vec2(pidx2 % NumX2, pidx2 / NumX2) + 0.5)
                        / vec2(NumX2, NumY2) * resolution;
        float gridW = resolution.x / float(NumX2);
        brushPos += gridW * (getRand(pidx2).xy - 0.5);
        brushPos.x += gridW * 0.5 * (float((pidx2 / NumX2) % 2) - 0.5);
        // ... stroke rendering per cell ...
    }
}
```

#### Gradient Orientation

```glsl
float compsignedmax(vec3 c) {
    vec3 a = abs(c);
    if (a.x > a.y && a.x > a.z) return c.x;
    if (a.y > a.x && a.y > a.z) return c.y;
    return c.z;
}

vec2 getGradMax(vec2 pos, float eps) {
    float lod = log2(2.0 * eps * sourceRes.x / resolution.x);
    vec2 d = vec2(eps, 0.0);
    return vec2(
        compsignedmax(getVal(pos + d.xy, lod) - getVal(pos - d.xy, lod)),
        compsignedmax(getVal(pos + d.yx, lod) - getVal(pos - d.yx, lod))
    ) / eps / 2.0;
}

vec2 g = getGradMax(brushPos, gridW * 1.0) * 0.5
       + getGradMax(brushPos, gridW * 0.12) * 0.5
       + 0.0003 * sin(pos / resolution * 20.0);
float gl = length(g);
vec2 n = normalize(g);
vec2 t = n.yx * vec2(1, -1);
```

`getVal` samples the source texture at a given LOD and returns the RGB value. Use `textureLod(texture0, pos / resolution, lod).rgb`.

#### Stroke Shape

```glsl
float stretch = sqrt(1.5 * pow(3.0, 1.0 / float(layer + 1)));
float wh = (gridW - 0.6 * gridW0) * 1.2;
float lh = wh;
wh *= brushSize * (0.8 + 0.4 * rand.y) / stretch;
lh *= brushSize * (0.8 + 0.4 * rand.z) * stretch;

vec2 uv = vec2(dot(pos - brushPos, n), dot(pos - brushPos, t))
           / vec2(wh, lh) * 0.5 + 0.5;

// Suppress strokes in flat areas
wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer)
     ? 0.0 : wh;
```

`gridW0` is `resolution.x / float(NumX)` (finest grid width). `rand` is `getRand(pidx2)`.

#### Stroke Bending

```glsl
uv.x -= 0.125 * strokeBend;
uv.x += uv.y * uv.y * strokeBend;
uv.x /= 1.0 - 0.25 * abs(strokeBend);
```

#### Alpha Mask and Brush Hair Texture

```glsl
float s = 1.0;
s *= uv.x * (1.0 - uv.x) * 6.0;
s *= uv.y * (1.0 - uv.y) * 6.0;
float s0 = clamp((s - 0.5) * 2.0, 0.0, 1.0);

float pat = texture(noiseTex, uv * 1.5 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x
          + texture(noiseTex, uv * 3.0 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x;

float alpha = s0 * 0.7 * pat;

float smask = clamp(
    max(cos(uv.x * PI * 2.0 + 1.5 * (rand.x - 0.5)),
        (1.5 * exp(-uv.y * uv.y / 0.0225) + 0.2) * (1.0 - uv.y)) + 0.1,
    0.0, 1.0);
alpha += s0 * smask;
alpha -= 0.5 * uv.y;
alpha = clamp(alpha, 0.0, 1.0);
```

#### Color Sampling and Compositing

```glsl
vec3 strokeColor = texture(texture0, brushPos / resolution).rgb
                 * mix(alpha * 0.13 + 0.87, 1.0, smask);

float mask = alpha * step(-0.5, -abs(uv.x - 0.5)) * step(-0.5, -abs(uv.y - 0.5));

fragColor = mix(fragColor, vec4(strokeColor, 1.0), mask);
```

#### Noise Lookup (`getRand`)

```glsl
vec4 getRand(int idx) {
    ivec2 texSize = textureSize(noiseTex, 0);
    ivec2 coord = ivec2(idx % texSize.x, idx / texSize.x) % texSize;
    return texelFetch(noiseTex, coord, 0);
}
```

### Pass 2: Relief Lighting (oil_paint.fs)

Reads stroke pass output, computes gradient-based normal, applies diffuse + specular lighting:

```glsl
vec2 uv = fragTexCoord;
float delta = 1.0 / resolution.y;
vec2 d = vec2(delta, 0.0);

float val_l = length(texture(texture0, uv - d.xy).rgb);
float val_r = length(texture(texture0, uv + d.xy).rgb);
float val_d = length(texture(texture0, uv - d.yx).rgb);
float val_u = length(texture(texture0, uv + d.yx).rgb);
vec2 grad = vec2(val_r - val_l, val_u - val_d) / delta;

vec3 n = normalize(vec3(grad, 150.0));
vec3 light = normalize(vec3(-1.0, 1.0, 1.4));
float diff = clamp(dot(n, light), 0.0, 1.0);
float spec = pow(clamp(dot(reflect(light, n), vec3(0, 0, -1)), 0.0, 1.0), 12.0) * specular;
float shadow = pow(clamp(dot(reflect(light * vec3(-1, -1, 1), n), vec3(0, 0, -1)), 0.0, 1.0), 4.0) * 0.1;

vec3 tint = vec3(0.85, 1.0, 1.15);
finalColor.rgb = texture(texture0, uv).rgb * mix(diff, 1.0, 0.9)
             + spec * tint + shadow * tint;
finalColor.a = 1.0;
```

### Parameters (Uniforms)

| Uniform | Type | Range | Default | Pass |
|---------|------|-------|---------|------|
| brushSize | float | 0.5-3.0 | 1.0 | Stroke |
| ~~brushDetail~~ | ~~float~~ | ~~0.0-0.5~~ | ~~0.1~~ | ~~Stroke~~ |
| strokeBend | float | -2.0-2.0 | -1.0 | Stroke |
| ~~quality~~ | ~~int~~ | ~~50-100~~ | ~~85~~ | ~~Stroke~~ |
| specular | float | 0.0-1.0 | 0.15 | Relief |
| layers | int | 3-11 | 8 | Stroke |

### Implementation Notes

- **`quality` removed**: Controlled stroke density via `NumGrid = 0x8000 * (1 - (quality/100)^2)`. At quality=100 NumGrid=0, producing zero strokes. The mapping was inverted and unintuitive. Fixed internally at `layerScaleFact = 0.85`.
- **`brushDetail` removed**: Gated flat-area stroke suppression via `gl * brushDetail < 0.003 / wh0`. At any typical gradient magnitude the threshold never triggered, making the parameter visually inert. Suppression block removed entirely.

---

## Phase 1: Config and Registration

**Goal**: Replace the Kuwahara config with the new 6-parameter struct and update all registration points.
**Depends on**: —
**Files**: `src/config/oil_paint_config.h`, `src/config/effect_config.h`

**Build**:
- Rewrite `oil_paint_config.h` — replace `radius` field with `brushSize`, `brushDetail`, `strokeBend`, `quality`, `specular`, `layers` using defaults from the Parameters table
- `effect_config.h` already has the enum, include, member, name, order entry, and `IsTransformEnabled` case — no changes needed

**Verify**: `cmake.exe --build build` compiles with no errors (UI/setup will reference old `radius` — fix in later phases).

**Done when**: Config struct has all 6 parameters with correct types and defaults.

---

## Phase 2: Shaders

**Goal**: Create both shader files implementing the full algorithm.
**Depends on**: —
**Files**: `shaders/oil_paint_stroke.fs`, `shaders/oil_paint.fs`

**Build**:
- Create `shaders/oil_paint_stroke.fs` — implements Pass 1 (stroke rendering) per the Technical Implementation section. Uniforms: `resolution`, `brushSize`, `brushDetail`, `strokeBend`, `quality`, `layers`. Samplers: `texture0` (source image), `texture1` (256x256 noise). Output: `finalColor`
- Rewrite `shaders/oil_paint.fs` — implements Pass 2 (relief lighting). Uniforms: `resolution`, `specular`. Sampler: `texture0` (stroke pass output). Output: `finalColor`

**Verify**: Files parse as valid GLSL 330 (check at runtime in Phase 3).

**Done when**: Both shader files exist with correct uniform declarations and algorithm from research.

---

## Phase 3: PostEffect Integration

**Goal**: Load both shaders, generate noise texture, create intermediate render target, cache uniform locations.
**Depends on**: Phase 2
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- `post_effect.h`: Add `oilPaintStrokeShader` member. Replace `oilPaintRadiusLoc` with stroke-pass uniform locations (`oilPaintBrushSizeLoc`, `oilPaintBrushDetailLoc`, `oilPaintStrokeBendLoc`, `oilPaintQualityLoc`, `oilPaintLayersLoc`) and relief-pass locations (`oilPaintSpecularLoc`). Add `oilPaintNoiseTex` (Texture2D), `oilPaintIntermediate` (RenderTexture2D), `oilPaintNoiseTexLoc` (int)
- `post_effect.cpp` LoadPostEffectShaders: Load `oil_paint_stroke.fs` as `oilPaintStrokeShader`, keep `oil_paint.fs` as `oilPaintShader`. Add stroke shader to success check
- `post_effect.cpp` GetShaderUniformLocations: Cache locations for both shaders. `oilPaintNoiseTexLoc = GetShaderLocation(pe->oilPaintStrokeShader, "noiseTex")`
- `post_effect.cpp` SetResolutionUniforms: Set resolution on both shaders
- `post_effect.cpp` PostEffectInit: Generate 256x256 RGBA noise image (random bytes via `rand()`), call `LoadTextureFromImage`, set bilinear+wrap filtering. Allocate `oilPaintIntermediate` via `RenderUtilsInitTextureHDR`
- `post_effect.cpp` PostEffectResize: Destroy and recreate `oilPaintIntermediate` at new resolution
- `post_effect.cpp` PostEffectUninit: `UnloadShader(pe->oilPaintStrokeShader)`, `UnloadTexture(pe->oilPaintNoiseTex)`, `UnloadRenderTexture(pe->oilPaintIntermediate)`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` — app launches without crash, oil paint disabled by default.

**Done when**: Both shaders load, noise texture generates, intermediate buffer allocates.

---

## Phase 4: Shader Setup and Pipeline

**Goal**: Wire the 2-pass rendering into the transform pipeline with special-case dispatch.
**Depends on**: Phase 1, Phase 3
**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`, `src/render/render_pipeline.cpp`

**Build**:
- `shader_setup.h`: Declare `void ApplyOilPaintStrokePass(PostEffect* pe, RenderTexture2D* source)` and update `SetupOilPaint` signature (unchanged)
- `shader_setup.cpp`: Implement `ApplyOilPaintStrokePass` — binds noise texture via `SetShaderValueTexture`, binds stroke-pass uniforms (brushSize, brushDetail, strokeBend, quality, layers), renders source into `oilPaintIntermediate` using `oilPaintStrokeShader`
- `shader_setup.cpp`: Update `SetupOilPaint` — binds only `specular` uniform for the relief pass (reads from `oilPaintIntermediate` via `texture0`)
- `shader_setup.cpp`: Update `GetTransformEffect` — keep returning `oilPaintShader` (relief pass) as the shader for the `RenderPass` call
- `render_pipeline.cpp`: Add special-case branch in transform loop:
  ```cpp
  if (effectType == TRANSFORM_OIL_PAINT) {
      ApplyOilPaintStrokePass(pe, src);
      // RenderPass below draws oilPaintIntermediate through relief shader
  }
  ```
  Modify the `RenderPass` call for oil paint to use `&pe->oilPaintIntermediate` as source instead of `src`:
  ```cpp
  RenderTexture2D* effectSrc = (effectType == TRANSFORM_OIL_PAINT) ? &pe->oilPaintIntermediate : src;
  RenderPass(pe, effectSrc, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
  ```

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` — enable oil paint, observe brush stroke effect with relief lighting.

**Done when**: Both passes execute correctly, source image transforms into painterly output.

---

## Phase 5: UI Panel

**Goal**: Replace the radius slider with controls for all 6 parameters.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects_style.cpp`

**Build**:
- Update `DrawStyleOilPaint` — replace the radius `ModulatableSlider` with:
  - `ModulatableSlider("Brush Size", &op->brushSize, "oilPaint.brushSize", "%.2f", modSources)` (0.5-3.0)
  - `ModulatableSlider("Brush Detail", &op->brushDetail, "oilPaint.brushDetail", "%.3f", modSources)` (0.0-0.5)
  - `ModulatableSlider("Stroke Bend", &op->strokeBend, "oilPaint.strokeBend", "%.2f", modSources)` (-2.0-2.0)
  - `ImGui::SliderInt("Quality##oilpaint", &op->quality, 50, 100)`
  - `ModulatableSlider("Specular", &op->specular, "oilPaint.specular", "%.2f", modSources)` (0.0-1.0)
  - `ImGui::SliderInt("Layers##oilpaint", &op->layers, 3, 11)`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` — UI shows all 6 parameters, sliders modify the effect in real-time.

**Done when**: All parameters controllable via UI with correct ranges.

---

## Phase 6: Preset Serialization

**Goal**: Update JSON serialization to match the new config struct.
**Depends on**: Phase 1
**Files**: `src/config/preset.cpp`

**Build**:
- Replace the `OilPaintConfig` JSON macro:
  ```cpp
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OilPaintConfig,
      enabled, brushSize, brushDetail, strokeBend, quality, specular, layers)
  ```
- `to_json` and `from_json` entries already exist and reference `e.oilPaint` — no changes needed

**Verify**: Enable oil paint, save preset, reload — settings persist. Load old preset with `"oilPaint": {"enabled": true, "radius": 4}` — deserializes gracefully to defaults (unknown fields ignored, missing fields get defaults).

**Done when**: Round-trip preset serialization works for all 6 parameters.

---

## Phase 7: Parameter Registration

**Goal**: Register 4 modulatable parameters for audio reactivity.
**Depends on**: Phase 1
**Files**: `src/automation/param_registry.cpp`

**Build**:
- Replace `{"oilPaint.radius", {2.0f, 8.0f}}` in PARAM_TABLE with:
  ```
  {"oilPaint.brushSize",   {0.5f, 3.0f}},
  {"oilPaint.brushDetail", {0.0f, 0.5f}},
  {"oilPaint.strokeBend",  {-2.0f, 2.0f}},
  {"oilPaint.specular",    {0.0f, 1.0f}},
  ```
- Replace `&effects->oilPaint.radius` in targets with:
  ```
  &effects->oilPaint.brushSize,
  &effects->oilPaint.brushDetail,
  &effects->oilPaint.strokeBend,
  &effects->oilPaint.specular,
  ```
- Ensure both arrays stay index-aligned (old entry was 1 slot, new is 4 — shift subsequent entries)

**Verify**: Enable oil paint, assign modulation route to brushSize from bass — brush strokes scale with audio.

**Done when**: All 4 parameters respond to modulation sources.
