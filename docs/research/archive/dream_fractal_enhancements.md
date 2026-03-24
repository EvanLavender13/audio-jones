# Dream Fractal Enhancements

Four additive enhancements to Dream Fractal: swappable carve modes that change the SDF primitive used to cut space, a fold mode that applies space-folding operations before carving, orbit trap coloring that maps iteration geometry to the gradient LUT, and a Julia offset that smoothly morphs the entire fractal structure when animated. All features are additive — Dream Fractal's existing look is carve mode 0 (sphere), fold disabled, coloring mode 0, Julia offset (0,0,0).


## Classification

- **Category**: GENERATORS > Field (enhancement to existing Dream Fractal)
- **Pipeline Position**: Same — generator stage, blended into accumulation buffer
- **Render Model**: Single-pass fragment shader (raymarching) — same as base effect

## Attribution

- **"Julia - Traps 1"** by iq — https://www.shadertoy.com/view/4d23WG — CC BY-NC-SA 3.0
- **"Mandelbulb Cathedral"** by erezrob — https://www.shadertoy.com/view/33tfDN
- **"Mandelbulb attempt"** by Krafpy — https://www.shadertoy.com/view/sdlGD8
- **"mandelbrot orbit trap periods"** by elenzil — https://www.shadertoy.com/view/Wl2Gz1

## References

- [Geometric Orbit Traps](https://iquilezles.org/articles/ftrapsgeometric/) — Inigo Quilez, trap shape definitions and distance functions
- [3D Julia Sets](https://iquilezles.org/articles/juliasets3d/) — Inigo Quilez, Julia constant in raymarched fractals
- [Distance Estimated 3D Fractals III: Folding Space](http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/) — Syntopia, tetrahedron fold operations
- [Fractal Rendering](https://cglearn.eu/pub/advanced-computer-graphics/fractal-rendering) — Box fold and sphere fold GLSL implementations
- [Menger Sponge Breakdown](https://lucodivo.github.io/menger_sponge.html) — Cross SDF and fold operations
- [Rendering the Mandelbulb](https://www.4rknova.com/blog/2025/09/01/mandelbulb) — 3D orbit trap distance functions with 4 trap modes

## Reference Code

### Dream Fractal Base Iteration (existing)

From the Dream Fractal research doc — the core IFS loop that these enhancements modify:

```glsl
// "Dream Within A Dream" by OldEclipse
// https://www.shadertoy.com/view/fclGWs

// Repeatedly carve spheres out of space
d=0., s=1.;
for(j=0.;j++<8.;){
    p*=3.;
    s*=3.;
    d=max(d,(.9-length(abs(mod(p-1.,2.)-1.)))/s);
    p.xz*=.1*mat2(8,6,-6,8);
}
```

Key line: `.9-length(abs(mod(p-1.,2.)-1.))` — sphere SDF (radius 0.9) evaluated at a folded repeating-grid position. `length()` is the sphere primitive; replacing it with other SDF primitives changes the carving shape.

### Orbit Trap in 3D Raymarching (Mandelbulb Cathedral by erezrob)

```glsl
// "Mandelbulb Cathedral" by erezrob
// https://www.shadertoy.com/view/33tfDN

vec2 mandelbulb(vec3 p) {
    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;
    float trap = 1e10;

    for (int i = 0; i < 12; i++) {
        r = length(z);
        if (r > 2.0) break;

        float theta = acos(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, POWER - 1.0) * POWER * dr + 1.0;

        float zr = pow(r, POWER);
        theta *= POWER;
        phi *= POWER;

        z = zr * vec3(
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
        ) + p;

        trap = min(trap, length(z.xz));  // orbit trap: min distance to Y axis
    }
    return vec2(0.5 * log(r) * r / dr, trap);
}

// Trap-to-color mapping (in mainImage):
vec3 matCol = 0.5 + 0.5 * cos(vec3(0.0, 0.6, 1.0) + trap * 4.0 + vec3(0.0, 1.0, 2.0));
matCol *= matCol;
```

### Orbit Trap Coloring Function (Krafpy)

```glsl
// "Mandelbulb attempt" by Krafpy
// https://www.shadertoy.com/view/sdlGD8

float SDF(vec3 p, out float trap){
    vec3 z = p;
    float n = 8.;
    float r = 0.;
    float dr = 1.;

    trap = dot(z,z);  // initial trap = squared distance to origin

    for(int i = 0; i < ITERATIONS; i++){
        r = length(z);
        if(r >= OUTRANGE) break;

        trap = min(dot(z,z), trap);  // accumulate minimum squared distance

        // ... power iteration ...
        z = r * vec3(cos(theta)*cos(phi), sin(theta)*cos(phi), sin(phi)) + p;
    }
    return 0.5 * log(r) * r / dr;
}

vec3 coloring(float trap){
    trap = clamp(pow(trap, 8.), 0., 1.);
    vec3 col = mix(vec3(1., 0.81, 0.64), vec3(0.8, 0.4, 0.1), trap);
    return col;
}
```

### IQ Julia Traps — Minimal Point Trap

```glsl
// "Julia - Traps 1" by iq
// https://www.shadertoy.com/view/4d23WG
// License: CC BY-NC-SA 3.0

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 z = 1.15*(2.0*fragCoord-iResolution.xy)/iResolution.y;

    vec2 c = 0.51*cos(vec2(0,699)+0.1*iTime) -
             0.25*cos(vec2(0,699)+0.2*iTime );

    float f = 1e20;
    for( int i=0; i<128; i++ )
    {
        z = vec2( z.x*z.x-z.y*z.y, 2.0*z.x*z.y ) + c;
        f = min( f, dot(z,z) );  // point trap at origin
    }

    f = 1.0+log2(f)/16.0;  // log remap for smooth gradient

    fragColor = vec4(f,f*f,f*f*f,1.0);
}
```

### 3D Orbit Trap Shapes (from 4rknova Mandelbulb article)

```glsl
// https://www.4rknova.com/blog/2025/09/01/mandelbulb

float trapDistance(vec3 z) {
    float trap = 0.0;
    if      (trapMode == 0) trap = abs(z.y);             // Plane trap (Y=0)
    else if (trapMode == 1) trap = abs(length(z) - 1.0); // Spherical shell (r=1)
    else if (trapMode == 2) trap = length(z.xy);         // Axis trap (Z axis)
    else if (trapMode == 3) trap = max(max(abs(z.x), abs(z.y)), abs(z.z)); // Cube
    return trap;
}
```

### Box Fold (from cglearn.eu)

```glsl
// https://cglearn.eu/pub/advanced-computer-graphics/fractal-rendering
void boxFold(inout vec3 z, float d) {
    z = clamp(z, -1.0, 1.0) * 2.0 - z;
}
```

### Tetrahedron Fold (from Syntopia)

```glsl
// http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/
// Three plane mirrors for tetrahedral symmetry
if(z.x + z.y < 0.0) z.xy = -z.yx;
if(z.x + z.z < 0.0) z.xz = -z.zx;
if(z.y + z.z < 0.0) z.zy = -z.yz;
```

### Cross SDF (from Menger Sponge Breakdown)

```glsl
// https://lucodivo.github.io/menger_sponge.html
float sdCross(vec3 rayPos) {
    const float halfWidth = 1.0;
    const vec3 corner = vec3(halfWidth, halfWidth, halfWidth);
    vec3 foldedPos = abs(rayPos);
    vec3 ctr = foldedPos - corner;
    float minComp = min(min(ctr.x, ctr.y), ctr.z);
    float maxComp = max(max(ctr.x, ctr.y), ctr.z);
    float midComp = ctr.x + ctr.y + ctr.z - minComp - maxComp;
    vec2 closestOutsidePoint = max(vec2(minComp, midComp), 0.0);
    vec2 closestInsidePoint = min(vec2(midComp, maxComp), 0.0);
    return length(closestOutsidePoint) - length(closestInsidePoint);
}
```

## Algorithm

### Enhancement 1: Carve Modes

The existing carving line uses `length()` for a sphere SDF:

```glsl
vec3 q = abs(mod(p - 1.0, 2.0) - 1.0);
d = max(d, (carveRadius - length(q)) / s);
```

Replace `length(q)` with different SDF primitives to change the carving shape. Uses a new reusable `CarveMode` enum in `src/config/carve_mode.h` (parallel to `fold_mode.h`):

| CarveMode | Name | Replace `length(q)` with | Visual Character |
|-----------|------|--------------------------|-----------------|
| CARVE_SPHERE | Sphere | `length(q)` | Round tunnels (original) |
| CARVE_BOX | Box | `max(max(q.x, q.y), q.z)` | Angular corridors |
| CARVE_CROSS | Cross | `min(max(q.y, q.z), min(max(q.x, q.z), max(q.x, q.y)))` | Crosshair voids (Menger-like) |
| CARVE_CYLINDER | Cylinder | `length(q.xz)` | Tubular channels |
| CARVE_OCTAHEDRON | Octahedron | `(q.x + q.y + q.z) * 0.577` | Diamond cavities |

The `carveRadius` param controls carving size for all modes. The `0.577` (1/sqrt(3)) normalizes the octahedron so `carveRadius` behaves consistently across modes.

### Enhancement 2: Orbit Trap Coloring

Add trap accumulation inside the existing IFS iteration loop:

- Before loop: `float trap = 1e10;`
- Each iteration (after computing `q`): `trap = min(trap, trapDistFn(q));`
- After loop: map `trap` to gradient LUT

Trap shapes (adapted from 4rknova reference to IFS context — `q` is the folded cell position):

| Mode | Name | Distance Function |
|------|------|------------------|
| 0 | Off | No trap tracking (use turbulence) |
| 1 | Point | `length(q)` — distance to cell center |
| 2 | Plane | `abs(q.y)` — distance to Y=0 plane |
| 3 | Shell | `abs(length(q) - trapRadius)` — distance to sphere shell |
| 4 | Cross | `min(length(q.xy), min(length(q.yz), length(q.xz)))` — distance to axes |

Trap → gradient LUT mapping (adapted from IQ's log remap):

```glsl
float t = clamp(1.0 + log2(max(trap, 1e-6)) / trapColorScale, 0.0, 1.0);
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
```

Coloring mode selection:
- **Mode 0** (Turbulence): Existing Dream Fractal coloring — unchanged
- **Mode 1** (Orbit Trap): Replace turbulence with `trap → gradient LUT` mapping
- **Mode 2** (Hybrid): Turbulence passes still warp `q` for spatial variation; trap distance selects gradient position instead of `length(q)`

### Enhancement 3: Julia Offset

Add per-iteration offset after twist rotation in the IFS loop:

```glsl
p += juliaOffset;  // vec3(juliaX, juliaY, juliaZ)
```

At `(0, 0, 0)` = original fractal. Non-zero values deform the structure. Coordinates stay bounded thanks to `mod()` wrapping in the carving step, so no precision issues. Slowly animating via modulation creates continuous structural morphing — tunnels warp, branches appear/disappear, the geometry flows.

Three separate float config fields (`juliaX`, `juliaY`, `juliaZ`) for independent modulation per axis.

### Enhancement 4: Fold Mode

Optional space-folding operation applied at the start of each iteration, before carving. Uses the existing `FoldMode` enum from `src/config/fold_mode.h`. Enabled via `foldEnabled` toggle, then `foldMode` selects which fold:

| FoldMode | Name | Operation |
|----------|------|-----------|
| FOLD_BOX | Box | `z = clamp(z, -1, 1) * 2 - z` |
| FOLD_MANDELBOX | Mandelbox | Box clamp + sphere inversion |
| FOLD_SIERPINSKI | Sierpinski | Tetrahedral plane reflections |
| FOLD_MENGER | Menger | Abs + full 3-axis sort |
| FOLD_KALI | Kali | Sphere inversion (abs/dot) |
| FOLD_BURNING_SHIP | Burning Ship | Abs cross-products |

When `foldEnabled` is false, no fold is applied (original behavior). The fold implementations are already defined in the shader fold library used by other fractal effects.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| carveMode | CarveMode | 0-4 | CARVE_SPHERE | SDF carving shape: sphere/box/cross/cylinder/octahedron |
| foldEnabled | bool | - | false | Enable space-folding before carving |
| foldMode | FoldMode | 0-5 | FOLD_BOX | Space fold operation (only when foldEnabled) |
| trapMode | int | 0-4 | 0 | Orbit trap shape: off/point/plane/shell/cross |
| trapRadius | float | 0.1-3.0 | 1.0 | Sphere shell trap radius (only for shell trap) |
| trapColorScale | float | 1.0-16.0 | 4.0 | Log scale divisor for trap -> LUT mapping |
| colorMode | int | 0-2 | 0 | Coloring: turbulence/orbit trap/hybrid |
| juliaX | float | -1.0-1.0 | 0.0 | Julia offset X - structural deformation |
| juliaY | float | -1.0-1.0 | 0.0 | Julia offset Y - structural deformation |
| juliaZ | float | -1.0-1.0 | 0.0 | Julia offset Z - structural deformation |

## Modulation Candidates

- **juliaX/Y/Z**: Primary morphing parameters - modulating creates continuous structural deformation. Three independent axes allow complex motion paths through fractal configuration space.
- **trapRadius**: Breathing the trap shell radius creates pulsing color boundary patterns
- **trapColorScale**: Shifts gradient density - low = smooth color fields, high = tight banding
- **foldEnabled**: Toggle creates sudden crystallization / dissolution of symmetry (discrete, best for song-section transitions)

### Interaction Patterns

- **carveMode x carveRadius** (cascading threshold): Different carving shapes create different structure density at the same radius. Box mode at small radius = nearly solid; sphere mode at same radius = mostly void. `carveRadius` modulation produces dramatically different dynamics depending on active carve mode.
- **juliaOffset x driftSpeed** (competing forces): Julia offset deforms the structure; drift moves the camera through it. High julia + slow drift = surreal morphing. High drift + zero julia = tunneling through static geometry. Modulating in opposition creates push-pull between exploration and mutation.
- **trapMode x turbulenceIntensity** (resonance): In hybrid coloring mode, orbit trap boundaries align or conflict with turbulence banding. Both high = moire-like color interference at fractal edges. Both low = calm, smooth gradients.
- **foldEnabled x twist** (cascading threshold): Space folding creates discrete symmetry; twist breaks it progressively. At zero twist + fold enabled = crystalline structure. Increasing twist dissolves the order into chaos. The specific `foldMode` determines the symmetry character - sierpinski gives tetrahedral, menger gives cubic, etc.

## Notes

- **Implementation order**: Carve modes are simplest (one `if` chain replacing `length(q)`). Orbit traps add one accumulator variable and a coloring branch. Julia offset is one line. Fold mode reuses existing shader fold library. All can be implemented incrementally.
- **New shared enum**: `CarveMode` in `src/config/carve_mode.h` - reusable by any future fractal effect that needs swappable SDF primitives.
- **Performance**: All enhancements add negligible per-iteration cost. Shader branching on `carveMode`/`foldMode`/`trapMode`/`colorMode` is fast since all pixels take the same branch (uniform-driven, not per-pixel).
- **Julia offset range**: ±1.0 is conservative. At larger offsets the fractal may dissolve. Useful range depends on `scaleFactor` — larger scale = smaller useful julia range. May need empirical tuning after implementation.
- **Octahedron normalization**: The `0.577` constant (1/sqrt(3)) normalizes octahedron distance so `carveRadius` controls size consistently across all carve modes. May need empirical adjustment.
- **Orbit trap with turbulence (hybrid)**: The three turbulence passes still warp `q` for spatial variation, but the final gradient LUT lookup position uses orbit trap distance instead of `length(q)`. This produces spatially-varying color patterns modulated by structural proximity.
