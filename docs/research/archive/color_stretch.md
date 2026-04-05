# Color Stretch

Endlessly zooming fractal texture that recursively subdivides space into a grid, accumulating color at each depth level. The viewer sees a self-similar tunnel of shifting colored tiles -- each tile contains a smaller copy of the whole pattern, zooming smoothly inward. Grid size and focus offset control the subdivision geometry, curvature warps the zoom into a dome, and spin rotates the whole tunnel.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage, after trail boost, blended onto scene via blend compositor
- **Section Index**: 12 (Texture)

## Attribution

- **Based on**: "Color Stretch" by KilledByAPixel (Frank Force)
- **Source**: https://www.shadertoy.com/view/4lXcD7
- **License**: CC BY-NC-SA 3.0

## References

- [Color Stretch](https://www.shadertoy.com/view/4lXcD7) - Complete reference implementation

## Reference Code

```glsl
//////////////////////////////////////////////////////////////////////////////////
// Color Stretch - Copyright 2019 Frank Force
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//////////////////////////////////////////////////////////////////////////////////

const float zoomSpeed            = .5;    // how fast to zoom (negative to zoom out)
const float zoomScale            = 0.1;    // how much to multiply overall zoom (closer to zero zooms in)
const int recursionCount        = 6;    // how deep to recurse
const int glyphSize                = 3;    // width & height of glyph in pixels
const float curvature            = 0.0;    // time warp to add curvature

//////////////////////////////////////////////////////////////////////////////////
// Precached values and math

const float glyphSizeF = float(glyphSize);
const float glyphSizeLog = log(glyphSizeF);
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
    float rt = max(float(r) - timePercent, 0.0);
    float rc = float(recursionCount);
    return rt / rc;
}

vec3 InitPixelColor() { return vec3(0, .5, .5); }
vec3 CombinePixelColor(vec3 color, float timePercent, int i, int r, vec2 pos, ivec2 glyphPos, ivec2 glyphPosLast)
{
    i = (i+r) + (glyphPosLast.y + glyphPos.y);
    //i+=glyphPosLast.x + glyphPos.x;

    vec3 myColor = vec3
    (
        mix(-0.2, 0.2, RandFloat(i)),
        mix(-0.2, 0.2, RandFloat(i + 10)),
        mix(-0.2, 0.2, RandFloat(i + 20))
    );

    // combine with my color
    float f = GetRecursionFade(r, timePercent);
    color += myColor*f;
    return color;
}

vec3 FinishPixel(vec3 color, vec2 uv)
{
    // color wander
    color.x += 0.03*iTime;

    // convert to rgb
    color = HsvToRgb(color);
    return color;
}

vec2 InitUV(vec2 uv)
{
   //v.y += 1.0;
    // wave
    //uv.x += 0.04*sin(10.0*uv.y + 0.17*iTime);
    //uv.y += 0.04*sin(10.0*uv.x + 0.13*iTime);
    //uv.x += 0.2*sin(2.0*uv.y + 0.31*iTime);
    //uv.y += 0.2*sin(2.0*uv.x + 0.27*iTime);

    // spin
    //float theta = 0.0;//pi/2.0;//0.05*iTime;
    //float c = cos(theta);
    //float s = sin(theta);
    //uv = vec2((uv.x*c - uv.y*s), (uv.x*s + uv.y*c));

    return uv;
}

//////////////////////////////////////////////////////////////////////////////////
// Fractal functions

ivec2 GetFocusPos(int i) { return ivec2(glyphSize/2); }

// get color of pos, where pos is 0-1 point in the glyph
vec3 GetPixelFractal(vec2 pos, int iterations, float timePercent)
{
    ivec2 glyphPosLast = GetFocusPos(-2);
    ivec2 glyphPos =     GetFocusPos(-1);
    vec3 color = InitPixelColor();

    for (int r = 0; r <= recursionCount + 1; ++r)
    {
        color = CombinePixelColor(color, timePercent, iterations, r, pos, glyphPos, glyphPosLast);
        if (r > recursionCount)
            return color;

        // update pos
        pos *= glyphSizeF;

        // get glyph and pos within that glyph
        glyphPosLast = glyphPos;
        glyphPos = ivec2(pos);

        // next glyph
        pos -= vec2(floor(pos));
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

    // time warp
    float time = iTime + curvature*pow(length(uv), 0.2);

    // time warp to add add some 3d curve
    //float c1 = 0.5*sin(0.3*iTime);
    //float c2 = 0.5*sin(0.23*iTime);
    //time += curvature*(c1*length(uv) + c2*uv.x*uv.y)*zoomSpeed;

    // get time
    float timePercent = time*zoomSpeed;
    int iterations = int(floor(timePercent));
    timePercent -= float(iterations);

    // update zoom, apply pow to make rate constant
    float zoom = pow(e, -glyphSizeLog*timePercent);
    zoom *= zoomScale;

    // get offset
    vec2 offset = vec2(0);
    const float gsfi = 1.0 / glyphSizeF;
    for (int i = 0; i < 13; ++i)
        offset += (vec2(GetFocusPos(i)) * gsfi) * pow(gsfi,float(i));

    // apply zoom & offset
    vec2 uvFractal = uv * zoom + offset;

    // check pixel recursion depth
    vec3 pixelFractalColor = GetPixelFractal(uvFractal, iterations, timePercent);
    //pixelFractalColor.y = pow(pixelFractalColor.y, 2.);
    //pixelFractalColor.z = pow(pixelFractalColor.z, 2.);
    pixelFractalColor = FinishPixel(pixelFractalColor, uv);

    // apply final color
    fragColor = vec4(pixelFractalColor, 1.0);
}
```

## Algorithm

### What to keep verbatim
- `RandFloat()` hash function
- `GetRecursionFade()` logic (fade weight per recursion level)
- `GetPixelFractal()` core loop: multiply pos by glyphSize, extract integer cell, subtract floor
- Zoom math: `pow(e, -glyphSizeLog * timePercent) * zoomScale`
- Offset convergence loop: `sum of (focusPos / glyphSize) * pow(1/glyphSize, i)`

### What to replace

| Original | Replacement | Reason |
|----------|-------------|--------|
| `iTime` | `time` uniform (CPU-accumulated phase) | Speed is accumulated on CPU per project convention |
| `iResolution` | `resolution` uniform (vec2) | Standard uniform name |
| `const int glyphSize = 3` | `uniform int glyphSize` | Exposed as config param (2-8) |
| `const int recursionCount = 6` | `uniform int recursionCount` | Exposed as config param (2-12) |
| `const float zoomScale = 0.1` | `uniform float zoomScale` | Exposed as config param |
| `const float curvature = 0.0` | `uniform float curvature` | Exposed as config param |
| `GetFocusPos()` returns `ivec2(glyphSize/2)` | `uniform vec2 focusOffset` added to `ivec2(glyphSize/2)` | Focus offset param shifts zoom target cell |
| `HsvToRgb()` + HSV color accumulation | `texture(gradientLUT, vec2(t, 0.5)).rgb` | Standard generator color via gradient LUT |
| `InitPixelColor()` / `CombinePixelColor()` HSV path | Map recursion depth `r / recursionCount` to `t`, sample gradient LUT | Replaces HSV color with LUT-driven palette |
| `FinishPixel()` hue wander + HSV-to-RGB | Remove entirely | No longer needed with gradient LUT |
| `InitUV()` (no-op in original) | Apply spin: `theta = spinPhase` uniform, rotate UV by 2x2 matrix | Spin phase accumulated on CPU |
| No FFT audio | Add FFT frequency lookup per recursion depth | `t = r / recursionCount` indexes both LUT and FFT |
| No glow | Add standard glow pattern | Brightness from FFT modulates glow intensity |

### Color mapping via recursion depth
The normalized recursion depth `t = float(r) / float(recursionCount)` serves as the shared index for both gradient LUT color and FFT frequency lookup. Deeper recursion levels map to higher frequencies. The hash-based color offset from the original is removed entirely -- the gradient LUT provides all coloring.

### Focus offset mechanics
The original `GetFocusPos()` always returns `ivec2(glyphSize/2)` (center cell). With `focusOffset`:
- Compute `ivec2 focus = ivec2(glyphSize/2) + ivec2(round(focusOffset))`
- Clamp to `[0, glyphSize-1]` to stay within the grid
- Use this focus for both the offset convergence loop and as the zoom target
- Non-center focus creates asymmetric zoom trajectories

### Spin
CPU accumulates `spinPhase += spinSpeed * deltaTime`. Shader applies 2x2 rotation to centered UV before zoom math.

### Curvature
Kept from original: `time += curvature * pow(length(uv), 0.2)`. Warps effective time by distance from center, creating a dome/tunnel illusion where center zooms faster than edges.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoomSpeed | float | -2.0 - 2.0 | 0.5 | Zoom rate (negative reverses) |
| zoomScale | float | 0.01 - 1.0 | 0.1 | Overall zoom level (smaller = more zoomed in) |
| glyphSize | int | 2 - 8 | 3 | Grid subdivision size per recursion level |
| recursionCount | int | 2 - 12 | 6 | Fractal recursion depth (more = finer detail) |
| curvature | float | 0.0 - 2.0 | 0.0 | Dome/tunnel time warp strength |
| spinSpeed | float | -ROTATION_SPEED_MAX - ROTATION_SPEED_MAX | 0.0 | UV rotation rate (radians/sec, accumulated on CPU) |
| focusOffsetX | float | -1.0 - 1.0 | 0.0 | Horizontal zoom target shift from center cell |
| focusOffsetY | float | -1.0 - 1.0 | 0.0 | Vertical zoom target shift from center cell |
| baseFreq | float | 27.5 - 440.0 | 55.0 | FFT low frequency bound (Hz) |
| maxFreq | float | 1000.0 - 16000.0 | 14000.0 | FFT high frequency bound (Hz) |
| gain | float | 0.1 - 10.0 | 2.0 | FFT energy multiplier |
| curve | float | 0.1 - 3.0 | 1.5 | FFT energy contrast curve |
| baseBright | float | 0.0 - 1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **zoomSpeed**: Faster/slower/reverse zoom creates dramatic tempo-locked tunnel pulsing
- **zoomScale**: Shifts the apparent depth, like pulling the camera in and out
- **glyphSize**: Changing subdivision on the fly reshuffles the entire pattern geometry
- **recursionCount**: More/fewer detail levels, like adjusting fractal resolution
- **curvature**: Dome intensity breathing creates a fish-eye pump effect
- **spinSpeed**: Rotation rate modulation creates swirling/stalling tunnel motion
- **focusOffsetX/Y**: Shifting zoom target makes the tunnel swerve laterally
- **gain**: Audio energy intensity
- **baseBright**: Floor brightness shifts overall mood

### Interaction Patterns

- **glyphSize x focusOffset (competing forces)**: Grid size sets the subdivision geometry while focus offset picks the zoom trajectory within that geometry. Small grid (2) with max offset = tight zigzag; large grid (8) with small offset = subtle asymmetry in a fine mesh. Modulating both from different audio bands makes the tunnel structure and trajectory shift independently with the music.
- **zoomSpeed x curvature (cascading threshold)**: At low curvature the zoom is uniform across the frame. As curvature rises, center-vs-edge zoom differential increases. Fast zoom + high curvature = center races deep while edges barely move, creating a vortex. Slow zoom + high curvature = gentle dome lens. Curvature gates the spatial drama that zoom speed provides.
- **spinSpeed x focusOffset (resonance)**: Spin rotates the UV plane, focus offset biases the zoom direction. When both peak together the trajectory becomes a tight helix -- rare bright geometric moments. When only one is active the motion is either pure rotation or pure lateral drift.

## Notes

- `recursionCount` is the main performance knob. Values above 10 add many loop iterations per pixel. Default 6 is safe.
- `glyphSize` affects the offset convergence loop (hardcoded at 13 iterations in original). With variable glyphSize, the convergence sum needs enough terms -- 13 is sufficient for glyphSize up to 8.
- The `RandFloat()` hash is integer-seeded. Changing `glyphSize` or `focusOffset` completely reshuffles the color/pattern hash, so even small param changes produce visually distinct results.
- `zoomScale` at very small values (< 0.02) can reveal numerical precision artifacts in the recursion.
