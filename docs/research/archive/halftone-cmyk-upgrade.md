# Halftone CMYK Upgrade

Upgrade the existing halftone from a single luminance-based dot grid to a proper four-color CMYK halftone. Each ink channel (Cyan, Magenta, Yellow, Key/Black) gets its own dot grid at a different screen angle, composited subtractively on white paper. The result looks like magnified newsprint — overlapping colored dots that mix to reproduce the full image.

## Classification

- **Category**: TRANSFORMS > Print (existing slot, in-place upgrade)
- **Pipeline Position**: Same as current halftone (section 5)

## References

- User-provided reference code (standard CMYK halftone technique, no attribution required)

## Reference Code

```glsl
#define M_PI 3.1415926535897932384626433832795

float vx(float x,float y, float a)
{
    float l=1.0/sqrt(a*a+(1.0-a)*(1.0-a));
    float u=x*a-y*(1.0-a);
    float v=x*(1.0-a)+y*a;
    u*=l;
    v*=l;
//    return (sin(u)+sin(v)+2.0)/2.0;
    float scale=0.0002*iResolution.x;
    u=fract(u*scale)-0.5;
    v=fract(v*scale)-0.5;
    return 1.7-sqrt(u*u+v*v)*4.;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy-iMouse.xy) / max(iResolution.x,iResolution.y)*4.0-0.5;
    float t=iTime;
    float r=length(uv);
    float a=atan(uv.x,uv.y)/M_PI/2.0+sin(r+t)*0.1;

    fragColor = vec4(
        vx(uv.x, uv.y,iMouse.x/iResolution.x*2.0-0.5),0,0,1
        );
//    return;

    fragColor = vec4(
         ((r-sin(r*M_PI*2.0+t*4.0)*0.1-sin(a*M_PI*22.0+t*4.0)*0.1+sin(a*M_PI*12.0+t)*0.4-1.5)+vx(uv.x*150.0,uv.y*150.0,0.12))>.5?1.:0.
        ,((r-sin(r*M_PI*3.0+t*5.0)*0.1-sin(a*M_PI*26.0+t*5.0)*0.1-sin(a*M_PI*8.0-t )*0.4-1.5)+vx(uv.x*150.0,uv.y*150.0,0.34))>.5?1.:0.
        ,((r-sin(r*M_PI*2.0+t*8.0)*0.1-sin(a*M_PI*24.0+t*6.0)*0.1-sin(a*M_PI*10.0  )*0.4-1.5)+vx(uv.x*150.0,uv.y*150.0,0.69))>.5?1.:0.
        ,1.0);
}
```

## Algorithm

**What to keep from reference**: The `vx()` dot grid function's core pattern — fract-based cell tiling with distance-based dot falloff, and the `inkDensity + dotPattern > threshold` compositing logic.

**What to replace**:
- Replace the `a` parametric rotation in `vx()` with proper angle-based rotation (`cos`/`sin` matrix), matching the existing halftone's `rotm()` function
- Replace `iResolution` / `iMouse` / `iTime` with our uniform conventions (`resolution`, `dotScale`, `rotation`)
- Replace the demo radial/angular pattern with actual input texture sampling + RGB→CMYK conversion
- Replace per-channel RGB output with subtractive CMYK compositing

**Adapted algorithm**:

1. **Dot grid function** `cmykDot(vec2 pixelCoord, float angle, float cellSize)`:
   - Rotate `pixelCoord` by `angle`
   - Divide by `cellSize`, take `fract() - 0.5` to get cell-local coords
   - Return `1.7 - length(cellLocal) * 4.0` (matches reference falloff curve)

2. **Per-cell sampling**: For each pixel, compute the grid cell center (snap to grid, unrotate back to texture space) and sample the input texture there — same approach as current halftone.
   - Since we have four grids at different angles, sample once at the K-channel grid center (45° is the dominant visual channel) and use that RGB for all four channel conversions. Sampling four times would be expensive and create visible misalignment artifacts.

3. **RGB → CMYK conversion**:
   ```
   K = 1 - max(R, G, B)
   invK = 1 - K
   C = invK > 0.001 ? (invK - R) / invK : 0.0
   M = invK > 0.001 ? (invK - G) / invK : 0.0
   Y = invK > 0.001 ? (invK - B) / invK : 0.0
   ```

4. **Ink presence test** (per channel): `step(0.5, channelDensity * dotSize + cmykDot(fc, rotation + channelAngle, dotScale))`
   - `dotSize` scales ink density, controlling dot coverage
   - When ink density = 0: only pixels at very center of dots show ink (tiny dots)
   - When ink density = 1: most of the cell fills with ink (large dots)

5. **Subtractive compositing**:
   ```
   R = (1 - cInk) * (1 - kInk)   // Cyan absorbs red
   G = (1 - mInk) * (1 - kInk)   // Magenta absorbs green
   B = (1 - yInk) * (1 - kInk)   // Yellow absorbs blue
   ```

6. **Screen angles** (constants in shader):
   - Cyan: 15° (0.2618 rad)
   - Magenta: 75° (1.3090 rad)
   - Yellow: 0°
   - Black: 45° (0.7854 rad)
   - Global `rotation` uniform added on top of each

**Parameter mapping**:
- Reference `scale = 0.0002 * iResolution.x` → our `dotScale` uniform (grid cell size in pixels, inverted: `1.0 / dotScale`)
- Reference rotation parameter `a` → proper angle constants + `rotation` uniform
- New: `dotSize` repurposed as ink density multiplier (affects dot coverage)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| dotScale | float | 2.0-20.0 | 8.0 | Grid cell size in pixels — smaller = finer print |
| dotSize | float | 0.5-2.0 | 1.0 | Ink density multiplier — higher = larger dots, more coverage |
| rotationSpeed | float | -PI..PI | 0.0 | Rotation rate of entire grid ensemble (rad/s) |
| rotationAngle | float | -PI..PI | 0.0 | Static rotation offset applied to all channels |

No new parameters — the existing four params map cleanly to the CMYK version.

## Modulation Candidates

- **dotScale**: Modulating grid density creates a zoom-in/zoom-out newsprint effect. Low values reveal individual dots, high values merge into near-solid ink.
- **dotSize**: Modulating ink coverage creates a breathing/pulsing effect — dots grow and shrink, revealing or flooding the paper.
- **rotationSpeed**: Modulating spin rate creates wobble or jitter in the print grid.

### Interaction Patterns

- **dotScale vs dotSize (cascading threshold)**: At large `dotScale` (coarse grid), dots are widely spaced and `dotSize` must be high for ink to visually connect. At small `dotScale` (fine grid), even modest `dotSize` produces near-solid coverage. Modulating `dotSize` with bass creates dramatic ink floods at coarse scales but subtle density shifts at fine scales — the grid resolution gates how much visual impact the coverage modulation has.

## Notes

- **Division by zero guard**: RGB→CMYK conversion divides by `(1-K)`. When K=1 (pure black pixel), CMY channels must be clamped to 0 to avoid NaN.
- **Single sample point**: All four channel densities are computed from one texture sample (at the K-grid cell center) rather than four separate samples. This avoids per-channel spatial offset artifacts and saves three texture lookups per pixel.
- **Existing behavior preservation**: Setting `dotSize` to 1.0 and viewing a grayscale input effectively produces single-channel halftone similar to the old behavior (K channel dominates, CMY near zero).
