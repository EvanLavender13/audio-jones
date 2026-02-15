# Bit Crush

Chunky pixel mosaics that constantly reorganize — an iterative lattice walk snaps pixels to integer grid cells via a time-varying hash, creating solid color blocks with hard edges. Nearby pixels cluster into the same cell (same color), but small changes in time cause entire regions to suddenly remap, producing a glitchy digital-mosaic aesthetic. Unlike pixelation (which just quantizes an input image), this generates structure from nothing.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after drawables, before transforms)
- **Sub-category peers**: Plasma, Interference, Moire Generator, Scan Bars, Glyph Field, Motherboard

## References

- User-provided Shadertoy code golf shader (source of truth, reproduced below)
- [Shadertoy programming tricks](https://shadertoyunofficial.wordpress.com/2019/01/02/programming-tricks-in-shadertoy-glsl/) — context on tweetable shader techniques

## Reference Code

```glsl
// Original tweetable shader — PRESERVE THIS VERBATIM
// This is the algorithm source of truth

float r(vec2 p){return cos(iTime*cos(p.x*p.y));}
void mainImage(out vec4 c,vec2 p){p*=.3;
for(int i;i++<32;p=ceil(p+r(ceil(p/8.))+r(floor(p/8.))*vec2(-1,1)))
c=sin(p.xyxy);}
```

### How it works

1. **`r(vec2 p)`** — time-varying pseudo-hash. `cos(iTime * cos(p.x * p.y))` returns [-1,1]. Position-dependent via the `p.x*p.y` product, smoothly oscillating over time.

2. **Initial scale**: `p *= 0.3` zooms out so more cells are visible.

3. **Iterative lattice walk (32 steps)**:
   - `ceil(p/8.)` and `floor(p/8.)` — two grid lookups at cell scale 8 (the cell you're in vs. the neighbor)
   - `r(ceil(...))` pushes x and y equally; `r(floor(...)) * vec2(-1,1)` pushes x and y in opposite directions
   - `ceil(...)` snaps result to integer coords — this creates the hard pixel edges
   - Nearby pixels that land in the same cell after all iterations get the same final position → same color → solid block

4. **Color**: `sin(p.xyxy)` maps final position to rainbow. We replace this with gradient LUT sampling.

## Algorithm

### Adaptation from reference

**Keep verbatim:**
- The `r()` hash function structure: `cos(time * cos(p.x * p.y))`
- The iterative walk with `ceil` snapping
- The dual grid lookup (`ceil(p/cellSize)` + `floor(p/cellSize)`) with opposing direction push

**Replace:**
- `sin(p.xyxy)` coloring → gradient LUT sampling: `t = fract(dot(finalPos, vec2(0.1, 0.13)))`, then `texture(gradientLUT, vec2(t, 0.5)).rgb`
- `iTime` → `time` uniform (accumulated in Setup)
- Hardcoded `0.3` scale → `scale` uniform
- Hardcoded `8.0` cell size → `cellSize` uniform
- Hardcoded `32` iterations → `iterations` uniform (int)

**Add:**
- FFT semitone glow: derive a frequency per cell from final position, look up FFT energy, modulate brightness. Map `t` through `baseFreq → maxFreq` in log space, sample `fftTexture`, apply `gain/curve/baseBright`.
- Center coordinates relative to `center` uniform before scaling (per shader coordinate conventions)
- `glowIntensity` uniform for overall FFT brightness control

### Coordinate handling

- `vec2 p = (fragTexCoord - center) * resolution * scale` — center-relative, resolution-scaled
- The lattice walk operates on these centered coords
- All spatial operations anchored to center, not bottom-left

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| scale | float | 0.05–1.0 | 0.3 | Overall zoom — more cells visible at lower values |
| cellSize | float | 2.0–32.0 | 8.0 | Lattice cell size — larger = coarser block regions |
| iterations | int | 4–64 | 32 | Walk depth — fewer = noisy, more = converged blocks |
| speed | float | 0.1–5.0 | 1.0 | Time multiplier for the r() hash oscillation |
| glowIntensity | float | 0.0–3.0 | 1.0 | FFT glow brightness |
| baseFreq | float | 27.5–440.0 | 55.0 | Lowest FFT frequency mapped to cells |
| maxFreq | float | 1000–16000 | 14000.0 | Highest FFT frequency mapped to cells |
| gain | float | 0.1–10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1–3.0 | 1.0 | FFT contrast curve |
| baseBright | float | 0.0–1.0 | 0.05 | Minimum cell brightness when no audio |

## Modulation Candidates

- **scale**: Zooms the mosaic in/out — block size breathes with modulation
- **cellSize**: Changes lattice granularity — blocks subdivide or merge
- **speed**: Controls reorganization rate — faster = more frantic glitching
- **iterations**: Changes convergence — fewer iterations = noisier, more chaotic pattern
- **glowIntensity**: Overall brightness pulse

### Interaction Patterns

- **scale × cellSize (competing forces)**: Scale controls how many cells are visible; cellSize controls how cells influence each other's walk paths. Increasing scale while decreasing cellSize creates dense fine noise; the opposite creates sparse large blocks. Modulating both from different sources creates tension between zoom and granularity.
- **iterations × cellSize (cascading threshold)**: Below ~8 iterations the pattern barely forms structure regardless of cellSize. Above ~20 iterations, the walk converges and cellSize determines final block character. Modulating iterations crosses this threshold — the pattern snaps between noise and structured mosaic.

## Notes

- Very cheap — single-pass fragment shader with a fixed-count loop. No textures read, no multi-pass.
- The `ceil` snapping means the pattern has inherent quantization — there are finitely many distinct pixel paths. This is a feature, not a bug.
- At very high iteration counts (>48) most pixels converge to a few attractor points → mostly uniform color. Keep default at 32.
- The `cos(iTime * cos(...))` hash is smooth — transitions between states are gradual flickers, not hard cuts. The "glitch" feel comes from nearby pixels crossing integer boundaries at different moments.
