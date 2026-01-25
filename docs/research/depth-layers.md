# Depth Layers

A general-purpose multi-layer parallax and distortion effect. Renders multiple copies of the input texture at varying depths with independent transforms and warps, then composites them together. Highly configurable to reproduce existing effects (Infinite Zoom, Sine Warp, Texture Warp) and explore new territory.

## Classification

- **Category**: TRANSFORMS → Motion (or new "Layered" subcategory)
- **Core Operation**: Multi-sample UV transform with weighted blending
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## References

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - Recursive `fbm(p + fbm(p))` distortion technique
- [Inigo Quilez - fBM](https://iquilezles.org/articles/fbm/) - Fractal noise layering with octaves, lacunarity, gain
- [The Book of Shaders - Cellular Noise](https://thebookofshaders.com/12/) - Worley/Voronoi noise for organic patterns
- [The Book of Shaders - fBM](https://thebookofshaders.com/13/) - Fractal Brownian Motion fundamentals
- [Catlike Coding - Texture Distortion](https://catlikecoding.com/unity/tutorials/flow/texture-distortion/) - Flow map animation with phase cycling
- [Emil Dziewanowski - Curl Noise](https://emildziewanowski.com/curl-noise/) - Divergence-free flow field generation
- [Cyanilux - Polar Coordinates](https://www.cyanilux.com/tutorials/polar-coordinates/) - Cartesian ↔ polar for spiral effects
- [Harry Alisavakis - Radial Blur](https://halisavakis.com/my-take-on-shaders-radial-blur/) - Multi-sample radial motion blur
- [Bridson et al. - Curl-Noise for Procedural Fluid Flow](https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf) - Academic reference for curl noise
- [Alan Zucconi - Parallax Shaders](https://www.alanzucconi.com/2019/01/01/parallax-shader/) - Depth-based UV offset techniques

## Algorithm

### Core Loop

```glsl
vec3 colorAccum = vec3(0.0);
float weightAccum = 0.0;

for (int i = 0; i < layers; i++) {
    // 1. Compute depth value for this layer
    float depth = depthFunction(i, layers, time, cycleSpeed);

    // 2. Compute weight (contribution) based on depth
    float weight = weightFunction(depth, falloffType, falloffPower);

    // 3. Transform UV for this layer
    vec2 layerUV = uv;
    layerUV = applyOffset(layerUV, depth, offsetX, offsetY, offsetSpeed, time);
    layerUV = applyScale(layerUV, depth, depthScale);
    layerUV = applyRotation(layerUV, depth, rotationAngle, rotationSpeed, time);
    layerUV = applyTwist(layerUV, twist);
    layerUV = applyWarp(layerUV, depth, warpType, warpParams, time);

    // 4. Optional: cascade mode feeds UV to next iteration
    if (cascade) uv = layerUV;

    // 5. Bounds handling
    if (outOfBounds(layerUV)) continue; // or mirror/wrap/clamp

    // 6. Sample and accumulate
    vec3 sample = texture(texture0, layerUV).rgb;
    colorAccum = blend(colorAccum, sample, weight, blendMode);
    weightAccum += weight;
}

// Normalize for weighted average mode
if (blendMode == WEIGHTED_AVERAGE && weightAccum > 0.0) {
    colorAccum /= weightAccum;
}
```

### Depth Functions

```glsl
float depthFunction(int i, int layers, float time, float cycleSpeed) {
    float phase = float(i) / float(layers);

    // Optional: cycle layers through depth over time
    if (cycleSpeed != 0.0) {
        phase = fract(phase + time * cycleSpeed);
    }

    switch (depthCurve) {
        case EXPONENTIAL:  return exp2(phase * depthScale);
        case LINEAR:       return 1.0 + phase * depthScale;
        case LOGARITHMIC:  return log2(1.0 + phase * depthScale);
        case INVERSE:      return 1.0 / (1.0 + phase * depthScale);
        case SIGMOID:      return 1.0 / (1.0 + exp(-(phase - 0.5) * depthScale));
    }
}
```

### Weight Functions

```glsl
float weightFunction(float depth, int falloffType, float power) {
    switch (falloffType) {
        case LINEAR:        return max(0.0, 1.0 - depth * power);
        case INVERSE:       return 1.0 / (depth * power);
        case INVERSE_SQ:    return 1.0 / (depth * depth * power);
        case EXPONENTIAL:   return exp(-depth * power);
        case COSINE:        return (1.0 - cos(depth * 3.14159)) * 0.5;
        case GAUSSIAN:      return exp(-pow(depth - focusDepth, 2.0) / (2.0 * power * power));
    }
}
```

### Warp Functions

```glsl
vec2 applyWarp(vec2 p, float depth, int warpType, WarpParams w, float time) {
    float strength = w.strength / depth; // Attenuate with depth

    switch (warpType) {
        case WARP_NONE:
            return p;

        case WARP_SINE:
            p.x += sin(p.y * w.freq + time * w.speed) * strength;
            p.y += sin(p.x * w.freq + time * w.speed * 1.3) * strength;
            return p;

        case WARP_NOISE_PERLIN:
            return p + vec2(perlinNoise(p * w.freq + time * w.speed),
                           perlinNoise(p * w.freq + time * w.speed + 100.0)) * strength;

        case WARP_NOISE_SIMPLEX:
            return p + vec2(simplexNoise(p * w.freq + time * w.speed),
                           simplexNoise(p * w.freq + time * w.speed + 100.0)) * strength;

        case WARP_NOISE_WORLEY:
            return p + (worleyNoise(p * w.freq + time * w.speed).xy - 0.5) * strength;

        case WARP_FBM:
            return p + vec2(fbm(p * w.freq + time * w.speed, w.octaves, w.lacunarity, w.gain),
                           fbm(p * w.freq + time * w.speed + 100.0, w.octaves, w.lacunarity, w.gain)) * strength;

        case WARP_TURBULENCE:
            return p + vec2(turbulence(p * w.freq + time * w.speed, w.octaves),
                           turbulence(p * w.freq + time * w.speed + 100.0, w.octaves)) * strength;

        case WARP_RIDGED:
            return p + vec2(ridgedNoise(p * w.freq + time * w.speed, w.octaves),
                           ridgedNoise(p * w.freq + time * w.speed + 100.0, w.octaves)) * strength;

        case WARP_DOMAIN:
            // Recursive: fbm(p + fbm(p + fbm(p)))
            vec2 q = vec2(fbm(p + vec2(0.0), w.octaves, w.lacunarity, w.gain),
                         fbm(p + vec2(5.2, 1.3), w.octaves, w.lacunarity, w.gain));
            vec2 r = vec2(fbm(p + w.warpScale * q + vec2(1.7, 9.2), w.octaves, w.lacunarity, w.gain),
                         fbm(p + w.warpScale * q + vec2(8.3, 2.8), w.octaves, w.lacunarity, w.gain));
            return p + r * strength;

        case WARP_CURL:
            return p + curlNoise(p * w.freq + time * w.speed) * strength;

        case WARP_RADIAL:
            vec2 dir = normalize(p - center);
            return p + dir * strength;

        case WARP_TANGENT:
            vec2 dir = normalize(p - center);
            vec2 tangent = vec2(-dir.y, dir.x);
            return p + tangent * strength;

        case WARP_TWIRL:
            float r = length(p - center);
            float angle = strength / (r + 0.001);
            return rotateAround(p, center, angle);

        case WARP_PINCH:
            vec2 dir = p - center;
            float r = length(dir);
            float scale = 1.0 + strength * (1.0 - smoothstep(0.0, w.radius, r));
            return center + dir * scale;

        case WARP_POLAR:
            // Convert to polar, manipulate, convert back
            vec2 centered = p - center;
            float r = length(centered);
            float theta = atan(centered.y, centered.x);
            // Could animate r or theta here
            return center + vec2(cos(theta), sin(theta)) * r;
    }
}
```

### Noise Primitives

```glsl
// Simplex noise (cheaper than Perlin, fewer artifacts)
float simplexNoise(vec2 p) {
    // IQ's simplex implementation
    const float K1 = 0.366025404; // (sqrt(3)-1)/2
    const float K2 = 0.211324865; // (3-sqrt(3))/6
    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = step(a.yx, a.xy);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    vec3 n = h * h * h * h * vec3(dot(a, hash(i)), dot(b, hash(i+o)), dot(c, hash(i+1.0)));
    return dot(n, vec3(70.0));
}

// fBM with configurable parameters
float fbm(vec2 p, int octaves, float lacunarity, float gain) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * simplexNoise(p * frequency);
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return value;
}

// Turbulence: fBM with abs() for sharp valleys
float turbulence(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * abs(simplexNoise(p * frequency));
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Ridged noise: inverted turbulence for sharp peaks
float ridgedNoise(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float prev = 1.0;
    for (int i = 0; i < octaves; i++) {
        float n = 1.0 - abs(simplexNoise(p * frequency));
        n = n * n * prev;
        value += amplitude * n;
        prev = n;
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Curl noise: divergence-free 2D flow
vec2 curlNoise(vec2 p) {
    float eps = 0.001;
    float n1 = simplexNoise(p + vec2(0, eps));
    float n2 = simplexNoise(p - vec2(0, eps));
    float n3 = simplexNoise(p + vec2(eps, 0));
    float n4 = simplexNoise(p - vec2(eps, 0));
    float dndx = (n3 - n4) / (2.0 * eps);
    float dndy = (n1 - n2) / (2.0 * eps);
    return vec2(dndy, -dndx); // Perpendicular to gradient
}

// Worley/cellular noise
vec2 worleyNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float minDist = 1.0;
    vec2 minPoint = vec2(0.0);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash(i + neighbor);
            vec2 diff = neighbor + point - f;
            float dist = length(diff);
            if (dist < minDist) {
                minDist = dist;
                minPoint = diff;
            }
        }
    }
    return vec2(minDist, length(minPoint));
}
```

## Parameters

### Structure (Layers)

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `layers` | int | 2-16 | 6 | Number of depth layers |
| `depthCurve` | enum | - | Exponential | How depth maps to layer index |
| `depthScale` | float | 0.5-10.0 | 2.0 | Depth range multiplier |
| `cascade` | bool | - | false | Iterative vs independent UV transform |

### Per-Layer Transform

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `offsetX` | float | -1.0 to 1.0 | 0.0 | Parallax X per depth unit |
| `offsetY` | float | -1.0 to 1.0 | 0.0 | Parallax Y per depth unit |
| `offsetSpeed` | float | 0.0-2.0 | 0.0 | Parallax drift rate |
| `rotationAngle` | float | -π to π | 0.0 | Static rotation per depth |
| `rotationSpeed` | float | ±ROTATION_SPEED_MAX | 0.0 | Animated rotation (rad/frame) |
| `twist` | float | -π to π | 0.0 | log(r)-based spiral twist |

### Warp

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `warpType` | enum | - | None | Distortion type (see Warp Types below) |
| `warpStrength` | float | 0.0-2.0 | 0.5 | Distortion amplitude |
| `warpFreq` | float | 0.5-10.0 | 2.0 | Spatial frequency |
| `warpSpeed` | float | 0.0-2.0 | 0.5 | Animation rate |
| `warpOctaves` | int | 1-8 | 4 | Noise octaves (fBM/turbulence/ridged) |
| `warpLacunarity` | float | 1.5-3.0 | 2.0 | Frequency multiplier per octave |
| `warpGain` | float | 0.3-0.7 | 0.5 | Amplitude multiplier per octave |
| `warpDepthAtten` | bool | - | true | Attenuate warp with depth |

### Blending

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `blendMode` | enum | - | WeightedAvg | Layer compositing mode |
| `falloffType` | enum | - | Inverse | Weight curve shape |
| `falloffPower` | float | 0.1-4.0 | 1.0 | Falloff curve intensity |
| `focusDepth` | float | 0.0-1.0 | 0.5 | Focus plane for Gaussian falloff |

### Animation

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `cycleSpeed` | float | -2.0 to 2.0 | 0.0 | Layer depth cycling rate |
| `centerX` | float | 0.0-1.0 | 0.5 | Focal center X |
| `centerY` | float | 0.0-1.0 | 0.5 | Focal center Y |

### Bounds

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `boundsMode` | enum | - | Discard | Out-of-bounds handling |
| `edgeSoftness` | float | 0.0-0.2 | 0.05 | Fade near boundaries |

## Warp Types

| ID | Name | Character | Key Params |
|----|------|-----------|------------|
| 0 | None | Pure parallax/scale | - |
| 1 | Sine | Smooth waves | freq, speed |
| 2 | Perlin | Gradient noise | freq, speed |
| 3 | Simplex | Cleaner gradient noise | freq, speed |
| 4 | Worley | Cellular/organic | freq, speed |
| 5 | fBM | Layered fractal noise | octaves, lacunarity, gain |
| 6 | Turbulence | Sharp valleys | octaves |
| 7 | Ridged | Sharp peaks | octaves |
| 8 | Domain Warp | Recursive `fbm(p+fbm(p))` | warpScale, octaves |
| 9 | Curl | Divergence-free swirl | freq, speed |
| 10 | Radial | Push/pull from center | strength |
| 11 | Tangent | Swirl around center | strength |
| 12 | Twirl | Radius-based rotation | strength, radius |
| 13 | Pinch | Radial scale distortion | strength, radius |
| 14 | Polar | Cartesian ↔ polar | - |

## Depth Curve Types

| ID | Name | Formula | Character |
|----|------|---------|-----------|
| 0 | Exponential | `exp2(phase * scale)` | Dramatic separation |
| 1 | Linear | `1 + phase * scale` | Even spacing |
| 2 | Logarithmic | `log2(1 + phase * scale)` | Compressed far |
| 3 | Inverse | `1 / (1 + phase * scale)` | Rapid falloff |
| 4 | Sigmoid | `1 / (1 + exp(-phase * scale))` | S-curve |

## Blend Modes

| ID | Name | Formula |
|----|------|---------|
| 0 | Weighted Average | `Σ(c*w) / Σw` |
| 1 | Additive | `Σ(c*w)` clamped |
| 2 | Max | `max(a, b)` |
| 3 | Screen | `1 - (1-a)*(1-b)` |
| 4 | Multiply | `a * b` |
| 5 | Soft Light | Pegtop formula |
| 6 | Difference | `abs(a - b)` |

## Falloff Types

| ID | Name | Formula |
|----|------|---------|
| 0 | Linear | `1 - depth * power` |
| 1 | Inverse | `1 / (depth * power)` |
| 2 | Inverse Square | `1 / (depth² * power)` |
| 3 | Exponential | `exp(-depth * power)` |
| 4 | Cosine | `(1 - cos(depth * π)) / 2` |
| 5 | Gaussian | `exp(-(depth - focus)² / σ²)` |

## Bounds Modes

| ID | Name | Behavior |
|----|------|----------|
| 0 | Discard | Skip layer if UV out of [0,1] |
| 1 | Clamp | Clamp UV to [0,1] (stretchy edges) |
| 2 | Mirror | `1 - abs(mod(uv, 2) - 1)` |
| 3 | Wrap | `fract(uv)` (tile) |

## Modulation Candidates

All float parameters register in `param_registry.cpp` for standard modulation:

**High-impact targets:**
- `warpStrength` - Bass/beat for pulsing distortion
- `cycleSpeed` - LFO for rhythmic zoom cycling
- `rotationSpeed` - Mid for swirling motion
- `offsetX/Y` - Centroid for tonal drift
- `twist` - Beat for spiral punches

**Subtle modulation:**
- `falloffPower` - Treble for depth-of-field shifts
- `warpFreq` - LFO for breathing patterns
- `depthScale` - Bass for depth breathing

## Preset Recipes

### Infinite Zoom Clone
```
layers: 6, depthCurve: Exponential, depthScale: 3.0
cycleSpeed: 1.0, cascade: false
warpType: None
blendMode: WeightedAvg, falloffType: Cosine
```

### Sine Warp Clone
```
layers: 4, cascade: true
warpType: Sine, warpStrength: 0.5, warpFreq: 2.0
rotationAngle: 0.5 (per layer via depth)
blendMode: WeightedAvg, falloffType: Inverse
```

### Texture Warp Clone
```
layers: 4, cascade: true
warpType: DomainWarp, warpOctaves: 4
blendMode: WeightedAvg
```

### Parallax Drift
```
layers: 8, depthCurve: Linear
offsetX: 0.1, offsetY: 0.05, offsetSpeed: 0.2
warpType: None
blendMode: Additive, falloffType: InverseSquare
```

### Psychedelic Swirl
```
layers: 12, depthCurve: Exponential
warpType: Curl, warpStrength: 1.0
twist: 0.5, rotationSpeed: 0.01
blendMode: Screen, falloffType: Exponential
```

### Turbulent Fog
```
layers: 8, cascade: true
warpType: Turbulence, warpOctaves: 6, warpStrength: 0.8
blendMode: WeightedAvg, falloffType: Gaussian, focusDepth: 0.3
```

## Implementation Notes

1. **Shader complexity**: With 15 warp types and multiple blend modes, consider either:
   - Uber-shader with switch statements (simpler, some branch divergence)
   - Shader permutations (faster, more compile time)

2. **Noise textures**: For performance, pre-bake noise into textures rather than computing procedurally. Provide 2-3 tiling noise textures (Perlin, Simplex, Worley) and sample them.

3. **Layer count**: With 16 layers and complex warps, could hit performance limits. Profile and consider dynamic quality adjustment.

4. **Cascade mode gotcha**: When `cascade=true`, the iteration order matters. Early layers influence later ones exponentially - small parameter changes have big effects.

5. **Angular params**: `rotationAngle`, `rotationSpeed`, `twist` follow standard conventions:
   - `*Speed` → rad/frame, CPU accumulation, ±ROTATION_SPEED_MAX
   - `*Angle` → radians, ±ROTATION_OFFSET_MAX
   - Display as degrees in UI via `ModulatableSliderAngleDeg`

6. **Center point**: Consider making `centerX/Y` draggable via `DrawInteractiveHandle` like other effects.

## UI Organization

Suggested collapsible sections:
1. **Layers** - count, depth curve, depth scale, cascade toggle
2. **Transform** - offset, rotation, twist
3. **Warp** - type dropdown, strength/freq/speed, fBM params (show/hide based on type)
4. **Blending** - mode, falloff type, falloff power, focus depth
5. **Animation** - cycle speed, center point
6. **Bounds** - mode, edge softness
