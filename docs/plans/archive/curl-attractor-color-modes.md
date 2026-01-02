# Curl & Attractor Flow Color Mode Support

Extend color mode configuration (solid, rainbow, gradient) to curl flow and attractor flow simulations, which currently use hard-coded coloring logic.

## Current State

Color modes work for waveforms/shapes via `ColorFromConfig()`, but GPU simulations have limited support:

- `src/render/color_config.h` - ColorConfig struct with SOLID, RAINBOW, GRADIENT modes
- `src/render/draw_utils.cpp:11-29` - `ColorFromConfig(config, t, opacity)` evaluates color at position t
- `src/simulation/physarum.cpp:13-45` - Reference: assigns per-agent hue from ColorConfig at init
- `src/simulation/curl_flow.cpp:172-182` - Only extracts saturation/value; hue from velocity angle
- `shaders/curl_flow_agents.glsl:244` - Hard-coded: `hue = (velocityAngle + PI) / (2.0 * PI)`
- `src/simulation/attractor_flow.cpp:43` - Random hue at init
- `shaders/attractor_agents.glsl:275` - Hard-coded: hue cycles at fixed rate

---

## Phase 1: ColorLUT Component

**Goal**: Create reusable 1D color lookup texture generator.

**Create**:
- `src/render/color_lut.h` - ColorLUT struct and function declarations
- `src/render/color_lut.cpp` - Implementation

**ColorLUT struct**:
```
Texture2D texture;        // 256x1 RGBA8
ColorConfig cachedConfig; // For change detection
```

**API**:
- `ColorLUT* ColorLUTInit(const ColorConfig* config)` - Allocate and generate texture
- `void ColorLUTUninit(ColorLUT* lut)` - Release resources
- `void ColorLUTUpdate(ColorLUT* lut, const ColorConfig* config)` - Regenerate if changed
- `Texture2D ColorLUTGetTexture(const ColorLUT* lut)` - Get texture for binding

**Generation logic**:
- Loop i = 0..255, compute t = i/255.0
- Call `ColorFromConfig(config, t, 1.0)` for each sample
- Store as RGBA8 pixels, upload via `LoadTextureFromImage`

**Done when**: ColorLUT compiles and can generate textures from any ColorConfig mode.

---

## Phase 2: Curl Flow Integration

**Goal**: Curl flow samples ColorLUT instead of computing velocity-based rainbow.

**Modify `src/simulation/curl_flow.h`**:
- Add forward declaration for ColorLUT
- Add `ColorLUT* colorLUT` field to CurlFlow struct
- Add `int colorLUTLoc` uniform location

**Modify `src/simulation/curl_flow.cpp`**:
- Include `render/color_lut.h`
- `CurlFlowInit`: Call `ColorLUTInit(&config->color)`
- `CurlFlowUninit`: Call `ColorLUTUninit`
- `LoadComputeProgram`: Get `colorLUTLoc` uniform location
- `CurlFlowUpdate`: Bind LUT texture at slot 3
- `CurlFlowApplyConfig`: Call `ColorLUTUpdate` when color changes

**Modify `shaders/curl_flow_agents.glsl`**:
- Add `uniform sampler2D colorLUT` (binding 3)
- Replace hue calculation (line 244-245) with LUT sample:
  ```glsl
  float t = (velocityAngle + PI) / (2.0 * PI);
  vec3 depositColor = texture(colorLUT, vec2(t, 0.5)).rgb;
  ```
- Remove unused `saturation`/`value` uniforms (or keep as brightness multiplier)

**Done when**: Curl flow renders correctly with all three color modes.

---

## Phase 3: Attractor Flow Integration

**Goal**: Attractor agents get fixed hue from ColorConfig at initialization.

**Modify `src/simulation/attractor_flow.cpp`**:

Update `InitializeAgents` signature to accept `const ColorConfig* color`:
```cpp
static void InitializeAgents(AttractorAgent* agents, int count,
                             AttractorType type, const ColorConfig* color)
```

Add color-based hue assignment (copy Physarum pattern):
- SOLID: Extract hue via `ColorConfigRGBToHSV`; if saturation < 0.1, distribute hues
- GRADIENT: Sample `GradientEvaluate` at `i/count`, extract hue
- RAINBOW: `hue = (rainbowHue + (i/count) * rainbowRange) / 360.0`

Update callers of `InitializeAgents`:
- `CreateAgentBuffer`: Pass `&config->color`
- `AttractorFlowReset`: Pass `&af->config.color`
- `AttractorFlowApplyConfig`: Pass `&af->config.color`

Add color change detection in `AttractorFlowApplyConfig`:
- Compare relevant ColorConfig fields
- Reinitialize agents (and clear trails) when color changes

**Modify `shaders/attractor_agents.glsl`**:
- Remove hue cycling (delete line 275: `agent.hue = fract(agent.hue + ...)`)
- Keep existing `hsv2rgb(agent.hue, saturation, value)` deposit logic

**Done when**: Attractor flow renders correctly with all three color modes.

---

## Phase 4: Cleanup & Verification

**Goal**: Remove dead code and verify all modes work.

**Cleanup**:
- Remove unused uniforms from curl flow shader if applicable
- Verify `hsv2rgb` still needed in curl shader (remove if LUT includes full RGB)

**Verification checklist**:
- [ ] Curl flow: solid mode shows uniform color varying only by saturation/value
- [ ] Curl flow: rainbow mode maps velocity direction to configured hue range
- [ ] Curl flow: gradient mode maps velocity direction to gradient stops
- [ ] Attractor flow: solid mode shows uniform color (or distributed if grayscale)
- [ ] Attractor flow: rainbow mode distributes hues across agents
- [ ] Attractor flow: gradient mode samples gradient by agent index
- [ ] Preset save/load preserves color settings
- [ ] UI color controls update simulations in real-time

**Done when**: All color modes work for both simulations with no regressions.
