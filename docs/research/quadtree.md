# Quadtree

Recursive cell grid that subdivides finer wherever moving points fall. Each pixel walks down a quadtree, halving its bounding box each step and choosing the quadrant that contains the pixel; subdivision continues only while at least one source point lies inside the current cell. The result is a hierarchical patchwork of square cells - large in empty regions, fine where points cluster - drawn as outlines (with optional fill) and colored either by recursion depth or by a per-cell hash, with audio energy mapped through the standard generator FFT pattern.

## Classification

- **Category**: GENERATORS > Texture (section 12)
- **Pipeline Position**: Generator (writes to scene buffer via blend compositor, same slot as Constellation, Galaxy, Subdivide)

## Attribution

- **Based on**: "Quadtree Random Points" by Bingle
- **Source**: https://www.shadertoy.com/view/wc2XRh
- **License**: CC BY-NC-SA 3.0 (Shadertoy default)

## References

- Bingle, "Quadtree Random Points" - the full algorithm; reference code below.

## Reference Code

```glsl
// Constants
#define POINTCOUNT 8
#define PI 3.14159

// One-liners
#define inBox(low,high,point) (low.x<=point.x && low.y<=point.y && high.x>point.x && high.y>point.y)
#define toUv(point) 2.0*(point-0.5*iResolution.xy)/iResolution.y

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec2 uv = toUv(fragCoord);
    vec2 low = floor(uv);
    vec2 high = ceil(uv);
    uv = mod(uv,vec2(1.0));

    vec2 mouse = toUv(iMouse.xy);
    vec2[POINTCOUNT] points;

    float t = iTime*0.4;
    for (int i=0;i<POINTCOUNT;i++){
        float fi = float(i);
        if (i==POINTCOUNT-1){
            points[POINTCOUNT-1] = toUv(iMouse.xy);
        }else{
            points[i] = vec2(
                (1.75-0.125*fi)*cos(1.333*t-(1.0-0.2*t)*fi/PI),
                (1.0-0.1*fi)*sin((fi+1.0)*t+PI*0.5)
            );
        }
    }

    int iters = 0;
    while (iters<8){
        iters++;
        bool sub = false;
        for (int i=0;i<POINTCOUNT;i++){
            if inBox(low,high,points[i]){
                sub = true;
                break;
            }
        }
        if (sub){
            vec2 center = (high+low)*0.5;
            if (uv.x>0.5){
                low.x = center.x;
                uv.x = uv.x*2.0-1.0;
            }else{
                high.x = center.x;
                uv.x *= 2.0;
            }
            if (uv.y>0.5){
                low.y = center.y;
                uv.y = uv.y*2.0-1.0;
            }else{
                high.y = center.y;
                uv.y *= 2.0;
            }
        }else{
            break;
        }
    }

    vec3 col = vec3(smoothstep(1.0-2.0*pow(2.0,float(iters))/iResolution.y,1.0,max(abs(uv.x-0.5),abs(uv.y-0.5))*2.0));//0.5 + 0.5*cos(iTime+3.0*uv.xyx+vec3(0,2,4));

    fragColor = vec4(col,1.0);
}
```

## Algorithm

The reference is a stateless fragment shader: every pixel walks a quadtree using the current point array. Adapt by replacing the per-index parametric point formula with a CPU-driven `DualLissajousUpdateCircular` (Ripple Tank pattern) and replacing the grayscale outline with the standard generator color/FFT path.

### Substitution Table (Keep / Replace)

| Reference | Action | Replacement |
|---|---|---|
| `toUv(fragCoord)` macro | KEEP verbatim | Centers and aspect-corrects (divide by `iResolution.y` only) so cells are square. Already conforms to the codebase's "center coords relative to screen center" rule. |
| `vec2 low = floor(uv); vec2 high = ceil(uv); uv = mod(uv, vec2(1.0));` | KEEP verbatim | Bounding box of current cell + local UV inside it. |
| `#define POINTCOUNT 8` | REPLACE | `uniform int pointCount;` (range 1-8). Shader-side array stays sized 8: `uniform vec2 sources[8];` |
| `vec2 mouse = toUv(iMouse.xy);` | REMOVE | No mouse coupling. |
| Per-index parametric loop assigning `points[i]` from `iTime` and the `iMouse` special case | REPLACE | Drop the loop entirely. Read directly from `sources[i]` uniform. CPU populates it via `DualLissajousUpdateCircular(&cfg->lissajous, deltaTime, cfg->baseRadius, 0.0f, 0.0f, count, sources)`. Coordinate space match: `DualLissajousUpdateCircular` outputs in the same centered/aspect-corrected space the reference's `toUv` produces, so positions are directly comparable to `floor(uv)`/`ceil(uv)`. |
| `while (iters<8)` | REPLACE | `while (iters < maxIterations)`, `uniform int maxIterations;` (range 1-8). |
| `for (int i=0; i<POINTCOUNT; i++) { if inBox(low,high,points[i]) {...} }` | REPLACE inner indexing | `for (int i = 0; i < pointCount; ++i) { if (inBox(low, high, sources[i])) {...} }`. GLSL 330 supports dynamic loop bounds (per `feedback_glsl_loops.md`); use `pointCount` directly, no hardcoded max + break. |
| Subdivision branch (center, low/high updates, uv remap) | KEEP verbatim | Core quadtree descent. |
| `vec3 col = vec3(smoothstep(1.0-2.0*pow(2.0,float(iters))/iResolution.y,1.0,max(abs(uv.x-0.5),abs(uv.y-0.5))*2.0));` | REPLACE | See "Color and FFT" below. The hardcoded `2.0` line-thickness multiplier becomes `uniform float lineWidth;`. |
| `fragColor = vec4(col, 1.0);` | KEEP | Final output. |

### Color and FFT (replaces final grayscale line)

Per `memory/generator_patterns.md`: each pixel produces one `t`, then `t` drives BOTH `gradientLUT` AND BAND_SAMPLES FFT lookup (continuous variant - no `layers` param, 4 adjacent bins starting at `t`).

```glsl
// 1. Compute outline factor (reference grayscale, but parameterized)
float edge = max(abs(uv.x - 0.5), abs(uv.y - 0.5)) * 2.0;
float outlineThreshold = 1.0 - lineWidth * pow(2.0, float(iters)) / resolution.y;
float outline = smoothstep(outlineThreshold, 1.0, edge);

// 2. Compute cell coverage = outline + optional fill
float coverage = outline + cellFillAmount * (1.0 - outline);

// 3. Compute t per colorMode
float t;
if (colorMode == 0) {
    // Depth: deeper cells -> higher t -> later in gradient + higher FFT bands
    t = float(iters) / float(maxIterations);
} else {
    // Hash: stable per-cell t from cell's low corner
    t = fract(sin(dot(low, vec2(12.9898, 78.233))) * 43758.5453);
}

// 4. Standard generator color + FFT (continuous BAND_SAMPLES variant)
vec3 baseColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; ++s) {
    float ts = t + (float(s) + 0.5) / float(BAND_SAMPLES) / float(textureSize(fftTexture, 0).x);
    float freq = baseFreq * pow(maxFreq / baseFreq, ts);
    float bin = freq / (sampleRate * 0.5);
    if (bin <= 1.0) { energy += texture(fftTexture, vec2(bin, 0.5)).r; }
}
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;

fragColor = vec4(baseColor * brightness * coverage, coverage);
```

The alpha-as-coverage convention matches other Texture generators and lets the blend compositor composite cleanly.

### Coordinate-space note

The reference's `toUv` produces coordinates centered at the screen middle, scaled by `1/iResolution.y` so cells are square regardless of aspect. The codebase's standard rule ("center coordinates relative to `center` uniform") is already satisfied because `toUv` is the centering. Do not re-center; do not multiply by `resolution.x`. Sources from `DualLissajousUpdateCircular` already live in the same normalized centered space.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `enabled` | bool | - | false | Effect on/off |
| `pointCount` | int | 1-8 | 6 | Number of subdivision-driving sources |
| `baseRadius` | float | 0.0-1.0 | 0.5 | Source orbit radius (passed to `DualLissajousUpdateCircular`) |
| `lissajous` | DualLissajousConfig | - | - | Embedded shared Lissajous controls (X/Y freq, phase, amplitude, motion speed) |
| `maxIterations` | int | 1-8 | 8 | Quadtree recursion depth cap |
| `lineWidth` | float | 0.5-4.0 | 2.0 | Outline thickness in pixels (multiplier on the screen-space compensation) |
| `cellFillAmount` | float | 0.0-1.0 | 0.0 | Interior cell fill opacity (0=outline only, 1=solid cells) |
| `colorMode` | int | 0-1 | 0 | 0=depth, 1=hash |
| `baseFreq` | float | 27.5-440 | 55.0 | FFT band lower frequency Hz |
| `maxFreq` | float | 1000-16000 | 14000.0 | FFT band upper frequency Hz |
| `gain` | float | 0.1-10.0 | 2.0 | FFT energy sensitivity |
| `curve` | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| `baseBright` | float | 0.0-1.0 | 0.15 | Floor brightness with no FFT energy |
| `blendMode` | EffectBlendMode | - | EFFECT_BLEND_SCREEN | Compositor blend mode |
| `blendIntensity` | float | 0.0-5.0 | 1.0 | Compositor strength |
| `gradient` | ColorConfig | - | - | Color gradient/LUT |

## Modulation Candidates

- **baseRadius**: orbit radius - cluster-vs-spread, pulls subdivision toward/away from center
- **lissajous.amplitude**: orbit shape envelope - tight loops vs sweeping arcs change cluster cadence
- **lissajous.motionSpeed**: how quickly clusters form and dissolve
- **lineWidth**: cell legibility - thin ghostly grids vs bold blocked frames
- **cellFillAmount**: outline-only vs filled-Mondrian look, sweepable mid-track
- **gain**: FFT response strength
- **curve**: FFT contrast - smooth gradient vs popping band activation
- **baseBright**: idle visibility floor between hits
- **blendIntensity**: standard generator output level

### Interaction Patterns

- **Cascading via point clustering**: When `lissajous.motionSpeed` or `lissajous.amplitude` drive Lissajous parameters into alignment, multiple sources land in the same coarse cell. Local recursion depth spikes -> deep cells appear -> `t` reaches the upper band of the gradient -> high FFT bands flare in those clusters. One audio source modulating Lissajous motion cascades into both spatial density AND high-band activation, so verses and choruses look structurally different rather than just brighter or darker.
- **Resonance: pointCount x maxIterations**: Both gate subdivision growth. Modulating both with related sources (or the same source via two routes) amplifies density punch on hits - more sources triggering more iterations creates explosive cell-density bursts that calm sections cannot reach. Modulating only one creates a soft change; modulating both creates a step.
- **Tension: lineWidth vs maxIterations**: Deep subdivision shrinks cells, the `pow(2, iters)` compensation keeps lines pixel-thin. Pushing `lineWidth` low at high subdivision -> cells dissolve into noise. Pushing `lineWidth` high at low subdivision -> cells become solid black borders dominating the frame. Inverse modulation (lineWidth up as iterations down) keeps visual weight constant; same-direction modulation (both up together) creates a "crystallization" moment where grids appear and then sharpen.
- **colorMode interaction with FFT**: In depth mode, `t` is integer-quantized to `maxIterations + 1` discrete values, so FFT lookup hits at most that many distinct band positions - the visual reads as discrete pulsing layers. In hash mode, `t` scatters continuously across `[0, 1]` per cell, so the full spectrum lights up across the canvas as a sparkle. Switching mode mid-track is itself an expressive move.

## Notes

- **Cost**: per-pixel work is `O(pointCount * maxIterations)` worst case = 64 inBox checks. Comfortable for a fragment shader at 1080p.
- **Quantization**: depth mode produces only `maxIterations + 1` distinct `t` values, so depth-driven FFT response steps in discrete layers rather than smoothly across the spectrum. This is the intended dynamic, not a bug.
- **Aspect correction**: cells are square in screen space because `toUv` divides by `resolution.y` only. Window resize keeps cell aspect correct without resize callback work; no `EFFECT_FLAG_NEEDS_RESIZE` needed.
- **No ping-pong**: stateless shader, no frame history. Use `REGISTER_GENERATOR` (not `REGISTER_GENERATOR_FULL`).
- **No mouse**: the reference's 8th source bound to `iMouse` is dropped; all sources come from `DualLissajousUpdateCircular`.
- **Hash function**: `fract(sin(dot(low, vec2(12.9898, 78.233))) * 43758.5453)` is the conventional GLSL position hash. Stable per cell across frames as long as `low` is stable, which it is (cell bounds depend only on pixel + descent, not on time).
