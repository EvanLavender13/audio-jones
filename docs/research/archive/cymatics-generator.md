# Cymatics Generator Reclassification

Refactor Cymatics from a compute-shader simulation (`src/simulation/`) to a fragment-shader generator (`src/effects/`) using the `REGISTER_GENERATOR_FULL` pattern. The visual output is unchanged: vibrating-plate interference patterns responding to audio. The motivation is identical to the Curl Advection reclassification — Cymatics is a pure per-pixel function with no agents, no scatter writes, no shared memory, and no simulation state. It evaluates wave interference from source positions and audio data, writes color, and is done.

## Classification

- **Category**: GENERATORS > Texture (or a new Simulation subcategory — user's call at plan time)
- **Pipeline Position**: Output stage, composited via BlendCompositor like all generators
- **Compute Model**: Single fragment shader (replaces compute shader + TrailMap)

## References

- Existing implementation: `src/simulation/cymatics.cpp`, `shaders/cymatics.glsl`
- Generator pattern: `src/effects/muons.cpp` (cleanest `REGISTER_GENERATOR_FULL` example)

## Reference Code

The existing compute shader (`shaders/cymatics.glsl`) is the source of truth. It is a pure function of current-frame inputs:

```glsl
// Fetch waveform with linear interpolation at ring buffer offset
float fetchWaveform(float delay) {
    delay = min(delay, float(bufferSize) * 0.9);
    float idx = mod(float(writeIndex) - delay + float(bufferSize), float(bufferSize));
    float u = clamp(idx / float(bufferSize), 0.001, 0.999);
    float val = texture(waveformBuffer, vec2(u, 0.5)).r;
    return val * 2.0 - 1.0;
}

void main() {
    // Map pixel to centered, aspect-corrected coordinates
    vec2 uv = (vec2(pos) + 0.5) / resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspect;

    // Sum interference from all sources with Gaussian falloff
    float totalWave = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        float dist = length(uv - sourcePos);
        float delay = dist * waveScale;
        float attenuation = exp(-dist * dist * falloff);
        totalWave += fetchWaveform(delay) * attenuation;

        // Optional mirror sources for boundary reflections (4 per source)
        if (boundaries) { ... }
    }

    // Optional contour banding, tanh compression, LUT color mapping
    float intensity = tanh(wave * visualGain);
    float t = intensity * 0.5 + 0.5;
    vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(intensity) * value;

    // Trail persistence: keep brighter of old and new
    vec4 newColor = vec4(color * brightness, brightness);
    vec4 existing = imageLoad(trailMap, pos);
    vec4 blended = max(existing, newColor);
    imageStore(trailMap, pos, blended);
}
```

No `atomicAdd`, no `shared`, no persistent state textures. Pure read-uniforms-and-textures, write-self.

## Algorithm

### What stays verbatim
- Wave interference summation: source distance → waveform delay → Gaussian attenuation
- Boundary reflections: 4 mirror sources per real source
- Contour banding, tanh compression, colorLUT mapping
- Trail persistence via `max(existing, newColor)`

### What changes

**Compute → single fragment shader:**

The entire compute shader becomes a fragment shader (`.fs`). Replace:
- `gl_GlobalInvocationID.xy` → `fragTexCoord` (already centered in the shader)
- `imageStore(trailMap, pos, blended)` → `gl_FragColor = blended`
- `imageLoad(trailMap, pos)` → `texture(previousFrame, fragTexCoord)` (reading ping-pong read buffer)

**TrailMap → built-in persistence:**
- Current: compute shader writes to TrailMap, then a separate TrailMap compute pass applies decay + diffusion
- New: fragment shader reads `previousFrame` (ping-pong read buffer), applies exponential decay (`previousFrame * decayFactor`), then takes `max(decayed, newColor)` — all in one pass
- Diffusion (Gaussian blur of trails) can be dropped initially or added as an optional post-blur pass

**Infrastructure swap:**
- Remove: `TrailMap*`, compute program, GL image binding, `CymaticsSupported()` (no longer needs GL 4.3)
- Add: `RenderTexture2D pingPong[2]` (visual output with persistence)
- Registration: `REGISTER_GENERATOR_FULL` instead of `REGISTER_SIM_BOOST`
- No state textures needed (unlike Curl Advection) — Cymatics has no persistent simulation state

**Waveform texture access:**
- Currently passed as `Texture2D waveformTexture` to `CymaticsUpdate()`
- As a generator, the Setup function needs to bind the waveform texture and write index as uniforms
- The `PostEffect` struct already carries audio resources — the setup function reads them from there

**Source position computation:**
- Currently computed on CPU in `CymaticsUpdate()` via `DualLissajousUpdateCircular()`
- As a generator, this moves to the Setup function. Source positions are uploaded as a uniform array (same as now).

**File moves:**
- `src/simulation/cymatics.cpp` → `src/effects/cymatics.cpp`
- `src/simulation/cymatics.h` → `src/effects/cymatics.h`
- `shaders/cymatics.glsl` (compute) → `shaders/cymatics.fs` (fragment)
- Remove from simulation init/update/resize loops in `render_pipeline.cpp` and `post_effect.cpp`
- Add to effect descriptor table via registration macro

### Render callback flow (per frame)

1. Setup function: compute source positions via `DualLissajousUpdateCircular()`, bind all uniforms including waveform texture + write index
2. **Single pass**: `BeginTextureMode(pingPong[writeIdx])` → draw fullscreen quad with cymatics fragment shader reading `pingPong[readIdx]` (previous visual, for decay + max blend) + `waveformTexture` (audio) → `EndTextureMode()`
3. Flip `readIdx`
4. Blend compositor composites `pingPong[readIdx]` onto the scene

Simpler than Curl Advection — one pass instead of two, no state textures.

## Parameters

All parameters remain identical — the config struct does not change.

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| waveScale | float | 1-50 | 10.0 | Pattern scale (higher = larger patterns) |
| falloff | float | 0-5 | 1.0 | Distance attenuation |
| visualGain | float | 0.5-5 | 2.0 | Output intensity |
| contourCount | int | 0-10 | 0 | Banding (0 = smooth) |
| sourceCount | int | 1-8 | 5 | Number of wave sources |
| baseRadius | float | 0.0-0.5 | 0.4 | Source orbit radius from center |
| lissajous | DualLissajousConfig | — | — | Source motion pattern |
| boundaries | bool | — | false | Enable edge reflections |
| reflectionGain | float | 0.0-1.0 | 1.0 | Mirror source attenuation |
| decayHalfLife | float | 0.1-5.0 | 0.5 | Trail persistence (seconds) |
| diffusionScale | int | 0-4 | 1 | Trail diffusion passes (may be dropped) |
| boostIntensity | float | 0.0-5.0 | 1.0 | Trail boost / blend intensity |
| blendMode | enum | — | SCREEN | Blend compositor mode |
| color | ColorConfig | — | — | Color mode (solid/gradient/palette/rainbow) |

## Modulation Candidates

All existing modulation registrations carry over unchanged. Same param IDs, same ranges.

- **waveScale**: pattern density — low values = tight ripples, high = broad standing waves
- **falloff**: attenuation sharpness — low = patterns fill screen, high = localized near sources
- **visualGain**: intensity drive — pushes into tanh compression for harder edges
- **baseRadius**: source orbit size — small = central cluster, large = sources near edges
- **reflectionGain**: boundary echo strength — creates standing wave patterns at edges

### Interaction Patterns

**Resonance (waveScale × sourceCount):** More sources create denser interference nodes. When waveScale is modulated alongside sourceCount changes, patterns can lock into standing-wave configurations at certain ratios and dissolve into chaos at others — brief moments of crystalline order.

**Competing forces (falloff vs baseRadius):** High falloff localizes patterns near sources. Large baseRadius pushes sources toward edges. Together they determine whether the center of the screen is a calm void or an interference hotspot — the visual weight shifts between center and periphery.

## Notes

- **GL version**: Drops from OpenGL 4.3 requirement to 3.3. Broader hardware compatibility.
- **Performance**: Simpler than Curl Advection — no multi-step advection loop, no neighborhood stencil. The main cost is the source loop (up to 8 sources × 5 mirror reflections = 40 waveform fetches). Trivial for a fragment shader.
- **2 render textures**: Only the visual ping-pong pair. No state textures needed. Lighter than Curl Advection.
- **Preset compatibility**: Config struct and field names stay identical, so existing presets load without migration.
- **Waveform texture plumbing**: The setup function needs access to the waveform ring buffer texture and current write index from the audio pipeline. Verify `PostEffect` or `RenderPipeline` exposes these before implementation.
- **Simulation infrastructure cleanup**: After both reclassifications (Curl Advection + Cymatics), the remaining simulations (Physarum, Curl Flow, Attractor Flow, Boids, Particle Life) are all genuinely agent-based. Clean boundary.
