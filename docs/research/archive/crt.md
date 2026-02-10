# CRT

Simulates a cathode-ray tube display: the image renders through an RGB phosphor mask where each pixel cell splits into glowing red, green, and blue sub-rectangles separated by dark seams. Two mask modes — shadow mask (staggered rectangular cells) and aperture grille (continuous vertical stripes) — pair with barrel distortion, scanlines, edge vignette, and a scrolling electron-beam pulse to complete the retro monitor look.

## Classification

- **Category**: TRANSFORMS > Retro
- **Pipeline Position**: Transform pass (user-ordered alongside other transforms)
- **Chosen Approach**: Full — this effect consolidates CRT functionality currently split across Glitch sub-modes into one cohesive effect, then adds the phosphor mask and pulsing glow that the codebase lacks entirely

## References

- [GM Shaders Mini: CRT](https://mini.gmshaders.com/p/gm-shaders-mini-crt) — XorDev's breakdown of shadow mask cell math, barrel distortion, vignette, and pulsing glow
- [libretro/glsl-shaders: dotmask.glsl](https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/dotmask.glsl) — Four mask types (shadow mask, aperture grille, stretched VGA, compressed TV) with maskDark/maskLight intensity control
- [libretro/glsl-shaders: crt-lottes.glsl](https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-lottes.glsl) — Timothy Lottes' CRT shader with Gaussian scanlines and switchable mask patterns

## Algorithm

### Phosphor Mask — Shadow Mask Mode

Divide screen pixels into rectangular cells, then subdivide each cell into three vertical R/G/B strips with soft dark borders. Alternate rows stagger by half a cell width.

```glsl
// cell and subcell coordinates
vec2 coord = pixel / maskSize;
vec2 subcoord = coord * vec2(3.0, 1.0);

// stagger every other column's y-offset
vec2 cellOffset = vec2(0.0, fract(floor(coord.x) * 0.5));

// which RGB channel lights up (0=R, 1=G, 2=B)
float ind = mod(floor(subcoord.x), 3.0);
vec3 maskColor = vec3(ind == 0.0, ind == 1.0, ind == 2.0) * 3.0;

// soft borders: squared falloff from subcell center
vec2 cellUV = fract(subcoord + cellOffset) * 2.0 - 1.0;
vec2 border = 1.0 - cellUV * cellUV * maskBorder;
maskColor *= border.x * border.y;

// blend mask onto image
color.rgb *= 1.0 + (maskColor - 1.0) * maskIntensity;
```

The `* 3.0` on maskColor compensates for only one channel being lit per subcell, keeping perceived brightness consistent.

### Phosphor Mask — Aperture Grille Mode

Continuous vertical R/G/B stripes with no row staggering. Simpler math — no `cellOffset`, border only in x-direction.

```glsl
float stripe = fract(pixel.x / maskSize * 0.333333);
float ind = floor(stripe * 3.0);
vec3 maskColor = vec3(ind == 0.0, ind == 1.0, ind == 2.0) * 3.0;

// vertical gap between stripes
float gap = 1.0 - pow(abs(fract(stripe * 3.0) * 2.0 - 1.0), 2.0) * maskBorder;
maskColor *= gap;

color.rgb *= 1.0 + (maskColor - 1.0) * maskIntensity;
```

### Scanlines

Darken horizontal lines at scanline frequency, with intensity modulated by image brightness (bright pixels resist scanlines more than dark ones).

```glsl
float scanline = sin(pixel.y * PI / scanlineSpacing) * 0.5 + 0.5;
scanline = pow(scanline, scanlineSharpness);
float bright = dot(color.rgb, vec3(0.299, 0.587, 0.114));
color.rgb *= mix(1.0 - scanlineIntensity, 1.0, scanline * mix(1.0, bright, scanlineBrightBoost));
```

### Barrel Distortion (Screen Curvature)

Maps flat UV coordinates onto a curved CRT surface. The XorDev approach scales UV by squared radial distance.

```glsl
vec2 centered = uv * 2.0 - 1.0;
centered *= 1.0 + (dot(centered, centered) - 1.0) * curvatureAmount;
uv = centered * 0.5 + 0.5;
```

Pixels outside [0,1] after distortion clamp to black (CRT bezel).

### Vignette

Edge darkening using separable x/y distance from center.

```glsl
vec2 edge = max(1.0 - centered * centered, 0.0);
float vignette = pow(edge.x * edge.y, vignetteExponent);
color.rgb *= vignette;
```

### Pulsing Glow

Horizontal brightness wave scrolling across the screen, simulating electron beam refresh banding.

```glsl
color.rgb *= 1.0 + pulseIntensity * cos(pixel.x / pulseWidth + time * pulseSpeed);
```

### Pass Order

Apply in this sequence within the shader:
1. Barrel distortion (warp UVs)
2. Sample texture at warped UV (quantized to mask cell center for shadow mask)
3. Scanlines
4. Phosphor mask
5. Pulsing glow
6. Vignette
7. Bezel clamp (black outside barrel bounds)

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| maskMode | int | 0–1 | 0 | 0 = shadow mask (staggered cells), 1 = aperture grille (vertical stripes) |
| maskSize | float | 2.0–24.0 | 8.0 | Pixel size of each RGB cell group |
| maskIntensity | float | 0.0–1.0 | 0.7 | Blend strength of phosphor pattern (0 = off, 1 = full) |
| maskBorder | float | 0.0–1.0 | 0.8 | Dark gap width between subcells |
| scanlineIntensity | float | 0.0–1.0 | 0.3 | Darkness of horizontal scan lines |
| scanlineSpacing | float | 1.0–8.0 | 2.0 | Pixels between scan lines |
| scanlineSharpness | float | 0.5–4.0 | 1.5 | Transition sharpness (higher = thinner lines) |
| scanlineBrightBoost | float | 0.0–1.0 | 0.5 | How much bright pixels resist scanline darkening |
| curvatureEnabled | bool | — | true | Enable barrel distortion |
| curvatureAmount | float | 0.0–0.15 | 0.06 | Barrel distortion strength |
| vignetteEnabled | bool | — | true | Enable edge darkening |
| vignetteExponent | float | 0.1–1.0 | 0.4 | Vignette curve (lower = sharper falloff) |
| pulseEnabled | bool | — | false | Enable horizontal brightness wave |
| pulseIntensity | float | 0.0–0.1 | 0.03 | Amplitude of brightness ripple |
| pulseWidth | float | 20.0–200.0 | 60.0 | Wavelength in pixels |
| pulseSpeed | float | 1.0–40.0 | 20.0 | Scroll speed |

## Migration from Glitch

The existing Glitch effect contains three CRT-related features to extract:

**Move to CRT effect:**
- `crtEnabled` + `curvature` (barrel distortion) → `curvatureEnabled` + `curvatureAmount`
- `vignetteEnabled` (CRT vignette) → `vignetteEnabled` + `vignetteExponent`
- Scanline overlay from Glitch's overlay mode → `scanlineIntensity` + `scanlineSpacing`

**Glitch keeps:** analog, digital, VHS, datamosh, row/column slice, diagonal bands, block mask, temporal jitter, block multiply

**Preset migration:** Existing presets with `glitch.crtEnabled = true` need a compatibility path — either auto-enable the new CRT effect on load, or document as a breaking change.

## Modulation Candidates

- **maskSize**: Zoom in/out on phosphor grid for breathing texture
- **maskIntensity**: Fade mask on/off for transitions
- **scanlineIntensity**: Pulse scan lines with rhythm
- **curvatureAmount**: Subtle screen flex
- **pulseIntensity**: Amplify electron beam ripple
- **pulseSpeed**: Sync refresh banding to tempo

## Notes

- Shadow mask mode quantizes texture sampling to cell centers (one sample per cell), reducing effective resolution. Aperture grille samples per-pixel with only vertical striping, preserving more detail.
- At small `maskSize` values (< 4px), the phosphor pattern aliases against display pixels. Consider clamping minimum based on display DPI, or document the artifact as intentional at extreme settings.
- The `* 3.0` brightness compensation assumes uniform RGB content. Saturated colors (one dominant channel) appear brighter through the mask than neutral grays — this matches real CRT behavior.
- Barrel distortion runs first so the phosphor mask aligns to screen pixels, not warped pixels. Reversing the order produces visible mask distortion near edges.
- Extracting CRT from Glitch reduces Glitch's shader by ~25 lines and 3 uniforms, simplifying both effects.
