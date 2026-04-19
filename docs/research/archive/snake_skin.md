# Snake Skin

Overlapping reptile scales tile the screen in a staggered grid, each with pointed spoke ridges, tapered shape, and 3D relief shading. Per-scale brightness varies randomly with occasional bright twinkle flashes. A horizontal sine wave undulates the grid like a slithering body. Gradient LUT colors the palette while spoke/edge/shadow shading is preserved as luminance variation on top, keeping the dimensional look. Scales scroll vertically at a modulatable rate (default zero - a still skin that starts crawling when modulated). Per-scale hash drives both LUT color and FFT frequency lookup, so each scale reacts to its own spectral band.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after drawables, before transforms)
- **Section index**: 12 (Texture)

## Attribution

- **Based on**: "Rainbow Snake" by Martijn Steinrucken aka BigWings (2015)
- **Source**: https://www.shadertoy.com/view/4dc3R2
- **License**: CC BY-NC-SA 3.0

## References

- [Rainbow Snake](https://www.shadertoy.com/view/4dc3R2) - Complete scale tiling, shape morphing, and compositing technique

## Reference Code

```glsl
// "Rainbow Snake" by Martijn Steinrucken aka BigWings - 2015
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define SIZE 10.
#define PI 3.1415

#define MOD3 vec3(.1031,.11369,.13787)
float hash12(vec2 p) {
     // From https://www.shadertoy.com/view/4djSRW
    // Dave Hoskins
    vec3 p3  = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float brightness(vec2 uv, vec2 id) {
    // returns the brightness of a scale, based on its id
    float t = iTime;
    float n = hash12(id);
    float c = mix(0.7, 1., n);                // random variation
    
    float x = abs(id.x-SIZE*.5);
    float stripes = sin(x*.65+sin(id.y*.5)+.3)*.5+.5;        // pattern
    stripes = pow(stripes, 4.);
    c *= 1.-stripes*.5;
    
    float y = floor(uv.y*SIZE);
    float twinkle = sin(t+n*6.28)*.5 +.5;
    twinkle = pow(twinkle, 40.);
    c += twinkle*.5;                 
    
    return c;
}

float spokes(vec2 uv, float spikeFrequency) {
    // creates spokes radiating from the top of the scale
    
    vec2 p = vec2(0., 1.);
    vec2 d = p-uv;
    float l = length(d);
    d /= l;
    
    vec2 up = vec2(1., 0.);
    
    float c = dot(up, d);
    c = abs(sin(c*spikeFrequency));
    c *= l;
    
    return c;
}

vec4 ScaleInfo(vec2 uv, vec2 p, vec3 shape) {
    
    float spikeAmount = shape.x;
    float spikeFrequency = shape.y;
    float sag = shape.z;
    
    uv -= p;
    
    uv = uv*2.-1.;
      
    float d2 = spokes(uv, spikeFrequency);
    
    uv.x *= 1.+uv.y*sag;
   
    float d1 = dot(uv, uv);                    // distance to the center of the scale
   
    float threshold = 1.;//sin(iTime)*.5 +.5;
    
    float d = d1+d2*spikeAmount;
    
    float f = 0.05;//fwidth(d);
    float c = smoothstep(threshold, threshold-f, d);
    
    return vec4(d1, d2, d, c);
}

vec4 ScaleCol(vec2 uv, vec2 p, vec4 i) {
    
    vec3 col1 = vec3(.1, .3, .2);
    vec3 col2 = vec3(.8, .5, .2);
    vec3 baseCol = vec3(.1, .3, .2)*3.;
    uv-=p;
    
    float grad = 1.-uv.y;
    float col = grad+i.x;
    col = col*.2+.5;
    
    vec4 c = vec4(col*baseCol, i.a);
    
    c.rgb = mix(c.rgb, col1, i.y*i.x*.5);        // add spokes
    c.rgb = mix(c.rgb, col2, i.x);                // add edge highlights
            
    c = mix(vec4(0.), c, i.a);
    
    float fade = 0.3;
    float shadow = smoothstep(1.+fade, 1., i.z);
  
    c.a = mix(shadow*.25, 1., i.a);
    
    return c;
}


vec4 Scale(vec2 uv, vec2 p, vec3 shape) {
    
    vec4 info = ScaleInfo(uv, p, shape);
    vec4 c = ScaleCol(uv, p, info);
    
    return c;
}

vec4 ScaleTex(vec2 uv, vec2 uv2, vec3 shape) {
    // id = a vec2 that is unique per scale, can be used to apply effects on a per-scale basis
    // shape = a vec3 describing the shape of the scale:
    //            x = the amount of spikyness, can go negative to bulge spikes the opposite way
    //            y = the number of spikes
    //          z = the taper of the scale (0=round -1=top wider 1=bottom wider)
    
    vec2 id = floor(uv2);
    uv2 -= id;
    
    // need to render a bunch of scales per pixel, so they can overlap
    vec4 rScale = Scale(uv2, vec2(.5, 0.01), shape);
       vec4 lScale = Scale(uv2, vec2(-.5, 0.01), shape);
    vec4 bScale = Scale(uv2, vec2(0., -0.5), shape);
    vec4 tScale = Scale(uv2, vec2(0., 0.5), shape);
    vec4 t2Scale = Scale(uv2, vec2(1., 0.5), shape);
    
    // every scale has a slightly different brightness + pattern
    rScale.rgb *= brightness(uv, id+vec2(1.,0.));
    lScale.rgb *= brightness(uv, id+vec2(0.,0.));
    
    bScale.rgb *= brightness(uv, id+vec2(0.,0.));
    
    tScale.rgb *= brightness(uv, id+vec2(0.,1.));
    t2Scale.rgb *= brightness(uv, id+vec2(2.,1.));
    
    // start with base color and alpha blend in all of the scales
    vec4 c =  vec4(.1, .3, .2,1.);
    c = mix(c, bScale, bScale.a);
    c = mix(c, rScale, rScale.a);
    c = mix(c, lScale, lScale.a);
    c = mix(c, tScale, tScale.a);
    c = mix(c, t2Scale, t2Scale.a);
    
   // c.rg = uv;
   // c.b = 0.;
    return c;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float aspect = iResolution.x/iResolution.y;
    
    vec2 uv = fragCoord.xy / iResolution.xy;  // get uvs in 0 to 1 range
    vec2 m = iMouse.xy/iResolution.xy;
    m = m*2. - 1.;
    float t = iTime;
    
    
    
       uv.x += sin(t+uv.y)*.1;
    
    uv.x*=2.;
    
    uv -=.5;
    
    
    vec2 uv2 = uv * SIZE;                        // uv2 -2.5 - 2.5
    uv2.y -= t;
    uv +=.5;
       
    float grad = (uv2.y+12.5)/15.;                // goes from 0-1 

    vec3 center = vec3(.6, 1., -.8);              // scale shape settings for center -> amount frequency sag
    vec3 outside = vec3(0.1, 8., -.9);          // settings for side
    
    float sideFade = pow(uv.x-1.,2.);
    vec3 shape = mix(center, outside, sideFade);// morph between scale shapes
    
   // shape = mix(center, outside, 1.);
    
  
    
    vec4 c = ScaleTex(uv, uv2, shape);            // sample scales
    c = mix(c, vec4(c.g), sideFade);            // fade color towards the edges
    
    t*=.1;                                        // rainbow....
    c.r += sin(t)*.4;
    c.g -= abs(sin(t*1.324))*.435;
    c.b += sin(t*0.324)*.3;
    
    float y = pow(uv.y-.5,2.)*4.;                // vignette
    c *= 1.-y;
    
    fragColor = vec4(c);
}
```

## Algorithm

### Keep verbatim
- `hash12()` - works as-is, provides per-scale random value
- `spokes()` - works as-is, generates spoke ridges radiating from scale top
- `ScaleInfo()` - works as-is, computes scale shape mask (spike amount, frequency, sag come from uniforms instead of a vec3)
- `Scale()` - composition wrapper, works as-is once ScaleCol is adapted
- `ScaleTex()` overlapping scale pattern - 5-scale overlap compositing, works as-is once brightness() is adapted

### Replace

| Reference | Ours | Reason |
|-----------|------|--------|
| `#define SIZE 10.` | `uniform float scaleSize` | Modulatable param |
| `iTime` in `brightness()` twinkle | `uniform float twinklePhase` | CPU-accumulated twinkle rate |
| `twinkle = sin(t+n*6.28)*.5+.5; twinkle = pow(twinkle, 40.); c += twinkle*.5;` | `twinkle = sin(twinklePhase+n*6.28)*.5+.5; twinkle = pow(twinkle, 40.); c += twinkle * twinkleIntensity;` | Modulatable twinkle intensity, CPU-accumulated phase |
| `abs(id.x-SIZE*.5)` in brightness stripes | `abs(id.x - scaleSize * 0.5)` | Use uniform instead of define |
| `iTime` scroll in mainImage (`uv2.y -= t`) | `uv2.y -= scrollPhase` | CPU-accumulated scroll speed |
| `sin(t+uv.y)*.1` horizontal wave | `sin(wavePhase + uv.y * scaleSize) * waveAmplitude` | CPU-accumulated wave phase, modulatable amplitude, scale UV frequency with grid size so wave period matches tile count |
| `uv.x *= 2.` snake body stretch | Remove | Uniform shape (no spatial morph) |
| `vec3 center/outside` + `sideFade` shape morph | `vec3 shape = vec3(spikeAmount, spikeFrequency, sag)` | Uniform shape from param uniforms |
| `c = mix(c, vec4(c.g), sideFade)` edge desaturation | Remove | No spatial morph |
| `ScaleCol()` hardcoded colors | LUT-based coloring (see below) | Gradient palette system |
| Rainbow color cycle (`c.r += sin(t)...`) | Remove | LUT handles palette |
| Vignette (`pow(uv.y-.5,2.)*4.`) | Remove | Handled by other effects |
| `iMouse` | Remove | Not used |
| `iResolution` | `uniform vec2 resolution` | Standard uniform |
| `fragCoord.xy / iResolution.xy` | `fragTexCoord` | Standard UV input |

### LUT-based ScaleCol adaptation

Replace the hardcoded color logic with gradient LUT sampling. The per-scale hash `n = hash12(id)` serves as the shared index `t` for both LUT color and FFT frequency lookup:

```
// In ScaleCol, replace hardcoded col1/col2/baseCol with:
float n = hash12(scaleId);  // passed through from ScaleTex
vec3 lutColor = texture(gradientLUT, vec2(n, 0.5)).rgb;

// Preserve luminance shading from reference
float lum = (grad + i.x) * 0.2 + 0.5;        // same as reference col calculation
vec3 spokeShade = lutColor * 0.3;              // darker variant for spoke detail
vec3 edgeShade = lutColor * 1.5;               // brighter variant for edge highlights

vec4 c = vec4(lum * lutColor, i.a);
c.rgb = mix(c.rgb, spokeShade, i.y * i.x * 0.5);  // spoke ridges darken
c.rgb = mix(c.rgb, edgeShade, i.x);                // edges brighten
```

### FFT audio reactivity

The same per-scale hash `n` used for LUT lookup also drives FFT frequency:

```
float freq = baseFreq * pow(maxFreq / baseFreq, n);
float bin = freq / (sampleRate * 0.5);
float energy = 0.0;
if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float audioBright = baseBright + mag;
```

Multiply `audioBright` into the per-scale brightness in `brightness()`, so each scale pulses with its assigned frequency band.

### Aspect ratio correction

Add `uv.x *= resolution.x / resolution.y` before tiling so scales remain square regardless of window aspect.

### Coordinate flow (mainImage equivalent)

```
vec2 uv = fragTexCoord;
uv.x *= resolution.x / resolution.y;          // aspect correction
uv.x += sin(wavePhase + uv.y * scaleSize) * waveAmplitude;
uv -= 0.5;
vec2 uv2 = uv * scaleSize;
uv2.y -= scrollPhase;
uv += 0.5;
vec3 shape = vec3(spikeAmount, spikeFrequency, sag);
vec4 c = ScaleTex(uv, uv2, shape);
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| scaleSize | float | 3.0-30.0 | 10.0 | Grid density - number of scale cells across the screen |
| spikeAmount | float | -1.0-2.0 | 0.6 | Spikiness of scale edges (negative bulges outward) |
| spikeFrequency | float | 0.0-16.0 | 1.0 | Number of spoke ridges per scale |
| sag | float | -2.0-0.0 | -0.8 | Scale taper (-2 = top wider, 0 = round) |
| scrollSpeed | float | -5.0-5.0 | 0.0 | Vertical scroll rate (accumulated on CPU) |
| waveSpeed | float | 0.1-5.0 | 1.0 | Horizontal undulation rate (accumulated on CPU) |
| waveAmplitude | float | 0.0-0.5 | 0.1 | Horizontal wave displacement |
| twinkleSpeed | float | 0.1-5.0 | 1.0 | Per-scale flash pulse rate (accumulated on CPU) |
| twinkleIntensity | float | 0.0-1.0 | 0.5 | Brightness of twinkle flashes |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency bound |
| maxFreq | float | 1000-16000 | 14000 | FFT high frequency bound |
| gain | float | 0.1-10.0 | 2.0 | FFT gain multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **scaleSize**: zooms grid density, fewer large scales vs many tiny ones
- **spikeAmount**: morphs between smooth round scales and jagged spiky plates
- **spikeFrequency**: changes spoke ridge count, smooth at 0 to spiny at 16
- **sag**: shifts scale taper from round to elongated
- **scrollSpeed**: skin crawls when driven, still when silent
- **waveAmplitude**: slither intensity, flat grid at 0 to dramatic undulation
- **twinkleIntensity**: sparkle gate, flashes appear/disappear
- **gain**: overall audio reactivity intensity

### Interaction Patterns

- **spikeAmount + gain** (competing forces): High spike amount creates aggressive jagged scales; FFT-driven brightness flaring on top creates tension where loud passages light up spiky geometry while quiet passages let smooth dark shapes dominate
- **twinkleIntensity + scrollSpeed** (resonance): Both active creates a shimmering cascade where scrolling scales pass through the twinkle phase window, faster scroll producing more frequent flash bursts
- **waveAmplitude + scaleSize** (cascading threshold): At small scale sizes the wave reads as subtle ripple; at large sizes the same amplitude tears the grid into dramatic undulation. Scale size gates how theatrical the wave appears

## Notes

- 5 overlapping Scale() calls per cell in ScaleTex is the main cost. At large scaleSize values (25+) this is many cells per screen but each cell is cheap. Performance should be fine for fullscreen.
- `smoothstep` threshold is hardcoded to 1.0 in the reference. Could be exposed as a param for scale size variation, but adds complexity for marginal benefit. Keep hardcoded.
- The reference's `fwidth(d)` for anti-aliasing is commented out and replaced with `0.05`. Keep the constant - fwidth in this context would need careful handling across the tiled grid.
- Three CPU-accumulated phases needed: `scrollPhase`, `wavePhase`, `twinklePhase`.
