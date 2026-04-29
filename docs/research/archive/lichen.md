# Lichen

Organic blobs of competing species expand, retreat, and spiral across the screen in a
never-ending cyclic chase. Three reaction-diffusion colonies grow coral-like tendrils
into each other's territory: each species consumes the next in a rock-paper-scissors
loop, so no single color can dominate. The boundaries between species shift and swirl
with sinusoidal turbulence, and FFT energy modulates the brightness so the whole living
surface pulses with the music.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: 13 (Field)
- **Compute Model**: Fragment shader with dual ping-pong texture pairs + color output pass

## Attribution

- **Based on**: "rps cell nomming :3" by frisk256
- **Source**: https://www.shadertoy.com/view/NcjXRh
- **License**: CC BY-NC-SA 3.0

## References

- [Karl Sims - Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html) - Gray-Scott equations, Laplacian kernel, parameter space overview
- [Pierre Couy - Gray-Scott shader](https://pierre-couy.dev/simulations/2024/09/gray-scott-shader.html) - GPU implementation with ping-pong textures, stability notes
- [Pearson's Parameterization (mrob.com)](http://www.mrob.com/pub/comp/xmorphia/index.html) - Complete catalog of Gray-Scott parameter regimes

## Reference Code

```glsl
// "rps cell nomming :3" by frisk256
// https://www.shadertoy.com/view/NcjXRh
// License: CC BY-NC-SA 3.0

// --- [Common] ---
#define r iResolution.xy
const float PI = 3.14159265358979;

const float k = 0.04;
const float j = 1.07;

// --- [Buffer A] (identical logic in Buffer B and C, different seed positions) ---
// Channel wiring creates cyclic competition:
//   iChannel0 = self (feedback), iChannel1 = hp (predator), iChannel2 = hn (prey)
//   A's predator is one neighbor, A's prey is the other; wired cyclically across A/B/C
void mainImage(out vec4 c,vec2 p) {
    vec4 hp = texture(iChannel1,p / r);
    vec4 hn = texture(iChannel2,p / r);
    float q = 1.0;
    // 8-octave sinusoidal coordinate warp - stirs territory boundaries
    for(int i = 0; i < 8; i++) {
        p.x += sin(p.y / r.y * PI * 2.0 * q + iTime * (fract(q / 4.0) - 0.3) * 2.5) / q * 0.1;
        p.y -= sin(p.x / r.x * PI * 2.0 * q + iTime * (fract(q / 5.0) - 0.2) * 2.5) / q * 0.1;
        q *= -1.681;
        q = fract(q / 10.0) * 10.0 - 5.0;
        q = floor(q) + 1.0;
    }
    // Asymmetric diffusion: activator (.x) at radius 2.5, inhibitor (.y) at radius 1.2
    c = texture(iChannel0,p / r);
    c.x += texture(iChannel0,(p + vec2(1,0) * 2.5) / r).x;
    c.x += texture(iChannel0,(p + vec2(0,1) * 2.5) / r).x;
    c.x += texture(iChannel0,(p - vec2(1,0) * 2.5) / r).x;
    c.x += texture(iChannel0,(p - vec2(0,1) * 2.5) / r).x;
    c.y += texture(iChannel0,(p + vec2(1,0) * 1.2) / r).y;
    c.y += texture(iChannel0,(p + vec2(0,1) * 1.2) / r).y;
    c.y += texture(iChannel0,(p - vec2(1,0) * 1.2) / r).y;
    c.y += texture(iChannel0,(p - vec2(0,1) * 1.2) / r).y;
    c /= 5.0;

    // Gray-Scott reaction with cyclic coupling (25 iterations per frame)
    for(int n = 0; n < 25; n++) {
        c.xy += (vec2(-1,1) * c.x * c.y * c.y + vec2(0.019,k * (hp - hn * j) -0.084) * vec2(1.0 - c.x,c.y)) * 0.4;
    }

    c = clamp(c,0.0,1.0);

    // Seed: each buffer places its initial blob at a different position
    if(iFrame == 0) {
        c = vec4(1,0,1,0);
        if(length(p - r * vec2(0.16)) + sin(p.x * p.y + fract(iDate.w / 73.4)) * 10.5 < 28.5) {
            c.y++;
        }
    }
}
// Buffer B: seed at vec2(0.5), Buffer C: seed at vec2(0.84)
// Otherwise identical to Buffer A

// --- [Image] ---
// Maps each species' inhibitor (.y) to one RGB channel
void mainImage(out vec4 c,vec2 p) {
    c.x += texture(iChannel0,p / r).y * 2.0;
    c.y += texture(iChannel1,p / r).y * 2.0;
    c.z += texture(iChannel2,p / r).y * 2.0;
}
```

## Algorithm

### Gray-Scott Reaction-Diffusion

Standard Gray-Scott with two chemicals per species:
- u (activator/substrate): diffuses fast, consumed by the reaction
- v (inhibitor/catalyst): diffuses slow, produced by the reaction, forms the visible patterns

Equations per species:
```
du = -u*v^2 + F*(1-u)
dv = +u*v^2 - K*v
```

The reference innovation: K (kill rate) is not constant but modulated by neighboring species:
```
K = k * (predator - prey * j) + killBase
```
- When the predator species is abundant nearby, K rises, this species decays
- When the prey species is abundant nearby, K falls, this species persists
- j > 1.0 gives prey a slight advantage (species tend to persist), preventing immediate collapse

### Adaptation to Codebase

**Texture packing (3 separate buffers -> 2 RGBA textures):**
- stateTex0: species 0 in .rg (u0, v0), species 1 in .ba (u1, v1)
- stateTex1: species 2 in .rg (u2, v2), .ba unused

Each ping-pong pair: statePingPong0[2] and statePingPong1[2] = 4 render textures.
Plus 1 colorRT for the final colored output = 5 render textures total.

**Simulation shader (runs twice per frame, once per output texture):**

Keep verbatim from reference:
- The 8-octave sinusoidal coordinate warp (with amplitude controlled by `warpIntensity` uniform)
- Asymmetric diffusion at two radii (activator at `activatorRadius`, inhibitor at `inhibitorRadius`)
- The 25-step reaction loop with the cyclic coupling term
- `clamp(c, 0.0, 1.0)` at the end

Replace from reference:
- 3 separate buffer passes -> single shader computes all 3 species, uses `uniform int passIndex` to select which channels to output (pass 0: species 0+1, pass 1: species 2)
- `iChannel0` self-feedback -> read from both `stateTex0` and `stateTex1` read buffers
- `iChannel1`/`iChannel2` prey/predator -> read from the appropriate channels of the packed textures
- `iTime` -> `time` uniform (CPU-accumulated phase)
- `iResolution` -> `resolution` uniform
- Hardcoded `k=0.04`, `j=1.07` -> uniforms `couplingStrength`, `predatorAdvantage`
- Hardcoded `0.019` (feed), `0.084` (kill base), `0.4` (time step) -> uniforms `feedRate`, `killRateBase`, `reactionRate`
- Hardcoded diffusion radii `2.5`/`1.2` -> uniforms `activatorRadius`, `inhibitorRadius`
- `0.1` warp amplitude -> `warpIntensity` uniform

**Render flow per frame:**
1. Bind statePingPong0[readIdx0] and statePingPong1[readIdx1] as inputs
2. Pass 1: BeginTextureMode(statePingPong0[writeIdx0]), passIndex=0, draw quad -> writes species 0+1
3. Pass 2: BeginTextureMode(statePingPong1[writeIdx1]), passIndex=1, draw quad -> writes species 2
4. Flip read indices
5. Color pass: read both state textures, sample gradient LUT, write to colorRT

Both passes read from the SAME read buffers (not the just-written ones), so
species coupling is consistent within a frame.

**Color output shader:**
- Read v0 from stateTex0.g, v1 from stateTex0.a, v2 from stateTex1.g
- Each species samples from 1/3 of the gradient LUT: species 0 at lutCoord 0.0-0.33, species 1 at 0.33-0.67, species 2 at 0.67-1.0
- Weight by inhibitor density: `color = v0*lut(v0*0.33) + v1*lut(0.33+v1*0.33) + v2*lut(0.67+v2*0.33)`
- FFT brightness: sample fftTexture, multiply final color by audio-derived intensity (standard baseFreq/maxFreq/gain/contrast/baseBright pattern)

**Seeding:**
- On init and reset: clear state textures to (1, 0, 1, 0) (u=1, v=0 for both packed species)
- Place seed blobs for each species at equidistant positions (reference: 0.16, 0.5, 0.84)
- Seed sets v=1 in a small radius with slight noise on the boundary

**Infrastructure pattern:** follows curl_advection (REGISTER_GENERATOR_FULL, dual state ping-pong + visual output, separate state shader and color shader).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| feedRate | float | 0.005-0.08 | 0.019 | Speed of substrate replenishment; higher = faster growth |
| killRateBase | float | 0.01-0.12 | 0.084 | Base decay rate before species coupling; tunes pattern regime |
| couplingStrength | float | 0.0-0.2 | 0.04 | How strongly predator/prey balance affects kill rate |
| predatorAdvantage | float | 0.8-1.5 | 1.07 | Asymmetry: >1 favors prey persistence, <1 favors predator |
| warpIntensity | float | 0.0-0.5 | 0.1 | Amplitude of sinusoidal boundary stirring |
| warpSpeed | float | 0.0-5.0 | 2.5 | Speed of the coordinate warp animation |
| activatorRadius | float | 0.5-5.0 | 2.5 | Diffusion sampling distance for activator (u); controls pattern scale |
| inhibitorRadius | float | 0.5-3.0 | 1.2 | Diffusion sampling distance for inhibitor (v) |
| reactionSteps | int | 5-50 | 25 | Reaction iterations per frame; quality vs performance |
| reactionRate | float | 0.1-0.8 | 0.4 | Time step per reaction iteration; higher = faster but less stable |
| brightness | float | 0.5-4.0 | 2.0 | Output brightness multiplier (reference uses 2.0 in Image pass) |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency bound (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT gain |
| contrast | float | 0.1-3.0 | 1.5 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **feedRate**: Growth speed; modulate to make colonies pulse - low feed starves patterns, high feed fills space with new growth
- **killRateBase**: Decay rate; modulate to sweep through Gray-Scott pattern regimes (spots, stripes, mitosis, coral)
- **couplingStrength**: Competition intensity; low = species coexist peacefully, high = aggressive territorial chasing
- **predatorAdvantage**: Shift competitive balance; modulating above/below 1.0 alternates which direction the cyclic chase spirals
- **warpIntensity**: Boundary turbulence; modulate to make territory edges breathe between rigid and fluid
- **warpSpeed**: Warp animation rate; creates urgency/calm in boundary motion
- **brightness**: Direct visual intensity scaling

### Interaction Patterns

**Cascading threshold - feedRate vs killRateBase:**
The Gray-Scott parameter space has a narrow crescent of interesting behavior (feed 0.01-0.08, kill 0.04-0.07). Outside this zone, the system collapses to uniform or empty. When feedRate is modulated low, killRateBase must also be in range for patterns to survive. Audio on feedRate creates moments where patterns bloom (high energy) or thin to fragile tendrils (low energy), but only if killRateBase holds the system in the viable zone.

**Competing forces - couplingStrength vs warpIntensity:**
High coupling drives sharp territorial fronts (species aggressively chase each other). High warp dissolves boundaries (coordinates shift, smearing the fronts). The tension between these two creates dynamic turbulent borders: coupling builds structure, warp tears it apart. Modulating them from different sources produces moments of crystalline order (coupling wins) alternating with chaotic dissolution (warp wins).

**Resonance - feedRate + couplingStrength:**
When both spike simultaneously, species grow fast AND chase aggressively. The screen fills with rapid spiraling wavefronts. When both drop, the system enters a quiet near-equilibrium with slow migration. If both are modulated by correlated sources, rare coincident peaks create brief explosive spiral events.

## Notes

- The 25-step inner reaction loop is pure ALU work per pixel (no texture reads between iterations). Performance cost scales with reactionSteps but is bounded by fragment throughput, not memory bandwidth.
- Gray-Scott is sensitive to parameter ratios. The viable parameter crescent is narrow. UI should include a reset button since bad parameter combinations can collapse the system to a dead state.
- The 2-pass simulation (one per output texture) computes all 3 species in both passes but outputs different channels. This wastes some ALU. If performance is tight, a compute shader variant could write both textures in one dispatch - but start with the fragment shader approach for simplicity.
- Diffusion radii interact with resolution. The reference assumes ~800px. At higher resolutions the radii may need scaling, or expose as a "pattern scale" multiplier.
- The warp octave sequence uses a specific deterministic sequence (q *= -1.681, wrap, floor+1). Keep this verbatim from the reference - it produces the characteristic organic stirring.
