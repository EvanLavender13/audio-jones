# Curl Advection Accumulation-Based Injection

Replace single-point Lissajous energy injection with distributed injection from the accumulation texture. Bright pixels in the accum buffer add energy to the fluid field.

## Current State

- `src/simulation/curl_advection.h:24-27` - injection config fields (intensity, amplitude, freqX/Y)
- `src/simulation/curl_advection.cpp:200-208` - Lissajous center calculation
- `shaders/curl_advection.glsl:134-140` - single-point injection block
- `src/ui/imgui_effects.cpp:357-362` - injection UI sliders
- `src/config/preset.cpp:110` - serialization macro includes injection fields
- `src/automation/param_registry.cpp:149` - injectionIntensity modulation entry

## Phase 1: Config and Struct Updates

**Goal**: Update config struct and remove Lissajous-related state.

**Build**:
- In `curl_advection.h`:
  - Remove `injectionAmplitude`, `injectionFreqX`, `injectionFreqY` from config
  - Add `injectionThreshold` (float, 0.0-1.0, default 0.1) - brightness cutoff
  - Remove `injectionCenterLoc` and `injectionTime` from struct
  - Add `injectionThresholdLoc` uniform location

**Done when**: Header compiles with new config shape.

---

## Phase 2: Shader Update

**Goal**: Replace single-point injection with distributed accum-based injection.

**Build**:
- In `curl_advection.glsl`:
  - Add `layout(binding = 4) uniform sampler2D accumTexture`
  - Add `uniform float injectionThreshold`
  - Remove `uniform vec2 injectionCenter`
  - Replace injection block (lines 134-140) with:

```glsl
// Accumulation-based energy injection
if (injectionIntensity > 0.0) {
    vec4 accumSample = texture(accumTexture, uv);
    float brightness = dot(accumSample.rgb, vec3(0.299, 0.587, 0.114));
    float contribution = max(brightness - injectionThreshold, 0.0);

    // Add energy proportional to brightness, direction based on velocity gradient
    // Use local curl direction to determine injection orientation
    vec2 injDir = length(result.xy) > 0.001 ? normalize(result.xy) : vec2(1.0, 0.0);
    result.xy += injectionIntensity * contribution * injDir;
}
```

**Done when**: Shader compiles (syntax check via test build).

---

## Phase 3: C++ Update Logic

**Goal**: Pass accumulation texture to compute shader.

**Build**:
- In `curl_advection.cpp`:
  - Change `CurlAdvectionUpdate` signature: add `Texture2D accumTexture` parameter
  - Remove `ca->injectionTime += deltaTime` and Lissajous center calculation
  - Get uniform location for `injectionThreshold` in `LoadComputeProgram`
  - Set `injectionThreshold` uniform before dispatch
  - Bind `accumTexture` to texture unit 4 before dispatch:
    ```cpp
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, accumTexture.id);
    ```

- In `curl_advection.h`:
  - Update function signature: `void CurlAdvectionUpdate(CurlAdvection* ca, float deltaTime, Texture2D accumTexture)`

**Done when**: C++ compiles with new signature.

---

## Phase 4: Pipeline Integration

**Goal**: Pass accumulation texture through render pipeline.

**Build**:
- In `render_pipeline.cpp`:
  - Change call at line 80 from `CurlAdvectionUpdate(pe->curlAdvection, deltaTime)` to `CurlAdvectionUpdate(pe->curlAdvection, deltaTime, pe->accumTexture.texture)`

**Done when**: Pipeline compiles and runs.

---

## Phase 5: UI and Serialization

**Goal**: Update UI sliders and preset serialization.

**Build**:
- In `imgui_effects.cpp`:
  - Remove sliders for `injectionAmplitude`, `injectionFreqX`, `injectionFreqY` (lines 359-362)
  - Add slider for `injectionThreshold` (0.0-1.0):
    ```cpp
    ImGui::SliderFloat("Inj Threshold##curlAdv", &e->curlAdvection.injectionThreshold, 0.0f, 1.0f, "%.2f");
    ```

- In `preset.cpp`:
  - Update serialization macro: remove `injectionAmplitude, injectionFreqX, injectionFreqY`, add `injectionThreshold`

- In `param_registry.cpp`:
  - Keep `injectionIntensity` entry (same semantics)

**Done when**: UI shows new threshold slider, presets load/save correctly.

---

## Phase 6: Testing

**Goal**: Verify the effect works as expected.

**Build**:
- Enable curl advection with a non-zero injection intensity
- Draw shapes or enable other effects that write to accumulation
- Verify bright areas inject energy into the fluid field
- Test threshold slider isolates bright features
- Test that intensity slider scales the effect

**Done when**: Visual confirmation of accum-based injection working.
