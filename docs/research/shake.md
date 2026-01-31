# Shake

Multi-sample stochastic jitter that samples the input texture at randomized UV offsets and averages them, creating a vibrating/motion-blur effect where edges appear soft and unstable. Unlike simple sin/cos screen shake (smooth camera sway), this produces per-frame randomized displacement with controllable blur intensity.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Within transform chain, user-orderable

## References

- User-provided Shadertoy reference - Multi-sample jitter with linear and Gaussian distributions
- [Godot Line Jitter Shader](https://godotshaders.com/shader/line-jitter-stroke-shake-effect/) - Noise-based per-pixel displacement (CC0)
- [Temporal Supersampling](https://bartwronski.com/2014/03/15/temporal-supersampling-and-antialiasing/) - Jitter offset patterns and temporal distribution

## Algorithm

### Core Technique

Sample the input texture N times at jittered UV positions, then average:

```glsl
// iq's hash-based RNG
float seed;
float rnd() { return fract(sin(seed++) * 43758.5453123); }

vec3 result = vec3(0.0);
for (int i = 0; i < samples; i++) {
    vec2 jitter = (vec2(rnd(), rnd()) * 2.0 - 1.0) * intensity;
    result += texture(inputTex, uv + jitter).rgb;
}
result /= float(samples);
```

### Temporal Seeding

Control how often the jitter pattern changes:

```glsl
// Rate in Hz (e.g., 12 = changes 12 times per second)
float seedTime = floor(time * rate) / rate;
seed = fract(seedTime) + uv.x * 12.9898 + uv.y * 78.233;
```

Adding UV to the seed ensures each pixel gets a different random sequence.

### Distribution Types

**Uniform (Linear)**: Equal probability across the jitter range. Sharp, even displacement.

```glsl
vec2 jitter = (vec2(rnd(), rnd()) * 2.0 - 1.0) * intensity;
```

**Gaussian (Box-Muller)**: Concentrated near center, rare large offsets. Softer, more natural blur.

```glsl
vec2 xi = vec2(rnd(), rnd());
float r = sqrt(-log(max(1.0 - xi.x, 0.0001)));  // Prevent log(0)
float theta = 6.28318 * xi.y;
vec2 jitter = vec2(cos(theta), sin(theta)) * r * intensity;
```

### Complete Fragment Shader

```glsl
#version 330

uniform sampler2D inputTex;
uniform vec2 resolution;
uniform float time;

uniform bool enabled;
uniform float intensity;    // 0.0 - 0.2
uniform int samples;        // 1 - 16
uniform float rate;         // Hz, how often jitter changes
uniform bool gaussian;      // false = uniform, true = gaussian

in vec2 fragTexCoord;
out vec4 fragColor;

float seed;
float rnd() { return fract(sin(seed++) * 43758.5453123); }

void main() {
    vec2 uv = fragTexCoord;

    if (!enabled || samples < 1) {
        fragColor = texture(inputTex, uv);
        return;
    }

    // Initialize seed with temporal and spatial variation
    float seedTime = floor(time * rate) / max(rate, 0.001);
    seed = fract(seedTime * 43.758) + uv.x * 12.9898 + uv.y * 78.233;

    vec3 result = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        vec2 jitter;

        if (gaussian) {
            // Box-Muller transform for Gaussian distribution
            vec2 xi = vec2(rnd(), rnd());
            float r = sqrt(-log(max(1.0 - xi.x, 0.0001)));
            float theta = 6.28318 * xi.y;
            jitter = vec2(cos(theta), sin(theta)) * r * intensity;
        } else {
            // Uniform distribution in [-1, 1]
            jitter = (vec2(rnd(), rnd()) * 2.0 - 1.0) * intensity;
        }

        result += texture(inputTex, uv + jitter).rgb;
    }

    fragColor = vec4(result / float(samples), 1.0);
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| enabled | bool | - | true | Toggles effect on/off |
| intensity | float | 0.0 - 0.2 | 0.02 | Maximum UV displacement distance. Higher = more violent shake. |
| samples | int | 1 - 16 | 4 | Sample count per pixel. More samples = smoother blur, higher cost. |
| rate | float | 1.0 - 60.0 | 12.0 | Jitter change frequency (Hz). Lower = slower, choppier shake. |
| gaussian | bool | - | false | Distribution type. Gaussian concentrates displacement near center. |

## Modulation Candidates

- **intensity**: Shake violence scales with modulation source
- **rate**: Jitter frequency changes create rhythmic stuttering
- **samples**: Lower samples during peaks for grittier look

## Notes

- **Performance**: Each sample requires a texture fetch. At 16 samples, this is 16x the bandwidth of a simple pass. Keep samples low (4-8) for real-time use.
- **Edge handling**: Jittered UVs may sample outside [0,1]. Use `GL_CLAMP_TO_EDGE` or add explicit clamping.
- **Aspect ratio**: Apply jitter in normalized UV space (not pixel space) for uniform shake regardless of resolution.
- **Rate = 0 edge case**: Clamp rate to minimum (0.001) to avoid division issues.
