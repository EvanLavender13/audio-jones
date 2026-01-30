# Sine Warp & Radial Pulse Unification

Add radial mode and depth blend toggle to Sine Warp. Add octaves and depth blend toggle to Radial Pulse. This allows comparison of both effects with equivalent feature sets to determine if they remain distinct.

## Specification

### Sine Warp Config Changes

```cpp
struct SineWarpConfig {
  bool enabled = false;
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float animRate = 0.3f;       // Animation rate (radians/second, 0.0-2.0)
  float octaveRotation = 0.5f; // Rotation per octave in radians (±π)
  bool radialMode = false;     // NEW: false=Cartesian, true=Polar
  bool depthBlend = true;      // NEW: true=sample each octave, false=sample once
};
```

### Radial Pulse Config Changes

```cpp
struct RadialPulseConfig {
  bool enabled = false;
  float radialFreq = 8.0f;    // Ring density (1.0-30.0)
  float radialAmp = 0.05f;    // Radial displacement strength (±0.3)
  int segments = 6;           // Petal count (2-16)
  float angularAmp = 0.1f;    // Tangential swirl strength (±0.5)
  float petalAmp = 0.0f;      // Radial petal modulation (±1.0)
  float phaseSpeed = 1.0f;    // Animation speed (±5.0 rad/sec)
  float spiralTwist = 0.0f;   // Angular phase shift per radius (radians)
  int octaves = 1;            // NEW: Number of octaves (1-8)
  bool depthBlend = false;    // NEW: true=sample each octave, false=sample once
};
```

### Sine Warp Shader Algorithm

```glsl
uniform bool radialMode;
uniform bool depthBlend;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 p = (fragTexCoord - 0.5) * 2.0;

    for (int i = 0; i < octaves; i++) {
        float freq = pow(2.0, float(i));
        float amp = 1.0 / freq;

        if (radialMode) {
            // Polar displacement: convert to polar, displace radius, convert back
            float radius = length(p);
            float angle = atan(p.y, p.x);

            // Radial sine displacement
            radius += sin(radius * freq + time) * amp * strength;

            // Convert back to Cartesian
            p = vec2(cos(angle), sin(angle)) * radius;
        } else {
            // Existing Cartesian displacement
            p.x += sin(p.y * freq + time) * amp * strength;
            p.y += sin(p.x * freq + time * 1.3) * amp * strength;
        }

        // Per-octave rotation (applies to both modes)
        float angle = float(i) * octaveRotation + rotation;
        float c = cos(angle);
        float s = sin(angle);
        p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);

        if (depthBlend) {
            // Sample at this depth
            vec2 mapped = p * 0.5 + 0.5;
            vec2 sampleUV = 1.0 - abs(mod(mapped, 2.0) - 1.0);
            float weight = amp;
            colorAccum += texture(texture0, sampleUV).rgb * weight;
            weightAccum += weight;
        }
    }

    if (depthBlend) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        // Sample once at final position
        vec2 mapped = p * 0.5 + 0.5;
        vec2 sampleUV = 1.0 - abs(mod(mapped, 2.0) - 1.0);
        finalColor = texture(texture0, sampleUV);
    }
}
```

### Radial Pulse Shader Algorithm

```glsl
uniform int octaves;
uniform bool depthBlend;

void main()
{
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    vec2 delta = fragTexCoord - vec2(0.5);
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    vec2 radialDir = (radius > 0.0001) ? normalize(delta) : vec2(1.0, 0.0);
    vec2 tangentDir = vec2(-radialDir.y, radialDir.x);

    vec2 totalDisp = vec2(0.0);

    for (int i = 0; i < octaves; i++) {
        float freq = radialFreq * pow(2.0, float(i));
        float amp = 1.0 / pow(2.0, float(i));

        float spiralPhase = phase + (radius * 2.0) * spiralTwist;
        float angularWave = sin(angle * float(segments) + spiralPhase);
        float petalMod = 1.0 + petalAmp * angularWave;

        float radialDisp = sin(radius * freq + phase) * radialAmp * amp * petalMod;
        float tangentDisp = angularWave * angularAmp * amp * radius;

        totalDisp += radialDir * radialDisp + tangentDir * tangentDisp;

        if (depthBlend) {
            vec2 uv = fragTexCoord + totalDisp;
            float weight = amp;
            colorAccum += texture(texture0, uv).rgb * weight;
            weightAccum += weight;
        }
    }

    if (depthBlend) {
        finalColor = vec4(colorAccum / weightAccum, 1.0);
    } else {
        vec2 uv = fragTexCoord + totalDisp;
        finalColor = texture(texture0, uv);
    }
}
```

### New Uniforms

**Sine Warp:**
| Uniform | Type | Config Field |
|---------|------|--------------|
| `radialMode` | bool (int) | `radialMode` |
| `depthBlend` | bool (int) | `depthBlend` |

**Radial Pulse:**
| Uniform | Type | Config Field |
|---------|------|--------------|
| `octaves` | int | `octaves` |
| `depthBlend` | bool (int) | `depthBlend` |

---

## Tasks

### Wave 1: Config Headers

Config changes must compile before other files can reference the new fields.

#### Task 1.1: Update SineWarpConfig

**Files**: `src/config/sine_warp_config.h`

**Build**:
1. Add `bool radialMode = false;` after `octaveRotation`
2. Add `bool depthBlend = true;` after `radialMode`
3. Update comment block to describe new fields

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Update RadialPulseConfig

**Files**: `src/config/radial_pulse_config.h`

**Build**:
1. Add `int octaves = 1;` after `spiralTwist`
2. Add `bool depthBlend = false;` after `octaves`
3. Update comment block to describe new fields

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks modify different files and can run simultaneously.

#### Task 2.1: Update Sine Warp Shader

**Files**: `shaders/sine_warp.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Add uniforms: `uniform bool radialMode;` and `uniform bool depthBlend;`
2. Replace main() with algorithm from spec:
   - Add radialMode branch inside the octave loop
   - Move sampling logic to conditional on depthBlend
   - Add final single-sample path when depthBlend is false
3. Keep existing mirror-repeat boundary handling

**Verify**: Shader compiles (build succeeds).

#### Task 2.2: Update Radial Pulse Shader

**Files**: `shaders/radial_pulse.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Add uniforms: `uniform int octaves;` and `uniform bool depthBlend;`
2. Replace main() with algorithm from spec:
   - Wrap existing displacement logic in octave loop
   - Scale freq by `pow(2.0, float(i))`, amp by `1.0 / pow(2.0, float(i))`
   - Accumulate displacement across octaves
   - Add depthBlend conditional for sampling strategy

**Verify**: Shader compiles (build succeeds).

#### Task 2.3: Update post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add after line ~167 (sineWarpOctaveRotationLoc):
   ```cpp
   int sineWarpRadialModeLoc;
   int sineWarpDepthBlendLoc;
   ```
2. Add after line ~315 (radialPulseSpiralTwistLoc):
   ```cpp
   int radialPulseOctavesLoc;
   int radialPulseDepthBlendLoc;
   ```

**Verify**: Compiles.

#### Task 2.4: Update post_effect.cpp

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find where sineWarp uniform locations are retrieved (search for `sineWarpTimeLoc`)
2. Add:
   ```cpp
   pe->sineWarpRadialModeLoc = GetShaderLocation(pe->sineWarpShader, "radialMode");
   pe->sineWarpDepthBlendLoc = GetShaderLocation(pe->sineWarpShader, "depthBlend");
   ```
3. Find where radialPulse uniform locations are retrieved (search for `radialPulseRadialFreqLoc`)
4. Add:
   ```cpp
   pe->radialPulseOctavesLoc = GetShaderLocation(pe->radialPulseShader, "octaves");
   pe->radialPulseDepthBlendLoc = GetShaderLocation(pe->radialPulseShader, "depthBlend");
   ```

**Verify**: Compiles.

#### Task 2.5: Update shader_setup_warp.cpp

**Files**: `src/render/shader_setup_warp.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `SetupSineWarp()`, add after existing SetShaderValue calls:
   ```cpp
   int radialMode = sw->radialMode ? 1 : 0;
   SetShaderValue(pe->sineWarpShader, pe->sineWarpRadialModeLoc, &radialMode,
                  SHADER_UNIFORM_INT);
   int depthBlend = sw->depthBlend ? 1 : 0;
   SetShaderValue(pe->sineWarpShader, pe->sineWarpDepthBlendLoc, &depthBlend,
                  SHADER_UNIFORM_INT);
   ```

**Verify**: Compiles.

#### Task 2.6: Update shader_setup_symmetry.cpp

**Files**: `src/render/shader_setup_symmetry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `SetupRadialPulse()`, add after existing SetShaderValue calls:
   ```cpp
   SetShaderValue(pe->radialPulseShader, pe->radialPulseOctavesLoc,
                  &rp->octaves, SHADER_UNIFORM_INT);
   int depthBlend = rp->depthBlend ? 1 : 0;
   SetShaderValue(pe->radialPulseShader, pe->radialPulseDepthBlendLoc,
                  &depthBlend, SHADER_UNIFORM_INT);
   ```

**Verify**: Compiles.

#### Task 2.7: Update imgui_effects_warp.cpp

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `DrawWarpSine()`, inside the `if (e->sineWarp.enabled)` block, add after Octave Rotation slider:
   ```cpp
   ImGui::Checkbox("Radial Mode##sineWarp", &e->sineWarp.radialMode);
   ImGui::Checkbox("Depth Blend##sineWarp", &e->sineWarp.depthBlend);
   ```

**Verify**: Compiles.

#### Task 2.8: Update imgui_effects_symmetry.cpp

**Files**: `src/ui/imgui_effects_symmetry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `DrawSymmetryRadialPulse()`, inside the `if (e->radialPulse.enabled)` block, add after Spiral Twist slider:
   ```cpp
   ImGui::SliderInt("Octaves##radpulse", &rp->octaves, 1, 8);
   ImGui::Checkbox("Depth Blend##radpulse", &rp->depthBlend);
   ```

**Verify**: Compiles.

#### Task 2.9: Update preset.cpp

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Find `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SineWarpConfig, ...)` (line ~162)
2. Add `radialMode, depthBlend` to the end of the field list
3. Find `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialPulseConfig, ...)` (line ~233)
4. Add `octaves, depthBlend` to the end of the field list

**Verify**: Compiles.

---

## Final Verification

After all waves complete:
- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Run application, enable Sine Warp:
  - [ ] Toggle Radial Mode - visual changes from turbulence to concentric rings
  - [ ] Toggle Depth Blend off - result is sharper, single-sample
- [ ] Run application, enable Radial Pulse:
  - [ ] Increase Octaves - rings become multi-scale
  - [ ] Toggle Depth Blend on - result is ghosty/layered
- [ ] Save and load preset with new options - values persist
