# Polyhedral Mirror

A raymarched reflective polyhedron viewed from inside. The camera orbits within a mirrored platonic solid, bouncing light off crystalline faces that color-shift by reflection depth. Glowing wireframe edges trace the geometry. Configurable shape (all 5 platonic solids), reflection depth, camera distance, and rotation speeds. Quiet passages show shallow reflections; loud passages unlock deep kaleidoscopic bouncing.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator (same slot as Spin Cage, Polymorph)
- **Compute Model**: Fragment shader SDF ray marching (no compute pass)

## Attribution

- **Based on**: "Icosahedral Party" by OldEclipse
- **Source**: https://www.shadertoy.com/view/33tSzs
- **License**: CC BY-NC-SA 3.0

## References

- [IQ Distance Functions](https://iquilezles.org/articles/distfunctions/) — sdCapsule (clamped line segment SDF, better than reference's infinite cylinder)
- [IQ Normals for an SDF](https://iquilezles.org/articles/normalsSDF/) — finite-difference normal estimation
- Existing codebase: `src/config/platonic_solids.h` — vertex/edge data for all 5 solids
- Existing codebase: `src/effects/spin_cage.cpp` — CPU rotation + uniform array upload pattern
- Existing codebase: `src/effects/polymorph.cpp` — generator with platonic solid data, sdCapsule edges

## Reference Code

```glsl
// "Icosahedral Party" by OldEclipse
// https://www.shadertoy.com/view/33tSzs
// License: CC BY-NC-SA 3.0

#define PHI 1.618

// 2d Rotation matrix
mat2 Rot(float th){
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

// SDF for a plane with normal vector n
float sdPlane(vec3 p, vec3 n, float h){
    return dot(p,n) + h;
}

// Vertices of a icosahedron
vec3 icosahedronVertices[12] = vec3[](
    normalize(vec3( 0,  1,  PHI)),
    normalize(vec3( 0, -1,  PHI)),
    normalize(vec3( 0,  1, -PHI)),
    normalize(vec3( 0, -1, -PHI)),

    normalize(vec3( 1,  PHI,  0)),
    normalize(vec3(-1,  PHI,  0)),
    normalize(vec3( 1, -PHI,  0)),
    normalize(vec3(-1, -PHI,  0)),

    normalize(vec3( PHI,  0,  1)),
    normalize(vec3(-PHI,  0,  1)),
    normalize(vec3( PHI,  0, -1)),
    normalize(vec3(-PHI,  0, -1))
);

// Vertex connections of a icosahedron
ivec2 icosahedronEdges[30] = ivec2[](
    ivec2(0, 1), ivec2(0, 4), ivec2(0, 5), ivec2(0, 8), ivec2(0, 9),
    ivec2(1, 6), ivec2(1, 7), ivec2(1, 8), ivec2(1, 9), ivec2(2, 3),
    ivec2(2, 4), ivec2(2, 5), ivec2(2,10), ivec2(2,11), ivec2(3, 6),
    ivec2(3, 7), ivec2(3,10), ivec2(3,11), ivec2(4, 5), ivec2(4, 8),
    ivec2(4,10), ivec2(5, 9), ivec2(5,11), ivec2(6, 7), ivec2(6, 8),
    ivec2(6,10), ivec2(7, 9), ivec2(7,11), ivec2(8,10), ivec2(9,11)
);

// SDF of cylinder
float sdCylinder(vec3 p, vec3 start, vec3 dir, float radius) {
    // Project point onto cylinder axis
    vec3 rel = p - start;
    vec3 proj = rel - dir * dot(rel, dir);
    return length(proj) - radius;
}

// Distance map, also keeps track of type (0 = icosahedron, 1 = sphere, 2 = cylinder edges)
vec2 map(vec3 p){
    float d = 1e6;
    float type = 0.;

    vec3 a = normalize(vec3(1., 1., 1.));
    vec3 b = normalize(vec3(-1., 1., 1.));
    vec3 c = normalize(vec3(1., -1., 1.));
    vec3 e = normalize(vec3(1., 1., -1.));

    d = min(d, sdPlane(p, a, 1.));
    d = min(d, sdPlane(p, -a, 1.));
    d = min(d, sdPlane(p, b, 1.));
    d = min(d, sdPlane(p, -b, 1.));
    d = min(d, sdPlane(p, c, 1.));
    d = min(d, sdPlane(p, -c, 1.));
    d = min(d, sdPlane(p, e, 1.));
    d = min(d, sdPlane(p, -e, 1.));

    a = normalize(vec3(0., PHI, 1. / PHI));
    b = normalize(vec3(0., PHI, -1. / PHI));

    d = min(d, sdPlane(p, a, 1.));
    d = min(d, sdPlane(p, -a, 1.));
    d = min(d, sdPlane(p, b, 1.));
    d = min(d, sdPlane(p, -b, 1.));

    a = normalize(vec3(1. / PHI, 0., PHI));
    b = normalize(vec3(-1. / PHI, 0., PHI));

    d = min(d, sdPlane(p, a, 1.));
    d = min(d, sdPlane(p, -a, 1.));
    d = min(d, sdPlane(p, b, 1.));
    d = min(d, sdPlane(p, -b, 1.));

    a = normalize(vec3(PHI, 1. / PHI, 0.));
    b = normalize(vec3(PHI, -1. / PHI, 0.));

    d = min(d, sdPlane(p, a, 1.));
    d = min(d, sdPlane(p, -a, 1.));
    d = min(d, sdPlane(p, b, 1.));
    d = min(d, sdPlane(p, -b, 1.));

    float dCyl = 1e6;
    for (int i = 0; i < 30; i++) {
        vec3 a = icosahedronVertices[icosahedronEdges[i].x] / .81;
        vec3 b = icosahedronVertices[icosahedronEdges[i].y] / .81;
        vec3 dir = normalize(b - a);
        dCyl = min(dCyl, sdCylinder(p, a, dir, 0.05));
    }
    if (dCyl < d){
        d = dCyl;
        type = 2.;
    }

    return vec2(d, type);
}

// Get normal vector
vec3 getNormal(vec3 p) {
    float d = map(p).x;
    vec2 e = vec2(0.0001, 0.0);

    vec3 n = d - vec3(
        map(p - e.xyy).x,
        map(p - e.yxy).x,
        map(p - e.yyx).x
    );

    return normalize(n);
}

// Get texture color based on position and normal
vec3 tex3D(sampler2D tex, vec3 p, vec3 n ){
    n = max(abs(n), 0.001);         // Prevent divide-by-zero
    n /= dot(n, vec3(1.0));         // Normalize weights to sum to 1

    vec3 xProj = texture(tex, p.yz).rgb;
    vec3 yProj = texture(tex, p.zx).rgb;
    vec3 zProj = texture(tex, p.xy).rgb;

    return xProj * n.x + yProj * n.y + zProj * n.z;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = (fragCoord * 2. - iResolution.xy)/iResolution.y;
    float ti = iTime * .3;

    // Add two rotations to ray origin and target direction
    mat2 R = Rot(ti * .5);
    mat2 R2 = Rot(ti * .2);
    vec3 ro = vec3(0, 0, -.4);
    ro.xz = R * ro.xz;
    ro.yz = R2 * ro.yz;
    R = Rot(ti * .4);
    R2 = Rot(ti * .3);
    vec3 ta = ro + vec3(1.2 * uv, 1);
    ta.xz = R * ta.xz;
    ta.yz = R2 * ta.yz;
    vec3 rd = normalize(ta-ro);
    vec3 lightPos = vec3(0, 0.4, 0);

    // Raymarching setup
    float maxIters = 100.;
    float minDist = 0.001;
    float t = 0.;
    float iters = 0.;
    float reflCount = 0.;
    vec3 col = vec3(0);
    vec3 prevCol = col;

    for (float i; i<maxIters; i++){
        vec3 pos = ro + t * rd;

        vec2 map = map(pos);
        float d = map.x;
        float type = map.y;

        if (d < minDist){
            vec3 n = getNormal(pos);

            // If we hit a cylinder edge, don't do any more reflections and calculate final color
            if (type == 2.){
                col += tex3D(iChannel0, pos * 5., n);

                if (reflCount > 0.){
                    float p = pow(.6, reflCount);
                    col = mix(prevCol, col, p);
                }

                break;
            }

            // Color based on normal vector with variations over time
            vec3 m = n;
            m.xz *= R;
            m.yz *= R2;
            col = vec3(abs(m));

            // Diffuse lighting
            vec3 l = normalize(lightPos - pos);
            float diffuse = max(dot(n, l), 0.);
            col = col * (.5 + .5*diffuse);

            // If we have a reflected ray, mix previous color with current color
            if (reflCount > 0.){
                float p = pow(.6, reflCount);
                col = mix(prevCol, col, p);
            }

            prevCol = col;

            if (reflCount == 5.) {
                break;
            }

            // Set up reflected ray
            ro = pos;
            rd = reflect(rd, n);
            t = 0.01;

            reflCount++;
        }

        t += d;
    }

    // Final effect to make everything look nicer
    col *= col;

    fragColor = vec4(col,1.0);
}
```

## Algorithm

### Substitution Table (Reference → AudioJones)

| Reference | Replace With | Reason |
|-----------|-------------|--------|
| Hardcoded icosahedron vertices (12) | `uniform vec3 vertices[20]` + `uniform int vertexCount` | Support all 5 solids (max 20 vertices for dodecahedron) |
| Hardcoded icosahedron edges (30) | `uniform vec3 edgeA[30]`, `uniform vec3 edgeB[30]`, `uniform int edgeCount` | Pre-resolved edge endpoints from CPU, avoids ivec2 index arrays |
| Hardcoded 20 face plane normals | `uniform vec3 faceNormals[20]`, `uniform int faceCount` | Face normals derived from dual solid's vertices (see below) |
| `sdCylinder()` (infinite) | IQ's `sdCapsule()` (clamped) | Properly handles edge endpoints instead of infinite cylinders |
| `iChannel0` triplanar texturing on edges | Gradient LUT sampling: `texture(gradientLUT, vec2(reflDepth, 0.5))` at contrasting position | Standard generator coloring |
| `abs(normal)` face color | Gradient LUT sampling by `reflCount / maxBounces` | Reflection depth maps to gradient position |
| `col *= col` tonemap | Remove — gradient LUT handles color mapping | Avoids double-darkening with LUT colors |
| `iTime * .3` | `uniform float time` (accumulated on CPU) | Speed accumulation on CPU per conventions |
| `iResolution` | `uniform vec2 resolution` | Standard uniform |
| Fixed 5 reflection bounces | `uniform int maxBounces` (1-8) | Configurable |
| Fixed camera distance `-.4` | `uniform float cameraDistance` (0.1-0.8) | Configurable zoom |
| Fixed edge radius `0.05` | `uniform float edgeRadius` (0.01-0.15) | Configurable thickness |
| Hardcoded rotation speeds `.5, .2, .4, .3` | `uniform float angleXZ`, `uniform float angleYZ` (accumulated on CPU) | Two-axis rotation, modulatable speeds |
| `mainImage(out vec4, in vec2)` | `void main()` with `fragTexCoord`, `gl_FragColor` | raylib fragment shader convention |
| Hardcoded plane offset `1.` | `uniform float planeOffset` | Varies per solid — inradius of the face planes |

### Face Normal Derivation via Platonic Duality

The face normals of a platonic solid are the vertex positions of its dual solid (both normalized to unit sphere):

| Solid | Face Count | Dual Solid | Dual Vertex Count |
|-------|-----------|------------|-------------------|
| Tetrahedron | 4 | Tetrahedron (self-dual) | 4 |
| Cube | 6 | Octahedron | 6 |
| Octahedron | 8 | Cube | 8 |
| Dodecahedron | 12 | Icosahedron | 12 |
| Icosahedron | 20 | Dodecahedron | 20 |

CPU computes the dual index: `int dualIdx[] = {0, 2, 1, 4, 3}` (SHAPES index mapping).
Upload `SHAPES[dualIdx[shapeIdx]].vertices` as `faceNormals[]`.

### Plane Offset Per Solid

The plane offset `h` in `sdPlane(p, n, h) = dot(p, n) + h` determines how far the face planes sit from the origin. For unit-sphere-inscribed solids, this is the distance from center to face (inradius ratio):

| Solid | planeOffset |
|-------|------------|
| Tetrahedron | 0.3333 |
| Cube | 0.5774 |
| Octahedron | 0.5774 |
| Dodecahedron | 0.7946 |
| Icosahedron | 0.7946 |

CPU precomputes and uploads as a single float uniform.

### Edge Scaling

The reference divides edge vertices by `0.81` to scale edges slightly outside the face planes (preventing z-fighting). Each solid needs its own edge scale factor so edges sit on the surface. CPU computes: `edgeScale = 1.0 / planeOffset` (edges scaled to intersect face planes).

### Diffuse Lighting

Keep the reference's diffuse lighting model: `col * (0.5 + 0.5 * diffuse)` with a fixed light at origin. This provides face shading variation within each gradient color.

### Reflection Color Mixing

Keep the reference's reflection accumulation: `mix(prevCol, col, pow(reflectivity, reflCount))`. Add `reflectivity` as a configurable parameter (reference uses 0.6).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| shape | int | 0-4 | 4 | Platonic solid: 0=tetra, 1=cube, 2=octa, 3=dodeca, 4=icosa |
| orbitSpeedXZ | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.5 | Camera orbit rate around vertical axis (rad/s) |
| orbitSpeedYZ | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.2 | Camera orbit rate around horizontal axis (rad/s) |
| cameraDistance | float | 0.1-0.8 | 0.4 | Distance from origin to camera |
| maxBounces | int | 1-8 | 5 | Maximum reflection bounces |
| reflectivity | float | 0.1-1.0 | 0.6 | Reflection color mixing factor per bounce |
| edgeRadius | float | 0.01-0.15 | 0.05 | Wireframe edge thickness |
| edgeGlow | float | 0.0-2.0 | 1.0 | Edge brightness multiplier |
| maxIterations | int | 32-128 | 80 | Raymarch step limit (performance knob) |

## Modulation Candidates

- **maxBounces**: reflection depth — shallow at low energy, deep kaleidoscopic at peaks
- **cameraDistance**: zoom — pulls in/out with audio, changing perspective dramatically
- **orbitSpeedXZ / orbitSpeedYZ**: rotation rate — speeds shift the visual rhythm
- **edgeRadius**: wireframe prominence — thicker edges dominate, thinner ones vanish
- **edgeGlow**: edge brightness — pulses with transients
- **reflectivity**: how much each bounce preserves — high values make deep reflections vivid, low values fade them

### Interaction Patterns

- **Cascading threshold (maxBounces × reflectivity)**: When maxBounces is low (1-2), reflectivity has no visible effect — there's nothing to accumulate. As bounces increase past 3-4, reflectivity determines whether deep reflections are vivid or ghostly. Mapping maxBounces to broadband energy and reflectivity to a different band creates gated complexity — the visual only "opens up" when both conditions coincide.
- **Competing forces (edgeRadius × cameraDistance)**: Thick edges consume screen space, reducing visible face area for reflections. Close camera + thick edges = wireframe-dominated. Far camera + thin edges = pure mirror hall. The tension between these two parameters shifts the visual identity between "glowing cage" and "infinite reflections."

## Notes

- **Performance**: The SDF evaluates all face planes + all edge capsules per raymarching step, multiplied by reflection bounces. Icosahedron (20 faces, 30 edges) is heaviest; tetrahedron (4 faces, 6 edges) is lightest. The `maxIterations` parameter is the primary performance knob.
- **Edge z-fighting**: Edge scale factor must be tuned per solid so capsules sit flush with face planes. The `1.0 / planeOffset` formula should work but may need per-solid fine-tuning.
- **Reflection ray offset**: The reference uses `t = 0.01` to offset the reflected ray origin. This prevents self-intersection. If artifacts appear with smaller solids (tetrahedron), this offset may need to scale with solid size.
- **Gradient LUT mapping**: Face color maps `reflCount / float(maxBounces)` to gradient U coordinate (0.0 = first bounce, 1.0 = deepest). Edge color samples at a fixed offset (e.g., 0.0 or 1.0) for contrast.
