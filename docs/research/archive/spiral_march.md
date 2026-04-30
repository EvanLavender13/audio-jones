# Spiral March

A raymarched flight through an infinite tumbling lattice of SDF primitives, where straight rays trace helical paths because the sample point itself rotates as it marches. Reads as drilling through a corridor of spinning diamonds (or boxes, spheres, tori) that tumble in place while the world slides past the camera. The "march distance + iteration count" output of the raymarch loop drives both color (via gradient LUT) and audio reactivity (via FFT lookup at the matching frequency), so near cells light up with bass and far cells with treble.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (after particle compute, before transforms). Same slot as Voxel March, Cyber March, Geode.
- **Compute Model**: Single fragment shader, no compute pass, no ping-pong textures.

## Attribution

- **Based on**: "Evil Doom Spiral" by QnRA
- **Source**: https://www.shadertoy.com/view/Nc2SRd
- **License**: CC BY-NC-SA 3.0

## References

- [Inigo Quilez - Distance Functions](https://iquilezles.org/articles/distfunctions/) — canonical SDF primitive forms (sdOctahedron, sdBox, sdSphere, sdTorus) and domain repetition (`opRepetition`)
- [Inigo Quilez - Palettes](https://iquilezles.org/articles/palettes/) — cosine palette function used in the reference (`palette(t, a, b, c, d) = a + b*cos(2pi*(c*t + d))`); replaced in this adaptation by gradient LUT sampling
- [src/effects/voxel_march.cpp](../../src/effects/voxel_march.cpp) — sibling raymarched Field generator, used as the implementation template (uniform binding, phase accumulation, FFT pattern, generator output macro)

## Reference Code

Verbatim from Shadertoy. Implementing agents must transcribe from this code, not from prose summaries.

```glsl
mat2 rot2D(float angle) {
    float s = sin(angle);
    float c = cos(angle);

    return mat2(c, -s, s, c);
}

vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.283185*(c*t+d) );
}

float sdOctahedron( vec3 p, float s)
{
    p = abs(p);
    return (p.x+p.y+p.z-s)*0.57735027;
}

float map(vec3 p) {
    vec3 ohPos = p;

    float scale = 1.0; //2.0 - sin(iTime);

    ohPos.z += iTime * .2;

    ohPos.xy = fract(ohPos.xy) - 0.5;

    float m = 0.2;// / .1;
    ohPos.z = mod(ohPos.z, m) - m/2.;

    //ohPos.xz *= rot2D(iTime);
    ohPos.xy *= rot2D(iTime);
    ohPos.yz *= rot2D(iTime);

    ohPos *= scale;

    return sdOctahedron(ohPos, 0.25) / scale;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float FOV = 0.15;
    vec2 uv = (fragCoord * 2.0 - iResolution.xy) / iResolution.y;

    vec3 ro = vec3(0, 0, -1);
    vec3 rd = normalize(vec3(uv * FOV, 1));

    float yAngle = 0.53;
    ro.yz *= rot2D(yAngle);
    rd.yz *= rot2D(yAngle);
    float angle = 100.;
    ro.yz *= rot2D(angle);
    rd.yz *= rot2D(angle);

    float dt = 0.0;
    int i;
    for (i = 0; i < 80; i++) {
        vec3 p = ro + rd * dt;

        p.xy *= rot2D(dt * 0.8);

        float d = map(p);

        if (d < .001 || dt > 100.) break;

        dt += d * 0.9;
    }


    // Output to screen
    fragColor = vec4(palette(
        dt * 0.012 + float(i) * 0.005,
        vec3(0.5, 0.5, 0.5),
        vec3(0.5, 0.5, 0.5),
        vec3(1.0, 1.0, 0.0),
        vec3(0.50, 0.23, 0.67)
    ), 1.0);
}
```

## Algorithm

The reference's three signature mechanics are preserved verbatim. Knobs replace hardcoded scalars. The IQ palette is replaced by gradient LUT sampling, and FFT reactivity is added on the same `t` index per `generator_patterns.md`.

### Substitution Table

| Reference | Adaptation | Notes |
|-----------|------------|-------|
| `iTime` (in `ohPos.z += iTime * .2`) | `flyPhase` uniform | CPU-accumulated: `flyPhase += flySpeed * deltaTime`. `flySpeed` is the new knob (reference value 0.2). |
| `iTime` (in `ohPos.xy *= rot2D(iTime); ohPos.yz *= rot2D(iTime)`) | `cellPhase` uniform | CPU-accumulated: `cellPhase += cellRotateSpeed * deltaTime`. Reference value 1.0. Per-cell tumble. |
| `dt * 0.8` (in `p.xy *= rot2D(dt * 0.8)`) | `dt * spiralRate` | `spiralRate` uniform. Reference value 0.8. THIS IS THE SIGNATURE KNOB — the doom-spiral effect. |
| `0.2` (cell pitch in `mod(ohPos.z, m) - m/2.`) | `cellPitchZ` uniform | Z-axis cell spacing. Reference value 0.2. |
| `0.25` (octahedron size, `sdOctahedron(ohPos, 0.25)`) | `shapeSize` uniform | Reference value 0.25. |
| `sdOctahedron(...)` | `sdfMap(ohPos, shapeSize, shapeType)` | Branch on `shapeType` (uniform int): 0=Octahedron, 1=Box, 2=Sphere, 3=Torus. See SDF Branch below. |
| `0.15` (FOV) | `fov` uniform | Reference value 0.15. |
| `0.53` (yAngle) | `pitchAngle` uniform | Reference value 0.53 rad. Standard angle convention: store radians, display degrees, range +/- ROTATION_OFFSET_MAX. |
| `100.` (second angle, also yz rotation) | `rollAngle` uniform | Reference value 100.0 rad wraps many times. Visible orientation is `mod(100, 2*PI) ~ 5.13 rad`, which is outside the standard angle range `[-PI, +PI]`. Store the equivalent wrapped value `-0.531 = 100 - 16*PI` so the slider lands on a legal value with the same visible orientation. |
| `80` (max iterations) | `marchSteps` uniform int | Reference value 80. Range 30-120. |
| `0.9` (step factor `dt += d * 0.9`) | `stepFactor` uniform | Reference value 0.9. |
| `100.` (max distance) | `maxDist` uniform | Reference value 100.0. |
| `0.001` (hit epsilon) | Keep hardcoded | Numerical convergence, not aesthetic. |
| `0.57735027` (1/sqrt(3) in sdOctahedron) | Keep hardcoded | Mathematical constant. |
| `palette(t, vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), vec3(1.0,1.0,0.0), vec3(0.50,0.23,0.67))` | `texture(gradientLUT, vec2(fract(t), 0.5)).rgb` where `t = dt * tColorDistScale + float(i) * tColorIterScale` | Reference uses `tColorDistScale = 0.012`, `tColorIterScale = 0.005`. Wrap with `fract()` since `t` exceeds 1. Default gradient should approximate the reference yellow-purple-pink palette. |
| (no audio in reference) | Add FFT lookup using same `t` | `freq = baseFreq * pow(maxFreq / baseFreq, fract(t))`, then sample `fftTexture` and multiply brightness by `baseBright + mag` per `generator_patterns.md`. Near cells (small `dt`) light up with low frequencies; far cells with high. |

### SDF Branch

Replace the hardcoded `sdOctahedron(ohPos, 0.25)` call with a switch on `shapeType` (uniform int), using IQ's canonical primitives:

```glsl
float sdfMap(vec3 p, float s, int shapeType) {
    if (shapeType == 0) {
        // Octahedron (bound form)
        vec3 q = abs(p);
        return (q.x + q.y + q.z - s) * 0.57735027;
    } else if (shapeType == 1) {
        // Box
        vec3 q = abs(p) - vec3(s);
        return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
    } else if (shapeType == 2) {
        // Sphere
        return length(p) - s;
    } else {
        // Torus (major = s, minor = s * 0.4)
        vec2 q = vec2(length(p.xz) - s, p.y);
        return length(q) - s * 0.4;
    }
}
```

The torus minor radius is parameterized as a fraction of `s` (0.4) so a single `shapeSize` knob covers all four shapes sensibly.

### Coordinate Convention

The reference uses `(fragCoord * 2.0 - iResolution.xy) / iResolution.y`, which already produces centered coordinates in (0,0)-at-center space. This satisfies the codebase's "shader coordinates must be centered" rule. No changes needed.

### What to Preserve Verbatim

- The three-line domain repetition: `ohPos.z += flyPhase; ohPos.xy = fract(ohPos.xy) - 0.5; ohPos.z = mod(ohPos.z, cellPitchZ) - cellPitchZ * 0.5;`
- The two-axis per-cell rotation: `ohPos.xy *= rot2D(cellPhase); ohPos.yz *= rot2D(cellPhase);`
- The ray-spiral inside the march loop: `p.xy *= rot2D(dt * spiralRate);`
- The march body: ray sample, `sdfMap`, hit/maxDist break, `dt += d * stepFactor`.
- Camera setup with two yz rotations applied to both `ro` and `rd`.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `shapeType` | int (combo) | 0-3 | 0 (Octahedron) | SDF primitive: Octahedron / Box / Sphere / Torus |
| `shapeSize` | float | 0.05 - 0.45 | 0.25 | Half-extent of the SDF primitive (half-pitch upper bound: cell would touch its neighbor at 0.5) |
| `cellPitchZ` | float | 0.05 - 1.0 | 0.20 | Z-axis spacing between cells (XY repeat is fixed at 1.0 by `fract`) |
| `flySpeed` | float | 0.0 - 5.0 | 0.20 | Forward translation rate of the lattice (z-axis) |
| `cellRotateSpeed` | float (speed) | 0 - ROTATION_SPEED_MAX | 1.0 | Per-cell tumble rate (rad/sec). Drives both xy and yz rotations of local cell space. |
| `spiralRate` | float | 0.0 - 3.0 | 0.8 | Ray-space rotation per unit march distance. THE doom-spiral knob. |
| `fov` | float | 0.05 - 1.0 | 0.15 | Field of view scalar (low = telephoto, high = wide angle) |
| `pitchAngle` | float (angle) | -ROTATION_OFFSET_MAX to +ROTATION_OFFSET_MAX | 0.53 | Camera pitch (yz rotation #1) |
| `rollAngle` | float (angle) | -ROTATION_OFFSET_MAX to +ROTATION_OFFSET_MAX | -0.531 (= 100.0 - 16*PI, the reference's `100.0` wrapped into [-PI, +PI]; same orientation as `mod(100, 2pi) ~ 5.13`) | Camera roll (yz rotation #2). |
| `marchSteps` | int | 30 - 120 | 80 | Max raymarch iterations |
| `stepFactor` | float | 0.5 - 1.0 | 0.9 | Distance multiplier per step (lower = safer but slower) |
| `maxDist` | float | 20.0 - 200.0 | 100.0 | Far clipping distance for the march |
| `tColorDistScale` | float | 0.0 - 0.1 | 0.012 | Color/FFT index contribution from march distance |
| `tColorIterScale` | float | 0.0 - 0.05 | 0.005 | Color/FFT index contribution from iteration count |
| `gradient` | ColorConfig | n/a | yellow -> magenta -> pink approximation | Color LUT |
| `baseFreq` | float | 27.5 - 440 | 55.0 | FFT lookup frequency floor |
| `maxFreq` | float | 1000 - 16000 | 14000.0 | FFT lookup frequency ceiling |
| `gain` | float | 0.1 - 10 | 2.0 | FFT magnitude gain |
| `curve` | float | 0.1 - 3.0 | 1.5 | FFT magnitude contrast |
| `baseBright` | float | 0.0 - 1.0 | 0.15 | Brightness floor (audio adds on top) |
| `blendIntensity` | float | 0.0 - 5.0 | 1.0 | Output blend amount (STANDARD_GENERATOR_OUTPUT) |
| `blendMode` | enum | n/a | EFFECT_BLEND_SCREEN | Composite mode (STANDARD_GENERATOR_OUTPUT) |

## Modulation Candidates

- **flySpeed**: changes how fast the lattice rushes past — felt as zoom rate
- **spiralRate**: governs the doom-spiral curl; 0 = straight tunnel through grid, high = vertigo helix
- **cellRotateSpeed**: tumble rate of individual cells; independent of flight
- **shapeSize**: pulses cells from sparse points to nearly-touching blobs
- **cellPitchZ**: stretches or compresses the lattice along the flight axis
- **fov**: zoom-like effect, narrows or widens the corridor
- **pitchAngle / rollAngle**: re-frame the entire scene
- **gain / baseBright**: standard audio contrast / brightness floor

### Interaction Patterns

- **spiralRate × flySpeed (competing forces)**: low spiral + high fly = straight flight through a sliding grid; high spiral + low fly = drilling/vertigo with little forward motion; both high = full doom corkscrew. Routing one to bass and one to treble produces visibly different verse/chorus dynamics — quiet sections fly straight, loud sections corkscrew.
- **cellRotateSpeed × flySpeed (competing forces)**: slow fly + fast tumble = chaotic interior with shapes spinning around the camera; fast fly + slow tumble = passing tumbling shapes that read as discrete objects rather than churning soup.
- **fov × spiralRate (resonance)**: tight fov amplifies the perceived twist of the spiral (fewer rays span more world, so each ray's curl is more visible); wide fov dilutes it. When both modulate together with the same source, the spiraling intensifies non-linearly during peaks.
- **shapeSize × cellPitchZ (cascading threshold)**: when `shapeSize > cellPitchZ * 0.5`, octahedrons start overlapping neighbors and the lattice fuses into continuous geometry. This is a visible threshold — modulating either across that line produces a sudden character change rather than smooth scaling.

## Notes

- **Performance**: Up to 80-120 raymarch iterations per pixel. Falls in the same cost class as Voxel March / Cyber March / Geode. Half-res rendering is not standard for generators in this codebase, so expect full-res cost.
- **Step factor**: Reference uses 0.9, which is generous. The octahedron `sdOctahedron` returns a *bound* (not exact distance) — multiplying it by 0.57735 makes it conservative enough that 0.9 step is safe. For other shapes (box/sphere/torus return exact distances), 0.9 is slightly aggressive but visually acceptable. Lower to 0.7 if banding appears.
- **Roll angle wrap**: The reference's `angle = 100.0` is just a number the author picked — its visible effect is `mod(100, 2*pi) = 5.13 rad`, which falls outside the standard `[-PI, +PI]` angle range. The default stored value is `-0.531 = 100 - 16*PI` (the same orientation, mapped into the legal range).
- **No camera pan**: The reference is centered; this adaptation does not add a 2D position offset. If a future revision wants screen-space drift, embed `DualLissajousConfig` per the codebase convention (see `feedback_lissajous_for_2d_positions.md`).
- **Color identity vs LUT**: The reference's IQ-palette default produces a yellow / magenta / pink doom palette. The default `gradient` should approximate it but users can swap freely via the standard color controls.
- **Distinct from siblings**: Voxel March (sphere shells), Cyber March (neon blocks), Geode (cubes inside a sphere) all use straight rays. None of them spiral the ray itself. The `p.xy *= rot2D(dt * spiralRate)` line is the unique identity — the fidelity check after implementation must confirm this is preserved and visible.
