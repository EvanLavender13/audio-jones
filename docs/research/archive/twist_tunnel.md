# Twist Tunnel

Nested wireframe platonic solids spiral inward with progressive scale and rotation, creating a recursive tunnel of glowing edges. Each layer is smaller and more twisted than the last, while a Lissajous-driven camera orbits the structure. Outer layers react to bass, inner layers to treble, and color flows from a gradient LUT sampled by depth — so the tunnel breathes with the music from the outside in.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Output stage — trail boost → generators → transforms
- **Section Index**: 10 (Geometric)

## Attribution

- **Based on**: "Twisty cubes" by ChunderFPV
- **Source**: https://www.shadertoy.com/view/lclXzH
- **License**: CC BY-NC-SA 3.0
- **Also referenced**: wireframe code from FabriceNeyret2 — https://www.shadertoy.com/view/XfS3DK

## References

- [Spin Cage implementation](../../src/effects/spin_cage.cpp) — Platonic solid vertex/edge tables, glow pattern, FFT band-averaging, gradient LUT sampling (all reusable)
- [DualLissajousConfig](../../src/config/dual_lissajous_config.h) — Embedded quasi-periodic camera orbit

## Reference Code

```glsl
// "Twisty cubes" by ChunderFPV
// https://www.shadertoy.com/view/lclXzH
// License: CC BY-NC-SA 3.0
// wireframe code modified from FabriceNeyret2: https://www.shadertoy.com/view/XfS3DK

#define H(a) (cos(radians(vec3(-30, 60, 150))+(a)*6.2832)*.5+.5)  // hue
#define A(v) mat2(cos((v*3.1416) + vec4(0, -1.5708, 1.5708, 0)))  // rotate
#define s(a, b) c = max(c, .01/abs(L( u, K(a, v, h), K(b, v, h) )+.02)*k*10.*o); // segment
//#define s(a, b) c += .02/abs(L( u, K(a, v, h), K(b, v, h) )+.02)*k*o*(1.-i);  // alt segment

// line
float L(vec2 p, vec3 A, vec3 B)
{
    vec2 a = A.xy,
         b = B.xy - a;
         p -= a;
    float h = clamp(dot(p, b) / dot(b, b), 0., 1.);
    return length(p - b*h) + .01*mix(A.z, B.z, h);
}

// cam
vec3 K(vec3 p, mat2 v, mat2 h)
{
    p.zy *= v; // pitch
    p.zx *= h; // yaw
    if (texelFetch(iChannel0, ivec2(80, 2), 0).x < 1.) // P key
        p *= 4. / (p.z+4.); // perspective view
    return p;
}

void mainImage( out vec4 C, in vec2 U )
{
    vec2 R = iResolution.xy,
         u = (U+U-R)/R.y*1.2,
         m = (iMouse.xy*2.-R)/R.y;

    float t = iTime/60.,
          l = 15.,  // loop size
          j = 1./l, // increment size
          r = .8,   // scale size
          o = .1,   // brightness
          i = 0.;   // starting increment

    if (iMouse.z < 1.) // not clicking
        m = sin(t*6.2832 + vec2(0, 1.5708)), // move in circles
        m.x *= 2.; // stretch mouse x (circle to ellipse)

    mat2 v = A(m.y), // pitch
         h; // yaw (set in loop)

    vec3 p = vec3(0, 1, -1),    // cube coords
         c = .2*length(u)*H(t), // background
         k; // cube color (set in loop)

    // cubes
    for (; i<1.; i+=j)
    {
        k = H(i+iTime/3.)+.2; // cube color
        h = A(m.x+i); // rotate
        p *= r; // scale
        s( p.yyz, p.yzz )
        s( p.zyz, p.zzz )
        s( p.zyy, p.zzy )
        s( p.yyy, p.yzy )
        s( p.yyy, p.zyy )
        s( p.yzy, p.zzy )
        s( p.yyz, p.zyz )
        s( p.yzz, p.zzz )
        s( p.zzz, p.zzy )
        s( p.zyz, p.zyy )
        s( p.yzz, p.yzy )
        s( p.yyz, p.yyy )
    }

    // squaring c for contrast, tanh limits brightness to 1 (less blowout)
    C = vec4(tanh(c*c*3.), 1);
}
```

## Algorithm

### CPU-side (Setup function)

1. **Lissajous camera**: Call `DualLissajousUpdate()` → `outX` becomes `cameraPitch`, `outY` becomes `cameraYaw` (values are radians, amplitude ~0.3 gives ±17° orbit)
2. **Twist accumulation**: `twistPhase += twistSpeed * deltaTime`
3. **Upload vertex/edge data**: Raw vertex positions as `vec4 vertices[20]` (xyz, w=0) and edge connectivity as `vec2 edgeIdx[30]` (x=vertA, y=vertB as float, cast to int in shader). Upload `vertexCount` and `edgeCount`. Data comes from Spin Cage's existing `ShapeDescriptor` tables.
4. **Upload layer params**: `layerCount`, `scaleRatio`, `twistAngle`, `twistPhase`, `perspective`, `scale`, `cameraPitch`, `cameraYaw`
5. **Upload glow/audio/color**: `lineWidth`, `glowIntensity`, `contrast`, FFT params, gradient LUT

### GPU-side (Fragment shader)

Adapts the reference's nested loop structure with generic vertex/edge arrays instead of cube-specific swizzles.

```
1. Center UV:  uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y

2. Build camera matrices from uniforms:
     pitchMat = mat2(cos(cameraPitch*PI + vec4(0, -HALF_PI, HALF_PI, 0)))
     // yaw built per-layer (includes twist)

3. For each layer i (0..layerCount-1):
     t = float(i) / float(layerCount - 1)
     layerScale = scale * pow(scaleRatio, float(i))
     yawAngle = cameraYaw + twistAngle * float(i) + twistPhase
     yawMat = mat2(cos(yawAngle*PI + vec4(0, -HALF_PI, HALF_PI, 0)))

     // Per-layer color + FFT (shared by all edges in this layer)
     color = texture(gradientLUT, vec2(t, 0.5)).rgb
     fftMag = bandAveragedFFT(t)   // same log-space spread as Spin Cage
     brightness = baseBright + pow(clamp(fftMag * gain, 0, 1), curve)

     For each edge j (0..edgeCount-1):
       // Get vertex positions, scale
       vertA = vertices[int(edgeIdx[j].x)].xyz * layerScale
       vertB = vertices[int(edgeIdx[j].y)].xyz * layerScale
       // Camera transform (reference K() function)
       vertA.zy *= pitchMat;  vertA.zx *= yawMat
       vertB.zy *= pitchMat;  vertB.zx *= yawMat
       // Perspective project
       projA = vec3(vertA.xy / (vertA.z + perspective), vertA.z)
       projB = vec3(vertB.xy / (vertB.z + perspective), vertB.z)
       // Line distance with depth dimming (reference L() function)
       dist = lineDist(uv, projA, projB)
       // Glow
       glow = pixelWidth / (pixelWidth + abs(dist))
       // Max blend (reference default)
       c = max(c, color * glow * glowIntensity * brightness)

4. Output:  vec4(tanh(c * c * contrast), 1.0)
```

### What to keep from reference

- `L()` line distance with z-depth dimming (`+ .01*mix(A.z, B.z, h)`) — provides subtle front/back depth cues within each layer
- `K()` camera model: pitch on zy plane, yaw on zx plane, perspective `xy / (z + d)` — same structure
- `A()` rotation matrix construction: `mat2(cos(v*PI + vec4(0, -HALF_PI, HALF_PI, 0)))` — exact formula
- Per-layer progressive scale: `p *= r` → `scale * pow(scaleRatio, i)`
- Per-layer yaw twist: `h = A(m.x + i)` → `cameraYaw + twistAngle * i + twistPhase`
- Max blending: `c = max(c, ...)` — user confirmed, not additive
- `tanh(c*c*3.)` output — keep with exposed contrast param

### What to replace

| Reference | Replacement |
|-----------|-------------|
| `H(a)` hue function | `texture(gradientLUT, vec2(t, 0.5)).rgb` per layer |
| `vec3(0, 1, -1)` + swizzle vertices | `vertices[]` uniform array (arbitrary platonic solid) |
| 12 hardcoded `s()` macro calls | Loop over `edgeIdx[]` array |
| `iMouse` auto-orbit | `DualLissajousConfig` → `cameraPitch`/`cameraYaw` uniforms |
| `l = 15.` fixed loop count | `layerCount` uniform |
| `r = .8` fixed scale | `scaleRatio` uniform |
| `o = .1` fixed brightness | `baseBright + FFT magnitude` per layer |
| `k = H(i+iTime/3.)+.2` color | Gradient LUT sampled at layer `t` |
| `.2*length(u)*H(t)` background | None — generator composites via blend on black |
| `iTime/3.` hue animation | Handled by gradient LUT (user controls externally) |
| `texelFetch(iChannel0, ...)` P key toggle | Always perspective (controlled by `perspective` param) |

### Performance note

Per-pixel cost: `layerCount × edgeCount` line-distance evaluations + `layerCount` FFT band samples. FFT is per-layer (not per-edge) since all edges in a layer share the same `t`.

| Shape | Edges | 12 layers | 20 layers |
|-------|-------|-----------|-----------|
| Tetrahedron | 6 | 72 | 120 |
| Cube | 12 | 144 | 240 |
| Octahedron | 12 | 144 | 240 |
| Dodecahedron | 30 | 360 | 600 |
| Icosahedron | 30 | 360 | 600 |

Cube at 12 layers (144 evals) is comparable to Spin Cage's icosahedron (30 evals) — well within budget. Icosahedron at 20 layers (600 evals) is heavy but feasible for a generator.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| shape | int (enum) | 0-4 | 1 (Cube) | Platonic solid: Tetra, Cube, Octa, Dodeca, Icosa |
| layerCount | int | 2-20 | 12 | Number of nested wireframe layers |
| scaleRatio | float | 0.5-0.99 | 0.8 | Scale multiplier between successive layers |
| twistAngle | float | -PI..+PI | 0.5 | Static twist offset per layer (radians) |
| twistSpeed | float | -PI..+PI | 0.1 | Twist accumulation rate (rad/s) |
| perspective | float | 1.0-20.0 | 4.0 | Camera projection depth |
| scale | float | 0.1-5.0 | 1.2 | Overall wireframe size |
| lineWidth | float | 0.5-10.0 | 2.0 | Glow falloff width (pixel units) |
| glowIntensity | float | 0.1-5.0 | 1.0 | Glow brightness |
| contrast | float | 0.5-10.0 | 3.0 | tanh output squaring factor |
| lissajous | DualLissajousConfig | embedded | see below | Camera orbit motion |
| baseFreq | float | 27.5-440.0 | 55.0 | Low end of FFT frequency spread (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | High end of FFT frequency spread (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.15 | Base edge visibility without audio |
| gradient | ColorConfig | embedded | gradient mode | Gradient LUT for layer coloring |
| blendMode | EffectBlendMode | enum | Screen | Blend mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend strength |

**Lissajous overrides** (camera-appropriate defaults):
```
DualLissajousConfig lissajous = {
    .amplitude = 0.3f,    // ±17° orbit range
    .motionSpeed = 1.0f,
    .freqX1 = 0.07f,      // pitch frequency
    .freqY1 = 0.11f,      // yaw frequency (irrational ratio for non-repeating)
};
```

## Modulation Candidates

- **twistSpeed**: Accelerates/decelerates the spiral winding — bursts spin the tunnel, silence lets it settle
- **twistAngle**: Changes how tightly each layer is offset — modulating creates breathing spiral geometry
- **scaleRatio**: Controls how fast layers shrink — high values (0.95) keep layers similar, low (0.5) collapses to center
- **perspective**: Breathing zoom between flat orthographic and dramatic depth
- **scale**: Pulsing overall wireframe size
- **lineWidth**: Thickening/thinning glow changes edge overlap visibility
- **glowIntensity**: Brightness pulsing
- **lissajous.amplitude**: Camera orbit range — audio can push viewpoint wider or tighter
- **lissajous.motionSpeed**: Camera orbit speed — creates lurching/smooth camera shifts

### Interaction Patterns

**twistAngle × scaleRatio (cascading threshold)**: At high scale ratios (0.95+), inner layers are nearly the same size as outer — twist is the only visual differentiator between layers, so twist modulation dominates the visual. At low scale ratios (0.5), inner layers vanish fast and twist barely matters because those layers are tiny. The visual impact of twist modulation is gated by how many layers remain visible, which scaleRatio controls.

**layerCount × lineWidth (resonance)**: More layers = more overlapping edges at intersection nodes. Wider lines lower the threshold for edges to visually interact. When both spike together, intersection nodes bloom dramatically as dozens of edges overlap within glow range. With few layers or thin lines, each layer reads as an isolated wireframe.

**lissajous.amplitude × twistAngle (competing forces)**: Camera orbit reveals different faces of the 3D structure, while twist controls which direction the tunnel spirals. When both are active, the spiral direction appears to shift as the camera orbits — neither parameter dominates, creating a flowing, organic sense of depth that changes character throughout a song.

## Notes

- Vertex/edge tables are shared with Spin Cage — the `ShapeDescriptor` arrays and `SHAPES[]` table should be extracted to a shared header (e.g., `src/config/platonic_solids.h`) so both generators reference the same data
- The `A()` rotation macro uses `v * PI` scaling — in the reference, mouse range ~(-1,+1) maps to ~(-PI,+PI) rotation. Our Lissajous outputs raw radians, so the pitch/yaw uniforms need matching scale. Either multiply by `1/PI` before uploading, or remove the `*PI` from the shader's matrix construction and pass raw radians directly.
- Inner layers approach zero size (`0.8^20 ≈ 0.012`) — the shader could early-out when `layerScale < threshold` to skip invisible layers, but this is an optimization for later
- The reference's `u` coordinate is scaled by 1.2 (`(U+U-R)/R.y*1.2`) — this zooms the viewport slightly. Our version uses standard centered UV without the zoom factor; the `scale` parameter provides equivalent control
