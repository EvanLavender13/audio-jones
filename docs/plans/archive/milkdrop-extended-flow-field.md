# MilkDrop Extended Flow Field

Extends the existing feedback flow field with four MilkDrop features: movable center pivot, directional stretch, angular variation, and procedural warp animation. Creates the characteristic psychedelic undulating motion of classic MilkDrop presets.

## Current State

- `shaders/feedback.fs` - Spatial flow field with radial modulation for zoom/rot/dx/dy. Center hardcoded to `vec2(0.5)`.
- `src/config/effect_config.h:150-159` - `FlowFieldConfig` with 8 radial parameters
- `src/config/feedback_flow_config.h` - `FeedbackFlowConfig` for luminance gradient flow
- `src/render/post_effect.h:97-110` - 14 feedback uniform locations
- `src/render/shader_setup.cpp:100-128` - `SetupFeedback()` passes all uniforms
- `src/ui/imgui_effects.cpp:97-127` - Flow Field UI section
- `src/automation/param_registry.cpp:19-30` - flowField and feedbackFlow params registered

## Research Reference

`docs/research/milkdrop-extended-flow-field.md` - Contains exact shader algorithms, MilkDrop parameter ranges, and transformation order.

## Technical Implementation

### Phase 1 Shader: Center, Stretch, Angular

**Transformation order** (matches MilkDrop):
1. Zoom with radial/angular modulation
2. Stretch relative to center (NEW)
3. Rotation around center (MODIFIED to use cx/cy)
4. Translation

**Center pivot (cx, cy)**:
```glsl
uniform float cx;  // 0-1, default 0.5
uniform float cy;  // 0-1, default 0.5

vec2 center = vec2(cx, cy);
vec2 uv = fragTexCoord - center;
```

**Stretch (sx, sy)** - applied after zoom, before rotation:
```glsl
uniform float sx;  // 0.9-1.1, default 1.0
uniform float sy;  // 0.9-1.1, default 1.0

// After zoom: uv *= zoom;
uv.x /= sx;
uv.y /= sy;
```

**Angular modulation** - polar angle varies zoom/rot/dx/dy:
```glsl
uniform float zoomAngular;      // -0.1 to 0.1
uniform int   zoomAngularFreq;  // 1-8
uniform float rotAngular;       // -0.05 to 0.05
uniform int   rotAngularFreq;   // 1-8

// Compute polar angle (aspect-corrected)
float ang = atan(normalized.y, normalized.x);

// Extend existing parameter computation:
float zoom = zoomBase + rad * zoomRadial + sin(ang * float(zoomAngularFreq)) * zoomAngular;
float rot = rotBase + rad * rotRadial + sin(ang * float(rotAngularFreq)) * rotAngular;
```

### Phase 2 Shader: Procedural Warp

**warpFactors** - oscillating coefficients computed in shader from time:
```glsl
uniform float warpTime;  // CPU-accumulated (deltaTime * warpSpeed)
uniform float warp;      // 0-2, amplitude multiplier
uniform float warpScaleInverse;  // 1.0/warpScale

vec4 warpFactors = vec4(
    11.68 + 4.0 * cos(warpTime * 1.413 + 10.0),
    8.77  + 3.0 * cos(warpTime * 1.113 + 7.0),
    10.54 + 3.0 * cos(warpTime * 1.233 + 3.0),
    11.49 + 4.0 * cos(warpTime * 0.933 + 5.0)
);
```

**UV displacement** - applied after stretch, before rotation:
```glsl
// pos = uv in -0.5 to 0.5 range (before adding center back)
uv.x += warp * 0.0035 * sin(warpTime * 0.333 + warpScaleInverse * (pos.x * warpFactors.x - pos.y * warpFactors.w));
uv.y += warp * 0.0035 * cos(warpTime * 0.375 - warpScaleInverse * (pos.x * warpFactors.z + pos.y * warpFactors.y));
uv.x += warp * 0.0035 * cos(warpTime * 0.753 - warpScaleInverse * (pos.x * warpFactors.y - pos.y * warpFactors.z));
uv.y += warp * 0.0035 * sin(warpTime * 0.825 + warpScaleInverse * (pos.x * warpFactors.x + pos.y * warpFactors.w));
```

---

## Phase 1: Center, Stretch, Angular Modulation

**Goal**: Add movable center pivot, directional stretch, and angular parameter variation to the feedback shader.

### Config

Extend `FlowFieldConfig` in `src/config/effect_config.h`:
```cpp
struct FlowFieldConfig {
    // Existing 8 radial params...

    // Center pivot (NEW)
    float cx = 0.5f;
    float cy = 0.5f;

    // Directional stretch (NEW)
    float sx = 1.0f;
    float sy = 1.0f;

    // Angular modulation (NEW)
    float zoomAngular = 0.0f;
    int   zoomAngularFreq = 2;
    float rotAngular = 0.0f;
    int   rotAngularFreq = 2;
};
```

### Shader

Modify `shaders/feedback.fs`:
- Add 8 new uniforms (cx, cy, sx, sy, zoomAngular, zoomAngularFreq, rotAngular, rotAngularFreq)
- Replace hardcoded `vec2(0.5)` center with `vec2(cx, cy)`
- Add polar angle computation for angular modulation
- Insert stretch after zoom, before rotation
- Extend zoom/rot formulas with angular terms

### PostEffect

`src/render/post_effect.h` - Add 8 uniform location ints:
- `feedbackCxLoc`, `feedbackCyLoc`, `feedbackSxLoc`, `feedbackSyLoc`
- `feedbackZoomAngularLoc`, `feedbackZoomAngularFreqLoc`
- `feedbackRotAngularLoc`, `feedbackRotAngularFreqLoc`

`src/render/post_effect.cpp`:
- Get all 8 uniform locations in `GetShaderUniformLocations()`

### Shader Setup

`src/render/shader_setup.cpp` - Extend `SetupFeedback()` with 8 new SetShaderValue calls.

### UI Controls

`src/ui/imgui_effects.cpp` - Add to Flow Field section:
- Center: Two sliders for cx/cy (0-1)
- Stretch: Two sliders for sx/sy (0.9-1.1)
- Angular subsection: Four controls (zoomAngular, zoomAngularFreq, rotAngular, rotAngularFreq)

### Preset Serialization

`src/config/preset.cpp` - Add 8 new fields to `FlowFieldConfig` JSON macro.

### Parameter Registration

`src/automation/param_registry.cpp`:
- Add 6 float params (cx, cy, sx, sy, zoomAngular, rotAngular)
- Integer params (freq) not modulatable

**Done when**: Moving cx/cy shifts the rotation pivot visually. Stretch creates anamorphic distortion. Angular modulation creates symmetric spiral patterns (e.g., 2-fold creates opposing rotations).

---

## Phase 2: Procedural Warp Animation

**Goal**: Add MilkDrop's animated procedural warp distortion.

### Config

Create `src/config/procedural_warp_config.h`:
```cpp
struct ProceduralWarpConfig {
    float warp = 0.0f;       // 0-2, amplitude (0 = disabled)
    float warpSpeed = 1.0f;  // 0.1-2.0, time multiplier
    float warpScale = 1.0f;  // 0.1-100, spatial frequency (lower = larger waves)
};
```

Add to `EffectConfig` in `src/config/effect_config.h`:
- Include header
- Add `ProceduralWarpConfig proceduralWarp;` member

### PostEffect State

`src/render/post_effect.h`:
- Add `float warpTime;` accumulated time
- Add 3 uniform locations: `feedbackWarpLoc`, `feedbackWarpTimeLoc`, `feedbackWarpScaleInverseLoc`

`src/render/post_effect.cpp`:
- Initialize `pe->warpTime = 0.0f` in `PostEffectInit()`
- Get 3 uniform locations

### Time Accumulation

`src/render/render_pipeline.cpp`:
- Add `pe->warpTime += deltaTime * pe->effects.proceduralWarp.warpSpeed;` in the time update section (~line 172)

### Shader

Modify `shaders/feedback.fs`:
- Add 3 uniforms: `warp`, `warpTime`, `warpScaleInverse`
- Add warpFactors computation and UV displacement (see Technical Implementation)
- Apply warp after stretch, before rotation

### Shader Setup

`src/render/shader_setup.cpp` - Extend `SetupFeedback()`:
- Pass warp, warpTime, and `1.0f / warpScale` as warpScaleInverse

### UI Controls

`src/ui/imgui_effects.cpp` - Add "Procedural Warp" subsection in Flow Field:
- Warp amplitude slider (0-2)
- Warp speed slider (0.1-2.0)
- Warp scale slider (0.1-100)

### Preset Serialization

`src/config/preset.cpp`:
- Add NLOHMANN_DEFINE_TYPE macro for ProceduralWarpConfig
- Add to_json/from_json for proceduralWarp

### Parameter Registration

`src/automation/param_registry.cpp`:
- Register `proceduralWarp.warp`, `proceduralWarp.warpSpeed`, `proceduralWarp.warpScale`

**Done when**: Setting `warp > 0` creates animated undulating distortion. Higher warpScale creates finer waves. warpSpeed controls animation rate. The characteristic MilkDrop "psychedelic flowing" look emerges.

---

## Verification Checklist

- [ ] Center (cx/cy) shifts zoom/rotation pivot point
- [ ] Stretch (sx/sy) creates anamorphic distortion independent of zoom
- [ ] Angular modulation creates N-fold symmetric patterns
- [ ] Procedural warp animates smoothly with time
- [ ] warpScale controls spatial frequency of distortion
- [ ] All params save/load in presets
- [ ] All float params respond to modulation
- [ ] Build succeeds with no warnings

---

## Implementation Notes

**UI Reorganization**: Flow Field section uses subsections with `ImGui::SeparatorText()`:
- Base: Zoom, Spin, DX, DY (uniform values)
- Radial: Zoom, Spin, DX, DY (distance-from-center coefficients)
- Angular: Zoom, Spin, DX, DY + Freq sliders (polar-angle modulation)
- Center, Stretch, Warp, Gradient Flow

Slider labels use `##section` suffixes for unique ImGui IDs (e.g., `"Zoom##base"`, `"Zoom##radial"`).

**Angular DX/DY**: Extended beyond original MilkDrop spec to include `dxAngular`, `dxAngularFreq`, `dyAngular`, `dyAngularFreq` for full parity with radial modulation. Creates N-fold symmetric translation patterns.
