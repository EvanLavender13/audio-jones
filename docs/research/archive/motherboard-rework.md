# Motherboard Rework

Flat neon circuit-board traces with branching pathways, where each fold-depth layer glows to a different frequency band. The camera pans continuously over an infinite tiled plane while animated "data packets" stream along the traces. Loud passages flood the board with light; quiet passages reveal subtle flowing current.

## Classification

- **Category**: GENERATORS > Texture (replaces existing Motherboard generator)
- **Pipeline Position**: Generator stage (between trail boost and transforms)

## References

- User-provided Shadertoy shaders (MIT License) — Kali fractal `p.x*p.y` hyperbolic inversion variants
- Technique origin: "Kaliset" fractal family — `abs(z)/f(z) - c` iteration

## Reference Code

### Shader A — Circuit traces with exp() glow rendering

```glsl
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

mat2 rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c,s,-s,c);
}

vec3 fractal(vec2 p)
{
       p=vec2(p.x/p.y,1./p.y);
    p.y+=iTime*sign(p.y);
    p.x+=sin(iTime*.1)*sign(p.y)*4.;
    p.y=fract(p.y*.05);
    float ot1=1000., ot2=ot1, it=0.;
    for (float i=0.; i<10.; i++) {
        p=abs(p);
        p=p/clamp(p.x*p.y,0.15,5.)-vec2(1.5,1.);
        float m=abs(p.x);
        if (m<ot1) {
            ot1=m+step(fract(iTime*.2+float(i)*.05),.5*abs(p.y));
            it=i;
        }
        ot2=min(ot2,length(p));
    }

    ot1=exp(-30.*ot1);
    ot2=exp(-30.*ot2);
    return hsv2rgb(vec3(it*.1+.5,.7,1.))*ot1+ot2;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy-.5;
    uv.x*=iResolution.x/iResolution.y;
    float aa=6.;
    uv*=rot(sin(iTime*.1)*.3);
    vec2 sc=1./iResolution.xy/(aa*2.);
    vec3 c=vec3(0.);
    for (float i=-aa; i<aa; i++) {
        for (float j=-aa; j<aa; j++) {
            vec2 p=uv+vec2(i,j)*sc;
            c+=fractal(p);
        }
    }
    fragColor = vec4(c/(aa*aa*4.)*(1.-exp(-20.*uv.y*uv.y)),1.);
}
```

### Shader B — Streaming data flow along traces

```glsl
mat2 rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c,s,-s,c);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 fractal(vec2 p) {
    float o=100.,l=o;
    p*=rot(-.3);
    p.x*=1.+p.y*.7;
    p*=.5+sin(iTime*.1)*.3;
    p+=iTime*.02;
    p=fract(p);
    for (int i=0; i<10; i++) {
        p*=rot(radians(90.));
        p.y=abs(p.y-.25);
        p=p/clamp(abs(p.x*p.y),0.,3.)-1.;
        o=min(o,abs(p.y-1.5)-.2)+fract(p.x+iTime*.3+float(i)*.2)*.5;
        l=min(l,length(max(vec2(0.),abs(p)-.5)));

    }
    o=exp(-5.*o);
    l=smoothstep(.1,.11,l);
    return hsv2rgb(vec3(o*.5+.6,.8,o+.1))+l*vec3(.4,.3,.2);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    uv-=.5;
    uv.x*=iResolution.x/iResolution.y;
    vec3 col=vec3(0.);
    float aa=4.;
    vec2 eps=1./iResolution.xy/aa;
    for (float i=-aa; i<aa; i++){
        for (float j=-aa; j<aa; j++){
            col+=fractal(uv+vec2(i,j)*eps);
        }
    }
    col/=pow(aa*2.,2.);
    fragColor = vec4(col,1.0);
}
```

## Algorithm

### Core Change

Replace Motherboard's current box-fold iteration (`abs(p) - offset`) with the Kali hyperbolic inversion:

```
p = abs(p) / clamp(abs(p.x * p.y), clampLo, clampHi) - foldConstant;
```

The `p.x * p.y` denominator is what creates the rectilinear circuit-trace character with T-junctions and varying line widths. The clamp bounds control fractal density and complexity.

### Coordinate Setup (flat overhead, no perspective)

1. Normalize to centered, aspect-corrected coordinates (same as current Motherboard)
2. Apply rotation matrix (accumulated `rotationSpeed * deltaTime`)
3. Add accumulated pan offset (`panAccum`)
4. Scale by `zoom`
5. `fract(p)` for infinite tiling

### Orbit Traps

Two trap distances tracked per iteration:
- **Primary (traces):** `abs(p.x)` — minimum across iterations. Determines `minit` (which iteration layer claimed the pixel).
- **Secondary (junctions):** `length(p)` — minimum scaled distance for junction glow points.

### Data Streaming

Add animated `fract()` term to primary orbit trap distance:
```
ot1 += fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity;
```
Each iteration layer streams at a slightly different phase offset (`i * 0.2`), creating layered flow visible when FFT brightness is low.

### Rendering

Replace current `smoothstep * 1/dist` with sharper `exp(-k * dist)`:
```
trace = exp(-30.0 * ot1);
junction = exp(-30.0 * ot2);
```

### FFT Mapping (keep current convention)

- `minit` identifies which iteration claimed each pixel
- Map iteration to log-spaced frequency band: `baseFreq * pow(maxFreq/baseFreq, minit/iterations)`
- Sample 4 FFT bins across band, average, apply gain/curve
- `brightness = baseBright + energy`
- Sample gradient LUT at `minit / iterations` for layer color

### What to Keep from Reference Code

- The `abs(p) / clamp(p.x*p.y, lo, hi) - c` iteration verbatim
- `exp(-k * dist)` rendering for sharp neon traces
- `fract()` streaming modulation on orbit trap (Shader B's technique)
- Dual orbit traps: `abs(p.x)` for traces, `length(p)` for junctions

### What to Replace

- `hsv2rgb()` coloring → gradient LUT sampling (`texture(gradientLUT, vec2(minit/iterations, 0.5)).rgb`)
- Perspective projection (`p.x/p.y, 1./p.y`) → flat centered coordinates + `fract()`
- Hardcoded constants → config struct uniforms
- Supersampled AA loop → drop entirely (real-time performance)
- `iChannel0` audio → `fftTexture` with per-iteration band sampling

## Parameters

### Geometry

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| iterations | int | 4-16 | 12 | Fold depth; each iteration = one frequency band |
| zoom | float | 0.5-4.0 | 2.0 | Scale factor before tiling |
| clampLo | float | 0.01-1.0 | 0.15 | Inversion lower bound — lower = denser branching |
| clampHi | float | 0.5-5.0 | 2.0 | Inversion upper bound — higher = gentler inversion |
| foldConstant | float | 0.5-2.0 | 1.0 | Post-inversion translation magnitude |
| rotAngle | float | ±π | 0.0 | Per-iteration fold rotation |

### Animation

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| panSpeed | float | -2.0..2.0 | 0.3 | Drift speed through fractal space (CPU-accumulated) |
| flowSpeed | float | 0-2.0 | 0.3 | Data streaming animation speed (CPU-accumulated) |
| flowIntensity | float | 0-1.0 | 0.3 | Streaming effect visibility (0 = off) |
| rotationSpeed | float | ±π | 0.0 | Whole-pattern rotation (CPU-accumulated) |

### Rendering

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| glowIntensity | float | 0.001-0.1 | 0.01 | Primary trace glow multiplier |
| accentIntensity | float | 0-0.1 | 0.02 | Junction/node glow strength |

### Audio (standard)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440 | 55.0 | Lowest frequency band |
| maxFreq | float | 1000-16000 | 14000 | Highest frequency band |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 0.7 | Contrast exponent |
| baseBright | float | 0-1.0 | 0.15 | Minimum brightness when silent |

### Blend (standard)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| blendMode | enum | — | SCREEN | Compositing mode |
| blendIntensity | float | 0-1 | 1.0 | Opacity |

## Modulation Candidates

- **clampLo / clampHi**: Morph fractal density — tight clamp = sparse bold traces, wide = dense fine branching
- **foldConstant**: Shifts trace layout, creating morphing pattern transitions
- **zoom**: Pulsing scale oscillation through the tiling
- **panSpeed**: Audio-reactive drift — louder = faster travel
- **flowIntensity**: Controls streaming visibility
- **rotAngle**: Slow rotation creates kaleidoscopic morphing of the trace pattern
- **glowIntensity**: Overall brightness envelope

### Interaction Patterns

**Cascading threshold (flowIntensity × gain):** When gain is high (loud music), FFT brightness dominates and the streaming animation becomes invisible under the glow. When gain is low (quiet passage), the streaming becomes the primary visual. This creates natural verse/chorus dynamics — quiet sections show gentle data flow, loud sections blast the circuit into full brightness.

**Competing forces (clampLo × clampHi):** Low clampLo pushes toward dense fine branching while high clampHi pushes toward smooth simple curves. Modulating them in opposition creates tension between complexity and simplicity — the trace pattern breathes between intricate and bold.

## Notes

- **No supersampling**: Reference shaders use 64-144 samples per pixel for AA. Drop entirely for real-time. The `exp()` rendering produces softer falloff than raw orbit traps, which mitigates aliasing. Existing pipeline MSAA handles the rest.
- **CPU accumulation**: `panSpeed`, `flowSpeed`, and `rotationSpeed` all accumulate on CPU. Shader receives accumulated values, never raw speeds.
- **Breaking change**: The config struct fields change completely. Existing presets with Motherboard sections will load defaults for the new parameters. The visual output is entirely different, so old values wouldn't be meaningful anyway.
- **Iteration count vs performance**: Each iteration adds fold computation. 16 iterations is the max — beyond that, orbit traps converge and additional layers add visual noise without structural detail.
