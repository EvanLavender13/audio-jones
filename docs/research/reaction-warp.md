# Reaction Warp

Evolving organic displacement based on reaction-diffusion dynamics. Maintains internal reaction state that creates flowing, morphing distortion patterns. Accumulation texture content perturbs the reaction, keeping patterns alive and responsive to visuals.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: Compute shader updates reaction state; fragment shader displaces UVs based on reaction gradients
- **Pipeline Position**: Hybrid — compute runs in feedback phase, warp applies in transform stage

## Architecture

Unlike pure warps (stateless per-frame), this effect has two stages:

**Stage 1: Reaction Update (Compute Shader, Feedback Phase)**
- Maintains trail map with reaction state (R: reaction value, G: blurred value)
- Reads accumulation texture as perturbation source
- Applies simplified reaction-diffusion + gradient advection
- Runs alongside other simulations (physarum, curl flow)

**Stage 2: Displacement (Fragment Shader, Transform Phase)**
- Reads reaction trail map
- Computes gradient from reaction values
- Displaces UVs when sampling input texture
- Outputs warped content

## References

- [Shadertoy: Reaction-Diffusion](https://www.shadertoy.com/view/XsG3z1) - Shane's simplified approach with stochastic decay
- [Karl Sims: Reaction-Diffusion](http://www.karlsims.com/rd.html) - Gray-Scott parameter space reference

## Algorithm

### Reaction Update (Compute Shader)

Based on simplified "living" reaction-diffusion that never settles:

```glsl
layout(rgba16f, binding = 0) uniform image2D reactionMap;  // R: value, G: blurred
layout(binding = 1) uniform sampler2D accumMap;

uniform float reactionStrength;   // How strongly values react
uniform float advectionStrength;  // How much gradients push UVs
uniform float accumInfluence;     // How much accumulation perturbs

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(coord) + 0.5) / resolution;
    vec2 texelSize = 1.0 / resolution;

    // Sample blurred value from previous frame (stored in G channel)
    float blurred = imageLoad(reactionMap, coord).g;

    // Compute gradient from blurred values (central differences)
    float gradX = (texelFetch(reactionMap, coord + ivec2(1,0)).g -
                   texelFetch(reactionMap, coord - ivec2(1,0)).g) * 0.5;
    float gradY = (texelFetch(reactionMap, coord + ivec2(0,1)).g -
                   texelFetch(reactionMap, coord - ivec2(0,1)).g) * 0.5;
    vec2 gradient = vec2(gradX, gradY);

    // Advect UV based on gradient (patterns push themselves)
    vec2 advectedUV = uv + gradient * texelSize * advectionStrength;

    // Sample previous reaction value at advected position
    vec4 prev = texture(reactionMapSampler, advectedUV);
    float reaction = prev.r;

    // Perturbation from accumulation texture
    float accumLuma = dot(texture(accumMap, uv).rgb, vec3(0.299, 0.587, 0.114));
    float perturbation = (accumLuma - 0.5) * accumInfluence;

    // Reaction with stochastic decay (prevents settling)
    reaction += perturbation;
    reaction += (blurred - reaction) * reactionStrength;  // Diffusion toward blur
    reaction *= 0.998;  // Gentle decay

    // Compute new blur (3x3 weighted average)
    float newBlurred = blur3x3(reactionMap, coord);

    imageStore(reactionMap, coord, vec4(reaction, newBlurred, 0.0, 0.0));
}
```

### Displacement (Fragment Shader)

```glsl
uniform sampler2D inputTex;
uniform sampler2D reactionMap;
uniform float warpStrength;

vec4 reactionWarp(vec2 uv) {
    // Sample reaction gradient
    vec2 texelSize = 1.0 / resolution;
    float gradX = (texture(reactionMap, uv + vec2(texelSize.x, 0)).r -
                   texture(reactionMap, uv - vec2(texelSize.x, 0)).r) * 0.5;
    float gradY = (texture(reactionMap, uv + vec2(0, texelSize.y)).r -
                   texture(reactionMap, uv - vec2(0, texelSize.y)).r) * 0.5;

    // Displace UV
    vec2 warpedUV = uv + vec2(gradX, gradY) * warpStrength;

    return texture(inputTex, warpedUV);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| reactionStrength | float | 0.01–0.3 | How strongly values diffuse toward neighbors |
| advectionStrength | float | 1–10 | How much gradients push patterns; higher = more flow |
| accumInfluence | float | 0–1 | How much accumulation texture perturbs reaction |
| warpStrength | float | 0–50 | UV displacement magnitude in pixels |
| decayRate | float | 0.99–0.999 | How quickly patterns fade without input |

## Audio Mapping Ideas

- **warpStrength** ← Bass energy: Stronger displacement on bass
- **advectionStrength** ← Mid frequencies: Faster pattern flow
- **accumInfluence** ← Beat detection: Pulse perturbation on beats

## Differences from Gray-Scott

| Gray-Scott | Reaction Warp |
|------------|---------------|
| Two chemicals (A, B) | Single reaction value + blur |
| feed/kill parameters | Stochastic decay |
| Converges to patterns | Never settles (continuous advection) |
| Sensitive tuning | Robust parameters |
| Generates visible spots/stripes | Produces displacement field |

## Notes

- Requires its own trail map (separate from physarum, curl flow)
- Compute shader runs once per frame in feedback phase
- Without accumulation content, patterns eventually fade (decay)
- With accumulation content, patterns continuously evolve
- Can run alongside other simulations — each has independent trail map
- The warp stage is a standard transform effect; only the reaction update is special
