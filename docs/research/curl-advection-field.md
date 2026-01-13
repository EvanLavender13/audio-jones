# Curl-Advection Field

A self-advecting vector field simulation that creates organic veiny and vortical patterns through competing differential operators. The field advects itself while curl creates rotation, divergence creates sources/sinks, and laplacian diffuses - the balance between these forces produces emergent structure.

## Classification

- **Category**: SIMULATIONS (new simulation type)
- **Core Operation**: Vector field self-advection with curl rotation, divergence feedback, laplacian diffusion
- **Pipeline Position**: Same as Physarum - generates to texture, composites to output

## References

- [GPU Gems: Fast Fluid Dynamics](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu) - Advection, curl, divergence fundamentals
- [WebGL Fluid: Pressure Solving](https://ostefani.dev/tech-notes/webgl-fluid-divergence-pressure) - Divergence correction techniques
- [Bitangent Noise](https://atyuwen.github.io/posts/bitangent-noise/) - Divergence-free field generation
- User-provided Shadertoy shader source

## Algorithm

Single compute pass with multi-step advection. Much simpler than full Navier-Stokes - no pressure solve, no incompressibility constraint.

### Differential Operators (3x3 Stencil)

All operators computed from 9 neighbor samples with diagonal weighting:

```glsl
// Stencil weights
#define _D 0.6      // diagonal contribution
#define _K0 -20.0/6.0  // laplacian center
#define _K1 4.0/6.0    // laplacian edges
#define _K2 1.0/6.0    // laplacian corners
#define _G0 0.25       // gaussian center
#define _G1 0.125      // gaussian edges
#define _G2 0.0625     // gaussian corners

// Curl (rotation/vorticity): ∂v/∂x - ∂u/∂y
curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y
     + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y
           + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);

// Divergence (expansion/compression): ∂u/∂x + ∂v/∂y
div = uv_s.y - uv_n.y - uv_e.x + uv_w.x
    + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y
          + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);

// Laplacian (diffusion): ∇²field
lapl = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s)
              + _K2*(uv_nw + uv_sw + uv_ne + uv_se);

// Gaussian blur (smoothing)
blur = _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s)
              + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
```

### Multi-Step Self-Advection

The field traces its own flow lines, accumulating blur along the path:

```glsl
#define STEPS 40

vec2 off = uv.xy;      // Current offset
vec2 offd = off;       // Direction (rotated by curl)
vec3 ab = vec3(0);     // Accumulated blur

for (int i = 0; i < STEPS; i++) {
    advect(off, ...);              // Sample at offset position
    offd = rotate(offd, ts*curl);  // Curl twists the path
    off += offd;                   // Step along twisted path
    ab += blur / float(STEPS);     // Accumulate
}
```

This creates streaky, flowing patterns as values spread along curved paths.

### Field Update

Combines all forces into final update:

```glsl
// Pressure-like term from laplacian of divergence
float sp = ps * lapl.z;

// Curl rotation scale
float sc = cs * curl;

// Divergence with feedback and smoothing
float sd = uv.z + dp * div + pl * lapl.z;

// Combine: self-amplification + diffusion + pressure + divergence damping
vec2 tab = amp * ab.xy      // Self-amplification of advected field
         + ls * lapl.xy     // Laplacian diffusion
         + norm * sp        // Pressure gradient
         + uv.xy * ds * sd; // Divergence feedback

// Apply curl rotation to final update
vec2 rab = rotate(tab, sc);

// Blend with previous frame
vec3 abd = mix(vec3(rab, sd), uv, upd);
```

## Parameters

| Parameter | Default | Range | Effect |
|-----------|---------|-------|--------|
| `STEPS` | 40 | 10-80 | Advection iterations (cost scales linearly) |
| `ts` | 0.2 | 0.0-1.0 | Advection curl - how much paths spiral |
| `cs` | -2.0 | -4.0 to 4.0 | Curl scale - vortex rotation strength |
| `ls` | 0.05 | 0.0-0.2 | Laplacian scale - diffusion/smoothing |
| `ps` | -2.0 | -4.0 to 4.0 | Pressure scale - compression waves |
| `ds` | -0.4 | -1.0 to 1.0 | Divergence scale - source/sink strength |
| `dp` | -0.03 | -0.1 to 0.1 | Divergence update - feedback rate |
| `pl` | 0.3 | 0.0-1.0 | Divergence smoothing |
| `amp` | 1.0 | 0.5-2.0 | Self-amplification - instability driver |
| `upd` | 0.4 | 0.1-0.9 | Update smoothing - temporal stability |

### Parameter Interactions

The emergent behavior comes from **competing forces**:

| Force | Creates | Opposed By |
|-------|---------|------------|
| Curl rotation (`cs`) | Vortices, spirals | Diffusion (`ls`) |
| Self-amplification (`amp`) | Growth, instability | Update smoothing (`upd`) |
| Divergence (`ds`) | Flow sources/sinks | Divergence smoothing (`pl`) |
| Pressure (`ps`) | Compression waves | Laplacian diffusion (`ls`) |

**Regime examples:**
- High `cs`, low `ls` → Tight vortices
- High `amp`, low `upd` → Chaotic growth
- High `ds`, high `pl` → Smooth flowing streams
- Balanced → Organic veiny networks

## Performance Analysis

| Component | Cost | Notes |
|-----------|------|-------|
| Stencil (9 samples) | Cheap | Single texture fetch per neighbor |
| Advection loop | O(STEPS) | 40 iterations × 9 samples = 360 fetches |
| Field update | Cheap | ALU only, no additional fetches |
| **Total per pixel** | ~400 samples | Tunable via STEPS |

**Comparison:**
- Kernel Flow Transport: 676 samples (fixed)
- Curl-Advection (40 steps): ~400 samples (tunable)
- Curl-Advection (20 steps): ~200 samples
- Physarum: O(1) per pixel, O(agents) compute

Reducing STEPS to 15-25 provides good visuals at ~150-250 samples/pixel.

## Compatibility Assessment

**Compatible** with AudioJones pipeline:

1. **Single buffer**: Only needs one RGBA texture (xy = velocity, z = divergence scalar)
2. **Fragment shader**: No compute required, runs as post-process
3. **Self-contained**: No external dependencies, initializes from noise

**Implementation options:**

| Approach | Buffers | Integration |
|----------|---------|-------------|
| Standalone simulation | 1 (ping-pong) | New simulation like Physarum |
| Transform effect | 1 (ping-pong) | Runs on feedback texture |
| Hybrid | 2 | Simulate separately, composite |

**Recommended**: Standalone simulation with trail map output, same architecture as Physarum.

## Audio Mapping Ideas

| Parameter | Audio Source | Effect |
|-----------|--------------|--------|
| `amp` | Bass energy | Louder = more chaotic growth |
| `cs` | Mid frequency | Controls vortex tightness |
| `STEPS` | Treble | More detail at high frequencies |
| `ts` | Beat detection | Pulse spiral tightness on beats |
| Injection | FFT peaks | Add velocity at frequency positions |

### Injection Method

Mouse injection in original shader:
```glsl
if (iMouse.z > 0.0) {
    vec2 d = (fragCoord.xy - iMouse.xy) / iResolution.x;
    vec2 m = 0.1 * normalize(d) * exp(-length(d) / 0.02);
    abd.xy += m;
}
```

Replace mouse with audio-reactive injection points (beat positions, FFT peaks, waveform samples).

## Rendering

Original uses simple sinusoidal color mapping:
```glsl
fragColor = sin(a.x*4.0 + vec4(1,3,5,4)) * 0.25
          + sin(a.y*4.0 + vec4(1,3,2,4)) * 0.25
          + 0.5;
```

Better options for AudioJones:
- Output velocity magnitude as grayscale to trail map
- Use existing color config (hue from velocity angle)
- Heightfield shading from divergence channel (z)

## Comparison to Other Simulations

| Aspect | Physarum | Kernel Flow | Curl-Advection |
|--------|----------|-------------|----------------|
| Agents | Yes | No | No |
| Stencil size | 3-point | 13×13 | 3×3 |
| Advection | Agent movement | None | Multi-step self |
| Pattern variety | High | Low | High |
| Physical basis | Chemotaxis | Transport | Fluid-like |
| Cost | O(agents) | O(pixels×169) | O(pixels×STEPS×9) |
| Tunable cost | Agent count | No | STEPS |

## Implementation Notes

1. **Initialization**: Seed with noise texture, not zeros
2. **Clamping**: Clamp velocity magnitude to 1.0, divergence to [-1,1]
3. **Boundary**: Wrap UV coordinates (toroidal topology)
4. **Precision**: RGBA16F sufficient, RGBA32F preferred
5. **Resolution**: Can run at half-resolution, upscale result

## Sources

- GPU Gems Chapter 38 (NVIDIA) - Fluid dynamics fundamentals
- User-provided Shadertoy shader
- WebGL fluid simulation references
