# Risograph

Bold flat ink layers with grainy, speckled coverage printed slightly out of register onto warm paper. Three ink colors (hot pink, teal, golden yellow) decompose the image into stencil-like channels that overlap to create subtractive mix colors in their intersection zones. The imperfection IS the aesthetic — grain erodes ink coverage, misregistration shifts layers apart, and paper texture bleeds through wherever ink thins out.

## Classification

- **Category**: TRANSFORMS > Graphic
- **Pipeline Position**: Transform chain (user-ordered, alongside Toon/Halftone/Kuwahara)

## References

- [Maxime Heckel — Shades of Halftone](https://blog.maximeheckel.com/posts/shades-of-halftone/) - RGBtoCMYK function (Matt DesLauriers), subtractive compositing pattern
- [Stefan Gustavson — CMYK Halftone Shader](https://github.com/genekogan/Processing-Shader-Examples/blob/master/TextureShaders/data/halftone_cmyk.glsl) - CMY decomposition, simplex noise for texture, full public domain shader

## Reference Code

### RGB to CMYK conversion (Matt DesLauriers via Maxime Heckel)

```glsl
vec4 RGBtoCMYK (vec3 rgb) {
  float r = rgb.r;
  float g = rgb.g;
  float b = rgb.b;
  float k = min(1.0 - r, min(1.0 - g, 1.0 - b));
  vec3 cmy = vec3(0.0);

  float invK = 1.0 - k;

  if (invK != 0.0) {
    cmy.x = (1.0 - r - k) / invK;
    cmy.y = (1.0 - g - k) / invK;
    cmy.z = (1.0 - b - k) / invK;
  }

  return clamp(vec4(cmy, k), 0.0, 1.0);
}
```

### Subtractive compositing pattern (Maxime Heckel)

```glsl
vec3 outColor = vec3(1.0);
outColor.r *= (1.0 - CYAN_STRENGTH * dotC);
outColor.g *= (1.0 - MAGENTA_STRENGTH * dotM);
outColor.b *= (1.0 - YELLOW_STRENGTH * dotY);
outColor *= (1.0 - BLACK_STRENGTH * dotK);
```

### CMY decomposition + simplex noise (Stefan Gustavson, public domain)

```glsl
vec3 texcolor = texture2D(texture, st).rgb;
float n = 0.1*snoise(st*200.0);
n += 0.05*snoise(st*400.0);
n += 0.025*snoise(st*800.0);

vec4 cmyk;
cmyk.xyz = 1.0 - texcolor;
cmyk.w = min(cmyk.x, min(cmyk.y, cmyk.z));
cmyk.xyz -= cmyk.w;
```

### Simplex 2D noise (Stefan Gustavson, public domain)

```glsl
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289((( x * 34.0) + 1.0) * x); }

float snoise(vec2 v) {
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
    -0.577350269189626, 0.024390243902439);
  vec2 i = floor(v + dot(v, C.yy));
  vec2 x0 = v - i + dot(i, C.xx);
  vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod289(i);
  vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m; m = m*m;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 a0 = x - floor(x + 0.5);
  m *= 1.792843 - 0.853735 * (a0*a0 + h*h);
  vec3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}
```

## Algorithm

### Color decomposition

Use simple CMY decomposition from Gustavson's shader: `cmy = 1.0 - rgb`. Extract K (black) component: `k = min(cmy.r, min(cmy.g, cmy.b)); cmy -= k`. The K component gets folded back as a darkening multiplier during compositing rather than a 4th ink layer (authentic to 3-drum risograph).

The three CMY channels map directly to the riso ink colors:
- C channel intensity -> teal ink amount
- M channel intensity -> hot pink ink amount
- Y channel intensity -> golden yellow ink amount

### Misregistration

Each ink layer samples the input texture at a slightly different UV offset. Offsets drift over time using sin/cos at different phases per layer:
- `offset_c = misregAmount * vec2(sin(time * 0.7), cos(time * 0.9))`
- `offset_m = misregAmount * vec2(sin(time * 1.1 + 2.0), cos(time * 0.6 + 1.0))`
- `offset_y = misregAmount * vec2(sin(time * 0.8 + 4.0), cos(time * 1.3 + 3.0))`

Each layer independently decomposes its offset-sampled pixel to CMY and uses only its own channel.

### Grain

Use Gustavson's multi-octave simplex noise (verbatim from reference code above). Each ink layer gets its own noise sample with a different seed offset to prevent grain correlation between layers. Grain erodes ink coverage — where noise exceeds a threshold, ink is removed and paper shows through:

```
grain_mask = smoothstep(1.0 - grainIntensity, 1.0, noise * 0.5 + 0.5)
ink_amount *= (1.0 - grain_mask)
```

Grain animates by adding `time * grainSpeed` to the noise seed offset.

### Posterization (optional)

Before decomposition, optionally reduce tonal range: `rgb = floor(rgb * levels + 0.5) / levels`. Creates the flat, hard-edged stencil regions characteristic of heavy risograph printing. When posterize = 0, this step is skipped.

### Subtractive compositing

Adapt the Maxime Heckel compositing pattern for custom ink colors. Each ink acts as a filter that absorbs its complement:

```
paper = mix(vec3(1.0), vec3(0.96, 0.93, 0.88), paperTone)
result = paper
result *= 1.0 - c_amount * inkDensity * (1.0 - tealInk)
result *= 1.0 - m_amount * inkDensity * (1.0 - pinkInk)
result *= 1.0 - y_amount * inkDensity * (1.0 - yellowInk)
result *= 1.0 - k * inkDensity * 0.7   // K darkens uniformly
```

Where ink colors are hardcoded riso tints:
- `tealInk = vec3(0.0, 0.72, 0.74)`
- `pinkInk = vec3(0.91, 0.16, 0.54)`
- `yellowInk = vec3(0.98, 0.82, 0.05)`

### Paper texture

Overlay a second noise pass at finer scale for paper fiber texture. This modulates the paper color itself (not the ink), creating subtle warm/cool variation in areas where no ink prints.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| grainScale | float | 50-800 | 200.0 | Size of grain noise pattern (pixels) |
| grainIntensity | float | 0.0-1.0 | 0.4 | How aggressively grain erodes ink (0=smooth, 1=very speckly) |
| grainSpeed | float | 0.0-2.0 | 0.3 | Grain animation rate |
| misregAmount | float | 0.0-0.02 | 0.005 | UV offset distance per ink layer |
| misregSpeed | float | 0.0-2.0 | 0.2 | How fast misregistration drifts |
| inkDensity | float | 0.2-1.5 | 1.0 | Overall ink coverage multiplier |
| posterize | int | 0-16 | 0 | Tonal step count (0=off/continuous) |
| paperTone | float | 0.0-1.0 | 0.3 | Paper warmth (0=pure white, 1=warm cream) |

## Modulation Candidates

- **grainIntensity**: more/less speckle texture — smooth washes vs gritty zine
- **misregAmount**: layers slide in/out of alignment — tight registration vs sloppy fringe
- **inkDensity**: ink gets thicker/thinner — saturated vs washed-out
- **posterize**: tonal banding shifts — continuous tone vs hard stencil steps
- **grainSpeed**: grain crawls faster/slower
- **paperTone**: paper shifts from cool white to warm cream

### Interaction Patterns

**inkDensity vs grainIntensity** (competing forces): high density fills coverage, high grain erodes it. Their balance determines the character. When both modulated by different audio bands, loud sections get thick saturated ink while quiet sections get thin speckled coverage — the visual weight tracks the music's energy without either parameter dominating.

**posterize vs grainIntensity** (cascading threshold): posterization creates flat tonal bands with hard edges. Grain then textures WITHIN those flat regions. Low posterize + high grain = organic erosion with smooth transitions. High posterize + high grain = crisp stencil blocks with aggressive speckle inside. The posterize parameter gates whether grain produces soft organic texture or hard graphic texture.

## Notes

- Single-pass fragment shader, no extra render textures needed
- The 3 ink colors are hardcoded to a perceptually-tuned riso set — no user color selection
- Simplex noise is used verbatim from Gustavson's public domain implementation
- At grainIntensity = 0 and posterize = 0, the effect reduces to pure color separation with misregistration (clean studio print look)
- Heavy posterization (levels = 2-3) with high grain produces the most authentic gritty zine aesthetic
- K channel is NOT a 4th ink layer — it's folded into a uniform darkening multiplier to avoid a muddy 4-color overlap
