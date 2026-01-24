# Oil Paint — Geometry-Based Brush Strokes

Places individual brush strokes on a multi-scale grid, oriented perpendicular to image gradients. Each stroke samples the source color at its center and renders with tapered edges and brush-hair texture. A second pass adds relief lighting for 3D paint surface appearance. Produces visible painterly results on any content, including smooth gradients.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Multi-scale grid → gradient-oriented brush stroke placement → alpha compositing → relief lighting
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)
- **Infrastructure**: 2-pass rendering (stroke render + relief lighting), requires noise texture

## References

- [Shadertoy: flockaroo Oil Paint (WGGGRh)](https://www.shadertoy.com/view/WGGGRh) — Complete 2-buffer implementation (Buffer A: strokes, Image: relief). Code pasted by user and verified.
- [Shaderoo geometry version](https://shaderoo.org/?shader=N6DFZT) — Original geometry-based implementation by same author

## Algorithm

### Overview (2 passes)

1. **Stroke Rendering**: Multi-scale grid of brush positions → gradient computation → oriented stroke shape with hair texture → layer-by-layer alpha compositing
2. **Relief Lighting**: Gradient-based normal → diffuse + specular + vignette

### Pass 1: Stroke Rendering

#### Grid Setup

Distribute brush stroke positions across a multi-scale grid. Coarse layers have few large strokes; fine layers have many small strokes.

```glsl
float layerScaleFact = float(qualityPercent) / 100.0;  // e.g. 0.85
float ls = layerScaleFact * layerScaleFact;

// Total grid positions at finest level
int NumGrid = int(float(0x8000) * min(pow(resolution.x / 1920.0, 0.5), 1.0) * (1.0 - ls));
float aspect = resolution.x / resolution.y;
int NumX = int(sqrt(float(NumGrid) * aspect) + 0.5);
int NumY = int(sqrt(float(NumGrid) / aspect) + 0.5);

// Max layers: stop when coarsest layer has ~10 strokes in Y
int maxLayer = int(log2(10.0 / float(NumY)) / log2(layerScaleFact));
```

#### Per-Pixel Stroke Lookup

For each pixel, check the 9 nearest grid cells (3x3 neighborhood) at each layer. Iterate layers from coarse to fine, compositing each stroke on top:

```glsl
for (int layer = maxLayer; layer >= 0; layer--) {
    int NumX2 = int(float(NumX) * pow(layerScaleFact, float(layer)) + 0.5);
    int NumY2 = int(float(NumY) * pow(layerScaleFact, float(layer)) + 0.5);

    for (int ni = 0; ni < 9; ni++) {
        int nx = ni % 3 - 1;
        int ny = ni / 3 - 1;

        // Grid cell index for this pixel
        int n0 = int(dot(floor(pos / resolution * vec2(NumX2, NumY2)), vec2(1, NumX2)));
        int pidx2 = n0 + NumX2 * ny + nx;

        // Brush center = grid cell center + noise jitter
        vec2 brushPos = (vec2(pidx2 % NumX2, pidx2 / NumX2) + 0.5)
                        / vec2(NumX2, NumY2) * resolution;
        float gridW = resolution.x / float(NumX2);
        brushPos += gridW * (getRand(pidx).xy - 0.5);
        // Trigonal offset: displace every 2nd row
        brushPos.x += gridW * 0.5 * (float((pidx2 / NumX2) % 2) - 0.5);
        // ... stroke rendering ...
    }
}
```

#### Gradient-Based Orientation

Compute image gradient at two scales and blend. The gradient normal determines stroke orientation (strokes run perpendicular to edges):

```glsl
// Multi-scale gradient using channel with largest magnitude
vec2 getGradMax(vec2 pos, float eps) {
    float lod = log2(2.0 * eps * sourceRes.x / resolution.x);
    vec2 d = vec2(eps, 0.0);
    return vec2(
        compsignedmax(getVal(pos + d.xy, lod) - getVal(pos - d.xy, lod)),
        compsignedmax(getVal(pos + d.yx, lod) - getVal(pos - d.yx, lod))
    ) / eps / 2.0;
}

// Blend coarse + fine gradient for stable orientation
vec2 g = getGradMax(brushPos, gridW * 1.0) * 0.5
       + getGradMax(brushPos, gridW * 0.12) * 0.5
       + 0.0003 * sin(pos / resolution * 20.0);  // break uniformity in flat areas
float gl = length(g);
vec2 n = normalize(g);  // gradient direction (across edges)
vec2 t = n.yx * vec2(1, -1);  // tangent (along edges)
```

The `compsignedmax` function picks the color channel with the largest absolute gradient, preserving its sign:

```glsl
float compsignedmax(vec3 c) {
    vec3 a = abs(c);
    if (a.x > a.y && a.x > a.z) return c.x;
    if (a.y > a.x && a.y > a.z) return c.y;
    return c.z;
}
```

#### Stroke Shape

Each stroke is an oriented rectangle with aspect ratio that varies by layer. Coarser layers produce longer strokes:

```glsl
// Stretch factor: coarse layers are more elongated
float stretch = sqrt(1.5 * pow(3.0, 1.0 / float(layer + 1)));
float wh = (gridW - 0.6 * gridW0) * 1.2;  // base width
float lh = wh;  // base length
wh *= brushSize * (0.8 + 0.4 * rand.y) / stretch;  // narrow across stroke
lh *= brushSize * (0.8 + 0.4 * rand.z) * stretch;  // long along stroke

// Transform pixel into stroke-local UV [0,1]x[0,1]
vec2 uv = vec2(dot(pos - brushPos, n), dot(pos - brushPos, t))
           / vec2(wh, lh) * 0.5 + 0.5;

// Suppress strokes in flat areas (low gradient magnitude)
wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer)
     ? 0.0 : wh;
```

#### Stroke Bending

Optional curvature bends the stroke along its length:

```glsl
uv.x -= 0.125 * strokeBend;
uv.x += uv.y * uv.y * strokeBend;  // quadratic bend
uv.x /= 1.0 - 0.25 * abs(strokeBend);
```

#### Alpha Mask and Brush Hair Texture

Tapered falloff at stroke edges, plus anisotropic noise for brush-hair appearance:

```glsl
// Soft rectangular mask with smooth edges
float s = 1.0;
s *= uv.x * (1.0 - uv.x) * 6.0;  // fade at left/right
s *= uv.y * (1.0 - uv.y) * 6.0;  // fade at top/bottom
float s0 = clamp((s - 0.5) * 2.0, 0.0, 1.0);

// Brush-hair texture: anisotropic noise (stretched along stroke)
float pat = texture(noiseTex, uv * 1.5 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x
          + texture(noiseTex, uv * 3.0 * sqrt(resolution.x / 600.0) * vec2(0.06, 0.006)).x;

// Combine: hair pattern modulates alpha
float alpha = s0 * 0.7 * pat;

// Stroke highlight mask: cosine lobe + exponential tip fade
float smask = clamp(
    max(cos(uv.x * PI * 2.0 + 1.5 * (rand.x - 0.5)),
        (1.5 * exp(-uv.y * uv.y / 0.0225) + 0.2) * (1.0 - uv.y)) + 0.1,
    0.0, 1.0);
alpha += s0 * smask;
alpha -= 0.5 * uv.y;  // darken toward stroke tail
alpha = clamp(alpha, 0.0, 1.0);
```

#### Color Sampling and Compositing

Each stroke samples the source at its center position, modulated by the brush texture. Strokes composite back-to-front:

```glsl
vec3 strokeColor = texture(source, brushPos / resolution).rgb
                 * mix(alpha * 0.13 + 0.87, 1.0, smask);

// Clip to stroke bounds
float mask = alpha * step(-0.5, -abs(uv.x - 0.5)) * step(-0.5, -abs(uv.y - 0.5));

// Alpha blend onto canvas
fragColor = mix(fragColor, vec4(strokeColor, 1.0), mask);
```

### Pass 2: Relief Lighting

Compute a surface normal from the painted image gradient, then apply directional lighting for 3D paint-surface appearance:

```glsl
vec2 uv = fragCoord / resolution;
float delta = 1.0 / resolution.y;
vec2 d = vec2(delta, 0.0);

// Gradient of painted image (brightness-based)
float val_l = length(texture(paintedTex, uv - d.xy).rgb);
float val_r = length(texture(paintedTex, uv + d.xy).rgb);
float val_d = length(texture(paintedTex, uv - d.yx).rgb);
float val_u = length(texture(paintedTex, uv + d.yx).rgb);
vec2 grad = vec2(val_r - val_l, val_u - val_d) / delta;

// Normal from gradient (z-component controls relief depth)
vec3 n = normalize(vec3(grad, 150.0));

// Lighting
vec3 light = normalize(vec3(-1.0, 1.0, 1.4));
float diff = clamp(dot(n, light), 0.0, 1.0);
float spec = pow(clamp(dot(reflect(light, n), vec3(0, 0, -1)), 0.0, 1.0), 12.0) * specStrength;
float shadow = pow(clamp(dot(reflect(light * vec3(-1, -1, 1), n), vec3(0, 0, -1)), 0.0, 1.0), 4.0) * 0.1;

vec3 tint = vec3(0.85, 1.0, 1.15);  // cool specular tint
fragColor.rgb = texture(paintedTex, uv).rgb * mix(diff, 1.0, 0.9)
             + spec * tint + shadow * tint;
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| brushSize | float | 0.5-3.0 | 1.0 | Scales stroke width and length together |
| brushDetail | float | 0.0-0.5 | 0.1 | Gradient threshold below which strokes are suppressed (0 = paint everything) |
| strokeBend | float | -2.0-2.0 | -1.0 | Quadratic curvature along stroke length |
| quality | int | 50-100 | 85 | Layer scale factor as percentage; lower = more layers, more strokes |
| specular | float | 0.0-1.0 | 0.15 | Relief lighting specular intensity |
| layers | int | 3-11 | 8 | Maximum number of scale layers (caps the coarse-to-fine range) |

## Modulation Candidates

- **brushSize**: Larger strokes when increased (impressionist look), finer detail when decreased (tighter rendering)
- **strokeBend**: More curvature creates swirling, Van Gogh-like motion
- **specular**: Higher values create wet, glossy paint appearance; lower values flatten to matte
- **brushDetail**: Lower threshold paints over flat areas; higher preserves source in uniform regions

## Infrastructure Requirements

2-pass rendering with one intermediate texture:

1. Source + Noise → Stroke Rendering → Intermediate texture
2. Intermediate → Relief Lighting → Output

Requires a noise texture (256x256 or larger, random RGBA values) loaded at initialization and bound as `texture1` during the stroke pass.

## Notes

- Performance scales with `layers` parameter and resolution. Each pixel iterates `layers × 9` grid cells. At 1080p with 8 layers, that is ~70 iterations per pixel with 3-5 texture fetches each.
- The multi-scale approach guarantees coverage: coarse layers fill the canvas, fine layers add detail. No pixel is left unpainted.
- The `0.0003 * sin(pos / resolution * 20.0)` term in gradient computation breaks uniformity in flat areas, ensuring strokes have varied orientations even on smooth content.
- Strokes composite back-to-front (coarse first, fine on top), matching natural painting order.
- The brush-hair noise uses anisotropic UV scaling (10:1 ratio) to create elongated bristle-like texture along the stroke direction.
- Relief lighting z-component (150.0) controls how subtle the surface normals are — lower values create more dramatic 3D surface.
- The existing multi-pass infrastructure (from the failed Kuwahara attempt) can be reused directly.
