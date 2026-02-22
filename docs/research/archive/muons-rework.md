# Muons Rework

Add trail persistence, FFT audio reactivity, and fix blown-out pixels. Transforms Muons from a flickery one-trick procedural animation into a responsive, lingering volumetric display where different frequency bands illuminate different depths.

## Classification

- **Category**: GENERATORS > Filament (existing effect, rework)
- **Pipeline Position**: Generator stage (unchanged)
- **Change Scope**: Upgrade from `REGISTER_GENERATOR` to `REGISTER_GENERATOR_FULL` (ping-pong trail persistence)

## References

- `src/effects/attractor_lines.cpp` — Ping-pong trail persistence pattern with `decayHalfLife`, sized init, render, resize
- `src/effects/fireworks.cpp` — Same ping-pong pattern, `decayFactor = expf(-0.693147f * deltaTime / halfLife)`
- `src/effects/motherboard.cpp` — FFT depth-layer pattern where march/layer index maps to log-spaced frequency bins
- `src/render/render_utils.h` — `RenderUtilsInitTextureHDR`, `RenderUtilsDrawFullscreenQuad`, `RenderUtilsClearTexture`

## Reference Code

All reference code lives in the codebase files listed above. The three patterns to combine:

### Trail persistence (from attractor_lines.fs)
```glsl
// Shader: read previous frame, decay, composite new content
vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
finalColor = vec4(max(color * c, prev * decayFactor), 1.0);
```

### Decay factor computation (from attractor_lines.cpp)
```cpp
// C++: framerate-independent exponential decay
const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
```

### FFT depth-layer sampling (from motherboard.fs pattern)
```glsl
// Map loop index to log-spaced frequency, sample FFT texture
float t = float(i) / float(marchSteps - 1);
float freq = baseFreq * exp(t * log(maxFreq / baseFreq));
float bin = freq / (sampleRate * 0.5);
float fft = texture(fftTexture, vec2(bin, 0.5)).r;
float audio = baseBright + gain * pow(fft, curve);
```

## Algorithm

Three changes to the existing Muons implementation:

### 1. Trail Persistence (C++ side)
Upgrade to `REGISTER_GENERATOR_FULL` pattern:
- Add `RenderTexture2D pingPong[2]` and `int readIdx` to `MuonsEffect`
- Sized init: allocate ping-pong HDR textures at screen dimensions
- Render function: ping-pong pass (BeginTextureMode on write, bind previousFrame from read, swap)
- Resize function: reallocate ping-pong textures
- Blend compositor reads from `pingPong[readIdx].texture` instead of `generatorScratch`
- Add `decayHalfLife` config param, compute `decayFactor` on CPU, send as uniform
- Add `previousFrame` and `decayFactor` uniforms to shader

### 2. Trail Persistence (shader side)
After computing the raymarched color for the current frame:
```
vec3 prev = texelFetch(previousFrame, ivec2(gl_FragCoord.xy), 0).rgb;
finalColor = vec4(max(newColor, prev * decayFactor), 1.0);
```
Uses `max()` compositing (same as Attractor Lines) — new bright content stamps over decaying trails without additive blowout.

### 3. FFT Audio Reactivity (shader side)
Each march step maps to a log-spaced frequency bin:
- Step `i` out of `marchSteps` maps to frequency between `baseFreq` and `maxFreq` in log space
- Sample `fftTexture` at that bin position
- Scale the step's color contribution by `baseBright + gain * pow(fft, curve)`
- Near shells (low step index) respond to bass, far shells (high index) respond to treble

New uniforms: `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`

### 4. Blown Pixel Fix (shader side)
Current accumulation `color += sampleColor / max(d * s, 1e-6)` explodes when rays graze ring surfaces.
Fix: raise the floor on the divisor to prevent extreme spikes:
```
color += sampleColor / max(d * s, 0.01);
```
The exact floor value may need tuning — `0.01` prevents the worst spikes while preserving the glow character. The `max()` compositing in the trail pass also helps because it doesn't stack infinitely like additive would.

## Parameters

### New params

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| decayHalfLife | float | 0.1-10.0 | 2.0 | Trail persistence duration in seconds |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity multiplier |
| curve | float | 0.1-3.0 | 1.0 | FFT contrast curve exponent |
| baseBright | float | 0.0-1.0 | 0.1 | Minimum brightness floor when audio is silent |

### Existing params (unchanged)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 4-40 | 10 | Trail density / depth layers (also determines FFT band count) |
| turbulenceOctaves | int | 1-12 | 9 | Path complexity |
| turbulenceStrength | float | 0.0-2.0 | 1.0 | FBM displacement amplitude |
| ringThickness | float | 0.005-0.1 | 0.03 | Wire gauge of trails |
| cameraDistance | float | 3.0-20.0 | 9.0 | Depth into volume |
| colorFreq | float | 0.5-50.0 | 33.0 | Color cycles along ray depth |
| colorSpeed | float | 0.0-2.0 | 0.5 | LUT scroll rate |
| brightness | float | 0.1-5.0 | 1.0 | Intensity multiplier before tonemap |
| exposure | float | 500-10000 | 3000.0 | Tonemap divisor |

## Modulation Candidates

- **decayHalfLife**: Short = crisp responsive trails, long = dreamy lingering glow
- **turbulenceStrength**: Smooth paths vs chaotic tangles
- **cameraDistance**: Zoom in/out through the volume
- **ringThickness**: Thin precise filaments vs thick glowing tubes
- **gain**: Overall audio sensitivity
- **baseBright**: Silent-floor glow level — modulate down to zero for true silence = darkness
- **brightness**: Post-accumulation intensity

### Interaction Patterns

**Cascading threshold — marchSteps x gain x baseBright**: With `baseBright` near zero and moderate `gain`, only frequency bands with real energy produce visible shells. Quiet bands stay dark. This means verse/chorus produce structurally different images — bass-heavy sections light up near shells while cymbal fills light up the deep volume. `marchSteps` controls how many frequency bands participate (more steps = finer spectral resolution, fewer = broader bands).

**Competing forces — decayHalfLife x turbulenceStrength**: Long trails with high turbulence create dense overlapping tangles that fill the screen. Short trails with low turbulence produce clean, sparse filament traces. The tension between persistence and chaos controls visual density without a dedicated "density" knob.

## Notes

- `marchSteps` serves double duty: visual density AND FFT frequency resolution. More steps = more filaments AND finer frequency bands. This is an intentional synergy, not a conflict.
- The blown pixel fix (`max(d * s, 0.01)` floor) should be tuned visually. Too aggressive kills the glow; too permissive still spikes. The trail `max()` compositing provides a second safety net.
- HDR ping-pong textures (RGBA16F) prevent banding in the decay trails over many frames.
- No audio section in existing Muons UI — will need a new "Audio" section following standard conventions (Base Freq, Max Freq, Gain, Contrast, Base Bright).
