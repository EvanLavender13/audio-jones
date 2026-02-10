# Purposeful Terrain Warping

The pseudo-3D landscape effect in SMOOTHBOB emerges from Texture Warp (Polar mode, 8 iterations) combined with Heightfield Relief. The warped color blobs create smooth gradients that heightfield relief interprets as terrain slopes. This document explores two approaches to create this effect more purposefully.

## Why the Current Effect Works

Texture Warp's Polar mode converts RGB to HSV, using hue as flow direction and saturation as magnitude:
```glsl
vec3 hsv = rgb2hsv(s);
float angle = hsv.x * 6.28318530718;
offset = vec2(cos(angle), sin(angle)) * hsv.y;
```

Multiple iterations compound these offsets, creating self-similar blob patterns. Heightfield Relief then uses Sobel gradients to derive surface normals and applies directional lighting. The brain interprets the resulting shadows/highlights as 3D terrain.

The limitation: results depend entirely on input colors. No direct control over terrain shape.

## References

- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - Nested fbm warping for organic patterns
- [Interactive Domain Warping](https://st4yho.me/domain-warping-an-interactive-introduction/) - Iterative warping with falloff
- [Catlike Coding - Texture Distortion](https://catlikecoding.com/unity/tutorials/flow/texture-distortion/) - Flow-based UV manipulation

---

## Approach A: Directional Texture Warp Enhancement

Enhance existing Texture Warp with directional bias and optional noise injection.

### Classification

- **Category**: TRANSFORMS → Warp (enhancement to existing effect)
- **Core Operation**: Bias iterative UV warping along a preferred axis
- **Pipeline Position**: Transform chain (reorderable)

### Algorithm

Add directional anisotropy by scaling the offset vector along a ridge axis:

```glsl
// New uniforms
uniform float ridgeAngle;    // Direction of ridges (radians)
uniform float anisotropy;    // 0.0 = isotropic, 1.0 = fully directional
uniform float noiseAmount;   // Blend in procedural noise when input lacks texture

// In the warp loop, after computing offset:
vec2 ridgeDir = vec2(cos(ridgeAngle), sin(ridgeAngle));
vec2 perpDir = vec2(-ridgeDir.y, ridgeDir.x);

// Decompose offset into ridge-parallel and ridge-perpendicular components
float ridgeComponent = dot(offset, ridgeDir);
float perpComponent = dot(offset, perpDir);

// Scale perpendicular component by anisotropy (1.0 - anisotropy suppresses it)
perpComponent *= (1.0 - anisotropy);

// Reconstruct biased offset
offset = ridgeDir * ridgeComponent + perpDir * perpComponent;

// Optional: inject noise when source texture is uniform
if (noiseAmount > 0.0) {
    float n = fbm(warpedUV * noiseScale);
    offset += noiseAmount * vec2(cos(n * 6.28), sin(n * 6.28));
}
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| ridgeAngle | float | 0 to 2π | Direction of terrain ridges (UI: degrees) |
| anisotropy | float | 0.0 to 1.0 | 0 = isotropic (current behavior), 1 = ridges fully aligned |
| noiseAmount | float | 0.0 to 1.0 | Procedural noise injection for uniform inputs |
| noiseScale | float | 1.0 to 20.0 | Frequency of injected noise |

### Audio Mapping Ideas

- **ridgeAngle** ← LFO or spectral centroid for slowly rotating terrain
- **anisotropy** ← Bass energy (low = chaotic, high = ordered ridges)
- **noiseAmount** ← Beat detection (pulse in noise on transients)

### Implementation Notes

- Requires adding 4 new uniforms to `TextureWarpConfig`
- Needs fbm function added to shader (or separate noise texture)
- UI: Add "Directional" subsection under Texture Warp

---

## Approach B: Domain Warping Effect (New)

Add a dedicated domain warping effect based on Inigo Quilez's technique.

### Classification

- **Category**: TRANSFORMS → Warp (new effect)
- **Core Operation**: Nested fbm-based UV displacement
- **Pipeline Position**: Transform chain (reorderable)

### Algorithm

The core technique warps the domain with fbm, then warps again:

```glsl
// From Inigo Quilez
float pattern(vec2 p, out vec2 q, out vec2 r) {
    q.x = fbm(p + vec2(0.0, 0.0));
    q.y = fbm(p + vec2(5.2, 1.3));

    r.x = fbm(p + 4.0 * q + vec2(1.7, 9.2));
    r.y = fbm(p + 4.0 * q + vec2(8.3, 2.8));

    return fbm(p + 4.0 * r);
}
```

For UV warping (sampling input texture):

```glsl
uniform sampler2D texture0;
uniform float warpStrength;   // Overall displacement magnitude
uniform float warpScale;      // Noise frequency
uniform int warpIterations;   // 1 = single warp, 2 = double (q then r)
uniform float falloff;        // Amplitude reduction per iteration
uniform vec2 timeOffset;      // Animated drift (accumulated on CPU)

vec2 fbm2(vec2 p) {
    // Return 2D fbm for x and y channels
    return vec2(
        fbm(p + vec2(0.0, 0.0)),
        fbm(p + vec2(5.2, 1.3))
    );
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p = uv * warpScale + timeOffset;

    float amp = warpStrength;
    for (int i = 0; i < warpIterations; i++) {
        vec2 displacement = fbm2(p);
        uv += displacement * amp;
        p = uv * warpScale + timeOffset + vec2(1.7 * float(i), 9.2 * float(i));
        amp *= falloff;
    }

    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

### FBM Implementation

Standard fbm with octaves:

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // Smoothstep

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| warpStrength | float | 0.0 to 0.5 | Displacement magnitude |
| warpScale | float | 1.0 to 10.0 | Noise frequency (smaller = larger features) |
| warpIterations | int | 1 to 3 | Nesting depth (1 = simple, 3 = complex terrain) |
| falloff | float | 0.3 to 0.8 | Amplitude reduction per iteration |
| driftSpeed | float | 0.0 to 1.0 | Animation speed (CPU accumulates timeOffset) |

### Audio Mapping Ideas

- **warpStrength** ← Overall energy / RMS
- **warpScale** ← Spectral centroid (bright = fine detail, dark = large features)
- **falloff** ← Beat detection (transients reduce falloff for sharper terrain)
- **driftSpeed** ← Tempo detection

### Implementation Notes

- New shader file: `shaders/domain_warp.fs`
- New config struct: `DomainWarpConfig`
- fbm has fixed 5 octaves; could expose as parameter if needed
- timeOffset accumulated on CPU to avoid animation jumps when parameters change

---

## Comparison

| Aspect | A: Directional Texture Warp | B: Domain Warp |
|--------|----------------------------|----------------|
| Input dependency | Uses existing texture colors | Works on any input (noise-driven) |
| Control | Ridge direction, anisotropy | Scale, iterations, falloff |
| Predictability | Still depends on source content | Consistent terrain-like results |
| Implementation | Modify existing shader | New shader + config |
| Performance | Minimal overhead | fbm sampling adds cost (~5 octaves × iterations) |

### Recommendation

**Approach A** preserves the "happy accident" behavior while adding directional control - good for when the input already has interesting color variation.

**Approach B** guarantees terrain-like results regardless of input - better for consistent, predictable landscapes that work with any source.

Both pair well with Heightfield Relief for the 3D lighting effect.
