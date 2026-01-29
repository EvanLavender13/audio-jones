# Disco Ball

A faceted mirror sphere that reflects the input texture from each tile at a slightly different angle, with scattered light spots projected onto the background area outside the sphere.

## Classification

- **Category**: TRANSFORMS > Cellular
- **Pipeline Position**: After feedback, before output (alongside Voronoi, Lattice Fold)

## References

- User-provided Shadertoy shaders (3 variations)
- [Disco Ball - Wikipedia](https://en.wikipedia.org/wiki/Disco_ball) - Physical behavior of facet reflection

## Algorithm

### Ray-Sphere Intersection

Analytic intersection avoids raymarching cost:

```glsl
float RaySphere(vec3 ro, vec3 rd, vec3 center, float radius) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return -1.0;  // miss
    return -b - sqrt(h);       // near intersection
}
```

### Spherical UV Tiling

Convert hit position to phi/theta, quantize into grid:

```glsl
vec2 phiTheta = vec2(
    atan(pos.z, pos.x) + sphereAngle,  // longitude + rotation
    acos(pos.y / sphereRadius)          // latitude
);
vec2 tileId = floor(phiTheta / tileSize);
vec2 tilePos = fract(phiTheta / tileSize);  // position within tile [0,1]
```

### Facet Normal Perturbation

Each tile has a flat face. Compute tile center, offset normal for bevel edges:

```glsl
// Snap to tile center
vec2 tileCenter = tileId * tileSize;

// Reconstruct position at tile center
vec3 facetPos;
facetPos.x = radius * sin(tileCenter.y) * cos(tileCenter.x);
facetPos.y = radius * cos(tileCenter.y);
facetPos.z = radius * sin(tileCenter.y) * sin(tileCenter.x);

// Edge bump: darker/raised at tile boundaries
vec2 edge = min(tilePos * 10.0, (1.0 - tilePos) * 10.0);
float bump = clamp(min(edge.x, edge.y), 0.0, 1.0);

// Perturb normal slightly at edges
facetPos.y += (1.0 - bump) * bumpHeight;
vec3 normal = normalize(facetPos);
```

### Texture Reflection Sampling

```glsl
vec3 refl = reflect(rayDir, normal);

// Sample input texture using reflection as UV
// Map 3D reflection to 2D: use spherical projection or matcap style
vec2 reflUV = refl.xy * 0.5 + 0.5;
vec3 color = texture(inputTex, reflUV).rgb;

// Darken edges, boost center
color *= bump * intensity;
```

### Background Light Projection

For pixels outside the sphere, compute scattered light spots from facets:

```glsl
// Virtual spotlight position (e.g., above and in front of ball)
vec3 lightPos = vec3(0.0, 2.0, -3.0);

vec3 bgColor = vec3(0.0);

// Iterate visible facets (or subsample for performance)
for (int lat = 0; lat < latSteps; lat++) {
    for (int lon = 0; lon < lonSteps; lon++) {
        // Facet center in spherical coords
        float theta = (float(lat) + 0.5) / float(latSteps) * PI;
        float phi = (float(lon) + 0.5) / float(lonSteps) * TAU + sphereAngle;

        // Facet position and normal
        vec3 facetPos = spherePos + radius * vec3(
            sin(theta) * cos(phi),
            cos(theta),
            sin(theta) * sin(phi)
        );
        vec3 facetNormal = normalize(facetPos - spherePos);

        // Light direction to facet
        vec3 lightDir = normalize(facetPos - lightPos);

        // Reflected light direction
        vec3 reflDir = reflect(lightDir, facetNormal);

        // Check if reflection points toward this pixel's world position
        vec3 pixelWorld = rayOrigin + rayDir * farPlane;  // approximate
        vec3 toPixel = normalize(pixelWorld - facetPos);

        float alignment = dot(reflDir, toPixel);
        if (alignment > spotCutoff) {
            // Soft spotlight falloff
            float spot = pow(alignment, spotExponent);
            float dist = length(pixelWorld - facetPos);
            float atten = 1.0 / (1.0 + dist * dist * 0.01);
            bgColor += spotColor * spot * atten;
        }
    }
}
```

**Performance note**: Full facet iteration is expensive. Options:
1. Subsample (every Nth facet)
2. Limit to front-facing facets via `dot(facetNormal, -rayDir) > 0`
3. Use noise-based approximation instead of explicit iteration

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| sphereRadius | float | 0.2 - 1.5 | 0.8 | Size of disco ball on screen |
| spherePos | vec2 | screen coords | center | Ball position (draggable) |
| tileSize | float | 0.05 - 0.3 | 0.12 | Facet grid density (smaller = more tiles) |
| rotationSpeed | float | -2.0 - 2.0 | 0.5 | Ball spin rate (radians/sec) |
| bumpHeight | float | 0.0 - 0.2 | 0.1 | Edge bevel depth |
| reflectIntensity | float | 0.5 - 5.0 | 2.0 | Brightness of reflected texture |
| lightPos | vec3 | world coords | (0, 2, -3) | Virtual spotlight position |
| spotBrightness | float | 0.0 - 3.0 | 1.0 | Background light spot intensity |
| spotSize | float | 0.9 - 0.99 | 0.95 | Cutoff angle for spots (higher = smaller spots) |
| spotColor | vec3 | RGB | (1,1,1) | Tint of projected light spots |

## Modulation Candidates

- **rotationSpeed**: Audio-reactive spin creates classic disco effect
- **reflectIntensity**: Pulse brightness with beat
- **spotBrightness**: Background lights react to bass
- **tileSize**: Morph facet density with audio energy
- **sphereRadius**: Subtle size pulse on kick

## Notes

- **Facet count**: tileSize of 0.12 creates ~50x25 = 1250 facets. Background light iteration should subsample.
- **Reflection mapping**: The reference uses cubemap-style lookup. For 2D input texture, spherical projection or matcap-style `refl.xy * 0.5 + 0.5` works.
- **Multiple spheres**: The reference shows nested spheres with different radii and counter-rotation. Could expose as layer count parameter.
- **Mood colors**: Reference cycles through tint colors. Could tie to gradient or audio-reactive palette.
