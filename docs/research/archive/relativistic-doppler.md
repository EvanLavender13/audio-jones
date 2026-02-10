# Relativistic Doppler

Simulates the visual distortion of approaching light speed: spatial compression toward the direction of travel (relativistic aberration) combined with Doppler color shifting where colors blue-shift ahead and red-shift behind. Creates a "warp drive" or "hyperspace jump" aesthetic distinct from simple zooming.

## Classification

- **Category**: TRANSFORMS > Motion
- **Pipeline Position**: Within transform chain, user-orderable

## References

- [MIT OpenRelativity Shader](https://github.com/MITGameLab/OpenRelativity/blob/master/Assets/OpenRelativity/Shaders/relativity.shader) - Complete Unity shader with Doppler, aberration, RGB↔XYZ wavelength shifting
- [Modern Physics - Relativistic Aberration Formula](https://modern-physics.org/relativistic-aberration-formula/) - Derivation of cos(θ') = (cos(θ) - v/c) / (1 - (v/c)cos(θ))
- [Alan Zucconi - Wavelength to RGB](https://www.alanzucconi.com/2017/07/15/improving-the-rainbow/) - GPU bump-function approach for spectral color conversion
- [GitHub Gist - Hue Shift](https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786) - Optimized YIQ and Rodrigues-based hue rotation

## Algorithm

### Core Physics

**Relativistic Aberration** compresses the view toward the direction of motion:

```
cos(θ') = (cos(θ) - β) / (1 - β·cos(θ))
```

Where β = v/c (velocity as fraction of light speed), θ = angle from motion direction in rest frame, θ' = observed angle.

**Doppler Factor** determines wavelength shift:

```
D = 1 / (γ · (1 - β·cos(θ)))
```

Where γ = 1/sqrt(1 - β²) is the Lorentz factor. D > 1 = blue shift (toward motion), D < 1 = red shift (away from motion).

**Headlight Effect** increases brightness toward motion direction:

```
I' = I · D³
```

### 2D Image Adaptation

For a post-process effect, treat the screen center (or configurable point) as the direction of travel:

1. **Angle from center**: θ = atan2(distance from center) mapped to [0, π]
2. **UV aberration**: Compress UVs toward center based on aberration formula
3. **Color shift**: Shift hue based on Doppler factor at each pixel's angle
4. **Intensity**: Optional brightness boost toward center

### Fragment Shader

```glsl
#version 330

uniform sampler2D inputTex;
uniform vec2 resolution;
uniform float time;

uniform bool enabled;
uniform float velocity;       // 0.0 - 0.99 (fraction of c)
uniform vec2 center;          // Normalized center point (0.5, 0.5 default)
uniform float aberration;     // 0.0 - 1.0, spatial compression strength
uniform float colorShift;     // 0.0 - 1.0, Doppler color intensity
uniform float headlight;      // 0.0 - 1.0, brightness boost toward center

in vec2 fragTexCoord;
out vec4 fragColor;

// Optimized hue shift via Rodrigues rotation
vec3 hueShift(vec3 color, float hue) {
    const vec3 k = vec3(0.57735);
    float c = cos(hue);
    float s = sin(hue);
    return color * c + cross(k, color) * s + k * dot(k, color) * (1.0 - c);
}

void main() {
    if (!enabled) {
        fragColor = texture(inputTex, fragTexCoord);
        return;
    }

    vec2 uv = fragTexCoord;
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);

    // Vector from center to pixel (aspect-corrected)
    vec2 toPixel = (uv - center) * aspect;
    float dist = length(toPixel);
    vec2 dir = dist > 0.0001 ? toPixel / dist : vec2(0.0);

    // Angle from "forward" (center = 0, edge = π)
    // Map distance to angle: center is θ=0, max distance is θ=π
    float maxDist = length((vec2(1.0) - center) * aspect);
    float theta = (dist / maxDist) * 3.14159;

    // Lorentz factor
    float beta = velocity;
    float gamma = 1.0 / sqrt(max(1.0 - beta * beta, 0.0001));

    // Relativistic aberration: θ' = acos((cos(θ) - β) / (1 - β·cos(θ)))
    float cosTheta = cos(theta);
    float cosThetaPrime = (cosTheta - beta) / (1.0 - beta * cosTheta);
    cosThetaPrime = clamp(cosThetaPrime, -1.0, 1.0);
    float thetaPrime = acos(cosThetaPrime);

    // Aberrated distance (compressed toward center)
    float aberratedDist = (thetaPrime / 3.14159) * maxDist;
    vec2 aberratedUV = center + dir * aberratedDist / aspect;

    // Blend between original and aberrated UV
    vec2 finalUV = mix(uv, aberratedUV, aberration);

    // Sample texture
    vec3 color = texture(inputTex, finalUV).rgb;

    // Doppler factor: D = 1 / (γ · (1 - β·cos(θ)))
    float D = 1.0 / (gamma * (1.0 - beta * cosTheta));

    // Color shift: D > 1 = blue shift (negative hue), D < 1 = red shift (positive hue)
    // Map D to hue rotation: blue is roughly -0.6 rad from base, red is +0.6 rad
    float hueOffset = (1.0 - D) * 0.8 * colorShift;
    color = hueShift(color, hueOffset);

    // Headlight effect: boost brightness toward center
    float intensityBoost = pow(D, 3.0);
    color = mix(color, color * intensityBoost, headlight);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
```

### Wavelength-Accurate Color Shift (Alternative)

For more physically accurate color shifting, convert RGB to wavelength, shift, and convert back. The MIT OpenRelativity shader uses Gaussian curves to model CIE XYZ response:

```glsl
// Simplified: treat RGB channels as wavelengths and shift independently
// Red ≈ 650nm, Green ≈ 550nm, Blue ≈ 450nm
vec3 shiftWavelength(vec3 color, float D) {
    // Doppler shifts wavelength: λ' = λ / D
    // D > 1: wavelengths decrease (blue shift)
    // D < 1: wavelengths increase (red shift)

    // Approximate by redistributing energy between channels
    float shift = (D - 1.0) * 0.5;

    vec3 shifted;
    shifted.r = mix(color.r, color.g, clamp(shift, 0.0, 1.0));
    shifted.g = mix(color.g, shift > 0.0 ? color.b : color.r, abs(shift));
    shifted.b = mix(color.b, color.g, clamp(-shift, 0.0, 1.0));

    return shifted;
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| enabled | bool | - | true | Toggles effect on/off |
| velocity | float | 0.0 - 0.99 | 0.5 | Fraction of light speed. Higher = more extreme aberration and color shift. |
| center | vec2 | 0.0 - 1.0 | (0.5, 0.5) | Direction of travel. Aberration compresses toward this point. |
| aberration | float | 0.0 - 1.0 | 1.0 | Spatial compression strength. 0 = no UV warp, 1 = full relativistic aberration. |
| colorShift | float | 0.0 - 1.0 | 1.0 | Doppler hue shift intensity. 0 = no color change, 1 = full blue/red shift. |
| headlight | float | 0.0 - 1.0 | 0.3 | Brightness boost toward center. Simulates relativistic beaming. |

## Modulation Candidates

- **velocity**: Acceleration/deceleration creates warp-in/warp-out transitions
- **aberration**: Pulsing compression synced to rhythm
- **colorShift**: Color intensity follows audio energy
- **center**: Moving the "destination" creates directional travel

## Notes

- **Velocity cap at 0.99**: At β=1.0, the Lorentz factor becomes infinite. Cap at 0.99 for stability.
- **Center movement**: Animating center creates a "steering through space" effect. Combine with velocity modulation for dynamic hyperspace travel.
- **Subtle vs dramatic**: Low velocity (0.1-0.3) gives subtle color fringing. High velocity (0.7-0.95) creates dramatic tunnel vision with strong color banding.
- **Edge handling**: Aberrated UVs may sample outside [0,1] at high velocities. Use `GL_CLAMP_TO_EDGE` or mirror wrapping.
- **Performance**: Single-pass effect with minimal texture lookups. Comparable cost to other warp effects.
- **Combine with Infinite Zoom**: Layering this after Infinite Zoom amplifies the "traveling through tunnel" feel.
