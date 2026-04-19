# Drekker Paint

Geometric paint-stroke mosaic that slices the image into a diagonal grid of curved parallelogram cells. Each cell samples one color region from the source and sweeps it into a brushstroke shape using reciprocal curvature functions. The rigid grid structure combined with organic cell warping evokes Max Drekker's optical art style -- bold color blocks arranged with mathematical precision.

## Classification

- **Category**: TRANSFORMS > Painterly
- **Pipeline Position**: After Painterly group (post Oil Paint, Impressionist, Kuwahara)

## Attribution

- **Based on**: "Max Drekker Paint effect v2" by Cotterzz (fork of "Max Drekker Paint effect")
- **Source**: https://www.shadertoy.com/view/33SSDm (original: https://www.shadertoy.com/view/WX2SWz)
- **License**: CC BY-NC-SA 3.0
- **Visual reference**: Max Drekker (UK illustrator/motion designer) -- geometric optical art with bold color fields

## References

- [Max Drekker Paint effect v2](https://www.shadertoy.com/view/33SSDm) - Complete shader with inline comments explaining the algorithm
- [Max Drekker](https://maxdrekker.com/x) - Artist whose style the effect emulates

## Reference Code

```glsl
// "Max Drekker Paint effect v2" by Cotterzz
// https://www.shadertoy.com/view/33SSDm
// Fork of "Max Drekker Paint effect" by Cotterzz
// https://shadertoy.com/view/WX2SWz
// License: CC BY-NC-SA 3.0

void mainImage0( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.y;
    float xdiv = 11.; // number of horizontal cells
    float ydiv = 7.; // number of vertical cells
    if(iMouse.z>0.){xdiv = 3.+iMouse.x/30.;ydiv = 2.+iMouse.y/30.;}
    // curve - value for curvature to be used as reciprocal for values that get very high close to the edge, see below.
    float curve = 0.01;
    // rx - this is the horizontal cell number x divided by xdiv. 0-11 or whatever the count is.
    float rx = floor(uv.x * xdiv);
    // rix - this is how far along the horizontal cell we are: 0-1 continuous.
    float rix = fract(uv.x * xdiv);
    // rixl - this is inverse of how close we are to the left of the horizontal cell, using a reciprocal so the number tends to infinity as we get to it.
    // (using the reciprocal of the curve variable)
    float rixl = (curve/rix)-curve;
    // rixr - this is the same only for the right hand side.
    float rixr = (curve/(1.0-rix))-curve;
    // so now we have numbers that start at zero on one side and get very big as we get to the other edge
    // I would have used a single variable for this, but we want to curve in opposite directions, so separate is best.
    
    // ry - this is the vertical equivalent of rx, only we want the vertical space to veer upwards on the right, and downwards on the left
    float ry = floor((uv.y+rx/10./* diagonal pattern */) *ydiv+rixl-rixr); // ydiv for number of cells, plus rixl and minus rixr for curve
    
    // and this is the equivalent of rix, only again we use rixl and rixr to add curve
    float riy = fract((uv.y+rx/10.) *ydiv+rixl-rixr)-0.5;
    // NOTE here - rix is 0-1, but riy is 0-1-0, with 1 in the middle, and 0 at top and bottom
    // so riy now gives us 0-1-0 progression up the cell, so we can do this:
    riy=riy*riy*riy;
    // cubing riy like this gives us another curve, from top to bottom, so it's kinda doing a similar job to rixl/rixr
    
    // so now when we take the texture sample we can take pixels evenly spaced across with rx/xdiv
    // and up with ry/ydiv,  rx/10 is to match the diagonal pattern, and riy/iResolution.y*100 samples pixels up and down from the main pixel sampled for this cell
    fragColor = textureLod(iChannel0,vec2(rx/xdiv,(riy/iResolution.y*200.)+(ry/ydiv)-rx/10.),0.0);
    // (Thanks to IQ for this, fixes the awful effect you get from mipmap)
    
    if(abs(riy)>0.1||rix>0.9||rix<0.1){fragColor.a = 0.;}
}

void mainImage(out vec4 o, vec2 u)
{
    float s = 4., k; // s is the number of samples
    vec2 j = vec2(.5);
    o = vec4(0);
    vec4 c;
    mainImage0(c, u);
    for (k = s; k-- > .5; ) {
        mainImage0(c, u + j - .5);
        o += c;
        j = fract(j + vec2(.754877669, .569840296).yx);
    };o /= s;o.a==1.;
}
```

## Algorithm

Transcribe the reference code with these substitutions:

### Keep verbatim
- All cell math: `floor(uv.x * xdiv)`, `fract(...)`, reciprocal curvature `(curve/rix)-curve`
- Diagonal offset: `rx/10.`
- Vertical curvature: `riy = riy*riy*riy`
- Alpha masking: `abs(riy)>0.1||rix>0.9||rix<0.1`
- `textureLod` with LOD 0 to bypass mipmap artifacts
- Multi-sample AA loop with quasi-random offsets

### Replace
- `iResolution.y` -> `resolution.y` uniform
- `iResolution.x` -> `resolution.x` uniform (if needed for aspect)
- `iChannel0` -> `texture0` (raylib input texture)
- `iMouse` block -> remove entirely (params come from uniforms)
- Hardcoded `xdiv = 11.`, `ydiv = 7.` -> uniforms `xDiv`, `yDiv`
- Hardcoded `curve = 0.01` -> uniform `curve`
- Hardcoded `0.1` in alpha threshold -> uniform `gapSize`
- Hardcoded `rx/10.` diagonal factor -> uniform `diagSlant`
- Hardcoded `200.` in riy sample spread -> uniform `strokeSpread`
- Hardcoded `s = 4.` sample count -> keep hardcoded (4 is cheap, always looks better than 1)
- Alpha discard -> `discard` or mix with input based on pipeline convention

### Coordinate centering
The reference uses `fragCoord/iResolution.y` which places (0,0) at bottom-left. This is acceptable here because the effect is a grid subdivision -- all spatial operations (floor, fract) are translation-invariant. No rotation, noise, or distance fields are involved, so centering is not required.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| xDiv | float | 2.0-30.0 | 11.0 | Number of horizontal cells |
| yDiv | float | 2.0-20.0 | 7.0 | Number of vertical cells |
| curve | float | 0.001-0.1 | 0.01 | Reciprocal curvature intensity at cell edges |
| gapSize | float | 0.01-0.4 | 0.1 | Alpha threshold controlling visible gap between strokes |
| diagSlant | float | 0.0-0.3 | 0.1 | Diagonal slant factor (rx/diagSlant pattern) |
| strokeSpread | float | 50.0-500.0 | 200.0 | How far each stroke samples vertically from its center |

## Modulation Candidates

- **xDiv / yDiv**: cell density -- low values give large bold blocks, high values give fine mosaic
- **curve**: stroke curvature intensity -- low is flat/rigid, high is dramatically swept
- **gapSize**: stroke isolation -- small gaps merge into solid coverage, large gaps reveal dark grid lines between strokes
- **diagSlant**: angle of the diagonal pattern -- 0 is a straight grid, higher values tilt into parallelograms

### Interaction Patterns

- **curve x gapSize (competing forces)**: curve pushes cell content outward toward edges while gapSize masks those edges. High curve + low gap = strokes visibly bulge at cell boundaries. High curve + high gap = content compressed into thin center stripes. The balance determines whether strokes feel fat/overlapping or skeletal.
- **xDiv x yDiv x diagSlant (resonance)**: when xDiv/yDiv ratio aligns with diagSlant, cells tile into clean diagonal bands. Misaligned ratios create irregular stagger patterns. Modulating one axis against a static other creates drifting alignment/misalignment cycles.

## Notes

- The multi-sample AA loop uses quasi-random R2 sequence offsets (`.754877669, .569840296`). These are the plastic constant conjugates -- keep them verbatim.
- `textureLod(..., 0.0)` is critical. Regular `texture()` with mipmap causes visible seams at cell boundaries where UV jumps create incorrect LOD selection.
- The reference has a bug: `o.a==1.;` at the end is a comparison (no-op), not assignment. The intent was likely `o.a = 1.;` to force full opacity after averaging. Decide at implementation time whether to force alpha or preserve the averaged alpha for blend compatibility.
- `strokeSpread` (the `200.` constant) is resolution-dependent in the reference. May want to normalize by resolution to keep stroke appearance consistent across window sizes.
