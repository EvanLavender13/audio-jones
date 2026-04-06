# Infinity Matrix

Infinite recursive fractal zoom through self-similar binary digit glyphs. Each lit pixel of a glyph contains a complete smaller glyph grid, creating true self-similarity with continuous zoom navigation. Shallow recursion levels show large 0s and 1s; the deepest levels reveal tiny glyphs nested inside glyphs inside glyphs. The view zooms endlessly inward, navigating a deterministic path through the fractal along randomly-chosen lit pixels.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after drawables, before transforms)

## Attribution

- **Based on**: "Infinity Matrix" by KilledByAPixel (Frank Force)
- **Source**: https://www.shadertoy.com/view/Md2fRR
- **License**: CC BY-NC-SA 3.0

## References

- [Infinity Matrix - Shadertoy](https://www.shadertoy.com/view/Md2fRR) - Complete reference implementation (pasted below)

## Reference Code

### Buffer A (fractal render)

```glsl
//////////////////////////////////////////////////////////////////////////////////
// Infinity Matrix - Copyright 2017 Frank Force
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//////////////////////////////////////////////////////////////////////////////////

const float zoomSpeed            = 1.0;    // how fast to zoom (negative to zoom out)
const float zoomScale            = 0.1;    // how much to multiply overall zoom (closer to zero zooms in)
const int recursionCount        = 5;    // how deep to recurse
const float recursionFadeDepth    = 3.0;    // how deep to fade out
const int glyphSize                = 5;    // width & height of glyph in pixels
const int glyphCount            = 2;    // how many glyphs total
const float glyphMargin            = 0.5;    // how much to center the glyph in each pixel
const int glyphs[glyphSize*glyphCount] = int[]
(    // glyph sheet
    0x01110, 0x01110, 
    0x11011, 0x11110,
    0x11011, 0x01110, 
    0x11011, 0x01110,
    0x01110, 0x11111
);    //  0        1

//////////////////////////////////////////////////////////////////////////////////
// Precached values and math

const float glyphSizeF = float(glyphSize) + 2.0*glyphMargin;
const float glyphSizeLog = log(glyphSizeF);
const int powTableCount = 10;
const float gsfi = 1.0 / glyphSizeF;
const float powTable[powTableCount] = float[]( 1.0, gsfi, pow(gsfi,2.0), pow(gsfi,3.0), pow(gsfi,4.0), pow(gsfi,5.0), pow(gsfi,6.0), pow(gsfi,7.0), pow(gsfi,8.0), pow(gsfi,9.0));
const float e = 2.718281828459;
const float pi = 3.14159265359;

float RandFloat(int i) { return (fract(sin(float(i)) * 43758.5453)); }
int RandInt(int i) { return int(100000.0*RandFloat(i)); }

vec3 HsvToRgb(vec3 c) 
{
    float s = c.y * c.z;
    float s_n = c.z - s * .5;
    return vec3(s_n) + vec3(s) * cos(2.0 * pi * (c.x + vec3(1.0, 0.6666, .3333)));
}

//////////////////////////////////////////////////////////////////////////////////
// Color and image manipulation

float GetRecursionFade(int r, float timePercent)
{
    if (r > recursionCount)
        return timePercent;
    
    // fade in and out recusion
    float rt = max(float(r) - timePercent - recursionFadeDepth, 0.0);
    float rc = float(recursionCount) - recursionFadeDepth;
    return rt / rc;
}

vec3 InitPixelColor() { return vec3(0); }
vec3 CombinePixelColor(vec3 color, float timePercent, int i, int r, vec2 pos, ivec2 glyphPos, ivec2 glyphPosLast)
{
    vec3 myColor = vec3(0.6);
        
    myColor.r *= mix(0.0, 0.7, RandFloat(i + r + 11*glyphPosLast.x + 13*glyphPosLast.y));
    myColor.b *= mix(0.0, 0.7, RandFloat(i + r + 17*glyphPosLast.x + 19*glyphPosLast.y));
    myColor *= mix(0.3, 1.0, RandFloat(i + r + 31*glyphPosLast.x + 37*glyphPosLast.y));

    // combine with my color
    float f = GetRecursionFade(r, timePercent);
    color += myColor*f;
    return color;
}

vec3 FinishPixel(vec3 color, vec2 uv)
{
    // brighten
    color += vec3(0.07);
    
    // noise
    vec3 noise = vec3(1.0);    
    noise += mix( -0.2, 0.4, texture(iChannel0, 0.00111*uv*iResolution.y + vec2(-23.3*iTime, 37.5*iTime)).x);
    noise += mix( -0.2, 0.4, texture(iChannel0, 0.00182*uv*iResolution.y + vec2(13.1*iTime, -20.1*iTime)).x);
    color *= noise;
    
    // make green
    color *= vec3(0.8, 1.0, 0.8);
    return color;
}

vec2 InitUV(vec2 uv)
{
    // wave
    uv.x += 0.1*sin(2.0*uv.y + 1.0*iTime);
    uv.y += 0.1*sin(2.0*uv.x + 0.8*iTime);
    return uv;
}


//////////////////////////////////////////////////////////////////////////////////
// Fractal functions

int GetFocusGlyph(int i) { return RandInt(i) % glyphCount; }
int GetGlyphPixelRow(int y, int g) { return glyphs[g + (glyphSize - 1 - y)*glyphCount]; }
int GetGlyphPixel(ivec2 pos, int g)
{
    if (pos.x >= glyphSize || pos.y >= glyphSize)
        return 0;

    // pull glyph out of hex
    int glyphRow = GetGlyphPixelRow(pos.y, g);
    return 1 & (glyphRow >> (glyphSize - 1 - pos.x) * 4);
}

ivec2 focusList[max(powTableCount, recursionCount) + 2];
ivec2 GetFocusPos(int i) { return focusList[i+2]; }

ivec2 CalculateFocusPos(int iterations)
{
    // count valid pixels in glyph
    int g = GetFocusGlyph(iterations-1);
    int c = 18;    // OPT - 1 and 0 glyps both have 18 pixels
    /*int c = 0;
    for (int y = glyphCount*(glyphSize - 1); y >= 0; y -= glyphCount)
    {
        int glyphRow = glyphs[g + y];
        for (int x = 0; x < glyphSize; ++x)
            c += (1 & (glyphRow >> 4*x));
    }*/

    // find a random valid pixel in glyph
    c -= RandInt(iterations) % c;
    for (int y = glyphCount*(glyphSize - 1); y >= 0; y -= glyphCount)
    {
        int glyphRow = glyphs[g + y];
        for (int x = 0; x < glyphSize; ++x)
        {
            c -= (1 & (glyphRow >> 4*x));
            if (c == 0)
                return ivec2(glyphSize - 1 - x, glyphSize - 1 - y/glyphCount);
        }
    }
}
  
int GetGlyph(int iterations, ivec2 glyphPos, int glyphLast, ivec2 glyphPosLast, ivec2 focusPos)
{ 
    if (glyphPos == focusPos)
        return GetFocusGlyph(iterations); // inject correct glyph     
            
    int seed = iterations + glyphPos.x * 313 + glyphPos.y * 411 + glyphPosLast.x * 557 + glyphPosLast.y * 121;
    return RandInt(seed) % glyphCount; 
}
      
// get color of pos, where pos is 0-1 point in the glyph
vec3 GetPixelFractal(vec2 pos, int iterations, float timePercent)
{
    int glyphLast = GetFocusGlyph(iterations-1);
    ivec2 glyphPosLast = GetFocusPos(-2);
    ivec2 glyphPos =     GetFocusPos(-1);
    
    bool isFocus = true;
    ivec2 focusPos = glyphPos;
    
    vec3 color = InitPixelColor();
    for (int r = 0; r <= recursionCount + 1; ++r)
    {
        color = CombinePixelColor(color, timePercent, iterations, r, pos, glyphPos, glyphPosLast);
        
        //if (r == 1 && glyphPos == GetFocusPos(r-1))
        //    color.z = 1.0; // debug - show focus
        
        if (r > recursionCount)
            return color;
           
        // update pos
        pos -= vec2(glyphMargin*gsfi);
        pos *= glyphSizeF;

        // get glyph and pos within that glyph
        glyphPosLast = glyphPos;
        glyphPos = ivec2(pos);

        // check pixel
        int glyphValue = GetGlyphPixel(glyphPos, glyphLast);
        if (glyphValue == 0 || pos.x < 0.0 || pos.y < 0.0)
            return color;
        
        // next glyph
        pos -= vec2(floor(pos));
        focusPos = isFocus? GetFocusPos(r) : ivec2(-10);
        glyphLast = GetGlyph(iterations + r, glyphPos, glyphLast, glyphPosLast, focusPos);
        isFocus = isFocus && (glyphPos == focusPos);
    }
}
 
//////////////////////////////////////////////////////////////////////////////////
    
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // use square aspect ratio
    vec2 uv = fragCoord;
    uv = fragCoord / iResolution.y;
    uv -= vec2(0.5*iResolution.x / iResolution.y, 0.5);
    uv = InitUV(uv);
    
    // get time 
    float timePercent = iTime*zoomSpeed;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);
    
    // update zoom, apply pow to make rate constant
    float zoom = pow(e, -glyphSizeLog*timePercent);
    zoom *= zoomScale;
    
    // cache focus positions
    for(int i = 0; i  < powTableCount + 2; ++i)
      focusList[i] = CalculateFocusPos(iterations+i-2);
    
    // get offset
    vec2 offset = vec2(0);
    for (int i = 0; i < powTableCount; ++i)
        offset += ((vec2(GetFocusPos(i)) + vec2(glyphMargin)) * gsfi) * powTable[i];
    
    // apply zoom & offset
    vec2 uvFractal = uv * zoom + offset;
    
    // check pixel recursion depth
    vec3 pixelFractalColor = GetPixelFractal(uvFractal, iterations, timePercent);
    pixelFractalColor = FinishPixel(pixelFractalColor, uv);
    
    // apply final color
    fragColor = vec4(pixelFractalColor, 1.0);
}
```

### Image pass (blur post-process - NOT USED)

```glsl
// Dropped - existing Bloom transform handles glow
const float blurSize = 1.0/512.0;
const float blurIntensity = 0.2;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   vec2 uv = fragCoord.xy/iResolution.xy;
   vec4 sum = vec4(0);
   sum += texture(iChannel0, vec2(uv.x - blurSize, uv.y)) * 0.5;
   sum += texture(iChannel0, vec2(uv.x + blurSize, uv.y)) * 0.5;
   sum += texture(iChannel0, vec2(uv.x, uv.y - blurSize)) * 0.5;
   sum += texture(iChannel0, vec2(uv.x, uv.y + blurSize)) * 0.5;
   sum += texture(iChannel0, vec2(uv.x - blurSize, uv.y - blurSize)) * 0.3;
   sum += texture(iChannel0, vec2(uv.x + blurSize, uv.y - blurSize)) * 0.3;
   sum += texture(iChannel0, vec2(uv.x - blurSize, uv.y + blurSize)) * 0.3;
   sum += texture(iChannel0, vec2(uv.x + blurSize, uv.y + blurSize)) * 0.3;

   fragColor = blurIntensity*sum + texture(iChannel0, uv);
}
```

## Algorithm

### Keep verbatim from reference

- Glyph bitmap definitions (`glyphs[]` array, `glyphSize = 5`, `glyphCount = 2`)
- `RandFloat` / `RandInt` hash functions
- All precached constants: `glyphSizeF`, `glyphSizeLog`, `gsfi`, `powTable[]`
- `GetGlyphPixel()` - bitmap extraction via hex nibble shifting
- `GetGlyphPixelRow()` - row lookup from glyph sheet
- `GetFocusGlyph()` / `CalculateFocusPos()` - focus path computation
- `GetGlyph()` - deterministic random glyph selection with focus injection
- `GetRecursionFade()` - depth fade calculation (replace `recursionCount` / `recursionFadeDepth` with uniforms)
- Core `GetPixelFractal()` recursion loop structure (margin subtraction, scaling, glyph lookup, early exit on empty pixel)
- Zoom calculation: `pow(e, -glyphSizeLog * timePercent) * zoomScale`
- Offset accumulation loop using `powTable[]`
- Focus list caching loop

### Replace

| Original | Replacement | Reason |
|----------|-------------|--------|
| `iTime` | `time` uniform (CPU-accumulated: `zoomPhase += zoomSpeed * deltaTime`) | Speed is always accumulated on CPU |
| `iResolution` | `resolution` uniform | Standard pipeline uniform |
| `fragCoord` | `fragTexCoord * resolution` centered per conventions | Shader coordinate conventions |
| `recursionCount` (const 5) | `recursionDepth` uniform int | Modulatable parameter |
| `recursionFadeDepth` (const 3.0) | `fadeDepth` uniform float | Modulatable parameter |
| `zoomScale` (const 0.1) | `zoomScale` uniform float | Modulatable parameter |
| `CombinePixelColor` random RGB coloring | Gradient LUT + FFT per recursion level (see below) | Standard generator color pipeline |
| `FinishPixel` (green tint + noise) | Remove entirely | Noise dropped; color comes from gradient LUT |
| `InitUV` wave (hardcoded 0.1 amplitude, iTime) | `waveAmplitude` uniform, `wavePhase` uniform (CPU-accumulated) | Modulatable parameters |
| Buffer B blur pass | Not implemented | Existing Bloom transform handles glow |
| `mainImage` signature | Standard `void main()` with `out vec4 fragColor` | Pipeline convention |

### Color and FFT integration

Recursion depth `r` is the shared index `t` for both gradient LUT and FFT:

```
t = float(r) / float(recursionDepth)
color from LUT:  texture(gradientLUT, vec2(t, 0.5)).rgb
freq from FFT:   baseFreq * pow(maxFreq / baseFreq, t)
bin:             freq / (sampleRate * 0.5)
energy:          texture(fftTexture, vec2(bin, 0.5)).r
brightness:      baseBright + pow(clamp(energy * gain, 0.0, 1.0), curve)
```

In the per-recursion loop (replacing `CombinePixelColor`):
1. Compute `t = float(r) / float(recursionDepth)`
2. Sample gradient LUT for this layer's color
3. Sample FFT for this layer's audio energy
4. Compute brightness from energy
5. Multiply LUT color by brightness
6. Scale by recursion fade factor `f` from `GetRecursionFade`
7. Accumulate additively: `color += lutColor * brightness * f`

Shallow layers (r=0, t~0) react to bass frequencies. Deep layers (r=max, t~1) react to treble. Bass drops flare the outer glyphs; hi-hats shimmer the tiny fractal detail.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoomSpeed | float | 0.1-3.0 | 1.0 | Zoom rate in fractal levels per second |
| zoomScale | float | 0.01-1.0 | 0.1 | Base zoom multiplier (closer to zero = more zoomed in) |
| recursionDepth | int | 2-8 | 5 | How many fractal levels to render |
| fadeDepth | float | 1.0-6.0 | 3.0 | How many levels visible simultaneously before fading |
| waveAmplitude | float | 0.0-0.5 | 0.1 | UV wave distortion amount |
| waveFreq | float | 0.5-8.0 | 2.0 | UV wave spatial frequency |
| waveSpeed | float | 0.1-3.0 | 1.0 | UV wave animation rate |
| baseFreq | float | 27.5-440.0 | 55.0 | Low end of FFT frequency range |
| maxFreq | float | 1000-16000 | 14000 | High end of FFT frequency range |
| gain | float | 0.1-10.0 | 2.0 | FFT energy amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum glyph brightness without audio |

## Modulation Candidates

- **zoomSpeed**: Speed up/slow down the infinite descent
- **zoomScale**: Shift between close-up (single glyph fills screen) and wide (many glyphs visible)
- **recursionDepth**: Add/remove fractal detail layers dynamically
- **fadeDepth**: Control how many layers are simultaneously visible
- **waveAmplitude**: Intensify or calm the UV wave distortion
- **waveFreq**: Tighten or spread the wave pattern
- **gain**: Overall audio sensitivity
- **baseBright**: Floor brightness of glyphs without audio input

### Interaction Patterns

- **recursionDepth x fadeDepth** (cascading threshold): `fadeDepth` gates how many fractal layers are visible at once. When `fadeDepth` is low relative to `recursionDepth`, only 1-2 layers show (sparse, flickering transitions). When `fadeDepth` approaches `recursionDepth`, all layers stack up (dense, rich). Modulating `fadeDepth` with a slow LFO while `recursionDepth` stays high creates breathing depth that reveals and hides entire fractal layers with the music.
- **waveAmplitude x zoomScale** (competing forces): Wave distortion at high zoom (zoomed out, many glyphs visible) creates large-scale organic wobble. At low zoom (zoomed in, single glyph), the same amplitude creates tight undulation. The interaction shifts the visual mood between organic flow and structured digital grid depending on which parameter dominates.

## Notes

- The `focusList` global array is written per-fragment-invocation in `main()` and read via `GetFocusPos()` in helper functions. This works in GLSL 330 because global variables are per-invocation in fragment shaders.
- The glyph hex encoding (`0x01110`) uses 4-bit nibbles per column. Each hex digit is a single pixel (0 or 1). The `>> 4*x` shift extracts one column. GLSL 330 supports hex int literals and bitwise ops natively.
- `recursionDepth` uniform replaces the original `const int recursionCount`. GLSL 330 supports dynamic loop bounds, so `for (r <= recursionDepth)` works directly without a hardcoded max + break pattern.
- The `powTable` precomputation depends on `glyphSizeF` which depends on `glyphMargin`. Since `glyphMargin` stays const (0.5), `powTable` can remain a const array.
- Performance scales with `recursionDepth` since the per-pixel loop iterates that many times, each with hash computations and glyph lookups. Default 5 should be comfortable; 8 is the ceiling to keep frame times reasonable.
- CPU accumulates two phase values: `zoomPhase += zoomSpeed * deltaTime` and `wavePhase += waveSpeed * deltaTime`. Both are passed as uniforms.
