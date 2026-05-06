# Frame Tunnel

A wireframe platonic-solid skeleton receding infinitely down a tunnel. Kaleidoscopic plane-fold IFS carves the polyhedral edges from raymarched space, while a `fract(log(p.z))` substitution along the camera axis creates seamless self-similar zoom — you fly forever into the same frame, getting closer without ever arriving. Tumbles on three independent rotation axes with gradient-LUT coloring sampled from depth.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Output stage > Generators (before Transforms)

## Attribution

- **Based on**: "Icosahedron frame" by gaz
- **Source**: https://www.shadertoy.com/view/st2GRd
- **License**: CC BY-NC-SA 3.0

Related references by the same author (same family, alternate fold normals):
- "polyhedron Frame" by gaz - https://www.shadertoy.com/view/7tS3Dy (CC BY-NC-SA 3.0)
- "Something 227" by gaz - https://www.shadertoy.com/view/3lyBDw (CC BY-NC-SA 3.0)

## References

- [Icosahedron frame](https://www.shadertoy.com/view/st2GRd) - Primary reference: kaleidoscopic plane-fold + log-zoom raymarcher producing icosahedral wireframe tunnel
- [polyhedron Frame](https://www.shadertoy.com/view/7tS3Dy) - Same family with different plane normal (octahedral fold, 4 iterations) and `min(yz, xz)` edge distance
- [Something 227](https://www.shadertoy.com/view/3lyBDw) - Same family with the log-fold distance applied across all three axis triplets for 3D lattice variant (not used here, but documents the technique)

## Reference Code

### Icosahedron Frame (primary)

```glsl
#define R(p,a,r)mix(a*dot(p,a),p,cos(r))+sin(r)*cross(p,a)
#define H(h)(cos((h)*6.3+vec3(0,23,21))*.5+.5)
void mainImage(out vec4 O, vec2 C)
{
    O=vec4(0);
    vec3 p,r=iResolution,n=vec3(-.5,-.809,.309),
    d=normalize(vec3((C-.5*r.xy)/r.y,1));
    for(float i=0.,e,g=0.;
        ++i<99.;
        O.xyz+=mix(vec3(1),H(dot(p,p)*.5),.7)*.05*exp(-.05*i*i*e)
    )
    {
        p=g*d;
        p.z-=10.;
        p=R(p,normalize(vec3(1,2,2)),iTime*.5);
        for(int j=0;j<5;j++)
            p.xy=abs(p.xy),
            p-=2.*min(0.,dot(p,n))*n;
        p.z=fract(log(p.z)-iTime*.5)-.5;
        g+=e=length(p.yz)-.01;
        // Dodecahedron Frame
        //g+=e=length(p.xz)-.01;
    }
}
```

### Polyhedron Frame (octahedral variant)

```glsl
#define R(p,a,r)mix(a*dot(p,a),p,cos(r))+sin(r)*cross(p,a)
#define H(h)(cos((h)*6.3+vec3(0,23,21))*.5+.5)
void mainImage(out vec4 O, vec2 C)
{
    O=vec4(0);
    vec3 p,r=iResolution,n=vec3(-.5,-.707,.5),
    d=normalize(vec3((C-.5*r.xy),r.y));
    for(float i=0.,e,g=0.;
        ++i<99.;
        O.xyz+=mix(vec3(1),H(length(p)*.5+iTime*3.),.7)*.05*exp(-.03*i*i*e)
    )
    {
        p=g*d;
        p.z-=10.;
        p=R(p,normalize(vec3(-1,-2,2)),iTime*.5);
        for(int j=0;j<4;j++)
            p.xy=abs(p.xy),
            p-=2.*min(0.,dot(p,n))*n;
        p.z=fract(log(p.z)-iTime*.5)-.5;
        g+=e=abs(min(length(p.yz),length(p.xz))-.03)+.001;
    }
}
```

### Something 227 (3D lattice variant, not implemented)

```glsl
#define R(p,a,r)mix(a*dot(p,a),p,cos(r))+sin(r)*cross(p,a)
#define H(t)(cos((vec3(0,2,-2)/3.+t)*6.24)*.5+.5)
#define D(a)length(vec2(fract(log(length(a.xy))-iTime*.5)-.5,a.z))/3.-.005*pow(l,.03)
void mainImage(out vec4 O, vec2 C)
{
    O=vec4(0);
    vec3 p,r=iResolution;
    for(float g,e,i=0.,l;
        ++i<99.;
        e<.005?O.xyz+=mix(vec3(1),H(l),.7)/i:p
        )
    {
        p=R(g*normalize(vec3((C-.5*r.xy)/r.y,1.))-vec3(0,0,6),
            normalize(vec3(1,2,0)),
            iTime*.2
        );
        l=length(p);
        g+=e=min(min(D(p),D(p.zxy)),D(p.yzx));
    }
}
```

## Algorithm

Build the shader as a transcription of the **Icosahedron Frame** reference with these mechanical substitutions. Open the reference and the substitution table side by side; do not generate from understanding.

### Substitution table

| Reference element | Replacement | Notes |
|---|---|---|
| `iResolution` | `resolution` (uniform `vec2`) | Standard convention |
| `iTime` (used in R) | per-axis accumulated angles `rotateAngle.x/y/z` (uniform `vec3`) | CPU-accumulated, never `time * speed` in shader |
| `iTime` in `fract(log(p.z) - iTime*0.5)` | `zoomPhase` (uniform `float`, accumulated CPU-side from `zoomSpeed`) | Speed accumulation rule |
| `R(p, normalize(vec3(1,2,2)), iTime*.5)` | three sequential `R()` calls along world X, Y, Z axes using `rotateAngle.x/y/z` | One axis per call |
| `n = vec3(-.5, -.809, .309)` (icosahedral) | `n` selected from a 5-entry constant array indexed by `shape` uniform `int` | See plane-normal table below |
| `for(int j=0; j<5; j++)` (5 fold iterations) | `for(int j=0; j<foldCount; j++)` where `foldCount` is selected per shape | GLSL 330 supports dynamic uniform loop bounds |
| `length(p.yz) - .01` (edge thickness) | `length(p.yz) - edgeRadius` (uniform `float`) | Range 0.001 to 0.05 |
| `mix(vec3(1), H(dot(p,p)*.5), .7)` | `gradientLUT(dot(p,p) * 0.5)` | Replace `cos`-palette with LUT sampling; no white-mix |
| `*.05*exp(-.05*i*i*e)` | `* exp(-glow * i * i * e)` with leading `0.05` either folded into the gradient brightness or kept as a separate `intensity` constant | `glow` is uniform `float`, range 0.01 to 0.2 |
| `++i<99.` | `for (int i = 0; i < marchSteps; i++)` with `marchSteps` as a uniform `int` (default 99) | GLSL 330 dynamic loop |
| `p.z -= 10.` | keep verbatim, or expose `cameraOffset` uniform if useful | TBD; keep `10.` for now |

### Plane normals and fold counts per platonic solid

| `shape` | Solid | Plane normal `n` | Fold count |
|---|---|---|---|
| 0 | Tetrahedron | (research needed - confirm) | (research needed) |
| 1 | Cube | (research needed) | (research needed) |
| 2 | Octahedron | `vec3(-0.5, -0.7071068, 0.5)` (from polyhedron Frame ref) | 4 |
| 3 | Dodecahedron | `vec3(-0.5, -0.809, 0.309)` with edge `length(p.xz)` (per icosa ref comment) | 5 |
| 4 | Icosahedron | `vec3(-0.5, -0.809, 0.309)` with edge `length(p.yz)` (icosa ref) | 5 |

Note the dodecahedron uses the same plane normal as icosahedron but swaps the edge-distance axes (`p.xz` instead of `p.yz`) - this is documented in the commented-out line of the Icosahedron Frame reference. Tetrahedron and cube normals/fold counts are not in the gaz references and must be derived during planning (or sourced from another reference shader). If unverified, the plan should explicitly flag them as TODO rather than guess.

### What to keep verbatim

- The `R()` rotation macro (Rodrigues rotation)
- The `for` loop scaffold with `g += e` distance accumulation
- The `fract(log(p.z) - zoomPhase) - 0.5` substitution along the camera axis
- The two-statement fold body: `p.xy = abs(p.xy); p -= 2.0 * min(0.0, dot(p, n)) * n;`
- The `p = g * d; p.z -= 10.0;` ray setup

### What to replace

- `H(...)` palette macro -> `gradientLUT(coord)` using the standard generator color path
- Single rotation axis -> three sequential per-axis rotations from accumulated angles
- Fixed defines (.01, .05, etc.) -> uniform-driven config fields per the parameter table

### Coordinate convention check

The reference computes `d = normalize(vec3((C - 0.5*r.xy) / r.y, 1))` - this centers the screen on the ray origin (CONVENTIONS-compliant: spatial ops centered, not raw `fragTexCoord`). Keep this pattern. When porting to the AudioJones shader, write it as `vec2 uv = (fragTexCoord * resolution - resolution * 0.5) / resolution.y` and build the ray direction from `uv`.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `shape` | int | 0-4 | 4 (Icosa) | Selects platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa |
| `rotateSpeedX` | float | -ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX | 0.0 | World-X tumble rate (rad/s, accumulated CPU-side) |
| `rotateSpeedY` | float | -ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX | 0.5 | World-Y tumble rate |
| `rotateSpeedZ` | float | -ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX | 0.0 | World-Z tumble rate |
| `zoomSpeed` | float | -2.0..+2.0 | 0.5 | Rate of `fract(log)` zoom phase advance (signed: negative reverses tunnel direction) |
| `edgeRadius` | float | 0.001..0.05 | 0.01 | Wireframe edge thickness (world units) |
| `glow` | float | 0.01..0.2 | 0.05 | March-distance falloff coefficient in `exp(-glow * i * i * e)` (smaller = more bleed, larger = sharper) |
| `marchSteps` | int | 32..128 | 99 | Raymarch iteration count (perf vs detail) |

Standard FFT audio block (per FFT Audio UI Conventions) - `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`. The FFT texture's role inside this shader is TBD during planning; the simplest mapping is to attenuate or boost the per-step `exp` contribution by an audio-derived factor sampled from the FFT texture at a depth-mapped frequency.

Standard generator output block - `ColorConfig color`, `EffectBlendMode blend`, blend intensity slider via `STANDARD_GENERATOR_OUTPUT`.

## Modulation Candidates

- **rotateSpeedX/Y/Z**: per-axis tumble rate; modulation creates wobbling, lurching, or strobed rotation
- **zoomSpeed**: pulse the tunnel forward/backward to the beat; sign flips reverse direction visibly
- **edgeRadius**: thin-to-thick edge breathing; at low values the wireframe is crisp, at high values edges merge into glowing blocks
- **glow**: tight/sharp tunnel vs soft/diffuse haze; modulating creates depth-of-field-like atmosphere shifts
- **shape**: discrete shape morphing on cue (live triggering)

### Interaction Patterns

- **Resonance: zoomSpeed + glow** - When `zoomSpeed` is high (fast forward motion) and `glow` is low (sharp falloff), the tunnel reads as a hard-edged forward rush. When both spike together (fast + diffuse), the tunnel "blooms" into a streaking motion blur. When both are low, the tunnel sits still and crisp. The pair gates between four distinct visual modes depending on coincident peaks.
- **Cascading threshold: edgeRadius gates color-band visibility** - Because color is sampled from `dot(p,p) * 0.5` along the march, very thin `edgeRadius` produces only sharp gradient slivers along the wireframe (limited color visibility). As `edgeRadius` grows, more of the gradient LUT is exposed across the thicker edge, unlocking color bands that were invisible at thin settings. A modulation that pushes `edgeRadius` up on chorus reveals the full palette; verses with thin edges read as monochromatic skeletons.
- **Competing forces: rotateSpeedY vs zoomSpeed** - Y-axis tumble rotates the polyhedral cross-section while zoom advances along Z. When Y rotation is fast and zoom is slow, the tunnel feels like a stationary spinning cage; when zoom dominates, it feels like flying through a static frame; balanced, the two compose into a corkscrew flight path.

## Notes

- Performance: raymarcher with up to 99 iterations and a 5-step inner fold per outer step. Each pixel does ~500 ops at default. Sibling effects (Polymorph, Spin Cage, Twist Cage, Polyhedral Mirror) operate at similar cost; should be acceptable.
- The `fract(log(p.z))` substitution requires `p.z > 0` to avoid `log` of non-positive values. The `p.z -= 10.0` setup pushes the ray origin away from the camera so this holds in practice; if the user's camera offset is exposed, the shader must clamp `p.z` to a small positive epsilon before `log`.
- The reference uses the inner fold loop's plane reflection `p -= 2.0 * min(0.0, dot(p, n)) * n` - the `min(0.0, ...)` is a half-space gate (only reflects when on the wrong side of the plane). Transcribe verbatim.
- `marchSteps` exposed but with a tight default; users can drop it to 64 for perf.
- Dodecahedron and icosahedron share a plane normal but differ only in which axis pair forms the edge-distance metric. Implement this as a per-shape selector that picks both `n`, `foldCount`, AND the edge-distance function `length(p.<axes>)`.
- Tetrahedron and cube fold parameters are not in the gaz references. The plan must either source these from another KIFS-style reference shader or flag them as out-of-scope (effect ships with only Octa/Dodeca/Icosa, and Tetra/Cube selections fall back to one of those).
