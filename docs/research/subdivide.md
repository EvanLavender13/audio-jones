# Subdivide

Organic quadrilateral cells splitting and morphing through recursive binary space partition, each cell glowing with audio-reactive gradient color. Cut lines use Catmull-Rom interpolated random positions that shift fluidly over time, creating a squishy living mosaic. Larger cells subdivide into smaller ones probabilistically, with deep cells fading and desaturated cells washing to grayscale.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage (generators render to scratch, composite via blend compositor)

## Attribution

- **Based on**: "Always Watching" by SnoopethDuckDuck
- **Source**: https://www.shadertoy.com/view/ffB3D1
- **License**: CC BY-NC-SA 3.0

## References

- [IQ 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) - sdPoly quad distance field
- [Dave Hoskins hash](https://www.shadertoy.com/view/4djSRW) - h11/h12 pseudo-random functions
- [Catmull-Rom noise](https://www.shadertoy.com/view/MsXGDj) - smooth random interpolation for cut positions

## Reference Code

```glsl
#define C(u,a,b) cross(vec3(u-a,0), vec3(b-a,0)).z > 0.
#define S(a) smoothstep(2./R.y, -2./R.y, a)

// https://www.shadertoy.com/view/4djSRW
float h11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float h12(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// https://www.shadertoy.com/view/MsXGDj
float catrom(float t)
{
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

// https://iquilezles.org/articles/distfunctions2d/
float sdArc( in vec2 p, in vec2 sc, in float ra, float rb )
{
    // sc is the sin/cos of the arc's aperture
    p.x = abs(p.x);
    return ((sc.y*p.x>sc.x*p.y) ? length(p-sc*ra) :
                                  abs(length(p)-ra)) - rb;
}

float sdPoly( in vec2[4] v, in vec2 p )
{
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for( int i=0, j=3; i<4; j=i, i++ )
    {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if( all(c) || all(not(c)) ) s*=-1.0;
    }
    return s*sqrt(d);
}

void mainImage( out vec4 o, vec2 u )
{
    vec2 R = iResolution.xy,
         tl = vec2(-1, 1) * R/R.y,
         tr = vec2( 1, 1) * R/R.y,
         bl = vec2(-1,-1) * R/R.y,
         br = vec2( 1,-1) * R/R.y;

    vec2 v = u,
         w = u / R;
    u = (u+u-R)/R.y;
   // u /= .98 +  .02 *dot(u,u);

    float t = iTime/2.2;// + mod(v.x+v.y,2.)*.002;
    vec4 bg = vec4(.33);

    vec2 ID;
    float area;
    float threshold = mix(.5,.032, smoothstep(2.,5., iTime + .0*mod(v.x+v.y,2.)));

    for (int i; i < 14; i++)
    {
        t += 7.3*h12(ID);

        float k = float(i)+1.;
        float K = 1./k;

        float to = mix(.01*(10.-k)*cos(t*k + (u.x+u.y)*k/2. + h12(ID)*10.), 3.,
                       smoothstep(5.,2., iTime));
        float mx1 = catrom(t);
        float mx2 = catrom(t+to);
        vec2 x1, x2;

        if (i%2 == 0)
        {
            x1 = mix(tl, tr, mx1);
            x2 = mix(bl, br, mx2);
            if (C(u,x1,x2)) tr = x1, br = x2, ID += vec2(K,0);
            else            tl = x1, bl = x2, ID -= vec2(K,0);
        }
        else
        {
            x1 = mix(tl, bl, mx1);
            x2 = mix(tr, br, mx2);
            if (C(u,x1,x2)) tl = x1, tr = x2, ID += vec2(0,K);
            else            bl = x1, br = x2, ID -= vec2(0,K);
        }

        area = tl.x*bl.y + bl.x*br.y + br.x*tr.y + tr.x*tl.y
             - tl.y*bl.x - bl.y*br.x - br.y*tr.x - tr.y*tl.x;

        if (h12(ID) < threshold) break;
    }

    float h = h12(ID);

    vec2 c = (tl+tr+bl+br)/4.;
    vec2 uc = u-c;
    vec4 co = vec4(0, floor(t+9.*(uc.x+uc.y))+.24, .64, 0);
    vec4 col = .8+.4*cos(3.14*(h*1e2 + co));
    o = col; // * vec4(C(u,tl,tr) && C(u,tr,br) && C(u,br,bl) && C(u,bl,tl));

    float l = max(length(tl-br), length(tr-bl));
    o *= .25 + .75*S( sdPoly(vec2[] (tl, bl, br, tr), u) + .0033 );
    o *= .75 + .25*S( sdPoly(vec2[] (tl, bl, br, tr), u+.07*l)   );

    if (h > threshold) o = mix(o, o.rrrr, .9);

    float s = h > .5 ? 1. : -1.;
    o *= .97+.03*S(abs(fract(20.*(uc.x+s*uc.y))-.5)-.25);

    o = clamp(o,0.,1.);
    o = mix(bg, o, exp(-.0007/area));

    float dm = min(length((bl+br)/2. - (tl+tr)/2.),
                   length((bl+tl)/2. - (br+tr)/2.));
    float sdm = smoothstep(.35, .47, dm);

    vec2 p = vec2(-.09,.02*cos(15.*h12(ID+.31)*t))*.75;
    vec2 q = vec2( .09,.02*sin(15.*h12(ID+.31)*t))*.75;

    uc.y -= .02;
    float sp1 = S(length(uc-p)       - .06);
    float sp2 = S(length(uc-p-.0075) - .045);
    float sp3 = S(length(uc-p-.0075) - .0225);
    float sq1 = S(length(uc-q)       - .06);
    float sq2 = S(length(uc-q-.0075) - .045);
    float sq3 = S(length(uc-q-.0075) - .0225);

    vec2 r = vec2(0, -.045 + .02*cos(20.*h12(ID+.4)*t));
    vec2 sc = vec2(sin(.8),cos(.8));
    float sr1 = S(sdArc(-uc-.0075-r, sc, .18, .03));
    float sr2 = S(sdArc(-uc-r, sc, .18, .015));

    o = mix(o, vec4(0), sdm*sp1);
    o = mix(o, vec4(1), sdm*sp2);
    o = mix(o, vec4(0), sdm*sp3);
    o = mix(o, vec4(0), sdm*sq1);
    o = mix(o, vec4(1), sdm*sq2);
    o = mix(o, vec4(0), sdm*sq3);
    o = mix(o, vec4(0), sdm*sr1);
    o = mix(o, vec4(1), sdm*sr2);

    o = mix(bg, o, pow(tanh(64.*w.x*(1.-w.x)*w.y*(1.-w.y)),.23));
}
```

## Algorithm

### Keep verbatim
- `h11()`, `h12()` hash functions
- `catrom()` Catmull-Rom interpolation
- `sdPoly()` quad distance field
- `C()` cross product side test macro
- `S()` smoothstep anti-alias macro
- BSP subdivision loop structure (alternating horizontal/vertical cuts, cross-product side test, ID accumulation, shoelace area calculation)
- Edge darkening SDF operations (both inner inset and shadow offset)
- Area fade formula: `mix(bg, o, exp(-areaFade / area))`
- Desaturation gate: `mix(o, luminance, desatAmount)` when `h > desatThreshold`

### Replace

| Original | Replacement | Reason |
|----------|-------------|--------|
| `iResolution.xy` | `resolution` uniform | Standard uniform name |
| `iTime/2.2` | `time` uniform (CPU-accumulated with `speed` config) | Speed accumulation on CPU, not shader |
| `smoothstep(2.,5., iTime)` threshold intro | `threshold` uniform directly | No intro animation, config param |
| `mix(.01*(10.-k)*cos(...), 3., smoothstep(5.,2., iTime))` | `squish * (10. - k) * cos(t*k + (u.x+u.y)*k/2. + h12(ID)*10.)` | Remove intro animation, expose `squish` as config. The `0.01` constant becomes `squish` param |
| `14` iteration cap | `maxIterations` uniform | Configurable subdivision depth |
| Cosine palette: `co = vec4(0, floor(...)+.24, .64, 0); col = .8+.4*cos(3.14*(h*1e2 + co))` | `vec3 col = texture(gradientLUT, vec2(h, 0.5)).rgb * brightness` | Gradient LUT + FFT brightness |
| `vec4 bg = vec4(.33)` | `vec3(0.0)` | Generators render on black, blend compositor handles compositing |
| `.25 + .75*S(...)` and `.75 + .25*S(...)` edge darkening | `mix(1.0, .25 + .75*S(...), edgeDarken)` and `mix(1.0, .75 + .25*S(...), edgeDarken)` | Configurable edge darkening intensity |
| `if (h > threshold) o = mix(o, o.rrrr, .9)` | `if (h > desatThreshold) o = mix(o, vec3(dot(o, vec3(0.299, 0.587, 0.114))), desatAmount)` | Proper luminance, configurable threshold and amount |
| Hardcoded `0.0007` in area fade | `areaFade` uniform | Configurable fade threshold |

### Remove
- `sdArc()` function (only used for face mouth)
- Diagonal stripe pattern: `o *= .97+.03*S(abs(fract(20.*(uc.x+s*uc.y))-.5)-.25)`
- All face drawing code: eye circles (`sp1`-`sp3`, `sq1`-`sq3`), mouth arc (`sr1`, `sr2`), `dm`/`sdm` face-size gating
- Vignette: `o = mix(bg, o, pow(tanh(64.*w.x*(1.-w.x)*w.y*(1.-w.y)),.23))`
- `vec2 v = u` and `vec2 w = u / R` (only used by removed features)

### Add
- FFT uniforms: `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`
- `gradientLUT` uniform
- Per-cell FFT brightness: cell hash `h` maps to a frequency band position in log space (`baseFreq` to `maxFreq`). Use standard BAND_SAMPLES=4 averaging. Apply `brightness = baseBright + pow(clamp(energy * gain, 0, 1), curve)`. Color and frequency are correlated - cells with similar gradient colors respond to similar frequencies.
- Output clamped to `vec4(col, 1.0)` for blend compositor

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| speed | float | 0.1-2.0 | 0.45 | Animation rate (CPU-accumulated) |
| squish | float | 0.001-0.05 | 0.01 | How wavy/organic the cut lines are |
| threshold | float | 0.01-0.9 | 0.15 | Subdivision probability cutoff - lower = more cells |
| maxIterations | int | 2-20 | 14 | Maximum BSP recursion depth |
| edgeDarken | float | 0.0-1.0 | 0.75 | SDF edge/shadow darkening intensity |
| areaFade | float | 0.0001-0.005 | 0.0007 | Area below which cells dissolve to black |
| desatThreshold | float | 0.0-1.0 | 0.5 | Cell hash above which desaturation applies |
| desatAmount | float | 0.0-1.0 | 0.9 | Strength of desaturation on gated cells |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT magnitude |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor when audio is silent |

## Modulation Candidates

- **speed**: faster/slower cell morphing
- **squish**: straight grid cuts vs organic wobbly cuts
- **threshold**: cell count oscillates - low values fragment the screen, high values leave few large cells
- **edgeDarken**: flat cells vs beveled/shadowed cells
- **areaFade**: controls how aggressively small cells dissolve
- **desatThreshold**: shifts the color/grayscale boundary across the mosaic
- **desatAmount**: full color vs washed out on gated cells

### Interaction Patterns

- **Cascading threshold (desatThreshold x FFT brightness)**: Cells near the desaturation boundary flip between grayscale and vivid as FFT brightness crosses the gate. Quiet passages wash the mosaic to gray; loud passages burst with color. The threshold becomes a song-structure detector - verse vs chorus produces visibly different palettes.
- **Competing forces (threshold x areaFade)**: Low threshold creates many tiny cells, but areaFade dissolves the smallest ones to black. Pushing subdivision deeper simultaneously fragments the image and erases the fragments, creating a dissolution texture where only mid-sized cells survive.
- **Resonance (squish x speed)**: High squish + moderate speed produces maximum organic wobble. At low speed the squish is barely visible; at high speed it blurs into chaos. The sweet spot where both align creates the most satisfying fluid motion.

## Notes

- The BSP loop runs per-pixel (up to `maxIterations` iterations). At 14 iterations this is moderate cost. Going above 16-18 may impact performance on mid-range GPUs.
- `catrom()` samples `h11()` 4 times per call, and is called twice per iteration. The hash functions are pure arithmetic (no texture fetches), so this is fast.
- `sdPoly()` is called twice for edge darkening. This is a 4-edge polygon distance field - lightweight.
- The `area` variable from the shoelace formula can go negative for degenerate quads (self-intersecting). The `exp(-areaFade/area)` handles this gracefully since negative area produces `exp(positive)` which clamps to 1.0 after the mix.
- `vec2[4]` array constructor syntax in `sdPoly(vec2[] (tl, bl, br, tr), u)` requires GLSL 4.30+. Raylib's default OpenGL context supports this on desktop. If targeting GLES, this would need to be rewritten as individual parameters.
