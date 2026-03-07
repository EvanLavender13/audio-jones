# Curl Advection Generator Reclassification

Refactor Curl Advection from a compute-shader simulation (`src/simulation/`) to a fragment-shader generator (`src/effects/`) using the `REGISTER_GENERATOR_FULL` pattern. The visual output is unchanged: flowing vein-like patterns that organically evolve and branch. The motivation is that Curl Advection is a per-texel field solver with no agents, no scatter writes, and no shared memory — it's a fragment shader in a compute shader costume.

## Classification

- **Category**: GENERATORS > Simulation (new subcategory, or Texture — user's call)
- **Pipeline Position**: Output stage, composited via BlendCompositor like all generators
- **Compute Model**: Fragment shader with ping-pong render textures (replaces compute shader + TrailMap)

## References

- Existing implementation: `src/simulation/curl_advection.cpp`, `shaders/curl_advection.glsl`
- Generator pattern: `src/effects/muons.cpp` (cleanest `REGISTER_GENERATOR_FULL` example)
- Ping-pong helpers: `src/effects/attractor_lines.cpp` (`InitPingPong` / `UnloadPingPong`)

## Reference Code

The existing compute shader (`shaders/curl_advection.glsl`) is the source of truth. Every operation is per-texel:

```glsl
// Core: 9-neighbor stencil for curl, divergence, laplacian
vec3 center = sampleState(uv);
vec3 uv_n   = sampleState(uv + vec2(0, texel.y));
// ... 7 more neighbors ...

float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + _D * (...);
float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + _D * (...);
vec3  lapl = _K0 * center + _K1 * (cardinals) + _K2 * (diagonals);

// Multi-step self-advection loop
for (int i = 0; i < steps; i++) {
    vec2 sampleUV = uv - off * texel;
    computeCurlAndBlur(sampleUV, texel, localCurl, localBlur);
    offd = rotate2d(offd, advectionCurl * localCurl);
    off += offd;
    ab += localBlur / float(steps);
}

// Field update: pressure, curl rotation, divergence feedback
vec2 rab = rotate2d(tab, curlScale * curl);
vec3 result = mix(vec3(rab, sd), center.xyz, updateSmoothing);

// Color: velocity angle → LUT, brightness = velocity magnitude
float angle = atan(result.y, result.x);
float t = (angle + PI) / (2.0 * PI);
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
```

No `imageAtomicAdd`, no `shared`, no `gl_GlobalInvocationID`-dependent scatter. Pure read-neighborhood, write-self.

## Algorithm

### What stays verbatim
- The entire PDE: curl/divergence/laplacian stencil, multi-step advection loop, field update equation, accumulation injection
- All uniforms and parameter semantics
- Color mapping: velocity angle → colorLUT → brightness-scaled output

### What changes

**Compute → two fragment shaders:**

1. **State update shader** (`.fs`): Reads `stateTexture` (the ping-pong read buffer), writes new velocity/divergence state to the ping-pong write buffer via render-to-texture. Replace `imageStore(stateOut, pos, ...)` with standard `gl_FragColor` output. Replace `gl_GlobalInvocationID.xy` UV math with `fragTexCoord`.

2. **Color output shader** (`.fs`): Reads the freshly-written state buffer, maps velocity angle through colorLUT, outputs colored pixels to the generator's visual ping-pong (with decay from previous frame). This pass also handles what TrailMap currently does — exponential decay blended with new color.

**TrailMap → built-in decay:**
- Current: separate TrailMap compute shader handles decay + diffusion
- New: the color output shader reads `previousFrame` (visual ping-pong read buffer) and blends `mix(newColor, previousColor * decayFactor, ...)` in one pass
- Diffusion (Gaussian blur of trails) can be dropped initially or implemented as an optional post-blur pass if needed

**Infrastructure swap:**
- Remove: `TrailMap*`, compute program, GL texture management, `CurlAdvectionSupported()` (no longer needs GL 4.3)
- Add: `RenderTexture2D statePingPong[2]` (RGBA16F, velocity state), `RenderTexture2D pingPong[2]` (visual output with decay)
- Registration: `REGISTER_GENERATOR_FULL` instead of `REGISTER_SIM_BOOST`

**File moves:**
- `src/simulation/curl_advection.cpp` → `src/effects/curl_advection.cpp`
- `src/simulation/curl_advection.h` → `src/effects/curl_advection.h`
- `shaders/curl_advection.glsl` (compute) → `shaders/curl_advection_state.fs` + `shaders/curl_advection_color.fs`
- Remove from simulation init/update/resize loops in `render_pipeline.cpp` and `post_effect.cpp`
- Add to effect descriptor table via registration macro

### Render callback flow (per frame)

1. **State pass**: `BeginTextureMode(statePingPong[writeIdx])` → draw fullscreen quad with state update shader reading `statePingPong[readIdx]` + `accumTexture` → `EndTextureMode()`
2. **Color pass**: `BeginTextureMode(pingPong[writeIdx])` → draw fullscreen quad with color shader reading `statePingPong[writeIdx]` (fresh state) + `pingPong[readIdx]` (previous visual, for decay) → `EndTextureMode()`
3. Flip both `stateReadIdx` and `readIdx`
4. Blend compositor composites `pingPong[readIdx]` onto the scene

## Parameters

All parameters remain identical — the config struct does not change.

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| steps | int | 10-80 | 40 | Advection iterations per frame |
| advectionCurl | float | 0.0-1.0 | 0.2 | How much paths spiral during advection |
| curlScale | float | -4.0-4.0 | -2.0 | Vortex rotation strength |
| laplacianScale | float | 0.0-0.2 | 0.05 | Diffusion/smoothing |
| pressureScale | float | -4.0-4.0 | -2.0 | Compression wave strength |
| divergenceScale | float | -1.0-1.0 | -0.4 | Source/sink strength |
| divergenceUpdate | float | -0.1-0.1 | -0.03 | Divergence feedback rate |
| divergenceSmoothing | float | 0.0-0.5 | 0.3 | Divergence smoothing |
| selfAmp | float | 0.5-2.0 | 1.0 | Self-amplification |
| updateSmoothing | float | 0.25-0.9 | 0.4 | Temporal stability |
| injectionIntensity | float | 0.0-1.0 | 0.0 | Energy injection from accumulation |
| injectionThreshold | float | 0.0-1.0 | 0.1 | Accumulation brightness cutoff |
| decayHalfLife | float | 0.1-5.0 | 0.5 | Trail decay half-life (seconds) |
| diffusionScale | int | 0-4 | 0 | Trail diffusion passes (may be dropped) |
| boostIntensity | float | 0.0-5.0 | 1.0 | Trail boost / blend intensity |
| blendMode | enum | — | SCREEN | Blend compositor mode |
| color | ColorConfig | — | — | Color mode (solid/gradient/palette/rainbow) |

## Modulation Candidates

All existing modulation registrations carry over unchanged. Same param IDs, same ranges.

- **advectionCurl**: spiral tightness of flow paths
- **curlScale**: rotation strength — sign flip reverses vortex direction
- **selfAmp**: energy amplification — higher values make the field more active
- **pressureScale + divergenceScale**: competing forces — pressure pushes outward, divergence creates sinks. Their ratio determines whether patterns expand or collapse.
- **injectionIntensity**: gates energy input from the visual scene — only active when accumulation has bright content
- **decayHalfLife**: trail persistence — short = crisp lines, long = smeared rivers

### Interaction Patterns

**Competing forces (pressureScale vs divergenceScale):** These push in opposite directions. Pressure creates expansion waves; divergence creates collapse points. When both are modulated by different audio bands, loud moments push patterns outward while bass pulls them inward — the field breathes with the music.

**Cascading threshold (injectionIntensity gated by injectionThreshold):** Energy injection only activates when the accumulation texture has bright content above the threshold. In quiet passages nothing injects; during loud visual moments the field receives energy bursts that propagate as new flow structures.

## Notes

- **GL version**: Drops from OpenGL 4.3 requirement to 3.3. Broader hardware compatibility.
- **Performance**: The advection loop (up to 80 iterations × 9 texture reads each) is identical work in fragment vs compute. No performance regression expected — compute had no cache advantage here (no shared memory usage).
- **4 render textures**: Two RGBA16F for state + two for visual output. ~32 bytes/pixel × 4 textures at 1080p ≈ 250 MB. Same total memory as current implementation (2 GL textures + TrailMap).
- **Diffusion**: The current TrailMap diffusion (separable Gaussian blur passes) may be dropped initially. If needed, it can be added as an optional blur pass between the color output and compositing, or folded into the color shader as a small kernel sample.
- **Preset compatibility**: The config struct and field names stay identical, so existing presets load without migration.
- **Simulation infrastructure cleanup**: After this and a Cymatics reclassification, the remaining simulations (Physarum, Curl Flow, Attractor Flow, Boids, Particle Life) are all genuinely agent-based with SSBOs and scatter writes. The simulation/generator boundary becomes clean.
