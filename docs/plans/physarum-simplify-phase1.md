# Physarum Simplification Phase 1: Remove Color, Add Trail Map

Strip agent color and color deposition. Agents read and write to a dedicated trail map texture rendered as a full-screen debug overlay.

## Problem

The current physarum implementation couples color handling (HSV conversions, hue-based attraction/repulsion) with core agent behavior. This complexity blocks future refactoring. Phase 1 isolates the trail system by:

1. Removing color from agents (no `hue` field, no `ColorConfig`)
2. Creating a dedicated single-channel trail map owned by Physarum
3. Rendering trail map as debug visual (full-screen grayscale)

This is phase 1 of a multi-step refactor. Future phases will add decay, compositing, and visual integration.

## Targets

| Location | Current | Target |
|----------|---------|--------|
| `src/render/physarum.h:12` | `float hue` in agent struct | Remove field |
| `src/render/physarum.h:23` | `ColorConfig color` in config | Remove field |
| `src/render/physarum.cpp:18-53` | Hue distribution logic | Remove entirely |
| `src/render/physarum.cpp:158-168` | Saturation/value extraction | Remove entirely |
| `shaders/physarum_agents.glsl:44-100` | HSV/RGB conversion functions | Remove entirely |
| `shaders/physarum_agents.glsl:104-129` | Hue-based trail scoring | Replace with intensity-only |
| `shaders/physarum_agents.glsl:180-194` | Color deposition | Replace with intensity deposition |
| `src/ui/ui_panel_effects.cpp:51-52` | Color controls | Remove |

## Architecture

### Trail Map Ownership

Physarum owns its trail map texture:

```
Physarum struct
├── trailMap (RenderTexture2D, R32F single-channel)
├── agents (SSBO)
└── computeProgram
```

### Data Flow (Simplified)

```
┌─────────────────┐
│  PhysarumUpdate │
│                 │
│  agents[] ──────┼──► sense trailMap intensity
│                 │◄── write trailMap intensity
│                 │
│  trailMap ──────┼──► (no decay this phase)
└─────────────────┘
         │
         ▼
┌─────────────────┐
│PhysarumDrawDebug│
│                 │
│  trailMap ──────┼──► full-screen grayscale
└─────────────────┘
```

### Agent Struct (After)

```cpp
typedef struct PhysarumAgent {
    float x;
    float y;
    float heading;
    float _pad;  // Maintain 16-byte alignment for GPU
} PhysarumAgent;
```

### Shader Sensing (After)

```glsl
float sampleTrail(vec2 pos)
{
    ivec2 coord = ivec2(mod(pos, resolution));
    return imageLoad(trailMap, coord).r;
}
```

### Shader Deposition (After)

```glsl
// Deposit intensity at agent position
ivec2 coord = ivec2(pos);
float current = imageLoad(trailMap, coord).r;
float deposited = min(current + depositAmount, 1.0);
imageStore(trailMap, coord, vec4(deposited, 0.0, 0.0, 0.0));
```

## Phases

### Phase 1.1: Strip Color from C++ Code

**Files:**
- `src/render/physarum.h`
- `src/render/physarum.cpp`

**Changes:**

1. Remove `hue` from `PhysarumAgent`, add `_pad` for alignment
2. Remove `ColorConfig color` from `PhysarumConfig`
3. Remove `saturationLoc`, `valueLoc` from `Physarum` struct
4. Add `RenderTexture2D trailMap` to `Physarum` struct
5. Remove `#include "color_config.h"`
6. Remove hue distribution in `InitializeAgents()`
7. Remove `ColorConfigChanged()` function
8. Remove saturation/value uniform handling in `PhysarumUpdate()`
9. Create R32F trail map texture in `PhysarumInit()`
10. Clear trail map in `PhysarumReset()`
11. Resize trail map in `PhysarumResize()`
12. Unload trail map in `PhysarumUninit()`
13. Change `PhysarumUpdate()` to use internal `trailMap` (remove `target` parameter)
14. Add `PhysarumDrawDebug()` function

### Phase 1.2: Simplify Shader

**File:** `shaders/physarum_agents.glsl`

**Changes:**

1. Remove `hue` from `Agent` struct, add padding
2. Change `trailMap` format from `rgba32f` to `r32f`
3. Remove `saturation` and `value` uniforms
4. Remove `hsv2rgb()` function
5. Remove `rgb2hue()` function
6. Replace `sampleTrailScore()` with `sampleTrail()` (returns intensity only)
7. Update movement logic to use intensity-based sensing
8. Replace color deposition with intensity deposition

### Phase 1.3: Update Integration Points

**Files:**
- `src/render/post_effect.cpp`
- `src/ui/ui_panel_effects.cpp`

**Changes:**

1. Remove `target` argument from `PhysarumUpdate()` call in `ApplyPhysarumPass()`
2. Add `PhysarumDrawDebug()` call (conditionally, controlled by debug flag or always-on for now)
3. Remove `UIDrawColorControls()` call for physarum
4. Remove `physarumColorMode` from `EffectsPanelDropdowns`

## File Changes

| File | Change |
|------|--------|
| `src/render/physarum.h` | Remove hue/color, add trailMap, add PhysarumDrawDebug |
| `src/render/physarum.cpp` | Remove color logic, add trail map management, add debug draw |
| `shaders/physarum_agents.glsl` | Strip color, use intensity-only sensing/deposition |
| `src/render/post_effect.cpp` | Update PhysarumUpdate call, add debug draw |
| `src/ui/ui_panel_effects.cpp` | Remove color controls |

## Verification

- [ ] Build succeeds with no errors
- [ ] Physarum checkbox enables/disables simulation
- [ ] Agent count slider still works
- [ ] Sensor/angle/step/deposit sliders still affect behavior
- [ ] Full-screen grayscale overlay shows trail intensity
- [ ] Trails accumulate where agents travel (no decay expected)
- [ ] No color-related uniforms or config remain

## Notes

- Trail map uses R32F format (single 32-bit float channel) for efficiency
- Agent struct maintains 16-byte alignment with `_pad` field for GPU compatibility
- Debug visual is temporary; future phases will composite trails into main output
- No decay implemented this phase; trails will accumulate indefinitely until reset
