# Wave Warp

Generalization of the existing Triangle Wave Warp into a multi-mode warp effect. The viewer sees UV displacement driven by fBM noise whose character changes with the selected wave basis function — sharp creased folds (triangle), smooth flowing undulations (sine), directional shearing (sawtooth), or blocky digital displacement (square).

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Same slot as current Triangle Wave Warp (in-place rename)
- **Relationship**: Replaces `triangle_wave_warp` entirely

## Attribution

- **Based on**: "Filaments" by nimitz (stormoid)
- **Source**: https://www.shadertoy.com/view/4lcSWs
- **License**: CC BY-NC-SA 3.0 Unported

## Reference Code

The existing shader (already in codebase as `shaders/triangle_wave_warp.fs`):

```glsl
// Triangle-Wave fBM Domain Warp
// Based on "Filaments" by nimitz (stormoid)
// https://www.shadertoy.com/view/4lcSWs
// License: CC BY-NC-SA 3.0 Unported
// Modified: Extracted noise warp as standalone radial post-process effect

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float noiseStrength; // Displacement magnitude (0.0-1.0)
uniform float time;          // Crease rotation angle (accumulated on CPU)

float tri(float x) { return abs(fract(x) - 0.5); }

vec2 tri2(vec2 p) {
    return vec2(tri(p.x + tri(p.y * 2.0)), tri(p.y + tri(p.x * 2.0)));
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float triangleNoise(vec2 p) {
    float z  = 1.5;
    float z2 = 1.5;
    float rz = 0.0;
    vec2  bp = p * 0.8;
    for (int i = 0; i <= 3; i++) {
        vec2 dg = tri2(bp * 2.0) * 0.5;
        dg *= rot(time);
        p  += dg / z2;
        bp *= 1.5;
        z2 *= 0.6;
        z  *= 1.7;
        p  *= 1.2;
        p  *= mat2(0.970, 0.242, -0.242, 0.970);
        rz += tri(p.x + tri(p.y)) / z;
    }
    return rz;
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p  = (uv - 0.5) * 2.0;

    float nz = clamp(triangleNoise(p), 0.0, 1.0);
    p *= 1.0 + (nz - 0.5) * noiseStrength;

    uv = p * 0.5 + 0.5;
    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

## Algorithm

Replace the hardcoded `tri()` basis function with a selectable wave function. The wave shape is used in two places:

1. **`wave2()` (was `tri2()`)** — generates 2D offset vectors for domain warping each octave
2. **`wave(p.x + wave(p.y))` (was `tri(...)`)** — the value accumulation per octave

### Wave functions

```glsl
// waveType uniform: 0=triangle, 1=sine, 2=sawtooth, 3=square

float wave(float x) {
    if (waveType == 0) return abs(fract(x) - 0.5);           // triangle
    if (waveType == 1) return sin(x * 6.283185) * 0.5 + 0.5; // sine
    if (waveType == 2) return fract(x);                        // sawtooth
    return step(0.5, fract(x));                                // square
}
```

All four functions map to [0, 1] range with period 1, so they're drop-in replacements. The uniform branch has zero divergence cost (all fragments take the same path).

### Changes from reference

- Replace `tri()` with `wave()` everywhere
- Replace `tri2()` with `wave2()` using same cross-coupling pattern
- Replace hardcoded `0.8` scale with `scale` uniform
- Replace hardcoded loop bound `3` with `octaves` uniform (use `int` uniform)
- Add `waveType` int uniform
- Keep everything else verbatim: the `rot()`, the per-octave rotation matrix, the amplitude/frequency scaling, the radial displacement in `main()`

### Rename

- Shader: `shaders/triangle_wave_warp.fs` -> `shaders/wave_warp.fs`
- C++ files: `src/effects/triangle_wave_warp.{h,cpp}` -> `src/effects/wave_warp.{h,cpp}`
- Structs: `TriangleWaveWarpConfig` -> `WaveWarpConfig`, `TriangleWaveWarpEffect` -> `WaveWarpEffect`
- Enum: `TRANSFORM_TRIANGLE_WAVE_WARP` -> `TRANSFORM_WAVE_WARP`
- Display name: `"Tri-Wave Warp"` -> `"Wave Warp"`
- Mod param prefix: `"triangleWaveWarp."` -> `"waveWarp."`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| waveType | int (enum) | 0-3 | 0 (triangle) | Basis function: triangle, sine, sawtooth, square |
| strength | float | 0.0-1.0 | 0.4 | Displacement magnitude |
| speed | float | 0.0-10.0 | 4.5 | Crease rotation rate (rad/s) |
| octaves | int | 1-6 | 4 | fBM iteration count — more = finer detail |
| scale | float | 0.1-4.0 | 0.8 | Initial noise frequency — higher = smaller features |

## Modulation Candidates

- **strength**: displacement intensity, primary audio-reactive parameter
- **speed**: crease rotation rate, good for tempo-locked modulation
- **scale**: noise zoom level, shifts between macro and micro distortion
- **octaves**: detail level, integer steps between smooth and intricate

### Interaction Patterns

- **Scale + strength (competing forces)**: Scale controls feature size, strength controls displacement amount. Large scale + high strength = dramatic broad warps; small scale + high strength = high-frequency jitter. Modulating from different bands creates structural verse/chorus contrast — bass drives broad warps, highs add fine jitter on top.
- **Wave type + octaves (cascading character)**: The wave shape determines each octave's texture, and stacking amplifies the character. Sine at 1-2 octaves is a gentle lens distortion. Sawtooth at 5-6 octaves is intricate directional shearing. Switching wave type mid-song (via integer modulation) produces dramatic textural shifts.

## Notes

- Square wave fBM produces flat regions with sharp transitions — at low octaves it creates large blocky displacement zones, useful for a digital/glitch aesthetic
- Sawtooth fBM has directional bias from its asymmetric ramp — the warp will have a subtle "grain direction"
- The per-octave rotation matrix (`mat2(0.970, 0.242, ...)`) is critical for all wave types — without it, sawtooth and square modes would show strong axis-aligned artifacts
- Performance: the octave count is the main cost driver. Capping at 6 keeps the loop short enough for real-time at any resolution
