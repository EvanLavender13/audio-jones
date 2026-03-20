# Protean Clouds

Volumetric raymarched clouds flying through an infinite tunnel of deformed periodic noise. Dense cores and wispy tendrils emerge from a cheap noise function built on sin/cos lattice deformation, lit by dual-sample diffuse gradients. Gradient LUT coloring replaces the original's hardcoded palette, with a density-vs-depth blend slider controlling how the LUT maps across the volume. FFT audio drives cloud brightness/emission - quiet passages produce moody dim formations, loud sections glow with luminous intensity.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator (section 13)
- **Flags**: `EFFECT_FLAG_BLEND`

## Attribution

- **Based on**: "Protean clouds" by nimitz (@stormoid)
- **Source**: https://www.shadertoy.com/view/3l23Rh
- **License**: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

## References

- [Protean clouds](https://www.shadertoy.com/view/3l23Rh) - Complete reference shader by nimitz. Deformed periodic grid volume noise with dynamic step size optimization.

## Reference Code

```glsl
// Protean clouds by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/3l23Rh
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// Contact the author for other licensing options

/*
    Technical details:

    The main volume noise is generated from a deformed periodic grid, which can produce
    a large range of noise-like patterns at very cheap evalutation cost. Allowing for multiple
    fetches of volume gradient computation for improved lighting.

    To further accelerate marching, since the volume is smooth, more than half the the density
    information isn't used to rendering or shading but only as an underlying volume    distance to
    determine dynamic step size, by carefully selecting an equation    (polynomial for speed) to
    step as a function of overall density (not necessarily rendered) the visual results can be
    the    same as a naive implementation with ~40% increase in rendering performance.

    Since the dynamic marching step size is even less uniform due to steps not being rendered at all
    the fog is evaluated as the difference of the fog integral at each rendered step.

*/

mat2 rot(in float a){float c = cos(a), s = sin(a);return mat2(c,s,-s,c);}
const mat3 m3 = mat3(0.33338, 0.56034, -0.71817, -0.87887, 0.32651, -0.15323, 0.15162, 0.69596, 0.61339)*1.93;
float mag2(vec2 p){return dot(p,p);}
float linstep(in float mn, in float mx, in float x){ return clamp((x - mn)/(mx - mn), 0., 1.); }
float prm1 = 0.;
vec2 bsMo = vec2(0);

vec2 disp(float t){ return vec2(sin(t*0.22)*1., cos(t*0.175)*1.)*2.; }

vec2 map(vec3 p)
{
    vec3 p2 = p;
    p2.xy -= disp(p.z).xy;
    p.xy *= rot(sin(p.z+iTime)*(0.1 + prm1*0.05) + iTime*0.09);
    float cl = mag2(p2.xy);
    float d = 0.;
    p *= .61;
    float z = 1.;
    float trk = 1.;
    float dspAmp = 0.1 + prm1*0.2;
    for(int i = 0; i < 5; i++)
    {
        p += sin(p.zxy*0.75*trk + iTime*trk*.8)*dspAmp;
        d -= abs(dot(cos(p), sin(p.yzx))*z);
        z *= 0.57;
        trk *= 1.4;
        p = p*m3;
    }
    d = abs(d + prm1*3.)+ prm1*.3 - 2.5 + bsMo.y;
    return vec2(d + cl*.2 + 0.25, cl);
}

vec4 render( in vec3 ro, in vec3 rd, float time )
{
    vec4 rez = vec4(0);
    const float ldst = 8.;
    vec3 lpos = vec3(disp(time + ldst)*0.5, time + ldst);
    float t = 1.5;
    float fogT = 0.;
    for(int i=0; i<130; i++)
    {
        if(rez.a > 0.99)break;

        vec3 pos = ro + t*rd;
        vec2 mpv = map(pos);
        float den = clamp(mpv.x-0.3,0.,1.)*1.12;
        float dn = clamp((mpv.x + 2.),0.,3.);

        vec4 col = vec4(0);
        if (mpv.x > 0.6)
        {

            col = vec4(sin(vec3(5.,0.4,0.2) + mpv.y*0.1 +sin(pos.z*0.4)*0.5 + 1.8)*0.5 + 0.5,0.08);
            col *= den*den*den;
            col.rgb *= linstep(4.,-2.5, mpv.x)*2.3;
            float dif =  clamp((den - map(pos+.8).x)/9., 0.001, 1. );
            dif += clamp((den - map(pos+.35).x)/2.5, 0.001, 1. );
            col.xyz *= den*(vec3(0.005,.045,.075) + 1.5*vec3(0.033,0.07,0.03)*dif);
        }

        float fogC = exp(t*0.2 - 2.2);
        col.rgba += vec4(0.06,0.11,0.11, 0.1)*clamp(fogC-fogT, 0., 1.);
        fogT = fogC;
        rez = rez + col*(1. - rez.a);
        t += clamp(0.5 - dn*dn*.05, 0.09, 0.3);
    }
    return clamp(rez, 0.0, 1.0);
}

float getsat(vec3 c)
{
    float mi = min(min(c.x, c.y), c.z);
    float ma = max(max(c.x, c.y), c.z);
    return (ma - mi)/(ma+ 1e-7);
}

//from my "Will it blend" shader (https://www.shadertoy.com/view/lsdGzN)
vec3 iLerp(in vec3 a, in vec3 b, in float x)
{
    vec3 ic = mix(a, b, x) + vec3(1e-6,0.,0.);
    float sd = abs(getsat(ic) - mix(getsat(a), getsat(b), x));
    vec3 dir = normalize(vec3(2.*ic.x - ic.y - ic.z, 2.*ic.y - ic.x - ic.z, 2.*ic.z - ic.y - ic.x));
    float lgt = dot(vec3(1.0), ic);
    float ff = dot(dir, normalize(ic));
    ic += 1.5*dir*sd*ff*lgt;
    return clamp(ic,0.,1.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 q = fragCoord.xy/iResolution.xy;
    vec2 p = (gl_FragCoord.xy - 0.5*iResolution.xy)/iResolution.y;
    bsMo = (iMouse.xy - 0.5*iResolution.xy)/iResolution.y;

    float time = iTime*3.;
    vec3 ro = vec3(0,0,time);

    ro += vec3(sin(iTime)*0.5,sin(iTime*1.)*0.,0);

    float dspAmp = .85;
    ro.xy += disp(ro.z)*dspAmp;
    float tgtDst = 3.5;

    vec3 target = normalize(ro - vec3(disp(time + tgtDst)*dspAmp, time + tgtDst));
    ro.x -= bsMo.x*2.;
    vec3 rightdir = normalize(cross(target, vec3(0,1,0)));
    vec3 updir = normalize(cross(rightdir, target));
    rightdir = normalize(cross(updir, target));
    vec3 rd=normalize((p.x*rightdir + p.y*updir)*1. - target);
    rd.xy *= rot(-disp(time + 3.5).x*0.2 + bsMo.x);
    prm1 = smoothstep(-0.4, 0.4,sin(iTime*0.3));
    vec4 scn = render(ro, rd, time);

    vec3 col = scn.rgb;
    col = iLerp(col.bgr, col.rgb, clamp(1.-prm1,0.05,1.));

    col = pow(col, vec3(.55,0.65,0.6))*vec3(1.,.97,.9);

    col *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.12)*0.7+0.3; //Vign

    fragColor = vec4( col, 1.0 );
}
```

## Algorithm

### Keep Verbatim

- `rot()` - 2D rotation matrix helper
- `m3` - noise rotation constant matrix (the specific values and 1.93 scale are load-bearing)
- `mag2()` - squared magnitude helper
- `linstep()` - linear step helper
- `disp()` - camera displacement function
- `map()` function structure: `p2` displacement, xy rotation, 5-iteration noise loop with `z *= 0.57`, `trk *= 1.4`, `p = p*m3` cascade, radial distance `cl` return
- `render()` loop: front-to-back compositing `rez = rez + col*(1. - rez.a)`, dual-offset lighting samples at `pos+.8` and `pos+.35`, density cubing `den*den*den`, `linstep(4.,-2.5, mpv.x)*2.3` edge fade, dynamic step size `clamp(0.5 - dn*dn*.05, 0.09, 0.3)`, fog integral differencing via `fogC`/`fogT`
- Camera fly-through: `ro = vec3(0,0,time)`, sinusoidal xy sway, `disp()` displacement, target/rightdir/updir construction

### Remove Entirely

- `getsat()` - only used by iLerp
- `iLerp()` - LUT replaces channel swapping color logic
- `col = iLerp(col.bgr, col.rgb, ...)` in main - LUT handles coloring
- `col = pow(col, vec3(.55,0.65,0.6))*vec3(1.,.97,.9)` - gamma/tint; post-effects and LUT handle this
- Vignette line - post-effects handle this
- `vec2 q = fragCoord.xy/iResolution.xy` - only used for vignette

### Substitution Table

| Original | Replace With | Where |
|---|---|---|
| `float prm1 = 0.;` | `uniform float morph;` | global |
| `vec2 bsMo = vec2(0);` | remove | global |
| All `iTime` | `time` | everywhere |
| `iResolution` | `resolution` | main |
| All `iMouse` / `bsMo` usage | remove | main, map |
| `prm1 = smoothstep(-0.4, 0.4, sin(iTime*0.3));` | remove (morph is a uniform) | main |
| `float dspAmp = 0.1 + prm1*0.2;` in map | `float dspAmp = turbulence;` | map() |
| `sin(p.z+iTime)*(0.1 + prm1*0.05) + iTime*0.09` | `sin(p.z+time)*(0.1 + morph*0.05) + time*0.09` | map() |
| `d = abs(d + prm1*3.)+ prm1*.3 - 2.5 + bsMo.y;` | `d = abs(d + morph*3.) + morph*0.3 - 2.5;` | map() |
| `for(int i=0; i<130; i++)` | `for(int i=0; i<MAX_STEPS; i++) { if(i >= marchSteps) break;` | render() |
| `float den = clamp(mpv.x-0.3,0.,1.)*1.12;` | `float den = clamp(mpv.x - densityThreshold, 0., 1.) * 1.12;` | render() |
| `if (mpv.x > 0.6)` | `if (mpv.x > densityThreshold + 0.3)` | render() |
| `col = vec4(sin(vec3(5.,0.4,0.2) + mpv.y*0.1 + sin(pos.z*0.4)*0.5 + 1.8)*0.5 + 0.5, 0.08);` | `float lutIndex = mix(den, t / MAX_DIST, colorBlend); col = vec4(texture(gradientLUT, vec2(lutIndex, 0.5)).rgb, 0.08);` | render() |
| `col.rgb *= linstep(4.,-2.5, mpv.x)*2.3;` | keep verbatim | render() |
| `col.xyz *= den*(vec3(0.005,.045,.075) + 1.5*vec3(0.033,0.07,0.03)*dif);` | `col.rgb *= den * brightness * (0.3 + 1.5*dif);` | render() |
| `vec4(0.06,0.11,0.11, 0.1)` fog color | `vec4(texture(gradientLUT, vec2(0.0, 0.5)).rgb * 0.3, 0.1) * fogIntensity` | render() |
| `ro.x -= bsMo.x*2.;` | remove | main |
| `rd.xy *= rot(-disp(time + 3.5).x*0.2 + bsMo.x);` | `rd.xy *= rot(-disp(time + 3.5).x*0.2);` | main |
| `gl_FragCoord.xy` | `fragTexCoord * resolution` | main |
| `fragColor = vec4(col, 1.0);` | `fragColor = vec4(scn.rgb, scn.a);` (premultiplied from front-to-back compositing) | main |

### New Additions

```glsl
#define MAX_STEPS 130
#define MAX_DIST 50.0

// Uniforms
uniform vec2 resolution;
uniform float time;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float morph;
uniform float colorBlend;
uniform float fogIntensity;
uniform float turbulence;
uniform float densityThreshold;
uniform int marchSteps;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

// FFT brightness computation (in main, before render call)
const int FFT_SAMPLES = 8;
float energy = 0.0;
float binLo = baseFreq / (sampleRate * 0.5);
float binHi = maxFreq / (sampleRate * 0.5);
for (int s = 0; s < FFT_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(FFT_SAMPLES));
    if (bin <= 1.0) {
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
}
float fftBright = pow(clamp(energy / float(FFT_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + fftBright;
```

`brightness` is passed into `render()` as an additional parameter and used in the lighting line (see substitution table).

`time` includes speed: accumulated on CPU as `accum += speed * deltaTime`, passed as uniform. Speed is NOT applied in the shader.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| speed | float | 0.5-6.0 | 3.0 | Flight speed through cloud volume (accumulated on CPU) |
| morph | float | 0.0-1.0 | 0.5 | Cloud morphology - density structure, rotation coupling, displacement mix |
| colorBlend | float | 0.0-1.0 | 0.3 | LUT index source: 0=density-mapped, 1=depth-mapped |
| fogIntensity | float | 0.0-1.0 | 0.5 | Atmospheric fog amount, color sampled from LUT dark end |
| turbulence | float | 0.05-0.5 | 0.15 | Displacement amplitude in noise loop |
| densityThreshold | float | 0.0-1.0 | 0.3 | Minimum density for visible cloud rendering |
| marchSteps | int | 40-130 | 80 | Raymarch iterations (quality vs performance) |
| baseFreq | float | 27.5-440 | 55 | FFT low frequency bound |
| maxFreq | float | 1000-16000 | 14000 | FFT high frequency bound |
| gain | float | 0.1-10.0 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness without audio |

## Modulation Candidates

- **speed**: tempo-reactive flight velocity
- **morph**: structural cloud changes - verse/chorus can reshape the entire volume
- **colorBlend**: shift between density and depth coloring with musical dynamics
- **fogIntensity**: atmospheric density responds to audio energy
- **turbulence**: fine detail intensity wiggles with audio
- **densityThreshold**: audio reveals or hides cloud structure

### Interaction Patterns

**morph x turbulence** (competing forces): morph changes large-scale density structure (shifts the `abs(d + morph*3)` term by up to 3 units), turbulence changes fine displacement amplitude. High morph swells and smooths the volume into broad formations. High turbulence fractures and agitates the noise into chaotic tendrils. Modulating both from different audio bands creates tension between structure and chaos - bass-driven morph builds broad shapes while treble-driven turbulence shreds their edges.

**fogIntensity x FFT brightness** (competing forces): fog dims and obscures, FFT brightness illuminates. High fog + quiet = murky impenetrable haze. High fog + loud = glowing diffuse atmosphere. Low fog + loud = sharp bright formations against dark void. Low fog + quiet = faint ghostly wisps. The push/pull between these two creates atmospheric dynamics that track musical energy.

**densityThreshold x morph** (cascading threshold): morph must push density past the threshold before clouds become visible at all. With high threshold only the densest morph peaks produce visible clouds - creating gated bursts of volume that appear and vanish with musical dynamics. Low threshold reveals everything morph produces, making the full volume continuously visible.

## Notes

- Runs at full resolution. March steps default to 80 (vs original 130) to maintain frame rate. The dynamic step size optimization and early alpha-termination keep quality high despite fewer steps.
- The noise function evaluates 3 `map()` calls per visible step (main sample + 2 lighting offsets), each with 5 sin/cos iterations. At 80 steps worst case that is 1200 trig evaluations per pixel. Monitor frame time.
- `MAX_DIST = 50.0` is the normalization constant for depth-based LUT indexing. Typical march distances are well under this due to early alpha termination.
- The `m3` matrix values and 1.93 scale factor are specifically tuned by nimitz for the noise quality. Do not modify.
- Front-to-back compositing in `render()` produces premultiplied alpha naturally, matching the blend compositor's expected input.
