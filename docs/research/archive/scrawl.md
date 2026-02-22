# Scrawl

Thick flowing marker curves drawn by an IFS fractal fold, rendered as bold glowing strokes over a gritty scanline texture. Looks like neon graffiti sprayed on a weathered steel wall — tangled curves that continuously drift and evolve with the music.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after trail boost, blended via blend compositor)
- **Compute Model**: Single-pass fragment shader

## References

- `shaders/motherboard.fs` — Existing IFS generator with per-iteration FFT glow, gradient LUT coloring, abs-fold loop. Direct structural template.
- User-provided Shadertoy shader — Source of the specific fold formula and thick-marker rendering technique.

## Reference Code

The user-provided Shadertoy shader (complete, unmodified):

```glsl
#define so texture(iChannel0,vec2(.5,0.)).r

mat2 rot(float a) {
    a=radians(a);
    float s=sin(a), c=cos(a);
    return mat2(c,s,-s,c);
}


vec3 fractal(vec2 p) {
    p/=1.-p.y;
    float a=floor(iTime*.3);
    p*=rot(45.*a);
    vec2 p2=p;
    p*=.3+asin(.9*sin(iTime*.5))*.2;
    p.y-=a;
    p.x+=iTime*.3+so*.2;
    p=fract(p*.5);
    float m=1000.;
    float it=0.;
    for (int i=0; i<8; i++) {
        float s=sign(p.y);
        p=abs(p)/clamp(abs(p.x*p.y),.5,1.)-1.-mod(floor(iTime*.2),3.)*.5;
        float l=abs(p.x+asin(.9*sin(length(p)*20.))*.3);
        m=min(m,l);
        if (m==l) {
            it=float(i);
        }

    }
    float f=smoothstep(.015,.01,m*.5);
    f*=step(-p2.y*.0+p2.x+it*.1-.4+sin(p.y*2.)*.1,.0);
    vec3 col=normalize(vec3(1.,.0,.5));
    col.rb*=rot(length(p2+it*.5)*200.+iTime*10.);
    col=normalize(col+.5)+step(.5,fract(p2.y*50.))*.5;
    return 2.-10.*col*(f*.9+.1)*(1.5-so*2.);
}


void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy - .5;
    uv.x*=iResolution.x/iResolution.y;
    uv*=1.+so*.1;
    int aa = 3;
    float f=max(abs(uv.x),abs(uv.y));
    vec2 pixelSize = .01 / normalize(iResolution.xy) / float(aa) * f;
    vec3 col = vec3(0.0);

    for (int i = -aa; i <= aa; i++)
    {
        for (int j = -aa; j <= aa; j++)
        {
            vec2 offset = vec2(float(i), float(j)) * pixelSize;
            col += fractal(uv + offset);
        }
    }

    float totalSamples = float((aa * 2 + 1) * (aa * 2 + 1));
    col /= totalSamples;
    col*=exp(-1.5*f);

    fragColor = vec4(col, 1.0);
}
```

## Algorithm

Adapt the reference shader's IFS fold into AudioJones generator conventions, using Motherboard as the structural template.

### What to keep from reference
- The core fold: `p = abs(p) / clamp(abs(p.x * p.y), 0.5, 1.0) - 1.0 - foldOffset` — this is what creates the distinctive curving marker strokes (unlike Motherboard's simpler `abs(p) - range` fold)
- Sinusoidal distance warping: `abs(p.x + asin(0.9 * sin(length(p) * warpFreq)) * warpAmp)` — gives the curves their organic wobble
- Thick rendering via `smoothstep` on the distance field minimum
- Scanline overlay via `step(0.5, fract(p.y * scanlineFreq))` — the gritty surface texture

### What to replace
- Remove perspective projection (`p/=1.-p.y`) — flat wall, not corridor
- Remove all `floor(iTime)` discrete snapping — use smooth continuous `time` uniform
- Remove the 7x7 supersampling loop — too expensive, rely on pipeline FXAA
- Remove hardcoded color (`normalize(vec3(1,0,.5))`) — use `gradientLUT` sampling by iteration depth
- Remove inverted output (`2. - 10.*col`) — dark background with glowing strokes
- Replace `iChannel0` audio with standard FFT semitone sampling (Motherboard pattern)
- Replace `iTime` scrolling with CPU-accumulated `scrollAccum` and `evolveAccum` uniforms

### Rendering approach
- Track minimum distance `m` and winning iteration `it` across the IFS loop (same as reference)
- Glow: `smoothstep(thickness, 0.0, m) * glowIntensity / max(abs(m), 0.001)` — Motherboard's glow formula but using the tracked minimum distance
- Color: `texture(gradientLUT, vec2(it / float(iterations), 0.5)).rgb` — iteration depth maps to LUT position
- Scanlines: multiply final color by `1.0 - scanlineStrength * step(0.5, fract(p.y * scanlineDensity))` — darkens alternating horizontal bands

### Space setup
- Center coordinates: `vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y * zoom`
- Tile: `p = fract(p * tileScale) - 0.5` (centered tiles, not bottom-left anchored)
- Apply rotation before fold loop: `p = rot * p` using CPU-accumulated rotation

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| iterations | int | 2-12 | 6 | Curve density — few = sparse bold strokes, many = dense tangle |
| foldOffset | float | 0.5-2.0 | 1.2 | Shifts the fold attractor, changing curve topology |
| thickness | float | 0.005-0.15 | 0.03 | Marker stroke width |
| warpFreq | float | 5.0-40.0 | 20.0 | Frequency of sinusoidal wobble on curves |
| warpAmp | float | 0.0-0.5 | 0.3 | Amplitude of sinusoidal wobble |
| tileScale | float | 0.2-2.0 | 0.5 | Repetition density of the tiled pattern |
| zoom | float | 1.0-8.0 | 4.0 | Overall view scale |
| scrollSpeed | float | -1.0-1.0 | 0.3 | Horizontal drift speed (accumulated on CPU) |
| evolveSpeed | float | -1.0-1.0 | 0.2 | Fold parameter evolution speed (accumulated on CPU) |
| rotationSpeed | float | -PI-PI | 0.0 | Global rotation speed (accumulated on CPU) |
| glowIntensity | float | 0.5-5.0 | 1.5 | Stroke brightness |
| scanlineDensity | float | 10.0-100.0 | 50.0 | Scanline count (higher = finer grit) |
| scanlineStrength | float | 0.0-0.8 | 0.3 | Scanline darkness (0 = none, 0.8 = heavy grit) |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency |
| maxFreq | float | 1000-16000 | 14000 | FFT high frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT gain |
| curve | float | 0.1-3.0 | 1.0 | FFT contrast |
| baseBright | float | 0.0-1.0 | 0.2 | Minimum brightness without audio |

## Modulation Candidates

- **iterations**: Density shifts — sparse verse, tangled chorus
- **foldOffset**: Topology morphing — curves reorganize into different shapes
- **thickness**: Stroke weight pulses with bass
- **warpAmp**: Wobble intensity — calm vs agitated curves
- **glowIntensity**: Overall brightness pulsing
- **scrollSpeed**: Drift rate changes with energy
- **scanlineStrength**: Grit fades in/out

### Interaction Patterns

**Cascading threshold (thickness + glowIntensity)**: When thickness is thin and glowIntensity is low, only the strongest curve intersections are visible. As either increases, progressively more of the curve network reveals — quiet sections show sparse bright nodes, loud sections expose the full tangle.

**Competing forces (foldOffset + warpAmp)**: foldOffset controls the geometric structure of the fold while warpAmp disrupts it with organic wobble. Low warpAmp with modulated foldOffset = clean geometric morphing. High warpAmp with stable foldOffset = organic trembling. Both modulated = chaotic evolution where structure and chaos fight for dominance.

## Notes

- No supersampling — the original's 7x7 grid (49 samples/pixel) is way too expensive for real-time. The thick stroke width and pipeline FXAA should keep aliasing manageable.
- The `clamp(abs(p.x*p.y), 0.5, 1.0)` divisor is what makes this fold unique vs Motherboard's simpler subtraction fold. It creates curved rather than angular structures because the division factor varies smoothly across the plane.
- `asin(0.9 * sin(...))` approximates a triangle wave — it's what gives the distance field its hand-drawn wobble rather than smooth sinusoidal waves.
- Speed params (scrollSpeed, evolveSpeed, rotationSpeed) accumulate on CPU, shader receives accumulated values.
