# Kaleidoscope Slider Refactor

Refactor kaleidoscope from mode-based (Polar/KIFS enum) to slider-based architecture (like voronoi.fs). Each technique becomes an intensity slider (0.0-1.0) that blends with others. Add four new radial techniques: Droste, Iterative Mirror, Hex Lattice, Conformal Power Map.

## Current State

- `shaders/kaleidoscope.fs` — Mode-based shader (mode=0 Polar, mode=1 KIFS)
- `src/config/kaleidoscope_config.h:4-7` — `KaleidoscopeMode` enum
- `src/config/kaleidoscope_config.h:9-27` — Config struct with mode field
- `src/ui/imgui_effects.cpp:283-309` — UI with mode dropdown
- `src/render/shader_setup.cpp:124-153` — Uniform setup
- `src/render/post_effect.h:43-54` — Uniform locations
- `src/render/post_effect.cpp:63-74` — Location initialization
- `src/automation/param_registry.cpp:40-41,99-100` — Registered params
- `src/config/preset.cpp:101-103` — JSON serialization

Reference pattern: `shaders/voronoi.fs`, `src/config/voronoi_config.h`

## Technical Implementation

### Shader Architecture

Compute each technique's UV transform independently, sample texture, blend by intensity weights:

```glsl
// Technique intensities (0.0 = disabled)
uniform float polarIntensity;
uniform float kifsIntensity;
uniform float drosteIntensity;
uniform float iterMirrorIntensity;
uniform float hexFoldIntensity;
uniform float powerMapIntensity;

void main() {
    float totalIntensity = polarIntensity + kifsIntensity + drosteIntensity +
                           iterMirrorIntensity + hexFoldIntensity + powerMapIntensity;
    if (totalIntensity <= 0.0) {
        finalColor = texture(texture0, fragTexCoord);
        return;
    }

    vec2 uv = fragTexCoord - 0.5 - focalOffset;
    vec3 color = vec3(0.0);
    float weight = 0.0;

    if (polarIntensity > 0.0) {
        color += samplePolar(uv) * polarIntensity;
        weight += polarIntensity;
    }
    if (kifsIntensity > 0.0) {
        color += sampleKIFS(uv) * kifsIntensity;
        weight += kifsIntensity;
    }
    if (drosteIntensity > 0.0) {
        color += sampleDroste(uv) * drosteIntensity;
        weight += drosteIntensity;
    }
    if (iterMirrorIntensity > 0.0) {
        color += sampleIterMirror(uv) * iterMirrorIntensity;
        weight += iterMirrorIntensity;
    }
    if (hexFoldIntensity > 0.0) {
        color += sampleHexFold(uv) * hexFoldIntensity;
        weight += hexFoldIntensity;
    }
    if (powerMapIntensity > 0.0) {
        color += samplePowerMap(uv) * powerMapIntensity;
        weight += powerMapIntensity;
    }

    finalColor = vec4(color / weight, 1.0);
}
```

### Technique 1: Polar (existing)

Standard kaleidoscope — mirror angle into radial segments.

**Source**: `kaleidoscope.fs:148-184` (current implementation)

**UV Transform**:
```glsl
vec3 samplePolar(vec2 uv) {
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    // Radial twist (inner rotates more)
    angle += twistAngle * (1.0 - radius);

    // Mirror corners into circle
    if (radius > 0.5) radius = 1.0 - radius;

    // Apply rotation
    angle += rotation;

    // Apply fBM warp if enabled
    if (warpStrength > 0.0) {
        vec2 polar = vec2(cos(angle), sin(angle)) * radius;
        vec2 noiseCoord = polar * noiseScale + time * warpSpeed;
        polar += vec2(fbm(noiseCoord), fbm(noiseCoord + vec2(5.2, 1.3))) * warpStrength;
        radius = length(polar);
        angle = atan(polar.y, polar.x);
    }

    // Segment mirroring
    float segmentAngle = TWO_PI / float(segments);
    float a = mod(angle, segmentAngle);
    a = min(a, segmentAngle - a);

    vec2 newUV = vec2(cos(a), sin(a)) * radius + 0.5;
    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}
```

### Technique 2: KIFS (existing)

Kaleidoscopic IFS — iterative polar folding with scale/offset.

**Source**: `kaleidoscope.fs:90-146` (current implementation)

**UV Transform**:
```glsl
vec3 sampleKIFS(vec2 uv) {
    vec2 p = uv * 2.0;

    // Apply twist + rotation
    float r = length(p);
    p = rotate2d(p, twistAngle * (1.0 - r * 0.5) + rotation);

    // Accumulate from multiple depths
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    for (int i = 0; i < kifsIterations; i++) {
        float r = length(p);
        float a = atan(p.y, p.x);

        // Polar fold into segment
        float foldAngle = TWO_PI / float(segments);
        a = abs(mod(a + foldAngle * 0.5, foldAngle) - foldAngle * 0.5);

        // Back to Cartesian
        p = vec2(cos(a), sin(a)) * r;

        // IFS contraction
        p = p * kifsScale - kifsOffset;

        // Per-iteration rotation
        p = rotate2d(p, float(i) * 0.3 + time * 0.05);

        // Sample at this depth
        vec2 iterUV = 0.5 + 0.4 * sin(p * 0.15 * TWO_PI * 0.5);
        float w = 1.0 / float(i + 2);
        colorAccum += texture(texture0, iterUV).rgb * w;
        weightAccum += w;
    }

    // Final sample + warp
    vec2 newUV = 0.5 + 0.4 * sin(p * 0.3);
    if (warpStrength > 0.0) {
        vec2 noiseCoord = newUV * noiseScale + time * warpSpeed;
        newUV += vec2(fbm(noiseCoord), fbm(noiseCoord + vec2(5.2, 1.3))) * warpStrength;
    }

    vec3 finalSample = texture(texture0, fract(newUV)).rgb;
    return (weightAccum > 0.0) ? mix(finalSample, colorAccum / weightAccum, 0.5) : finalSample;
}
```

### Technique 3: Droste (new)

Log-polar spiral transform — Escher-like infinite zoom spiral.

**Source**: [reindernijhoff.net](https://reindernijhoff.net/2014/05/escher-droste-effect-webgl-fragment-shader/), `docs/research/kaleidoscope-techniques.md:19-39`

**Parameters**:
- `drosteScale` (float, 2.0-256.0): Scale ratio between recursive copies
- `drosteBranches` (float, 0.0-8.0): Spiral branches (0 = discrete, >0 = continuous spiral)

**UV Transform**:
```glsl
vec3 sampleDroste(vec2 uv) {
    float lnr = log(length(uv) + 0.001);
    float th = atan(uv.y, uv.x) + rotation;

    // Spiral factor from scale
    float sn = -log(drosteScale) / TWO_PI;

    // Apply segment mirroring to angle before Droste transform
    if (segments > 1) {
        float segmentAngle = TWO_PI / float(segments);
        th = mod(th, segmentAngle);
        th = min(th, segmentAngle - th);
    }

    // Droste spiral transform
    float newR = exp(lnr - th * sn);
    float newAngle = sn * lnr + th + drosteBranches * time * 0.1;

    // Apply twist
    newAngle += twistAngle * (1.0 - newR);

    vec2 newUV = vec2(cos(newAngle), sin(newAngle)) * newR * 0.5 + 0.5;

    // Tile to handle zoom range
    newUV = fract(newUV);

    return texture(texture0, newUV).rgb;
}
```

### Technique 4: Iterative Mirror (new)

Repeated rotation + mirror creates crystalline fractal patterns.

**Source**: `docs/research/milkdrop-kaleidoscope-fractal.md:86-114`

**Parameters**:
- `iterMirrorIterations` (int, 1-10): Number of fold iterations

**UV Transform**:
```glsl
vec2 mirror(vec2 x) {
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

vec3 sampleIterMirror(vec2 uv) {
    uv = uv * 2.0;  // Scale to -1..1 range
    float a = rotation + time * 0.1;

    for (int i = 0; i < iterMirrorIterations; i++) {
        // Rotate
        float c = cos(a), s = sin(a);
        uv = vec2(s * uv.y - c * uv.x, s * uv.x + c * uv.y);

        // Mirror
        uv = mirror(uv);

        // Evolve angle (creates variation per iteration)
        a += float(i + 1);
        a /= float(i + 1);
    }

    // Apply twist based on distance
    float r = length(uv);
    float angle = atan(uv.y, uv.x) + twistAngle * (1.0 - r);
    uv = vec2(cos(angle), sin(angle)) * r;

    return texture(texture0, mirror(uv * 0.5 + 0.5)).rgb;
}
```

### Technique 5: Hex Lattice Fold (new)

Tessellated hexagonal symmetry — "stained glass" patterns.

**Source**: [Shadertoy wtdSzX](https://www.shadertoy.com/view/wtdSzX)

**Parameters**:
- `hexScale` (float, 1.0-20.0): Hex cell density

**UV Transform**:
```glsl
const vec2 HEX_S = vec2(1.0, 1.7320508);  // 1, sqrt(3)

float hex(vec2 p) {
    p = abs(p);
    return max(dot(p, HEX_S * 0.5), p.x);
}

vec4 getHex(vec2 p) {
    vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / HEX_S.xyxy) + 0.5;
    vec4 h = vec4(p - hC.xy * HEX_S, p - (hC.zw + 0.5) * HEX_S);
    return dot(h.xy, h.xy) < dot(h.zw, h.zw)
        ? vec4(h.xy, hC.xy)
        : vec4(h.zw, hC.zw + 0.5);
}

vec3 sampleHexFold(vec2 uv) {
    // Apply rotation
    uv = rotate2d(uv, rotation);

    // Apply twist
    float r = length(uv);
    float angle = atan(uv.y, uv.x) + twistAngle * (1.0 - r);
    uv = vec2(cos(angle), sin(angle)) * r;

    // Scale and animate
    vec2 hexUV = uv * hexScale + HEX_S.yx * time * 0.2;

    // Get hex cell coordinates
    vec4 h = getHex(hexUV);

    // h.xy = position within hex cell, h.zw = cell ID
    // Fold within hex cell for symmetry
    vec2 cellUV = h.xy;

    // 6-fold symmetry within hex
    float cellAngle = atan(cellUV.y, cellUV.x);
    float segAngle = TWO_PI / 6.0;
    cellAngle = mod(cellAngle, segAngle);
    cellAngle = min(cellAngle, segAngle - cellAngle);
    float cellR = length(cellUV);
    cellUV = vec2(cos(cellAngle), sin(cellAngle)) * cellR;

    // Map back to texture coords
    vec2 newUV = cellUV / hexScale + 0.5;

    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}
```

### Technique 6: Conformal Power Map (new)

z^n transformation — smooth radial multiplication via complex plane.

**Source**: [hturan.com](https://hturan.com/writing/complex-numbers-glsl)

**Parameters**:
- `powerMapN` (float, 0.5-8.0): Power exponent (2 = doubling, 0.5 = square root)

**UV Transform**:
```glsl
vec3 samplePowerMap(vec2 uv) {
    // Apply rotation first
    uv = rotate2d(uv, rotation);

    // Complex power: z^n in polar form
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    // Apply twist before power
    a += twistAngle * (1.0 - r);

    // Power transform: r^n, angle*n
    float newR = pow(r + 0.001, powerMapN);
    float newAngle = a * powerMapN;

    // Segment mirroring on the result
    if (segments > 1) {
        float segmentAngle = TWO_PI / float(segments);
        newAngle = mod(newAngle, segmentAngle);
        newAngle = min(newAngle, segmentAngle - newAngle);
    }

    vec2 newUV = vec2(cos(newAngle), sin(newAngle)) * newR * 0.5 + 0.5;

    return texture(texture0, clamp(newUV, 0.0, 1.0)).rgb;
}
```

---

## Phase 1: Config and Shader Refactor

**Goal**: Replace mode enum with intensity sliders, restructure shader.

**Build**:
- `src/config/kaleidoscope_config.h`: Remove `KaleidoscopeMode` enum, add intensity floats for all 6 techniques, add new technique parameters (drosteScale, drosteBranches, iterMirrorIterations, hexScale, powerMapN)
- `shaders/kaleidoscope.fs`: Refactor to helper functions per technique, add intensity uniforms, implement blending loop in main()

**Done when**: Shader compiles, existing Polar behavior works via `polarIntensity=1.0`.

---

## Phase 2: C++ Integration

**Goal**: Wire new uniforms through render pipeline.

**Build**:
- `src/render/post_effect.h`: Add uniform location fields for 6 intensities + new params
- `src/render/post_effect.cpp`: Initialize all new uniform locations via `GetShaderLocation`
- `src/render/shader_setup.cpp`: Update `SetupKaleido` to pass all intensity values and new params

**Done when**: All uniforms pass to shader correctly (verify via RenderDoc or test preset).

---

## Phase 3: UI Controls

**Goal**: Replace mode dropdown with intensity sliders.

**Build**:
- `src/ui/imgui_effects.cpp`: Remove mode dropdown, add `ModulatableSlider` for each intensity (0.0-1.0), add sliders for new technique params, organize into collapsible groups (Polar, KIFS, Droste, etc.)

**Done when**: UI shows all sliders, adjusting them changes shader output.

---

## Phase 4: Param Registry and Serialization

**Goal**: Enable modulation and preset save/load.

**Build**:
- `src/automation/param_registry.cpp`: Register all new intensity params and technique-specific params
- `src/config/preset.cpp`: Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for new KaleidoscopeConfig fields

**Done when**: Presets save/load new params, LFO modulation works on intensities.

---

## Phase 5: Preset Migration

**Goal**: Update existing presets to new format.

**Build**:
- Manually edit each preset JSON in `presets/`:
  - Remove `"mode"` field
  - If `mode` was 0 (Polar): add `"polarIntensity": 1.0`
  - If `mode` was 1 (KIFS): add `"kifsIntensity": 1.0`
  - All other intensities default to 0.0

**Done when**: All presets load without errors, visual output matches pre-refactor.

---

## Phase 6: Polish and Testing

**Goal**: Verify all techniques work correctly and blend smoothly.

**Build**:
- Test each technique in isolation (intensity=1.0, others=0.0)
- Test blending combinations (Polar+Droste, KIFS+Hex, etc.)
- Verify shared params (segments, twist, warp) apply correctly to all techniques
- Performance check with multiple techniques active

**Done when**: All techniques render correctly, blending produces expected results, no performance regression.
