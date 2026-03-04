# Spin Cage

A spinning wireframe platonic solid whose fast rotation causes overlapping projected edges to create emergent geometric interference patterns. The shape itself is a simple scaffold — the visual interest comes from the moiré-like webs that form as 3D edges alias against each other in 2D projection at high rotational speed. Five selectable platonic solids (tetrahedron through icosahedron) control pattern density via edge count.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Output stage — trail boost → generators → transforms
- **Section Index**: 10 (Geometric)

## Attribution

- **Based on**: "cube!" by catson
- **Source**: https://www.shadertoy.com/view/3XcBDl
- **License**: CC BY-NC-SA 3.0
- **Also referenced**: "bitwise cube (catson fork)" by jyn — https://www.shadertoy.com/view/WXcBWs (CC BY-NC-SA 3.0)

## References

- [Generating Platonic Solids in C++](https://danielsieger.com/blog/2021/01/03/generating-platonic-solids.html) - Vertex coordinates for all 5 platonic solids on unit sphere
- [Platonic Solids](https://paulbourke.net/geometry/platonic/) - Vertex coordinates, edge/face connectivity, and POVRay reference data
- [Rotation matrix (Wikipedia)](https://en.wikipedia.org/wiki/Rotation_matrix#In_three_dimensions) - 3-axis rotation construction (referenced in shader)

## Reference Code

### catson's cube shader (primary reference)

```glsl
// "cube!" by catson — https://www.shadertoy.com/view/3XcBDl
// License: CC BY-NC-SA 3.0

// goes from world space -> screen space, where:
// camera is centered on (0, 0, 0), and
// +x is right, +y is up, +z is out of screen
// note, of course, discontinuity at any p where z = -4
// https://youtu.be/qjWkNZ0SXfo?si=5SUtVXRjpheI7D63
vec2 project(vec3 p) { return vec2(p.x, p.y) / (p.z + 4.); }

// returns the distance from p (now in screen space) to the line segment a-b
// https://www.shadertoy.com/view/Wlfyzl
float line(in vec2 p, in vec2 a, in vec2 b) {
    vec2 ba = b - a;
    vec2 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
    return length(pa - h * ba);
}

// returns 1.0 if given distance is "on"
float on(float d) { return smoothstep(2./iResolution.y, 0., d); }

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord -.5*iResolution.xy)/iResolution.y;

    float t = iTime/3., ty = 50.*cos(t*1.5);

    // https://en.wikipedia.org/wiki/Rotation_matrix#In_three_dimensions
    mat3 R = mat3( // x-axis
         1,      0,  0,
         0, cos(t), -sin(t),
         0, sin(t),  cos(t)
    ) * mat3(      // y-axis
         cos(ty), 0, sin(ty),
         0,       1, 0,
        -sin(ty), 0, cos(ty)
    ) * mat3(      // z-axis
         cos(t), -sin(t), 0,
         sin(t),  cos(t), 0,
         0,       0,      1
    );

    // project world space cube vertices into screen space
    vec2 p11 = project(R * vec3(-1, -1, -1)),
         p12 = project(R * vec3( 1, -1, -1)),
         p13 = project(R * vec3( 1,  1, -1)),
         p14 = project(R * vec3(-1,  1, -1)),
         p21 = project(R * vec3(-1, -1,  1)),
         p22 = project(R * vec3( 1, -1,  1)),
         p23 = project(R * vec3( 1,  1,  1)),
         p24 = project(R * vec3(-1,  1,  1));

    // draw them
    fragColor = vec4(
        // face 1
        on(line( uv, p11, p12 )) +
        on(line( uv, p12, p13 )) +
        on(line( uv, p13, p14 )) +
        on(line( uv, p14, p11 )) +
        // face 2
        on(line( uv, p21, p22 )) +
        on(line( uv, p22, p23 )) +
        on(line( uv, p23, p24 )) +
        on(line( uv, p24, p21 )) +
        // connect them
        on(line( uv, p11, p21 )) +
        on(line( uv, p12, p22 )) +
        on(line( uv, p13, p23 )) +
        on(line( uv, p14, p24 ))
    );
}
```

### Line glow pattern (from Filaments generator)

```glsl
// Inverse-distance glow used by existing generators
float glow = GLOW_WIDTH / (GLOW_WIDTH + dist);
result += color * glow * glowIntensity * (baseBright + mag);
result = tanh(result);  // soft clamp
```

## Algorithm

### CPU-side (Setup function)

Move all rotation, projection, and vertex computation to the C++ Setup function — the shader should only do distance field evaluation:

1. Accumulate per-axis rotation angles: `angleX += speedX * speedMult * deltaTime` (same for Y, Z)
2. Build 3x3 rotation matrix from Euler angles (same Rx * Ry * Rz construction as reference)
3. For the selected shape, rotate each vertex by the matrix, then perspective-project to 2D: `projected.xy = (rotated.xy / (rotated.z + perspective)) * scale`
4. Compute per-edge `t` value based on color mode:
   - **Edge Index mode**: `t[i] = float(i) / edgeCount` — fixed mapping, edge 0 = start of gradient, edge N = end
   - **Depth mode**: `t[i] = normalize(avgZ(edgeEndpoints))` — average rotated Z of the two endpoints, mapped 0..1. Changes dynamically as shape tumbles.
5. Pack projected edge endpoints + t value: each `edges[i] = vec4(a.x, a.y, b.x, b.y)`, `edgeT[i] = t`
6. Upload `edges[]`, `edgeT[]`, and `edgeCount` as uniforms

### GPU-side (Fragment shader)

The shader is minimal — a distance-field loop with shared `t` for color and FFT:

1. Compute centered UV: `uv = (fragCoord - 0.5 * resolution) / resolution.y`
2. Loop over `edgeCount` edges, compute line-segment distance for each
3. Use `edgeT[i]` to sample both gradient LUT color and FFT magnitude:
   - Color: `vec3 color = texture(gradientLUT, vec2(edgeT[i], 0.5)).rgb`
   - FFT freq: log-space interpolation from `baseFreq` to `maxFreq` using `edgeT[i]`, then sample `fftTexture`
   - Brightness: `baseBright + pow(clamp(fftMag * gain, 0, 1), curve)`
4. Accumulate glow per-edge: `result += color * glow * glowIntensity * brightness`
5. Apply `tanh()` soft clamp to prevent blowout from overlapping edges

The `t` value unifies color and audio: in Edge Index mode, edge 0 is bass-colored and bass-reactive, edge N is treble-colored and treble-reactive. In Depth mode, the mapping rotates with the shape — whichever edge faces the camera becomes the "bass edge" (or treble, depending on orientation), so the audio response itself tumbles with the 3D rotation.

### What to keep from reference
- Line-segment distance function (`line()`) — use verbatim
- Perspective projection formula: `xy / (z + d)` — same math, moved to CPU
- Additive edge accumulation — overlapping edges naturally bloom brighter at intersection nodes

### What to replace
- `iTime`-based rotation → per-axis accumulated angles with speed params
- `smoothstep` on/off → inverse-distance glow falloff (Filaments-style `GLOW_WIDTH / (GLOW_WIDTH + dist)`)
- Hardcoded cube vertices → shape-selected vertex/edge arrays (CPU-side)
- White output → gradient LUT color + FFT brightness, both driven by shared `t` value per edge

### Vertex data (unit-sphere normalized)

All vertex sets are normalized to unit sphere (distance 1.0 from origin). The `scale` parameter multiplies before projection.

**Tetrahedron** (4 vertices, 6 edges):
Vertices at (0, 0, 1), (√(8/9), 0, −1/3), (−√(2/9), √(2/3), −1/3), (−√(2/9), −√(2/3), −1/3).
Edges: every pair — (0,1), (0,2), (0,3), (1,2), (1,3), (2,3).

**Cube / Hexahedron** (8 vertices, 12 edges):
Vertices at all (±a, ±a, ±a) where a = 1/√3.
Edges: 4 on front face, 4 on back face, 4 connecting front to back (same topology as reference code).

**Octahedron** (6 vertices, 12 edges):
Vertices at (±1, 0, 0), (0, ±1, 0), (0, 0, ±1).
Each vertex connects to 4 neighbors (all except its opposite). 12 edges total.

**Dodecahedron** (20 vertices, 30 edges):
Uses golden ratio φ = (1+√5)/2. Vertices: 8 at (±1, ±1, ±1)/√3, plus 12 at permutations of (0, ±1/φ, ±φ)/√3, all normalized to unit sphere. Each vertex has degree 3.

**Icosahedron** (12 vertices, 30 edges):
Vertices at cyclic permutations of (0, ±1, ±φ) normalized to unit sphere. Each vertex has degree 5.

Edge connectivity tables are derived from face definitions — each face contributes its perimeter edges, deduplicated.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| shape | int (enum) | 0-4 | 1 (Cube) | Platonic solid: Tetrahedron, Cube, Octahedron, Dodecahedron, Icosahedron |
| colorMode | int (enum) | 0-1 | 0 (Edge Index) | Shared t mapping: 0 = by edge index, 1 = by depth |
| speedX | float | -PI..+PI | 0.33 | X-axis rotation speed (rad/s) |
| speedY | float | -PI..+PI | 0.33 | Y-axis rotation speed (rad/s) |
| speedZ | float | -PI..+PI | 0.33 | Z-axis rotation speed (rad/s) |
| speedMult | float | 0.1-100.0 | 1.0 | Global speed multiplier — pushes into pattern territory |
| perspective | float | 1.0-20.0 | 4.0 | Camera distance / projection depth |
| scale | float | 0.1-5.0 | 1.0 | Wireframe size multiplier |
| lineWidth | float | 0.5-10.0 | 2.0 | Glow falloff width (pixel units) |
| glowIntensity | float | 0.1-5.0 | 1.0 | Glow brightness |
| baseFreq | float | 27.5-440.0 | 55.0 | Low end of FFT frequency spread (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | High end of FFT frequency spread (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve (contrast) |
| baseBright | float | 0.0-1.0 | 0.0 | Base edge visibility without audio |

## Modulation Candidates

- **speedMult**: Globally accelerates/decelerates rotation — bursts of speed create pattern flashes, slow moments show the bare wireframe
- **speedX / speedY / speedZ**: Individual axis modulation creates lopsided tumbling and moments of alignment
- **perspective**: Breathing zoom between flat orthographic and dramatic depth
- **scale**: Pulsing wireframe size
- **lineWidth**: Thickening/thinning lines changes how much edges overlap visually
- **glowIntensity**: Brightness pulsing

### Interaction Patterns

**speedMult × perspective (competing forces)**: High speed + close camera (low perspective) creates wild asymmetric pattern bursts where back-face edges shrink dramatically. High speed + far camera (high perspective) produces flatter, more symmetric mandala-like patterns. These two parameters push the visual character in opposite directions — modulating both creates shifting regimes.

**Per-axis speed ratios (resonance/alignment)**: When two axis speeds are near-integer ratios, the projected edges repeatedly trace the same paths creating simple repeating figures. Incommensurate ratios produce complex quasi-periodic patterns. Modulating one axis speed against steady others creates moments where patterns briefly crystallize then scatter — rare alignment events.

**lineWidth × speedMult (cascading threshold)**: At narrow line width, only exact edge overlaps produce visible bright nodes. Increasing line width lowers the "threshold" for edges to interact visually, turning sparse flickers into dense webs. Speed must be high enough for edges to overlap at all — lineWidth controls whether those overlaps register.

## Notes

- Max 30 edges (icosahedron) means 30 distance evaluations per fragment — well within fragment shader budget
- All vertex rotation and projection happens on CPU in the Setup function (max 20 vertices = trivial), keeping the shader minimal
- The `tanh()` soft clamp prevents additive blowout when many edges overlap at high speed
- Edge connectivity tables should be precomputed once at init, not per-frame — only projected positions update each frame
- The `perspective` parameter is the `+ d` in the projection denominator; values near 1.0 create extreme foreshortening, values near 20.0 approach orthographic
