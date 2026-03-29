# Isoflow

Flying through a turbulence-warped isosurface tunnel - smooth organic walls flowing
and rippling around you as depth-mapped FFT lights up the structure from front to back.
The gyroid surface (expandable to other shapes) is domain-warped with layered sinusoidal
turbulence that transforms rigid mathematical geometry into fluid, breathing corridors.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (blended onto scene)
- **Compute Model**: Single fragment shader

## Attribution

- **Based on**: "Kuantum Dalgalanmalari" by msttezcan
- **Source**: https://www.shadertoy.com/view/7fj3DK
- **License**: CC BY-NC-SA 3.0

## References

- [Shadertoy: Kuantum Dalgalanmalari](https://www.shadertoy.com/view/7fj3DK) - Primary reference shader
- Voxel March (`shaders/voxel_march.fs`) - Existing gyroid generator with depth-mapped FFT pattern

## Reference Code

```glsl
void mainImage(out vec4 o, vec2 u) {
    float s, // dist to gyroid
          i, // raymarch iterator
          d, // total dist
          T = iTime * 0.5;

    // raymarch pos, resolution temporarily
    vec3  p = iResolution;

    //  scale coords
    u = ( u - p.xy/2. ) / p.y
       // add some cam movement
       + vec2(sin(T*.2)*.3,
              sin(T*.5)*.1);


    // clear o, 128 iterations, seems to perform well enough and look good at 128
    for(o*=i; i++ < 128.;
        // accumulate color by p.y
        o +=(1.+cos(.03*p.y+vec4(3,1,0,0)))/s + sin(T)
    ) {
        // p = ro + rd * d, p.z += T * 1e2
        p = vec3( u*d, d+T*1e2 ),

        // iso lines effect
        p.z = p.z+cos(p.z*.7) + sin(p.z*.7) + tanh(T) ,

        // get out of the way of the gyroid and move along the x axis a bit
        p.x += 1e2 - cos(T)*1e1 + sin(T)*1e1,

        // squiggle wiggle the gyroid with @Xor style turbulence
        // this is responsible for the flowy nature of the gyroid
        // it's pretty boring without it
        p += cos(p.yzx/16.)*16. + sin(p.yzx/32.)*8.,

        // sample a large gyroid, these things are really fun
        // dot(sin(p), cos(p.yzx))
        // play with the scale (132 and 64) and adjust the multiplier (32)
        // Use your eyes to see if it's too fuzzy or if it's breaking up
        d += s =  .005+.8*abs(32.*dot(sin(p/132.), cos(p.yzx/94.) + sin(p.yzx/43.)));
    }

    // divide brightness and tanh tone map
    o = tanh(o/3e3) ;
}
```

## Algorithm

### Keep verbatim
- Gyroid SDF core: `dot(sin(p/A), cos(p.yzx/B) + sin(p.yzx/C))` with configurable A, B, C
- Turbulence warp: `p += cos(p.yzx/turbScale1)*turbAmp1 + sin(p.yzx/turbScale2)*turbAmp2`
- Iso-line warp on z: `p.z = p.z + cos(p.z*isoFreq) + sin(p.z*isoFreq)` with configurable strength
- Distance accumulation: `d += s = epsilon + weight * abs(thickness * sdf)`
- tanh tonemapping: `tanh(color * tonemapGain)`
- Ray setup: `rd = vec3(uv, 1.0)` (unnormalized, matching reference)

### Replace
| Reference | Ours |
|-----------|------|
| `iResolution`, `iTime` | `resolution`, `time` uniforms |
| `u = (u - p.xy/2.) / p.y` | `uv = (fragTexCoord * resolution - 0.5 * resolution) / resolution.y` |
| Camera sway `vec2(sin(T*.2)*.3, sin(T*.5)*.1)` | `DualLissajousConfig` pan uniforms: `uv += vec2(panX, panY)` |
| `p.x += 1e2 - cos(T)*1e1 + sin(T)*1e1` | Same lissajous pan applied to p.x offset: `p.x += panX * panScale` (or fold into lissajous amplitude) |
| `T*1e2` fly speed | `flyPhase` CPU-accumulated uniform |
| `cos(.03*p.y+vec4(3,1,0,0))/s` color accumulation | `texture(gradientLUT, vec2(t, 0.5)).rgb / s * brightness` where `t = d / maxDist` |
| `sin(T)` additive brightness | Drop - FFT brightness replaces this |
| `o/3e3` divisor | `tonemapGain` uniform |
| Hardcoded 128 iterations | `marchSteps` uniform (int) |
| Hardcoded scales (132, 94, 43) | Derive from single `gyroidScale` uniform: A=gyroidScale, B=gyroidScale*0.71, C=gyroidScale*0.33 (preserves reference ratios 132:94:43) |
| Hardcoded turbulence (16, 32) | `turbulenceScale`, `turbulenceAmount` uniforms |
| Hardcoded thickness (32) | `surfaceThickness` uniform |

### FFT integration (depth-mapped)
Same `t = d / maxDist` used for gradient LUT also drives FFT band lookup.
Use the standard `sampleFFTBand(t0, t1)` pattern from Voxel March where each
march step maps to a narrow frequency band:

```glsl
float t = clamp(d / maxDist, 0.0, 1.0);
float bw = 1.0 / float(marchSteps);
float brightness = sampleFFTBand(t, t + bw);
vec3 col = texture(gradientLUT, vec2(t, 0.5)).rgb;
color += col / s * brightness;
```

### Lissajous camera pan
CPU side: `DualLissajousUpdate()` produces `panX`, `panY` each frame.
Shader receives `uniform vec2 pan`. Applied in two places:
1. UV offset: `uv += pan` (camera view shift)
2. Position offset: `p.x += pan.x * panScale` (move through gyroid laterally)

`panScale` controls how much the lissajous motion translates into lateral
movement through the gyroid vs. just camera angle shift. Could be a single
`camDrift` float or split. Reference uses large offsets (1e2 base + 1e1
oscillation) to avoid the gyroid origin - a static `p.x += baseOffset` uniform
handles this without the time-varying part (lissajous replaces that).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gyroidScale | float | 20.0-200.0 | 132.0 | Primary gyroid cell size - larger = wider tunnels |
| surfaceThickness | float | 1.0-64.0 | 32.0 | Surface wall thickness multiplier |
| marchSteps | int | 32-128 | 96 | Raymarch iterations (quality vs. perf) |
| turbulenceScale | float | 4.0-64.0 | 16.0 | Turbulence warp wavelength - smaller = tighter wrinkles |
| turbulenceAmount | float | 0.0-32.0 | 16.0 | Turbulence warp amplitude - 0 = rigid gyroid |
| isoFreq | float | 0.1-2.0 | 0.7 | Iso-line ring frequency along z |
| isoStrength | float | 0.0-1.0 | 1.0 | Iso-line effect blend (0 = off) |
| flySpeed | float | 0.1-5.0 | 1.0 | Z-axis fly-through speed (CPU-accumulated) |
| camDrift | float | 0.0-1.0 | 0.3 | UV-space camera drift from lissajous |
| baseOffset | float | 50.0-200.0 | 100.0 | Lateral offset to avoid gyroid origin |
| tonemapGain | float | 0.0001-0.002 | 0.00033 | Brightness before tanh (reference: 1/3000) |
| lissajous | DualLissajousConfig | (see shared config) | (defaults) | Camera pan motion |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency bound |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound |
| gain | float | 0.1-10.0 | 2.0 | FFT gain |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.15 | FFT minimum brightness |

## Modulation Candidates

- **gyroidScale**: morph tunnel size - bass pulses blow tunnels wide open, treble tightens the mesh
- **surfaceThickness**: thin/thick surface walls - makes the structure skeletal vs. solid
- **turbulenceAmount**: rigidity to chaos - modulating this makes the gyroid breathe and flow
- **turbulenceScale**: wrinkle frequency - shifts between broad sweeping curves and tight organic ripples
- **isoFreq**: depth ring spacing - creates pulsing depth bands
- **isoStrength**: ring visibility - fading in/out reveals or hides the layered structure
- **flySpeed**: acceleration/deceleration through the tunnel
- **tonemapGain**: overall brightness/contrast pumping
- **camDrift**: camera wander intensity
- **lissajous.amplitude**: lateral motion range

### Interaction Patterns

**Cascading threshold: turbulenceAmount gates isoStrength visibility.**
When turbulence is low, the gyroid is a rigid lattice and iso-lines create
clean geometric rings. As turbulence rises, the rings distort and eventually
dissolve into flowing organic texture. Modulating turbulence with bass while
iso-lines respond to mids creates verse/chorus contrast - quiet sections show
crisp mathematical rings, loud sections melt into flowing chaos.

**Competing forces: gyroidScale vs. surfaceThickness.**
Large scale with thin surface = vast open tunnels with delicate lacy walls.
Small scale with thick surface = dense packed corridors. Modulating them in
opposition (one rises as the other falls) shifts between claustrophobic and
expansive without either dominating.

## Notes

- `marchSteps` at 128 is expensive for a generator. Default to 96, let users push it. 64 still looks decent.
- The reference uses unnormalized ray direction `vec3(uv, 1.0)` - this is intentional, it produces a wider FOV than `normalize(vec3(uv, 1.0))` and matches the reference look.
- `tanh` tonemapping needs `#version 330` (already standard in this codebase).
- The gyroid scale ratios (132:94:43 ~= 1.0:0.71:0.33) produce multi-frequency interference. Deriving B and C from a single `gyroidScale` preserves these ratios while giving one knob.
- `baseOffset` prevents the camera from sitting at the gyroid origin where the SDF = 0 everywhere. The reference uses 100.0.
- Future: `surfaceShape` enum could add sphere shells (`length(p) - r`), Schwarz P surface (`sin(x)+sin(y)+sin(z)`), etc.
