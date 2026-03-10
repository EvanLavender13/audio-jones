# Spectral Rings

Dense concentric rings radiating from center, each band lit by its corresponding FFT frequency. Colors sampled from gradient LUT with noise perturbation for organic variation. Rings expand/contract with a pulse parameter, colors scroll independently, and the whole structure can skew from perfect circles to off-center ellipses.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator stage (before transforms)

## Attribution

- **Based on**: "Rings [324 chars]" by XorDev
- **Source**: https://www.shadertoy.com/view/sstyDX
- **License**: CC BY-NC-SA 3.0

## References

- [2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) - IQ's onion ring technique: `abs(sdf) - r` for concentric layers
- [The Book of Shaders: Shapes](https://thebookofshaders.com/07/) - `fract(length(p))` for repeating radial bands

## Reference Code

```glsl
/*
    "Rings [324 chars]" by @XorDev
    Tweet: https://twitter.com/XorDev/status/1532211104583131136
    Twigl: https://t.co/eyzpt9hV4A

    Also see "Mars": https://www.shadertoy.com/view/sdcyWN

    Bokeh based on: shadertoy.com/view/fljyWd
    -3 thanks to iq

    <512 Chars playlist: shadertoy.com/playlist/N3SyzR
*/

#define S texture(iChannel0

void mainImage(out vec4 O, vec2 I)
{
    vec2 r = iResolution.xy,     //Resolution for texel calculations
    i = r/r,                     //Iteration variable
    d = (I.x-I-I+r*.2).yy/2e3,   //Depth of field vector
    p,                           //Sample point
    l;                           //Sample point length

    //Clear color
    for(O-=O;
    //Iterate approximately 16*16 times
    i.x<16.;
    //Approximately i = sqrt of the number of iterations
    i+=1./i)

        //Compute sample length and point with skewing
        l = length(p=(I+d*i)*mat2(2,1,-2,4)/r.y-vec2(-.1,.6)) / vec2(3,8),
        //Rotate DOF sample vector
        d *= -mat2(73,67,-67,73)/99.,
        //Add rings (radial and cartesian noise)
        O += pow(S,l)*S,p*mat2(cos(iTime*.1+vec4(0,33,11,0))))/l.x*.4,vec4(5,8,9,0));
    //Take fifth root for boosted contrast
    O=pow(O,.2+O-O);
}
```

## Algorithm

The reference packs enormous complexity into a code-golf shader. The adaptation extracts the core ideas into readable, parameterized form:

### What to keep from the reference
- **Radial ring structure**: `fract(length(p) * ringDensity)` creates concentric bands. The reference achieves this implicitly through its `pow(texture, radialDist)` accumulation — we make it explicit.
- **Matrix skew for eccentricity**: The reference's `mat2(2,1,-2,4)` skews circles into ellipses. We parameterize this as an eccentricity + angle control.
- **Slow rotation**: The reference's `mat2(cos(iTime*.1+vec4(0,33,11,0)))` rotates the color sampling. We separate this into ring rotation and color shift.
- **Power-curve contrast**: The reference's `pow(O, 0.2)` fifth-root gives rich lifted contrast. We keep this as a fixed final step.

### What to replace
- **iChannel0 texture sampling → gradient LUT**: Instead of reading a texture at radial distance, sample `gradientLUT` at a position derived from `fract(dist * ringDensity + colorShiftAccum)` plus noise offset for variation.
- **DOF blur loop → single-pass ring evaluation**: Drop the 256-iteration DOF accumulation. Instead, compute ring brightness directly from radial distance and FFT.
- **Implicit ring count → explicit layers parameter**: Ring count controlled by `layers` (integer), each mapped to a frequency band via log-space spread.

### Ring construction
1. Center coordinates: `pos = fragTexCoord * resolution - resolution * 0.5`
2. Apply eccentricity: `pos = mat2(1+ecc, skewX, skewY, 1-ecc) * pos` (parameterized from reference's `mat2(2,1,-2,4)`)
3. Apply rotation: `pos *= mat2(cos/sin of rotationAccum)`
4. Radial distance: `dist = length(pos) / maxRadius`
5. Ring band: `band = fract(dist * ringDensity + pulseAccum)` — pulse shifts bands outward/inward
6. Ring brightness: sharp band via `smoothstep` on band edges, giving bright rings separated by dark gaps
7. Ring width controlled by a `ringWidth` parameter (0 = hairline, 1 = solid fill)

### FFT mapping
- Each radial distance maps to a frequency via log-space spread from `baseFreq` to `maxFreq`
- Sample FFT texture at the corresponding bin
- Multiply ring brightness by `pow(fftValue * gain, curve) + baseBright`

### Color sampling
- Base LUT position: `dist * ringDensity + colorShiftAccum` (scrolls independently of ring pulse)
- Noise perturbation: `lut_u += noise2D(pos * noiseScale + time) * noiseAmount`
- Sample: `texture(gradientLUT, vec2(fract(lut_u), 0.5)).rgb`
- This gives varied color within rings — adjacent rings get different hues, and within a single ring the color shifts subtly

### Final composite
- Accumulate `color * ringBrightness * fftBrightness`
- Apply fifth-root contrast: `pow(color, vec3(0.2))`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| ringDensity | float | 4.0-64.0 | 24.0 | Number of visible ring bands |
| ringWidth | float | 0.05-1.0 | 0.5 | Width of bright band vs dark gap |
| pulseSpeed | float | -2.0-2.0 | 0.0 | Ring expansion/contraction rate (radians/sec) |
| colorShiftSpeed | float | -2.0-2.0 | 0.1 | LUT scroll speed independent of ring motion |
| rotationSpeed | float | -PI-PI | 0.1 | Ring structure rotation rate |
| eccentricity | float | 0.0-0.8 | 0.0 | Circle-to-ellipse deformation |
| skewAngle | float | -PI-PI | 0.0 | Angle of ellipse major axis |
| noiseAmount | float | 0.0-0.5 | 0.15 | Color variation within/between rings |
| noiseScale | float | 1.0-20.0 | 5.0 | Spatial frequency of color noise |
| baseFreq | float | 27.5-440.0 | 55.0 | Low frequency bound for FFT mapping |
| maxFreq | float | 1000-16000 | 14000.0 | High frequency bound for FFT mapping |
| gain | float | 0.1-10.0 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when audio silent |
| layers | int | 8-128 | 48 | FFT sampling resolution across radius |

## Modulation Candidates

- **ringDensity**: Modulating creates rings that breathe wider/tighter — low values give bold few bands, high values give dense fine lines
- **ringWidth**: Thin hairlines vs fat glowing bands; sharp visual contrast between verse (thin) and chorus (thick)
- **pulseSpeed**: Drives radial expansion/contraction rate to the beat
- **eccentricity**: Morphs between perfect circles and stretched ellipses
- **noiseAmount**: Clean uniform color vs chaotic varied color per ring
- **gain**: Overall FFT reactivity intensity

### Interaction Patterns

- **ringWidth × gain (cascading threshold)**: When `ringWidth` is narrow, only the highest-amplitude FFT bands produce visible rings — quiet frequencies fall in the dark gaps. Widening `ringWidth` reveals more of the spectrum. Bass-heavy sections show only a few bold inner rings; full-spectrum sections light up the whole field.
- **pulseSpeed × colorShiftSpeed (competing forces)**: When both push in the same direction, color stays locked to rings and the whole thing scrolls uniformly. When they oppose, colors visibly slide through the ring structure — creates a shimmering interference that's rare when both speeds are modulated by different sources.
- **eccentricity × rotationSpeed (resonance)**: Static eccentricity just tilts the rings. But with rotation, the elliptical compression sweeps around — frequency bands alternately bunch up and spread out as the axis rotates, creating a rhythmic density modulation that amplifies both parameters.

## Notes

- The fifth-root contrast (`pow(color, 0.2)`) is key to the reference's rich look — it lifts darks heavily. Consider making the exponent a parameter (0.1-1.0) for user control.
- Ring density above ~48 may alias at lower resolutions; consider anti-aliased `smoothstep` band evaluation.
- The noise perturbation on LUT sampling is what prevents the "barber pole" look of perfectly uniform gradient banding — it's essential to the organic feel.
- Performance: single-pass evaluation (no iterative accumulation like the reference) should be very lightweight.
