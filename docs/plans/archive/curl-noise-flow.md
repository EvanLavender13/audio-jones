# Curl Noise Flow Effect

Agents follow divergence-free curl noise flow fields, depositing colored trails. Trail density slows agents, creating emergent lane formation. Color derives from velocity angle mapped to the user's gradient.

**Architecture**: Exactly like physarum but with curl noise steering instead of sensor-based steering.

## Design Decisions

| Decision | Choice |
|----------|--------|
| Curl method | Single gradient curl: `(dN/dy, -dN/dx)` from 3D simplex noise |
| Trail feedback | Speed modulation (slow in dense areas, 10% minimum) |
| Feedback sensing | `accumSenseBlend` blends trail (0) vs feedback texture (1) |
| Drawable injection | Drawables rendered to trail map (like physarum) |
| Agent color | Velocity angle → hue (rainbow mode) |
| Trail steering | None (pure curl-following, fallback to previous direction if curl ≈ 0) |
| Noise shader | Inline in agent shader (no shared include) |
| Edge wrapping | `mod(mod(pos, res) + res, res)` handles negative values |

## Implementation Summary

### Files Created
- `src/render/curl_flow.h` - Config, state structs, function declarations
- `src/render/curl_flow.cpp` - Init/Uninit/Update/ProcessTrails/DrawDebug/BeginTrailMapDraw/EndTrailMapDraw
- `shaders/curl_flow_agents.glsl` - Compute shader with inline simplex noise

### Files Modified
- `src/config/effect_config.h` - Added `CurlFlowConfig curlFlow`
- `src/render/post_effect.h` - Added `CurlFlow* curlFlow`
- `src/render/post_effect.cpp` - Init/Uninit/Resize calls
- `src/render/render_pipeline.cpp` - `ApplyCurlFlowPass`, boost blend pass
- `src/main.cpp` - `RenderDrawablesToCurlFlow` for drawable injection
- `src/ui/imgui_effects.cpp` - Curl Flow UI section
- `src/config/preset.cpp` - CurlFlowConfig serialization
- `CMakeLists.txt` - Added curl_flow.cpp to sources

### CurlFlowConfig Fields
```cpp
bool enabled = false;
int agentCount = 100000;
float noiseFrequency = 0.005f;   // Spatial frequency (0.001-0.1)
float noiseEvolution = 0.5f;     // Temporal evolution speed (0.0-2.0)
float trailInfluence = 0.3f;     // Trail density slows agents (0.0-1.0)
float accumSenseBlend = 0.0f;    // 0 = trail only, 1 = feedback only
float stepSize = 2.0f;           // Movement speed (0.5-5.0)
float depositAmount = 0.1f;      // Trail deposit strength (0.01-0.2)
float decayHalfLife = 1.0f;      // Seconds for 50% decay (0.1-5.0)
int diffusionScale = 1;          // Diffusion kernel scale (0-4)
float boostIntensity = 1.0f;     // Trail boost strength (0.0-2.0)
EffectBlendMode blendMode;
ColorConfig color;
bool debugOverlay = false;
```

### Key Shader Logic
```glsl
// Curl from single 3D noise gradient
vec4 n = snoise3_grad(pos);
vec2 curl = vec2(n.z, -n.y);  // (dN/dy, -dN/dx)

// Normalize with fallback to previous direction
if (length(curl) > 0.001) {
    curl = normalize(curl);
} else {
    curl = vec2(cos(agent.velocityAngle), sin(agent.velocityAngle));
}

// Speed with minimum floor
float density = sampleBlendedDensity(pos);  // trail vs accum
float speed = stepSize * max(1.0 - trailInfluence * density, 0.1);

// Wrap handles negative values
pos = mod(mod(pos + curl * speed, resolution) + resolution, resolution);
```

### Pipeline Integration
1. `RenderDrawablesToCurlFlow` (main.cpp) - Injects drawables into trail map
2. `ApplyCurlFlowPass` (render_pipeline.cpp) - Update agents, process trails
3. Boost pass in `RenderPipelineApplyOutput` - Blends trails into output

## Status: Complete

All phases implemented. Effect runs parallel to physarum with identical infrastructure but different steering algorithm.
