# Cyber March

Endless dark space filled with colorful square shapes at every scale, like a science fiction depiction of cyberspace. Camera flies through the structure while it breathes and shifts. Built from iterated fractal folding (Mandelbox/Menger family) where abs-reflections and axis sorts produce self-similar boxy geometry without any explicit box SDF.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator pass (blended onto pipeline)
- **Compute Model**: Single-pass fragment shader (raymarch), same as Voxel March

## Attribution

- **Based on**: "Fractalic Explorer 2" by nayk
- **Source**: https://www.shadertoy.com/view/7fS3Wh
- **License**: CC BY-NC-SA 3.0

## References

- [Fractalic Explorer 2](https://www.shadertoy.com/view/7fS3Wh) - Source shader with iterated fold-scale-translate fractal raymarch (user-pasted code)

## Reference Code

```glsl
#define H(h)(cos((h)*6.3+vec3(0,23,21))*.5+.5)
#define R(p,a,r)mix(a*dot(p,a),p,cos(r))+sin(r)*cross(p,a)
#define S(p) p /= dot(p, p); p = fract(p) - 0.5;
#define G(p, s) p = floor(p * s) / s;
#define F(p, a) p = abs(p * r(a)); p.y -= 0.5;
#define r(a) mat2(cos(a + asin(vec4(0,1,-1,0))))
#define L(p, s) p = mod(p + s/2., s) - s/2.;
#define Q(p) p *= r(round(atan(p.x, p.y) * 4.) / 4.)
#define M(p) p = abs(p) - 0.5; p *= r(0.785); p = abs(p) - 0.2;
#define T(p) p *= r(floor(atan(p.x, p.y) * 6.) / 6.); p.x -= clamp(p.x, 0.1, 0.5);
#define K(p) p = abs(mod(p, 2.) - 1.); if(p.x < p.y) p = p.yx;
#define D(p) p.xy = vec2(log(length(p.xy)), atan(p.y, p.x));
#define Q2(p) p = abs(p-1.)-abs(p+1.)+p;

void mainImage(out vec4 O, vec2 C)
{
    vec4 data = texture(iChannel0, vec2(0.5, 0.5)/iResolution.xy);
    vec4 mData = texture(iChannel0, vec2(1.5, 0.5)/iResolution.xy);

    vec3 pos = data.xyz;
    float yaw = data.w;
    float pitch = mData.w;

    O = vec4(0);
    vec3 r = iResolution;
    vec3 d = normalize(vec3((C*2.-r.xy)/r.y, 0.30));

    float sp = sin(pitch), cp = cos(pitch);
    d.yz *= mat2(cp, -sp, sp, cp);
    float sy = sin(yaw), cy = cos(yaw);
    d.xz *= mat2(cy, -sy, sy, cy);

    for(float i=0., s, e, g=0.; i++ < 100.; )
    {
        vec3 p = g * d + pos;

        p = mod(p - 30., 60.) - 30.0;
        G(p.xz,1.0);
        Q(p.xz);
        G(p.xy,1.);
        F(p.xz,2.);
        G(p.yz,1.0);
        M(p.xz);
        M(p.zy);
        Q2(p.xy);

        s = 3.;

        for(int j=0; j<8; j++){
            Q2(p.zy);

            p = .3 - abs(p);
            p.x < p.z ? p = p.zyx : p;
            p.z < p.y ? p = p.xzy : p;

            s *= e = 1.4 + sin(iTime * .1) * .1;
            p = abs(p) * e - vec3(15, 120, 40);
        }
        g += e = length(p.yzzz) / s;
        O.xyz += mix(vec3(0.2,.3,2.), H(g * .1), .8) * 4. / e / 6e3;
    }
    O.a = 1.0;
}
```

## Algorithm

### Keep / Replace Table

| Reference | Ours | Notes |
|-----------|------|-------|
| `iResolution` | `resolution` uniform | |
| `iTime` | `morphPhase` uniform | CPU-accumulated phase for fold scale animation |
| `pos` (from iChannel0 texture) | `vec3(0.0, 0.0, flyPhase)` | Forward fly-through on Z axis |
| `yaw`, `pitch` (from iChannel0 texture) | `yaw`, `pitch` uniforms | CPU-accumulated from Lissajous |
| `100` march iterations | `marchSteps` uniform (int) | Configurable iteration count |
| `0.30` (FOV denominator in ray dir) | `fov` uniform | Controls field of view |
| `mod(p - 30., 60.) - 30.0` | `mod(p - domainSize, domainSize * 2.0) - domainSize` | `domainSize` uniform replaces hardcoded 30/60 |
| `8` fold iterations | `foldIterations` uniform (int) | Configurable fold depth |
| `1.4 + sin(iTime * .1) * .1` | `foldScale + morphAmount * sin(morphPhase)` | Split into base scale + animated morph amount |
| `vec3(15, 120, 40)` | `foldOffset` uniform (vec3) | Configurable fold translate |
| `s = 3.` | `initialScale` uniform | Starting scale accumulator |
| `H(g * .1)` cosine palette | `texture(gradientLUT, ...)` | Gradient LUT sampling |
| `mix(vec3(0.2,.3,2.), H(...), .8) * 4. / e / 6e3` | `surfColor / e * brightness` | Gradient color * FFT brightness * tonemapGain |
| `O.xyz += ...` raw accumulation | `tanh(color * tonemapGain)` | Tonemap like Voxel March |

### Pre-Fold Space Operations

Keep ALL pre-fold macros verbatim, expanded inline in the shader. These define the character of the effect:

```
G(p.xz, 1.0)  ->  p.xz = floor(p.xz * 1.0) / 1.0;
Q(p.xz)       ->  p.xz *= r(round(atan(p.x, p.z) * 4.0) / 4.0);
G(p.xy, 1.0)  ->  p.xy = floor(p.xy * 1.0) / 1.0;
F(p.xz, 2.0)  ->  p.xz = abs(p.xz * r(2.0)); p.z -= 0.5;
G(p.yz, 1.0)  ->  p.yz = floor(p.yz * 1.0) / 1.0;
M(p.xz)       ->  p.xz = abs(p.xz) - 0.5; p.xz *= r(0.785); p.xz = abs(p.xz) - 0.2;
M(p.zy)       ->  p.zy = abs(p.zy) - 0.5; p.zy *= r(0.785); p.zy = abs(p.zy) - 0.2;
Q2(p.xy)      ->  p.xy = abs(p.xy - 1.0) - abs(p.xy + 1.0) + p.xy;
```

Where `r(a) = mat2(cos(a), -sin(a), sin(a), cos(a))` (rotation matrix from the reference).

### Fractal Fold Loop

Keep verbatim from reference, parameterized:

```glsl
for (int j = 0; j < foldIterations; j++) {
    p.zy = abs(p.zy - 1.0) - abs(p.zy + 1.0) + p.zy;  // Q2(p.zy)
    p = 0.3 - abs(p);
    if (p.x < p.z) p = p.zyx;
    if (p.z < p.y) p = p.xzy;
    float e = foldScale + morphAmount * sin(morphPhase);
    s *= e;
    p = abs(p) * e - foldOffset;
}
```

### Distance Estimate and Color

```glsl
float e = length(p.yzzz) / s;   // keep verbatim
g += e;                           // accumulate march distance
float gradientT = fract(g * colorSpread);
vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;
```

### FFT Brightness

Same pattern as Voxel March. Depth-mapped FFT bands with `colorFreqMap` toggle:
- `colorFreqMap == 0`: map march step index to frequency band
- `colorFreqMap == 1`: map gradient position to frequency band

### Camera

CPU-accumulated `flyPhase` for forward Z motion. DualLissajous drives `yaw` and `pitch` uniforms that rotate the ray direction, giving a looking-around-while-flying-forward feel.

Ray construction (from reference, parameterized):
```glsl
vec3 rd = normalize(vec3(uv, fov));
rd.yz *= mat2(cp, -sp, sp, cp);  // pitch
rd.xz *= mat2(cy, -sy, sy, cy);  // yaw
vec3 ro = vec3(0.0, 0.0, flyPhase);
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 30-100 | 60 | Raymarch iteration count |
| stepSize | float | 0.1-1.0 | 0.5 | March step multiplier (not in ref, but useful for perf tuning) |
| fov | float | 0.2-1.0 | 0.3 | Ray direction Z component (lower = narrower FOV) |
| domainSize | float | 10.0-60.0 | 30.0 | Domain repetition half-period |
| foldIterations | int | 2-12 | 8 | Number of fold-scale-translate iterations |
| foldScale | float | 1.0-2.0 | 1.4 | Base scale factor per fold iteration |
| morphAmount | float | 0.0-0.5 | 0.1 | Amplitude of fold scale oscillation |
| morphSpeed | float | 0.0-2.0 | 0.1 | Rate of fold scale oscillation (CPU-accumulated) |
| foldOffset | vec3 | 1-200 per axis | (15, 120, 40) | Fold translate vector |
| initialScale | float | 1.0-10.0 | 3.0 | Starting scale accumulator |
| colorSpread | float | 0.01-1.0 | 0.1 | How much march distance spreads across gradient |
| tonemapGain | float | 0.0001-0.01 | 0.001 | Tonemap exposure |
| flySpeed | float | 0.0-5.0 | 1.0 | Forward camera speed (CPU-accumulated) |
| colorFreqMap | bool | - | false | Toggle FFT mapping: depth bands vs gradient color |
| baseFreq | float | 27.5-440 | 55 | FFT low bound Hz |
| maxFreq | float | 1000-16000 | 14000 | FFT high bound Hz |
| gain | float | 0.1-10 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when silent |

## Modulation Candidates

- **foldScale**: shifts the fractal structure between open/closed
- **morphAmount**: controls how much the structure breathes
- **fov**: zoom in/out feel
- **domainSize**: spacing between repeated domains
- **foldOffset** (per-axis): dramatically reshapes the fractal geometry
- **colorSpread**: shifts color patterning across depth
- **tonemapGain**: overall brightness/exposure

### Interaction Patterns

- **foldScale vs foldOffset**: foldScale controls how much each iteration magnifies, foldOffset controls where the fold centers. Small foldScale + large offset = sparse floating chunks. Large foldScale + small offset = dense packed corridors. Modulating both creates structural breathing where the space alternates between open and claustrophobic.
- **foldIterations vs foldScale**: more iterations with high scale = very fine detail at the cost of performance. Fewer iterations with high scale = bold chunky shapes. The detail level is multiplicative (scale^iterations), so small changes to either have dramatic geometric effect.

## Notes

- March step count directly impacts performance. 100 iterations from the reference is expensive; default to 60 with configurable range.
- The pre-fold macro chain (G, Q, F, M, Q2) is what gives this shader its specific character. These should be kept verbatim, not parameterized - they ARE the effect's identity.
- `foldOffset` vec3(15, 120, 40) has wildly different per-axis values. The Y=120 is load-bearing for the visual. Exposing per-axis control lets users explore but the default must match the reference.
- The `0.3` in `p = 0.3 - abs(p)` is the Mandelbox fold radius. Could be parameterized but keeping it fixed preserves the reference look.
- DualLissajous for yaw/pitch needs bounded output range to avoid disorienting full rotations. Clamp pitch to +/- 45 degrees, let yaw wrap freely.
