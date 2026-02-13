# Attractor Lines Multi-Particle FFT

12 particle traces on the same attractor, one per semitone. Active notes make their particle's trail glow bright while quiet semitones fade to near-invisible. The attractor shape reveals itself note-by-note — chords light up more of the structure, and the full shape only appears when all 12 chromatic pitches are sounding. Color comes from velocity (existing speed-based gradient), not per-particle assignment.

## Classification

- **Category**: GENERATORS > Filament (enhancement to existing Attractor Lines)
- **Pipeline Position**: Generator stage, same as current attractor_lines
- **Compute Model**: N/A — fragment shader with ping-pong state persistence (existing pattern)

## References

- Existing codebase: `shaders/attractor_lines.fs` — single-particle state at pixel (0,0), ping-pong trail accumulation, speed-based gradient coloring
- Existing codebase: `shaders/spectral_arcs.fs` lines 50-78 — per-semitone FFT lookup pattern
- User-provided Shadertoy shader — multi-particle Thomas attractor with row-0 pixel state persistence (16 particles, each stored at a different x-pixel in row 0)

## Reference Code

### Current single-particle state persistence (attractor_lines.fs)

```glsl
// Read single particle state from pixel (0,0)
vec3 last = texelFetch(previousFrame, ivec2(0, 0), 0).xyz;
if (dot(last, last) < 1e-10 || any(isnan(last)))
    last = getStartingPoint(attractorType);

// ... integration loop over numSteps ...

// Write state back to pixel (0,0)
if (gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0) {
    finalColor = vec4(next, 1.0);
    return;
}

// Speed-based coloring
float speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0);
vec3 color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb;
```

### Shadertoy multi-particle state persistence (user-provided)

```glsl
#define NUM_PARTICLES 16

// Read each particle's state from its own pixel in row 0
for (int pid = 0; pid < NUM_PARTICLES; pid++) {
    vec3 pos = texelFetch(iChannel0, ivec2(pid, 0), 0).xyz;
    for (float i = 0.0; i < STEPS; i++) {
        vec3 next = integrate(pos, dt);
        vec2 a = project(pos, cy, sy, cp, sp) * VIEW_SCALE;
        vec2 b = project(next, cy, sy, cp, sp) * VIEW_SCALE;
        float segD = dfLine(a, b, uv);
        if (segD < d) { d = segD; bestSpeed = length(next - pos) / dt; }
        pos = next;
    }
}

// Write particle states back to row 0
if (py == 0 && px < NUM_PARTICLES) {
    if (iFrame == 0) {
        float angle = float(px) * 6.28318 / float(NUM_PARTICLES);
        float r = 1.5;
        fragColor = vec4(r * cos(angle), r * sin(angle), r * sin(angle * 0.7 + 1.0), 0.0);
    } else {
        vec3 pos = texelFetch(iChannel0, ivec2(px, 0), 0).xyz;
        for (float i = 0.0; i < STEPS; i++) { pos = integrate(pos, dt); }
        fragColor = vec4(pos, 0.0);
    }
}
```

### Spectral arcs FFT semitone sampling pattern (spectral_arcs.fs)

```glsl
// Per-semitone FFT lookup: index -> frequency -> normalized bin
float freq = baseFreq * pow(2.0, float(i) / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = 0.0;
if (bin <= 1.0) {
    mag = texture(fftTexture, vec2(bin, 0.5)).r;
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
}
```

## Algorithm

Enhancement to existing `attractor_lines.fs`. Key changes:

### State persistence: 1 pixel → 12 pixels in row 0

- Pixels `(0..11, 0)` each store one particle's 3D position
- On reset (first frame or attractor type change), seed each particle at a different position spread around a circle of radius 1.5 in the attractor's domain (same technique as the Shadertoy reference)
- The current `gl_FragCoord.x < 1.0 && gl_FragCoord.y < 1.0` guard becomes `gl_FragCoord.y < 1.0 && gl_FragCoord.x < 12.0`
- Each state pixel integrates its own particle forward by numSteps using the existing RK4 integrator

### Distance field: single loop → nested particle × step loop

- Outer loop over 12 particles, inner loop over numSteps (same as current)
- Each particle tracks closest segment distance independently, but global closest-wins determines `bestSpeed` for gradient coloring (same as current speed-based approach)
- The winning particle index is also tracked to determine which semitone's FFT energy to use for brightness

### FFT-gated brightness

- For the winning particle (closest segment), compute its semitone's FFT energy using the spectral_arcs pattern: `freq = baseFreq * pow(2.0, float(winnerIdx) / 12.0)`, sample `fftTexture`, apply `gain`/`curve`
- Multiply the line intensity `c` by `(baseBright + mag)` — with baseBright=0 quiet notes are invisible, with baseBright>0 they show as a dim scaffold
- With `numOctaves > 1`, sum FFT energy across all octaves of that pitch class (same stacking spectral_arcs uses)

### Coloring: speed-based gradient (unchanged)

- Keep existing `speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0)` → `gradientLUT` lookup
- Speed-based coloring provides natural color variation that reflects attractor dynamics (fast regions vs slow regions) without the noise problem of per-particle color assignment
- FFT only modulates brightness, not color

### New uniforms needed

- `fftTexture` (sampler2D) — already available in the generator pipeline
- `sampleRate` (float) — needed for freq→bin conversion
- `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright` — standard FFT audio param set

### Performance note

- 12 particles × N steps means 12× more segment distance evaluations per pixel
- The default `steps` may want to decrease from 96 to ~48 to keep total work similar
- Total segments: 12 × 48 = 576 vs current 1 × 96 = 96 — roughly 6× more work but each segment is cheap (just a point-to-line distance)

## Parameters

Existing parameters unchanged. New additions:

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| numOctaves | int | 1-8 | 4 | Number of octaves — FFT energy for each particle sums across this many octaves of its pitch class |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest frequency mapped to first semitone |
| gain | float | 0.1-10.0 | 3.0 | FFT magnitude amplification |
| curve | float | 0.1-3.0 | 1.0 | FFT magnitude contrast shaping |
| baseBright | float | 0.0-1.0 | 0.05 | Minimum brightness for quiet semitones |

## Modulation Candidates

- **baseBright**: gates visibility — at 0 only active notes appear, raised shows full scaffold
- **gain**: sensitivity threshold — low gain means only loud notes punch through
- **viewScale**: zoom level — combined with baseBright creates "zoomed-in detail during quiet, pulled-back overview during loud"
- **intensity**: overall brightness ceiling
- **speed**: integration rate — affects trail character (slow = smooth curves, fast = jumpy)
- **decayHalfLife**: trail persistence — short = crisp lines, long = built-up glow

### Interaction Patterns

**baseBright × gain (cascading threshold)**: With baseBright near 0, gain controls how loud a note must be before its particle appears at all. Low gain + low baseBright = only the loudest notes visible (sparse, punchy). High gain + low baseBright = even quiet notes flicker into view. High baseBright = all 12 particles always visible regardless of gain. This creates verse-vs-chorus dynamics — quiet passages show 2-3 threads, loud sections light up all 12.

**speed × decayHalfLife (competing forces)**: Speed pushes particles forward (longer segments per frame), decay erases old segments. When both are high, you get long bright trails that fade quickly — a comet look. When speed is low but decay is long, short segments pile up into dense glowing regions. The balance determines whether the attractor looks like racing fibers or a slowly-revealed sculpture.

## Notes

- The 12-particle count is hardcoded (one per chromatic pitch class). With `numOctaves > 1`, multiple octaves of the same pitch class are summed into one particle's brightness — this is the same stacking that spectral_arcs uses.
- The starting positions spread around a circle ensure Thomas (and other symmetric attractors) populate all lobes from frame 1.
- When the attractor type changes, all 12 particle states must reset — the existing `lastType` change detection in the C++ side already clears the ping-pong textures.
- The inner loop count (steps × 12 particles) is the main performance knob. If too slow, reduce default steps.
