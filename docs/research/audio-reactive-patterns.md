# Audio Reactive Visual Patterns Research

Research on aesthetic techniques to complement AudioJones' circular waveform visualization.

## Current Implementation Summary

AudioJones renders up to 8 concurrent circular waveforms with physarum-style trail effects. The system captures 1024 audio samples at 20fps and applies separable Gaussian blur (5-tap kernel) with exponential decay (0.1–2.0s half-life). See `docs/architecture.md` for full system details.

**Existing capabilities:**
- Circular/linear waveform modes with cubic interpolation
- Per-waveform parameters: radius, amplitude, thickness, smoothness, rotation
- Solid or HSV rainbow gradient coloring
- Two-pass blur pipeline with ping-pong buffers
- Framerate-independent decay via half-life uniform

## Complementary Pattern Categories

### 1. Spectral/Frequency-Based Effects

**Radial Spectrum Bars**
Divide FFT output into 16–64 frequency bands. Render each band as a bar radiating outward from center. Low frequencies (bass) occupy inner rings; high frequencies extend to edges. Combine with existing waveforms as layered visualization.

Implementation: Add FFT pass to audio processing. Render bars before blur pass so they inherit trail effects. Use [audioMotion-analyzer](https://github.com/hvianna/audioMotion-analyzer) as reference for band splitting.

**Spectral Rings**
Stack 3–5 concentric rings, each responding to different frequency ranges. Inner ring pulses with sub-bass (20–60Hz), outer rings react to mids/highs. Creates depth without obscuring waveforms.

### 2. Beat-Reactive Effects

**Bloom Pulses**
Flash screen-wide bloom on detected beats. Increase blur kernel radius from 5-tap to 9-tap temporarily. Decay back to baseline over 100–200ms.

Implementation: Detect beats via amplitude threshold with 150ms debounce. Pass `beatIntensity` uniform to blur shaders. Scale blur radius: `radius = baseRadius + beatIntensity * 4`.

**Chromatic Aberration Kicks**
Split RGB channels outward on bass hits. Offset red channel +2px, blue channel -2px from center. Creates impact without new geometry.

Implementation: Add post-process pass after `blur_v.fs`. Sample texture 3x with per-channel UV offsets. Drive offset magnitude from low-frequency amplitude. Reference: [chromatic aberration tutorial](https://halisavakis.com/my-take-on-shaders-chromatic-aberration-introduction-to-image-effects-part-iv/).

**Shape Morphing**
Modulate waveform parameters on beats. Spike `amplitudeScale` by 1.5x, then decay exponentially. Pulse `thickness` from 2px to 6px. Creates organic "breathing" effect.

### 3. Particle Systems

**Trailing Particles**
Emit particles from waveform vertices on amplitude peaks. Particles inherit vertex color and velocity. Apply gravity + curl noise for organic motion. Fade over 0.5–2.0s.

Implementation: Store particle pool (256–1024 particles). Each particle: position, velocity, color, age. Update via compute pass or CPU. Render as points before blur pass. Reference: [Codrops particle tutorial](https://tympanus.net/codrops/2023/12/19/creating-audio-reactive-visuals-with-dynamic-particles-in-three-js/).

**Explosion Bursts**
On beat detection, spawn 50–100 particles from screen center. Radiate outward with velocity proportional to beat intensity. Particles dissolve via alpha fade or dissolve shader. Reference: [Minions Art particle shaders](https://www.patreon.com/posts/making-hit-with-28616038).

### 4. Geometric Patterns

**Kaleidoscope Mode**
Transform accumulated texture through polar coordinate mirroring. Divide circle into 4–12 radial segments, mirror alternating segments. Creates mandala effect from waveform trails.

Implementation: Add kaleidoscope pass after blur. Use `atan2(uv.y - 0.5, uv.x - 0.5)` for polar angle. Apply modulo and abs for segment mirroring. Simple fragment shader, ~20 lines. Reference: [shadertoy kaleidoscope](https://www.shadertoy.com/view/4f33Dl).

**Lissajous Overlay**
Plot left vs right stereo channels in XY mode. Creates organic looping curves that respond to stereo width and phase. Overlay on existing waveforms as secondary visualization.

Implementation: Sample left channel for X, right channel for Y. Scale to screen coordinates. Draw as line strip with alpha. Stereo audio already captured; requires minimal processing. Reference: [Lissajous Wikipedia](https://en.wikipedia.org/wiki/Lissajous_curve).

**Fractal Feedback**
Route output texture back as input with scale < 1.0 and rotation. Creates recursive zoom effect. Audio amplitude modulates zoom speed and rotation rate.

Implementation: Sample previous frame's texture with transformed UVs. Blend with current frame at 10–30% opacity. Requires additional render target. Reference: [MilkDrop feedback techniques](https://www.geisswerks.com/milkdrop/).

### 5. Organic/Simulation-Based

**Audio-Reactive Physarum Enhancement**
Current blur creates physarum-like trails. Enhance by adding agent-based simulation: spawn 10k–100k agents that follow waveform intensity gradients. Agents deposit color on movement, creating network patterns.

Implementation: Store agent positions in texture. Compute shader updates positions each frame. Agents sense nearby intensity and turn toward brightness. Reference: [physarum-audio-reactive](https://github.com/bu3nAmigue/physarum-audio-reactive), [Michael Fogleman's implementation](https://www.michaelfogleman.com/projects/physarum/).

**Metaball Centers**
Place 4–8 metaball centers at waveform peaks. Render via marching squares or signed distance field. Metaballs merge when peaks overlap, creating organic blob shapes. Audio amplitude drives metaball radius.

Implementation: Find local maxima in waveform samples. Use as metaball center positions. Compute SDF in fragment shader. Apply threshold for isosurface. Reference: [GPU Gems metaballs](https://developer.nvidia.com/gpugems/gpugems3/part-i-geometry/chapter-7-point-based-visualization-metaballs-gpu), [GameDev.net 2D metaballs](https://www.gamedev.net/tutorials/programming/graphics/exploring-metaballs-and-isosurfaces-in-2d-r2556/).

### 6. Post-Processing Effects

**Vignette Pulse**
Darken screen edges, brighten on beat. Creates focus/tunnel effect. Vignette intensity ranges from 0.3 (subtle) to 0.7 (dramatic).

**Scanlines/CRT Effect**
Overlay horizontal scanlines at 2–4px spacing. Modulate scanline brightness with audio amplitude for retro aesthetic. Optional phosphor glow simulation.

**Color Grading**
Apply LUT-based color transformation. Switch LUTs based on frequency content: warm palette for bass-heavy, cool palette for treble. Smooth interpolation between palettes over 500ms.

## Implementation Priority Matrix

| Pattern | Complexity | Visual Impact | Synergy with Existing |
|---------|------------|---------------|----------------------|
| Bloom Pulses | Low | High | Extends blur pipeline |
| Chromatic Aberration | Low | Medium | Post-process addition |
| Kaleidoscope Mode | Low | High | Uses existing trails |
| Lissajous Overlay | Low | Medium | Stereo data available |
| Spectral Rings | Medium | High | Parallel to waveforms |
| Beat Shape Morphing | Low | Medium | Modifies existing params |
| Trailing Particles | Medium | High | Needs particle system |
| Fractal Feedback | Medium | High | Needs extra render target |
| Physarum Agents | High | Very High | Major compute addition |
| Metaballs | High | Medium | Full SDF implementation |

## Shader Pipeline Extensions

Current pipeline:
```
accumTexture → blur_h → tempTexture → blur_v → accumTexture → screen
```

Extended pipeline with new effects:
```
accumTexture → blur_h → tempTexture → blur_v → accumTexture
                                                    ↓
                                              [kaleidoscope] (optional)
                                                    ↓
                                              [chromatic] (on beat)
                                                    ↓
                                              [bloom] (on beat)
                                                    ↓
                                                 screen
```

Each post-process pass adds ~0.5ms at 1080p. Total budget: 16.6ms at 60fps.

## Audio Analysis Extensions

**Current**: Raw 1024 samples, peak normalization, smoothing.

**Needed for new effects**:
- FFT: 1024-point real-to-complex transform. Produces 512 frequency bins. Use FFTW3, KissFFT, or compute shader.
- Beat detection: Track low-frequency (20–200Hz) energy. Compare to rolling average. Threshold crossings with debounce trigger beats.
- Stereo separation: Already captured but mixed to mono. Preserve L/R for Lissajous.

## Reference Implementations

| Project | Technique | Link |
|---------|-----------|------|
| MilkDrop 3 | Feedback, presets, HLSL shaders | [GitHub](https://github.com/milkdrop2077/MilkDrop3) |
| projectM | Cross-platform MilkDrop reimplementation | [GitHub](https://github.com/projectM-visualizer/projectm) |
| Synesthesia | Professional VJ software | [synesthesia.live](https://synesthesia.live/) |
| ÜBERVIZ | WebGL audio reactive demos | [uberviz.io](https://www.uberviz.io/) |
| Shader Park | Simplified 3D shader creation | [Codrops tutorial](https://tympanus.net/codrops/2023/02/07/audio-reactive-shaders-with-three-js-and-shader-park/) |
| Physarum Audio | WebGL slime mold + audio | [GitHub](https://github.com/bu3nAmigue/physarum-audio-reactive) |

## Open Questions

- **FFT library choice**: KissFFT (simple, header-only) vs compute shader (faster, more complex). Needs benchmarking.
- **Particle count limits**: CPU particles cap around 10k at 60fps. GPU particles via compute shader scale to 100k+. raylib lacks built-in compute; requires manual OpenGL.
- **Beat detection tuning**: Simple threshold works for electronic music. Complex tracks need onset detection algorithms. Needs user-adjustable sensitivity.

## Conclusion

Low-hanging fruit: bloom pulses, chromatic aberration, and kaleidoscope mode. Each requires one additional shader pass and minimal audio analysis. Medium-term: add FFT for spectral visualization and trailing particles. Long-term: full physarum agent simulation and metaball rendering require compute shader infrastructure.
