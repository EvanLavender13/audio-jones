# Stripe Shift

Hard-edged flat RGB color bars that kink and bend around whatever content is already on screen. Three stripe channels at slightly different frequencies create per-channel color separation — where all three align you get white, where only one fires you get pure R/G/B, and overlaps produce cyan/magenta/yellow. The input image's brightness displaces the stripe positions, so any shape drawn in the pipeline (waveforms, simulations, generators) becomes the template that sculpts the stripe geometry.

## Classification

- **Category**: TRANSFORMS > Retro
- **Pipeline Position**: After generators, user-reorderable with other transforms

## Attribution

- **Based on**: "大龙猫 - Quicky#009" by totetmatt
- **Source**: https://www.shadertoy.com/view/wdt3zn
- **License**: CC BY-NC-SA 3.0

## Reference Code

```glsl
#define cc floor(iTime*2.) + ceil(iTime*2.)-pow(fract(iTime),.5)

mat2 r(float a){return mat2(cos(a),sin(a),-sin(a),cos(a));}
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
        vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
        vec2 uvv =uv;
        uvv *=r(cc*sign(uv.x*uv.y));
        uv += step(0.1,length(mod(sqrt(max(abs(uvv.y-uvv.x),abs(uvv.x+uvv.y)))+iTime/10.,.5)));
       float d = 1.;
            vec3 col =  vec3(d-step(0.3,abs(cos(-iTime+uv.y*16.))),d-step(0.3,abs(sin(-iTime*0.9+uv.y*16.))),d-step(0.3,abs(cos(1.+iTime*1.1+uv.y*16.))));
    fragColor = vec4(col,1.0);
}
```

## Algorithm

### What the reference code does

1. **Quantized time (`cc`)**: `floor(t*2) + ceil(t*2) - pow(fract(t), 0.5)` — a staircase function with ease-out transitions between steps, giving rhythmic snapping motion.

2. **Quadrant rotation**: `sign(uv.x * uv.y)` is +1 in quadrants I/III, -1 in II/IV. Multiplied by `cc`, opposing quadrant pairs rotate in opposite directions — a pinwheel fold.

3. **Diamond distance displacement**: `sqrt(max(abs(y-x), abs(x+y)))` is a diamond/L∞ metric rotated 45°. Wrapped with `mod(..., 0.5)` into repeating bands, then `step()` makes a binary mask. `uv += step(...)` shifts UV by 1.0 in "on" regions.

4. **Per-channel stripes**: Three channels of `1.0 - step(0.3, abs(cos/sin(time + uv.y * 16)))` at slightly different time multipliers (1.0, 0.9, 1.1). Hard-edged horizontal bands with chromatic phase separation.

### Adaptation for transform effect

| Reference | Replace with |
|-----------|-------------|
| Diamond distance field (`sqrt(max(abs(...)))`) | Input texture: luminance `dot(rgb, vec3(0.299, 0.587, 0.114))` when `colorDisplace` = 0, or per-channel R/G/B when `colorDisplace` = 1 |
| `uv += step(0.1, ...)` (binary displacement of 1.0) | Per-channel: `yR += displace_r * displacement`, `yG += displace_g * displacement`, `yB += displace_b * displacement`. When `colorDisplace` = 0, all three use shared luminance. When = 1, each uses its own input channel. |
| Hardcoded `16.` stripe density | `stripeCount` uniform |
| Hardcoded `0.3` step threshold | `stripeWidth` uniform |
| Hardcoded time multipliers `1.0, 0.9, 1.1` | Base `speed` uniform + `channelSpread` uniform controlling per-channel offset |
| `cc` quantized time | `quantize` uniform (0.0 = smooth, 1.0 = full snap). Accumulated on CPU as phase. |
| `iTime` in stripe functions | CPU-accumulated `phase` uniform (speed * deltaTime) |
| Raw `fragTexCoord` for UV | Centered: `fragTexCoord - center` for spatial ops, `fragTexCoord` for texture sampling |

### Key design decision

The displacement is **continuous** rather than binary. The reference uses `step()` for a hard on/off, but continuous displacement lets input brightness create smooth stripe curvature around shapes, not just a binary shift. A `hardEdge` parameter could optionally re-introduce the step behavior.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| stripeCount | float | 4.0–64.0 | 16.0 | Number of stripe bands across the screen |
| stripeWidth | float | 0.05–0.5 | 0.3 | Thickness of each stripe (step threshold) |
| displacement | float | 0.0–4.0 | 1.0 | How much input brightness offsets stripe positions |
| speed | float | -ROTATION_SPEED_MAX–ROTATION_SPEED_MAX | 1.0 | Animation rate (accumulated on CPU) |
| channelSpread | float | 0.0–0.5 | 0.1 | Per-channel frequency/phase offset — 0 = monochrome, higher = more color separation |
| colorDisplace | float | 0.0–1.0 | 0.0 | 0 = shared luminance displaces all channels equally, 1 = each input R/G/B channel displaces its own stripe independently |
| hardEdge | float | 0.0–1.0 | 0.0 | Mix between continuous displacement (0) and binary step (1) |

## Modulation Candidates

- **stripeCount**: Density shifts — low for bold bars, high for fine texture
- **displacement**: Controls how visible input shapes are in the stripe pattern. At 0, uniform stripes; cranked up, stripes wrap tightly around content
- **stripeWidth**: Ratio of stripe to gap — thin stripes on black vs thick bright bands
- **channelSpread**: Color richness — 0 is white/black only, higher values unlock full RGB palette
- **speed**: Animation rate of stripe crawl
- **colorDisplace**: At 0, all stripes shift together (monochrome displacement). At 1, a red shape only kinks the R bars while G and B stay straight — input color drives per-channel separation
- **hardEdge**: Smooth curves vs harsh digital kinks

### Interaction Patterns

**Cascading threshold** — `displacement` gates shape visibility. Below ~0.2, stripes look uniform regardless of input content. Above that threshold, input shapes emerge. Driving displacement with a bass hit makes shapes flash into existence on the beat and dissolve between hits.

**Competing forces** — `stripeCount` vs `displacement`. High stripe count means each band is narrow, so displacement must be proportionally larger to create visible kinks. Modulating them from different audio sources creates tension — one pulls toward fine uniform texture while the other forces bold shape breaks.

**Resonance** — `channelSpread` near 0 produces mostly white/black. As it increases, color separation emerges gradually. Moments where all three channels briefly realign (from speed animation) create white flashes against a colorful background — rare bright peaks.

## Notes

- Speed MUST be accumulated on CPU (`phase += speed * deltaTime`), never in shader. Pass accumulated phase as uniform.
- The quantize behavior from `cc` could be implemented as a CPU-side phase quantization: `quantizedPhase = mix(phase, floor(phase * 2.0) / 2.0, quantize)`. This snaps the animation to rhythmic steps.
- Performance is trivial — no texture lookups beyond the single input sample, just trig functions and step.
