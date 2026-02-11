# Attractor Lines

Glowing line traces of chaotic attractors drawn via per-pixel distance-field evaluation. Each frame advances the integration state and renders new trajectory segments as soft luminous strokes, blending onto accumulated trails that slowly fade. First generator with self-feedback — owns a ping-pong render texture pair for trail persistence and stores integration state in pixel (0,0).

**Research**: `docs/research/attractor_lines.md`

## Design

### Types

#### `src/config/attractor_types.h` (new — extracted from attractor_flow.h)

```
AttractorType enum:
  ATTRACTOR_LORENZ = 0
  ATTRACTOR_ROSSLER
  ATTRACTOR_AIZAWA
  ATTRACTOR_THOMAS
  ATTRACTOR_DADRAS    // NEW
  ATTRACTOR_COUNT
```

Both `attractor_flow.h` and `attractor_lines.h` include this shared header.

#### `AttractorLinesConfig` (in `src/effects/attractor_lines.h`)

```
enabled: bool = false

// Attractor system
attractorType: AttractorType = ATTRACTOR_LORENZ
sigma: float = 10.0       // Lorenz coupling (1-30)
rho: float = 28.0         // Lorenz z-folding (10-50)
beta: float = 2.667       // Lorenz z-damping (0.5-5)
rosslerC: float = 5.7     // Rossler chaos transition (2-12)
thomasB: float = 0.208    // Thomas damping (0.1-0.3)
dadrasA: float = 3.0      // Dadras a (1-5)
dadrasB: float = 2.7      // Dadras b (1-5)
dadrasC: float = 1.7      // Dadras c (0.5-3)
dadrasD: float = 2.0      // Dadras d (0.5-4)
dadrasE: float = 9.0      // Dadras e (4-15)

// Line tracing
steps: int = 96           // Integration steps/frame (32-256), stored as float for modulation
timeScale: float = 0.01   // Euler step size (0.001-0.1)
viewScale: float = 0.025  // Attractor-to-screen scale (0.005-0.1)

// Appearance
intensity: float = 0.18   // Line brightness (0.01-1.0)
fade: float = 0.985       // Trail persistence/frame (0.9-0.999)
focus: float = 2.0        // Line sharpness (0.5-5.0)
maxSpeed: float = 50.0    // Velocity normalization ceiling (5-200)

// Transform
x: float = 0.5            // Screen X position (0.0-1.0)
y: float = 0.5            // Screen Y position (0.0-1.0)
rotationAngleX: float = 0.0  // Static X rotation (radians, -PI to PI)
rotationAngleY: float = 0.0  // Static Y rotation
rotationAngleZ: float = 0.0  // Static Z rotation
rotationSpeedX: float = 0.0  // X rotation rate (rad/s, -2 to 2)
rotationSpeedY: float = 0.0  // Y rotation rate
rotationSpeedZ: float = 0.0  // Z rotation rate

// Standard generator
gradient: ColorConfig = {.mode = COLOR_MODE_GRADIENT}
blendMode: EffectBlendMode = EFFECT_BLEND_SCREEN
blendIntensity: float = 1.0
```

#### `AttractorLinesEffect` (in `src/effects/attractor_lines.h`)

```
shader: Shader                    // attractor_lines.fs
gradientLUT: ColorLUT*
pingPong: RenderTexture2D[2]      // Trail persistence pair
readIdx: int                      // Which pingPong to read from (0 or 1)
rotationAccumX/Y/Z: float        // Accumulated rotation angles
<uniform location ints for each shader uniform>
```

The effect owns its ping-pong pair (unlike standard generators that use shared `generatorScratch`). Each frame:
1. Read `pingPong[readIdx]` as `previousFrame` sampler
2. Render new lines + faded previous trails into `pingPong[1 - readIdx]`
3. Swap: `readIdx = 1 - readIdx`
4. BlendCompositor composites `pingPong[readIdx]` onto the scene

Init allocates both textures via `RenderUtilsInitTextureHDR`. Resize unloads and reallocates. Uninit frees both.

### Algorithm

#### Shader: `shaders/attractor_lines.fs`

The shader reads integration state from pixel (0,0) of `previousFrame`, advances N Euler steps, evaluates per-pixel distance to all line segments, and writes new trails blended with faded previous frame.

**Attractor derivative functions** — five `vec3 derivative(vec3 p)` functions, one per attractor type. Identical to the ODE systems in the research doc:

- **Lorenz**: `dx = sigma*(y-x)`, `dy = x*(rho-z) - y`, `dz = x*y - beta*z`
- **Rossler**: `dx = -y - z`, `dy = x + 0.2*y`, `dz = 0.2 + z*(x - rosslerC)`
- **Aizawa**: `dx = (z-0.7)*x - 3.5*y`, `dy = 3.5*x + (z-0.7)*y`, `dz = 0.6 + 0.95*z - z^3/3 - (x^2+y^2)*(1+0.25*z) + 0.1*z*x^3`
- **Thomas**: `dx = sin(y) - thomasB*x`, `dy = sin(z) - thomasB*y`, `dz = sin(x) - thomasB*z`
- **Dadras**: `dx = y - dadrasA*x + dadrasB*y*z`, `dy = dadrasC*y - x*z + z`, `dz = dadrasD*x*y - dadrasE*z`

A dispatcher function selects the active system by `int attractorType` uniform.

**Per-attractor center offsets** — hardcoded `vec3` constants subtracted before projection:
- Lorenz: `(0.0, 0.0, 27.0)`
- Rossler: `(0.0, 0.0, 0.0)`
- Aizawa: `(0.0, 0.0, 0.0)`
- Thomas: `(0.0, 0.0, 0.0)`
- Dadras: `(0.0, 0.0, 0.0)`

**Per-attractor canonical starting points** — used on first frame (when state pixel reads as zero):
- Lorenz: `(1.0, 1.0, 1.0)`
- Rossler: `(1.0, 1.0, 0.0)`
- Aizawa: `(0.1, 0.0, 0.0)`
- Thomas: `(1.0, 1.1, 1.2)`
- Dadras: `(1.0, 1.0, 1.0)`

**Integration loop** (Euler):
```glsl
// Read state from pixel (0,0) of previous frame
vec4 statePixel = texelFetch(previousFrame, ivec2(0, 0), 0);
vec3 pos = statePixel.rgb;
// First frame detect: if state is zero, use canonical start
if (dot(pos, pos) < 1e-10) pos = startingPoint[attractorType];

vec3 projected[STEPS + 1];
float speeds[STEPS + 1];
for (int i = 0; i < steps; i++) {
    vec3 d = attractorDerivative(pos, attractorType);
    speeds[i] = length(d);
    // Project 3D -> 2D before storing
    projected[i] = project(pos);
    pos += d * timeScale;
}
projected[steps] = project(pos);
speeds[steps] = speeds[steps - 1];
```

**3D projection**:
```glsl
vec2 project(vec3 p) {
    p -= centerOffset[attractorType];
    p = rotationMatrix * p;
    return p.xy * viewScale + vec2(x, y);
}
```

The `rotationMatrix` is a `mat3` uniform built on the CPU from accumulated rotation angles (same XYZ Euler pattern as `attractor_flow.cpp`).

**Distance-field line evaluation** — for each pixel, find distance to nearest segment:
```glsl
float d = 1e6;
float bestSpeed = 0.0;
for (int i = 0; i < steps; i++) {
    vec2 a = projected[i].xy;
    vec2 b = projected[i + 1].xy;
    vec2 ab = b - a;
    float t = clamp(dot(uv - a, ab) / dot(ab, ab), 0.0, 1.0);
    float segD = distance(a + ab * t, uv);
    if (segD < d) {
        d = segD;
        bestSpeed = speeds[i + 1];
    }
}
```

Where `uv` is the fragment position in normalized screen coordinates (aspect-corrected).

**Brightness** — two-term glow + sharp core:
```glsl
float c = intensity * smoothstep(focus / resolution.y, 0.0, d);
c += (intensity / 8.5) * exp(-1000.0 * d * d);
```

**Coloring** — velocity maps to gradient LUT:
```glsl
float speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0);
vec3 color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb;
```

**Trail persistence + state storage**:
```glsl
// For pixel (0,0): store integration state
if (gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0) {
    finalColor = vec4(pos, 1.0);
    return;
}
// All other pixels: blend new lines onto faded previous frame
vec4 prev = texture(previousFrame, fragTexCoord);
finalColor = vec4(color * c + prev.rgb * fade, 1.0);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| attractorType | enum | 0-4 | LORENZ | No | Attractor Type |
| sigma | float | 1-30 | 10.0 | Yes | Sigma |
| rho | float | 10-50 | 28.0 | Yes | Rho |
| beta | float | 0.5-5 | 2.667 | Yes | Beta |
| rosslerC | float | 2-12 | 5.7 | Yes | Rossler C |
| thomasB | float | 0.1-0.3 | 0.208 | Yes | Thomas B |
| dadrasA | float | 1-5 | 3.0 | Yes | Dadras A |
| dadrasB | float | 1-5 | 2.7 | Yes | Dadras B |
| dadrasC | float | 0.5-3 | 1.7 | Yes | Dadras C |
| dadrasD | float | 0.5-4 | 2.0 | Yes | Dadras D |
| dadrasE | float | 4-15 | 9.0 | Yes | Dadras E |
| steps | float | 32-256 | 96 | Yes | Steps |
| timeScale | float | 0.001-0.1 | 0.01 | Yes | Time Scale |
| viewScale | float | 0.005-0.1 | 0.025 | Yes | View Scale |
| intensity | float | 0.01-1.0 | 0.18 | Yes | Intensity |
| fade | float | 0.9-0.999 | 0.985 | Yes | Fade |
| focus | float | 0.5-5.0 | 2.0 | Yes | Focus |
| maxSpeed | float | 5-200 | 50.0 | Yes | Max Speed |
| x | float | 0.0-1.0 | 0.5 | Yes | X Position |
| y | float | 0.0-1.0 | 0.5 | Yes | Y Position |
| rotationAngleX | float | -PI-PI | 0.0 | Yes | Rotation X |
| rotationAngleY | float | -PI-PI | 0.0 | Yes | Rotation Y |
| rotationAngleZ | float | -PI-PI | 0.0 | Yes | Rotation Z |
| rotationSpeedX | float | -2-2 | 0.0 | Yes | Spin X |
| rotationSpeedY | float | -2-2 | 0.0 | Yes | Spin Y |
| rotationSpeedZ | float | -2-2 | 0.0 | Yes | Spin Z |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity | <!-- Intentional addition: standard generator field required by BlendCompositor, not in research doc -->

### Constants

- Enum name: `TRANSFORM_ATTRACTOR_LINES_BLEND`
- Display name: `"Attractor Lines"`
- Category badge: `"GEN"`
- Section index: `10`
- Descriptor flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE`

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Extract AttractorType enum and create effect header

**Files**: `src/config/attractor_types.h` (create), `src/simulation/attractor_flow.h` (modify), `src/effects/attractor_lines.h` (create)

**Creates**: Shared `AttractorType` enum (with `ATTRACTOR_DADRAS`), `AttractorLinesConfig` struct, `AttractorLinesEffect` struct, function declarations

**Do**:
1. Create `src/config/attractor_types.h` — move the `AttractorType` enum from `attractor_flow.h` here, add `ATTRACTOR_DADRAS` before `ATTRACTOR_COUNT`. Include guard `ATTRACTOR_TYPES_H`.
2. Modify `src/simulation/attractor_flow.h` — remove the `AttractorType` typedef enum block, add `#include "config/attractor_types.h"` instead.
3. Create `src/effects/attractor_lines.h` — follow `filaments.h` as the structural template:
   - Include `raylib.h`, `config/attractor_types.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`
   - `AttractorLinesConfig` struct with all fields from the Design section
   - `AttractorLinesEffect` struct with: `Shader shader`, `ColorLUT *gradientLUT`, `RenderTexture2D pingPong[2]`, `int readIdx`, `float rotationAccumX/Y/Z`, and one `int` location field per shader uniform (resolution, previousFrame, attractorType, sigma, rho, beta, rosslerC, thomasB, dadrasA-E, steps, timeScale, viewScale, intensity, fade, focus, maxSpeed, x, y, rotationMatrix, gradientLUT)
   - Declare: `AttractorLinesEffectInit`, `AttractorLinesEffectSetup`, `AttractorLinesEffectRender`, `AttractorLinesEffectResize`, `AttractorLinesEffectUninit`, `AttractorLinesConfigDefault`, `AttractorLinesRegisterParams`
   - `AttractorLinesEffectRender` is the ping-pong pass (reads from own texture, writes to own texture). Signature: `void AttractorLinesEffectRender(AttractorLinesEffect *e, const AttractorLinesConfig *cfg, float deltaTime, int screenWidth, int screenHeight)`
   - `AttractorLinesEffectSetup` binds blend compositor uniforms (called during the blend pass, not the render pass)

**Verify**: `cmake.exe --build build` compiles (header-only changes + attractor_flow consumers still resolve the enum).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/attractor_lines.cpp` (create)

**Depends on**: Wave 1 complete (needs `attractor_lines.h`)

**Do**: Implement all functions declared in `attractor_lines.h`. Follow `filaments.cpp` as the structural template.

- `AttractorLinesEffectInit`: Load `shaders/attractor_lines.fs`, cache all uniform locations, allocate `ColorLUT`, create both `pingPong[2]` textures via `RenderUtilsInitTextureHDR` (needs screen dimensions — take `int width, int height` params, same as bloom init). Set `readIdx = 0`. Clear both ping-pong textures to black.
- `AttractorLinesEffectSetup`: Accumulate `rotationAccumX/Y/Z` from `cfg->rotationAngleX/Y/Z + rotationSpeedX/Y/Z * deltaTime`. Build 3x3 rotation matrix (same XYZ Euler pattern as `attractor_flow.cpp:228-248`). Update gradient LUT. Bind all uniforms including `rotationMatrix` as `SHADER_UNIFORM_MAT3` (use `SetShaderValueMatrix` or `SetShaderValueV` with count 9). Bind `previousFrame` texture to slot 1 (`SetShaderValueTexture` with `pingPong[readIdx].texture`). Bind `gradientLUT` texture to slot 2.
- `AttractorLinesEffectRender`: `BeginTextureMode(e->pingPong[1 - e->readIdx])`, draw fullscreen quad with the shader active, `EndTextureMode()`, then `e->readIdx = 1 - e->readIdx`. Use `RenderUtilsDrawFullscreenQuad` or equivalent.
- `AttractorLinesEffectResize`: Unload both ping-pong textures, reallocate via `RenderUtilsInitTextureHDR` with new dimensions. Reset `readIdx = 0`. Clear to black.
- `AttractorLinesEffectUninit`: Unload shader, free ColorLUT, unload both ping-pong textures.
- `AttractorLinesConfigDefault`: Return default-initialized `AttractorLinesConfig`.
- `AttractorLinesRegisterParams`: Register all modulatable params from the Parameters table. Use `ModEngineRegisterParam("attractorLines.<field>", &cfg->field, min, max)`. For rotation angles use `ROTATION_OFFSET_MAX`, for rotation speeds use `ROTATION_SPEED_MAX` from `ui_units.h`.

**Verify**: Compiles (shader not yet created, but the .cpp itself must compile).

#### Task 2.2: Fragment shader

**Files**: `shaders/attractor_lines.fs` (create)

**Depends on**: Wave 1 complete (needs to know uniform names from header)

**Do**: Implement the Algorithm section from this plan. This is the single source of truth for all formulas.

Uniforms: `resolution` (vec2), `previousFrame` (sampler2D), `attractorType` (int), `sigma`, `rho`, `beta`, `rosslerC`, `thomasB`, `dadrasA`, `dadrasB`, `dadrasC`, `dadrasD`, `dadrasE` (float), `steps` (float, cast to int in shader), `timeScale`, `viewScale`, `intensity`, `fade`, `focus`, `maxSpeed`, `x`, `y` (float), `rotationMatrix` (mat3), `gradientLUT` (sampler2D).

Key implementation notes:
- Use `texelFetch(previousFrame, ivec2(0, 0), 0)` to read state, NOT `texture()` (avoids filtering)
- The `projected` and `speeds` arrays must be sized by a compile-time constant. Use `#define MAX_STEPS 256` and loop up to `int(steps)`.
- Aspect-correct UV: `vec2 uv = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0) + 0.5` or similar. The projected points and the pixel UV must share the same coordinate space.
- Pixel (0,0) state write: use `gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0` guard. Store `vec4(pos, 1.0)`.
- Output `vec4(color * brightness + prev.rgb * fade, 1.0)` for all other pixels.

**Verify**: File exists, valid GLSL syntax (checked at runtime by shader load).

#### Task 2.3: Registration and integration

**Files**: `src/config/effect_config.h` (modify), `src/config/effect_descriptor.h` (modify), `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify), `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Wire the effect into the engine. Follow the add-effect checklist for each file:

1. **`src/config/effect_config.h`**:
   - Add `#include "effects/attractor_lines.h"` with other effect includes
   - Add `TRANSFORM_ATTRACTOR_LINES_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
   - Add `TRANSFORM_ATTRACTOR_LINES_BLEND` to `TransformOrderConfig::order` array (at end, before closing brace)
   - Add `AttractorLinesConfig attractorLines;` to `EffectConfig` struct

2. **`src/config/effect_descriptor.h`**:
   - Add descriptor row:
     ```
     [TRANSFORM_ATTRACTOR_LINES_BLEND] = {
         TRANSFORM_ATTRACTOR_LINES_BLEND, "Attractor Lines", "GEN", 10,
         offsetof(EffectConfig, attractorLines.enabled),
         EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE
     },
     ```

3. **`src/render/post_effect.h`**:
   - Add `#include "effects/attractor_lines.h"`
   - Add `AttractorLinesEffect attractorLines;` to `PostEffect` struct

4. **`src/render/post_effect.cpp`**:
   - In `PostEffectInit`: call `AttractorLinesEffectInit(&pe->attractorLines, &pe->effects.attractorLines, screenWidth, screenHeight)` with failure handling
   - In `PostEffectUninit`: call `AttractorLinesEffectUninit(&pe->attractorLines)`
   - In `PostEffectResize`: call `AttractorLinesEffectResize(&pe->attractorLines, width, height)`
   - In `PostEffectRegisterParams`: call `AttractorLinesRegisterParams(&pe->effects.attractorLines)`
   - In `PostEffectClearFeedback`: clear both ping-pong textures — `BeginTextureMode(pe->attractorLines.pingPong[0]); ClearBackground(BLACK); EndTextureMode();` and same for `[1]`. Reset `readIdx = 0`.

5. **`CMakeLists.txt`**: Add `src/effects/attractor_lines.cpp` to `EFFECTS_SOURCES`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader setup and render pipeline

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify), `src/render/render_pipeline.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:

1. **`src/render/shader_setup_generators.h`**: Declare `void SetupAttractorLines(PostEffect *pe);` and `void SetupAttractorLinesBlend(PostEffect *pe);`

2. **`src/render/shader_setup_generators.cpp`**:
   - Add `#include "effects/attractor_lines.h"`
   - `SetupAttractorLines`: calls `AttractorLinesEffectSetup(&pe->attractorLines, &pe->effects.attractorLines, pe->currentDeltaTime)` — binds all uniforms and textures for the render pass
   - `SetupAttractorLinesBlend`: calls `BlendCompositorApply(pe->blendCompositor, pe->attractorLines.pingPong[pe->attractorLines.readIdx].texture, pe->effects.attractorLines.blendIntensity, pe->effects.attractorLines.blendMode)` — composites the trail texture onto the scene

3. **`src/render/shader_setup.cpp`**:
   - Add `TRANSFORM_ATTRACTOR_LINES_BLEND` case in `GetTransformEffect`: return `{&pe->blendCompositor->shader, SetupAttractorLinesBlend, &pe->effects.attractorLines.enabled}`
   - Note: this returns the blendCompositor shader (not the attractor lines shader), matching all other generators

4. **`src/render/render_pipeline.cpp`**:
   - Add a special-case branch in the output loop (before the generic `EFFECT_FLAG_BLEND` branch):
     ```
     } else if (effectType == TRANSFORM_ATTRACTOR_LINES_BLEND) {
         SetupAttractorLines(pe);
         AttractorLinesEffectRender(&pe->attractorLines, &pe->effects.attractorLines,
                                     pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
         RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
     ```
   - The first two lines run the ping-pong render (Setup binds uniforms, Render draws to internal texture). The third line composites via BlendCompositor.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Preset serialization

**Files**: `src/config/preset.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:

1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `AttractorLinesConfig`. List all serializable fields: `enabled, attractorType, sigma, rho, beta, rosslerC, thomasB, dadrasA, dadrasB, dadrasC, dadrasD, dadrasE, steps, timeScale, viewScale, intensity, fade, focus, maxSpeed, x, y, rotationAngleX, rotationAngleY, rotationAngleZ, rotationSpeedX, rotationSpeedY, rotationSpeedZ, gradient, blendMode, blendIntensity`

2. Add `to_json` entry: `if (e.attractorLines.enabled) { j["attractorLines"] = e.attractorLines; }`

3. Add `from_json` entry: `e.attractorLines = j.value("attractorLines", e.attractorLines);`

**Verify**: Compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_gen_filament.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Add attractor lines UI to the Filament subcategory. Follow the existing constellation/filaments pattern in this file. Reference `imgui_effects.cpp:319-382` for the attractor-specific conditional param pattern.

1. Add includes: `#include "effects/attractor_lines.h"`, `#include "config/attractor_types.h"`
2. Add `static bool sectionAttractorLines = false;`
3. Add `DrawGeneratorsAttractorLines(EffectConfig *e, const ModSources *modSources, const ImU32 categoryGlow)`:
   - `DrawSectionBegin("Attractor Lines", categoryGlow, &sectionAttractorLines)`
   - Enable checkbox with `MoveTransformToEnd` on toggle
   - When enabled, show:
     - **Attractor Type**: `ImGui::Combo` with names array `{"Lorenz", "Rossler", "Aizawa", "Thomas", "Dadras"}`, cast to/from int
     - **System params** (conditional on `attractorType`):
       - Lorenz: sigma, rho, beta sliders
       - Rossler: rosslerC slider
       - Thomas: thomasB slider
       - Dadras: dadrasA-E sliders
     - **`SeparatorText("Tracing")`**: steps (use `ModulatableSliderInt`), timeScale (use `ModulatableSliderLog`), viewScale
     - **`SeparatorText("Appearance")`**: intensity, fade, focus, maxSpeed
     - **`SeparatorText("Transform")`**: x, y position sliders, rotation angle sliders (`ModulatableSliderAngleDeg`), rotation speed sliders (`ModulatableSliderSpeedDeg`)
     - **`SeparatorText("Output")`**: `DrawColorModeCombo` for gradient, blend mode combo, blend intensity slider
   - `DrawSectionEnd()`
4. Add call in `DrawGeneratorsFilament`: `ImGui::Spacing(); DrawGeneratorsAttractorLines(e, modSources, categoryGlow);`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "GEN" category badge
- [ ] Enabling effect renders glowing attractor line traces
- [ ] Trail persistence works (lines accumulate over time)
- [ ] Switching attractor type changes the ODE system
- [ ] 3D rotation tumbles the projection
- [ ] Gradient LUT colors lines by velocity
- [ ] Fade parameter controls trail half-life
- [ ] Window resize preserves effect (textures reallocate, state resets)
- [ ] Clear feedback resets trails
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
