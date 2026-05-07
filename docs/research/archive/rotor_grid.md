# Rotor Grid

A spinning radial cell-grid where each concentric ring carries more angular cells than the one inside it, producing a hypnotic tunnel of geometric tiles whose count grows toward the edges. Three coloring modes animate the same scaffold differently: a smooth gradient rainbow rotating around, a sweeping wedge that lights a narrow angular slice, or a randomized mosaic where each cell hashes to its own gradient position.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Same generator section as Pitch Spiral / Spectral Arcs / Spectral Rings (radial-grid family).

## Attribution

- **Based on**: "rrotationnal rotation [207ch]" and "irrotationnal rotation [208ch]" by FabriceNeyret2
- **Source**: https://www.shadertoy.com/view/sXf3WH and https://www.shadertoy.com/view/sXX3WH
- **License**: CC BY-NC-SA 3.0

## References

- [rrotationnal rotation](https://www.shadertoy.com/view/sXf3WH) - smooth-color variant of the radial cell-grid scaffold
- [irrotationnal rotation](https://www.shadertoy.com/view/sXX3WH) - wedge-highlight variant on the same scaffold

## Reference Code

```glsl
// rrotationnal rotation [207ch] by FabriceNeyret2
// https://www.shadertoy.com/view/sXf3WH
void mainImage( out vec4 O, vec2 U ) {
    O.xyz = iResolution;
    U += U - O.xy;                                      // centered coordinates (no need to normalize)
    float l = length(U)/.1/O.y,                         // normalized length in [0,10]
          a = atan(U.y,U.x)*4.*round(l) + 5.*iTime,     // circle parameterization
          d = cos( a )                                  // angular divisions
            * cos( 3.14*l );                            // circles
    O = min( abs(d/fwidth(d)), 1. )                     // AA draw
      * ( .6 + .6 * cos( a/4.  + vec4(0,23,21,0)  ) );  // cells color
}
```

```glsl
// irrotationnal rotation [208ch] by FabriceNeyret2
// https://www.shadertoy.com/view/sXX3WH
void mainImage( out vec4 O, vec2 U ) {
    O.xyz = iResolution;
    U += U - O.xy;                                      // centered coordinates (no need to normalize)
    float l = length(U)/.1/O.y,                         // normalized length in [0,10]
          a = atan(U.y,U.x)*4.*round(l) + 5.*iTime,     // circle parameterization
          d = cos( a )                                  // angular divisions
            * cos( 3.14*l );                            // circles
    O += abs(d/fwidth(d)) -O;                           // AA draw
    abs( mod(a/4.,6.283) - 3.14 ) < .4 ? O.gb *=0. : U; // red cells
}
```

## Algorithm

Both reference shaders share an identical scaffold; they diverge only in the final coloring step. Adapt by keeping the scaffold verbatim and parameterizing the coloring via a mode enum.

**Keep verbatim from reference:**
- Centered coordinates: `U = fragTexCoord * resolution - resolution * 0.5` (raylib convention).
- Radial length: `l = length(U) / ringSpacing / resolution.y` (replaces the literal `.1`).
- Angular index: `a = atan(U.y, U.x) * baseDivisions * round(l) + spinPhase`. The `baseDivisions * round(l)` term (literal `4.*round(l)` in reference) is the signature growth-with-radius — keep this verbatim.
- Cell field: `d = cos(a) * cos(ringFrequency * l + radialDrift)`. The `radialDrift` shifts the `cos(pi*l)` ring phase for the breathing motion.
- AA factor: `aa = min(abs(d / fwidth(d)), 1.0)`.

**Replace per codebase conventions:**
- `iTime` → CPU-accumulated `spinPhase` (per `feedback_speed_accumulation.md`). Inner rings spin at full `spinPhase`; outer rings receive `spinPhase + differentialTwist * round(l) * spinPhase` so the differential adds shearing as `round(l)` grows.
- `iResolution` → `resolution` uniform.
- `cos(a/4 + vec4(0,23,21,0))` per-channel hue → gradient LUT sampling (per `feedback_no_llm_isms.md` / generator conventions).
- 207 final color (`aa * (.6 + .6 * cos(...))`) and 208 final color (`aa` modulated by red wedge gating) → mode switch over three branches that all feed through `gradientLUT` and the FFT brightness path.

**Per-cell `t` derivation (drives BOTH gradient LUT and FFT lookup):**
- `MODE_SMOOTH`: `t = fract(a / (TWO_PI * baseDivisions))`. Matches the reference's `cos(a/4 + offset)` color cycle rate — `round(l)` rainbow wraps per ring, smooth angular transition between adjacent cells. Same `t` picks gradient hue and FFT band.
- `MODE_WEDGE`: `t = fract(a / (TWO_PI * baseDivisions))` for cells inside the wedge (`abs(mod(a / baseDivisions, TWO_PI) - PI) < wedgeWidth`); cells outside the wedge render as `vec3(1.0) * baseBright * aa` (white-grayscale at floor brightness, matching reference 208's grayscale-outside-wedge behavior) with no FFT gating.
- `MODE_RANDOM`: `t = fract(hash(ringIndex, angularSlot) + driftRate * driftPhase)`, where `ringIndex = round(l)` and `angularSlot = round(a / TWO_PI * baseDivisions * ringIndex)`. `driftPhase` is CPU-accumulated. `driftRate = 0` freezes the mosaic; nonzero shimmers it.

**FFT brightness (apply uniformly across modes after gradient sample):**
```
energy = 0.0
for s in 0..BAND_SAMPLES:
    ts = t + (s + 0.5) / BAND_SAMPLES / fftWidth
    freq = baseFreq * pow(maxFreq / baseFreq, ts)
    bin = freq / (sampleRate * 0.5)
    if bin <= 1.0: energy += sample(fftTexture, bin)
mag = pow(clamp(energy / BAND_SAMPLES * gain, 0, 1), curve)
brightness = baseBright + mag
col *= brightness * aa
```

**Parameter mapping from reference constants:**
- `.1` (in `length/.1/O.y`) → `ringSpacing`
- `4.` (in `atan*4.*round(l)`) → `baseDivisions`
- `5.` (in `5.*iTime`) → `spinSpeed`
- `3.14` (in `cos(3.14*l)`) → `ringFrequency`
- `.4` (in `< .4` wedge test) → `wedgeWidth`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| mode | enum | {SMOOTH, WEDGE, RANDOM} | SMOOTH | Coloring strategy |
| spinSpeed | float (rad/s) | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 5.0 | Base time-rotation rate; CPU-accumulated into `spinPhase` |
| ringSpacing | float | 0.05..0.5 | 0.1 | Radial scale; smaller = more rings on screen |
| baseDivisions | int (as float) | 1..8 | 4 | Base angular subdivisions; outer rings get `baseDivisions * round(l)` cells |
| ringFrequency | float | 1.0..6.283 | 3.14 | Ring spacing frequency in `cos(ringFrequency * l)` |
| differentialTwist | float | -2.0..2.0 | 0.0 | Outer-ring rotation differential; 0 = uniform spin, nonzero = galaxy-shear |
| radialDrift | float (rad) | -ROTATION_OFFSET_MAX..ROTATION_OFFSET_MAX | 0.0 | Phase shift on ring frequency; modulate for ring-breathing |
| wedgeWidth | float (rad) | 0.0..PI | 0.4 | Angular half-width of WEDGE-mode highlight |
| driftRate | float | 0.0..1.0 | 0.0 | RANDOM-mode gradient drift speed; 0 = static mosaic |
| color | ColorConfig | — | gradient default | Embedded gradient LUT + ColorConfig for hue source |
| baseFreq | float (Hz) | 27.5..440 | 55 | FFT base frequency |
| maxFreq | float (Hz) | 1000..16000 | 14000 | FFT max frequency |
| gain | float | 0.1..10 | 2.0 | FFT gain |
| curve | float | 0.1..3 | 1.5 | FFT contrast |
| baseBright | float | 0.0..1.0 | 0.15 | Floor brightness; FFT magnitude lifts each cell above this |
| blendIntensity | float | 0.0..5.0 | 1.0 | Standard generator blend opacity |
| blendMode | EffectBlendMode | — | EFFECT_BLEND_SCREEN | Standard generator blend mode (codebase convention for all generators) |

## Modulation Candidates

- **spinSpeed**: faster/slower whole-grid rotation
- **differentialTwist**: shears rings against each other; at extremes inner and outer counter-rotate
- **radialDrift**: rings expand/contract through screen as the `cos(pi*l)` phase shifts
- **ringSpacing**: density of rings; small values pack many rings, large values show only a few
- **baseDivisions**: angular cell count grows or shrinks across the whole grid
- **wedgeWidth**: WEDGE-mode highlight expands and contracts (no-op in other modes)
- **driftRate**: RANDOM-mode mosaic shimmer rate (no-op in other modes)
- **gain**: per-cell FFT response amplitude
- **curve**: per-cell FFT contrast
- **baseBright**: visibility floor

### Interaction Patterns

- **spinSpeed x differentialTwist (resonance)**: at `differentialTwist = 0` the whole grid rotates coherently; pushing twist away from 0 while modulating spin produces shearing waves where inner rings race ahead of outer rings (or vice versa). Both at zero = frozen grid; one without the other = limited expression. Combined modulation creates galaxy-like rotation that varies with audio.
- **radialDrift x ringFrequency (competing forces)**: `radialDrift` shifts ring phase, `ringFrequency` controls how tightly rings pack. Modulating drift while frequency is high produces fast ring pulses; modulating drift while frequency is low produces slow ring waves. The two together let the rings breathe at independent radial and temporal rates.
- **driftRate gated by mode (cascading threshold)**: `driftRate` only produces visible motion in `MODE_RANDOM`. Routing modulation to `driftRate` does nothing unless mode is RANDOM, which makes the random-mode mosaic a distinct "voice" — the same modulation route reads as inert in two modes and active in the third.

## Notes

- The growth-with-radius angular count (`baseDivisions * round(l)`) means cell aspect ratio stays roughly square as you move outward — this is the signature of the reference and must be preserved verbatim.
- `round(l)` is integer-valued per ring, so `ringIndex = round(l)` is the natural per-ring identifier for hashing in RANDOM mode.
- AA via `abs(d / fwidth(d))` is GLSL 330 compliant; no special handling required.
- Tonemap forbidden (per `feedback_no_reinhard.md`).
- No magic scalars: every reference constant maps to a named parameter (table above).
