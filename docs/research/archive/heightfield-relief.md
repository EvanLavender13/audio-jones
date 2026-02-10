# Heightfield Relief

Treats texture luminance as a heightfield and applies fake directional lighting based on surface normals derived from the Sobel gradient. Creates an embossed 3D appearance that enhances depth perception in images with existing structure (like Texture Warp output).

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Sobel gradient → surface normal → directional light dot product
- **Pipeline Position**: Output stage, after transforms (user order)

## References

- [Daniel Ilett - Edge Detection Tutorial](https://danielilett.com/2019-05-11-tut1-4-smo-edge-detect/) - Sobel operator explanation and GLSL implementation
- [Chalmers University Heightfield Tutorial](https://www.cse.chalmers.se/edu/course/TDA362/tutorials/heightfield.html) - Normal computation from height gradients
- `shaders/toon.fs:72-89` - Existing Sobel implementation in codebase

## Algorithm

### Step 1: Compute Sobel Gradients

Sample 3x3 neighborhood and apply Sobel kernels (already implemented in `toon.fs`):

```glsl
// Sobel kernels
// Gx: [-1 0 1]    Gy: [-1 -2 -1]
//     [-2 0 2]        [ 0  0  0]
//     [-1 0 1]        [ 1  2  1]

vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
```

### Step 2: Convert Gradient to Normal

Interpret gradients as height derivatives. The Z component controls how "steep" the surface appears:

```glsl
// Convert luminance gradients to surface normal
float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));

// reliefScale: higher = flatter surface, lower = more dramatic relief
vec3 normal = normalize(vec3(-gx, -gy, reliefScale));
```

### Step 3: Apply Directional Lighting

Dot product with light direction, biased to avoid pure black:

```glsl
// lightAngle in radians, converted to direction vector
vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));

// Lambertian lighting with bias
float lighting = dot(normal, lightDir) * 0.5 + 0.5;

// Apply to original color
vec3 result = texColor.rgb * mix(1.0, lighting, intensity);
```

### Optional: Specular Highlight

For shinier appearance:

```glsl
vec3 viewDir = vec3(0.0, 0.0, 1.0);  // orthographic assumption
vec3 halfDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
result += spec * specularIntensity;
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| intensity | float | 0.0 - 1.0 | 0.7 | Blend between original and lit result |
| reliefScale | float | 0.1 - 5.0 | 1.0 | Surface flatness (higher = subtler relief) |
| lightAngle | float | 0 - 2π | π/4 | Light direction around Z axis |
| lightHeight | float | 0.1 - 2.0 | 0.5 | Light elevation (higher = more top-down) |

## Audio Mapping Ideas

| Parameter | Audio Source | Behavior |
|-----------|--------------|----------|
| lightAngle | Time or beat | Rotating light source, shadows shift |
| intensity | Bass energy | Stronger relief on heavy beats |
| reliefScale | Mid frequencies | Flatter during quiet, dramatic during loud |

## Notes

- Works best on images with existing structure (gradients, edges). On flat color, produces no effect.
- Combines well with Texture Warp - apply relief after warp to emphasize the emergent 3D ridges.
- Consider offering "light color" parameter for tinted highlights.
- The Sobel sampling pattern matches `toon.fs` - could share code or extract common function.
