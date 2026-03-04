# Byzantine Buffering

Ornate labyrinthine patterns emerge from a dead-simple trick: alternate between diffusion and sharpening every frame. The field evolves continuously via periodic zoom reseeds that recycle the system's own output as perturbation. The result is an infinite fractal zoom through self-generating reaction-diffusion textures.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage (after trail boost, blended via compositor)
- **Compute Model**: Fragment shader ping-pong (two render textures)

## Attribution

- **Based on**: "Byzantine Buffering" by paniq
- **Source**: https://www.shadertoy.com/view/7tBGz1
- **License**: CC BY-NC-SA 3.0

## References

- [Byzantine Buffering](https://www.shadertoy.com/view/7tBGz1) - Original Shadertoy shader by paniq (source code below)

## Reference Code

### Common

```glsl
const int R = 360;
```

### Buffer A (simulation)

```glsl
vec4 process(vec2 c) {
    vec2 sz = iChannelResolution[0].xy;
    vec4 v0 = texture(iChannel0, (c + vec2(-1,0)) / sz);
    vec4 v1 = texture(iChannel0, (c + vec2( 1,0)) / sz);
    vec4 v2 = texture(iChannel0, (c + vec2(0,-1)) / sz);
    vec4 v3 = texture(iChannel0, (c + vec2(0, 1)) / sz);
    vec4 v4 = texture(iChannel0, c / sz);
    float w = ((iFrame % 2) == 0)?0.367879:3.0;
    float k = (1.0 - w) / 4.0;
    return w * v4 + k * (v0 + v1 + v2 + v3);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 C = iMouse.xy / iResolution.xy;
    if (iFrame == 0) {
        vec2 uv = (fragCoord / iResolution.xy) * 2.0 - 1.0;
        uv.x *= iResolution.x / iResolution.y;
        float c0 = length(uv) - 0.5;
        float c1 = length(uv - vec2(0.0,0.25));
        float c2 = length(uv - vec2(0.0,-0.25));
        float d = max(min(max(c0, -uv.x),c1-0.25),-c2+0.25);
        d = max(d, -c1+0.07);
        d = min(d, c2-0.07);
        d = min(d, abs(c0)-0.01);
        float w = sign(d) * sin(c0*80.0);
        fragColor = vec4(w);
    } else if ((iFrame % R) == 0) {
        vec2 c = vec2(fragCoord / iResolution.xy);
        fragColor = texture(iChannel0, (c - C) / 2.0 + C);
    } else {
        vec4 s = process(fragCoord);
        fragColor = clamp(s, -1.0, 1.0);
    }
}
```

### Image (display pass)

```glsl
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 C = iMouse.xy / iResolution.xy;
    vec2 uv = fragCoord / iChannelResolution[0].xy;
    float m = float(iFrame % R) / float(R);
    uv -= C;
    uv.x *= iResolution.x / iResolution.y;
    uv = uv / (1.0 + 0.1*dot(uv,uv));
    uv.x /= iResolution.x / iResolution.y;
    uv *= exp(mix(log(1.0),log(0.5),m));
    float f = 0.004;
    vec2 uv3 = uv * exp(0.0) + C;
    vec2 uv2 = uv * exp(-f) + C;
    vec2 uv1 = uv * exp(-f*2.0) + C;
    float r = texture(iChannel0, uv1).r*0.5+0.5;
    float g = texture(iChannel0, uv2).r*0.5+0.5;
    float b = texture(iChannel0, uv3).r*0.5+0.5;
    fragColor = vec4(pow(vec3(r,g,b), vec3(0.5)),1.0);
}
```

## Algorithm

The system has two passes: a **simulation pass** (ping-pong) and a **display pass**.

### Simulation Pass

A 5-tap von Neumann kernel alternates weights every frame:

- **Even frames (diffusion)**: center weight `diffusionWeight` (default 1/e ~ 0.368), neighbor weight `(1 - diffusionWeight) / 4`. Standard blur — spreads values outward.
- **Odd frames (sharpening)**: center weight `sharpenWeight` (default 3.0), neighbor weight `(1 - sharpenWeight) / 4 = -0.5`. Amplifies center, subtracts neighbors — creates edges and concentrates structure.

Both kernels sum to 1.0 (energy-conserving). Output clamped to [-1, 1].

The alternation creates activator-inhibitor dynamics: diffusion spreads, sharpening concentrates. The tension between them generates labyrinthine patterns without needing two chemical species.

**Zoom reseed**: Every `cycleLength` frames, the buffer resamples itself at `zoomAmount`x zoom toward `center`. The upscaling introduces interpolation artifacts that the kernel digests into fresh patterns. This is the energy source that prevents equilibrium — without it, the field converges to a static state.

### Display Pass

- Smooth counter-zoom over the cycle compensates for the discrete reseed: `scale = exp(mix(log(1.0), log(1.0 / zoomAmount), cycleProgress))`. This makes the zoom appear continuous rather than jumpy.
- Field value [-1, 1] maps to `t` via `t = value * 0.5 + 0.5`, then samples `gradientLUT`.
- Same `t` maps to FFT frequency for audio reactivity (standard generator frequency spread).

### Adaptation Notes

- **Replace**: yin-yang SDF seed with a simple initial pattern (concentric sine rings or noise)
- **Replace**: `iChannel0` self-feedback with ping-pong render textures (standard generator pattern)
- **Replace**: RGB offset-zoom colorization with gradient LUT sampling
- **Replace**: barrel distortion (redundant with existing transforms)
- **Keep**: alternating kernel weights, zoom reseed mechanic, counter-zoom in display pass
- **Add**: `center` uniform for zoom focus point
- **Add**: standard Audio section (baseFreq, maxFreq, gain, curve, baseBright) using `t` as frequency position

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| diffusionWeight | float | 0.1-0.9 | 0.368 | Even-frame center weight. Higher = more blur, softer patterns |
| sharpenWeight | float | 1.5-5.0 | 3.0 | Odd-frame center weight. Higher = crisper edges, more intricate detail |
| cycleLength | float | 60-600 | 360 | Frames between zoom reseeds. Shorter = more dynamic, longer = more settled |
| zoomAmount | float | 1.2-4.0 | 2.0 | Zoom factor per reseed. Larger = more new detail injected |
| baseFreq | float | 27.5-440 | 55.0 | Low end of FFT frequency spread (Hz) |
| maxFreq | float | 1000-16000 | 14000 | High end of FFT frequency spread (Hz) |
| gain | float | 0.1-10.0 | 1.0 | FFT energy multiplier |
| curve | float | 0.1-3.0 | 1.0 | FFT energy power curve |
| baseBright | float | 0.0-1.0 | 0.3 | Minimum brightness before FFT contribution |

## Modulation Candidates

- **diffusionWeight**: Shifts pattern character between blobby (high) and sharp (low)
- **sharpenWeight**: Controls edge intensity and detail density
- **cycleLength**: Changes how frequently new perturbation is injected
- **zoomAmount**: Alters how much new structure appears per reseed
- **gain**: Scales overall audio reactivity intensity

### Interaction Patterns

- **diffusionWeight x sharpenWeight (competing forces)**: These push in opposite directions. Diffusion smooths, sharpening concentrates. The visual result is their tension — modulating both from different sources creates a pattern character that shifts with the music. High sharpening + low diffusion = crystalline filigree. High diffusion + low sharpening = melting blobs.
- **cycleLength x kernel weights (cascading threshold)**: Short cycles inject fresh perturbation frequently, so even mild kernel weights produce visible evolution. Long cycles let the kernel fully settle — only strong sharpening prevents the field from going static. The cycle acts as a gate on how much the kernel dynamics matter.

## Notes

- The field may tend toward binary values (-1 or +1) with narrow transition bands. If true, FFT reactivity via `t` could feel bimodal (most pixels locked to lowest or highest frequency bin). Needs implementation testing — the actual value distribution depends on kernel weight dynamics and may be richer than expected.
- The kernel weights are carefully balanced in the original. Ranges should be conservative — pushing `sharpenWeight` too high or `diffusionWeight` too low could cause the field to explode (clamp prevents divergence but visual quality degrades).
- The zoom reseed is structurally necessary for continuous evolution. Without it, the field converges to equilibrium.
- Frame-rate dependent: the kernel runs once per frame. At 60fps target this matches the original Shadertoy behavior.
