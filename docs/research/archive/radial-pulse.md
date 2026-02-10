# Radial Pulse

Polar-native sine distortion creating concentric rings, petal symmetry, and animated spirals. Applies `sin(radius * freq)` for radial displacement and `sin(angle * segments)` for angular displacement. Combines both with phase animation for rotating spiral patterns.

## Classification

- **Category**: TRANSFORMS -> Symmetry
- **Core Operation**: UV displacement via sine functions in polar coordinates
- **Pipeline Position**: Reorderable transform (same stage as Kaleidoscope, KIFS)

## References

- [Ronja's Tutorials - Polar Coordinates](https://www.ronja-tutorials.com/post/053-polar-coordinates/) - HLSL polar conversion via atan2/length, swirl distortion examples
- [Cyanilux - Polar Coordinates](https://www.cyanilux.com/tutorials/polar-coordinates/) - Unity polar coords, flower pattern generation
- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - f(p + h(p)) warping pattern
- [Math LibreTexts - Polar Graphs](https://math.libretexts.org/Courses/Las_Positas_College/Math_39:_Trigonometry/04:_Further_Applications_of_Trigonometry/4.03:_Polar_Coordinates_-_Graphs) - Rose curve formulas r = a*sin(n*theta)
- [Harry Alisavakis - Wavy Displacement](https://halisavakis.com/my-take-on-shaders-custom-wavey-displacement/) - Sine wave UV displacement technique

## Algorithm

### Polar Coordinate Conversion

Convert Cartesian UV to polar (radius, angle):

```glsl
vec2 toPolar(vec2 uv, vec2 center) {
    vec2 delta = uv - center;
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);  // range: [-PI, PI]
    return vec2(radius, angle);
}
```

### Radial Displacement (Concentric Rings)

Apply sine to radius for expanding/contracting ring pattern:

```glsl
// radialFreq: waves per unit radius (higher = more rings)
// radialAmp: displacement strength
// phase: animation offset (radians)
float radialOffset = sin(radius * radialFreq + phase) * radialAmp;
```

The sine wave peaks push UV outward, troughs pull inward, creating concentric ring distortion.

### Angular Displacement (Petal Symmetry)

Apply sine to angle for n-fold rotational symmetry (rose curve pattern):

```glsl
// segments: petal count (integer for clean symmetry)
// angularAmp: rotation displacement strength
float angularOffset = sin(angle * segments + phase) * angularAmp;
```

Rose curve rules from polar math:
- If segments is odd: produces `segments` petals
- If segments is even: produces `2 * segments` petals

### Combined Spiral Animation

Combine radial and angular displacement with linked phase:

```glsl
// spiralTwist: how much angular displacement depends on radius
float spiralPhase = phase + radius * spiralTwist;
float angularOffset = sin(angle * segments + spiralPhase) * angularAmp;
```

This creates rotating spiral arms where inner and outer regions rotate at different rates.

### Full UV Displacement

Apply displacement back to Cartesian coordinates:

```glsl
vec2 radialPulse(vec2 uv, vec2 center, float radialFreq, float radialAmp,
                 int segments, float angularAmp, float phase, float spiralTwist) {
    vec2 delta = uv - center;
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Radial displacement (rings)
    float newRadius = radius + sin(radius * radialFreq + phase) * radialAmp;

    // Angular displacement (petals) with spiral twist
    float spiralPhase = phase + radius * spiralTwist;
    float newAngle = angle + sin(angle * float(segments) + spiralPhase) * angularAmp;

    // Convert back to Cartesian
    vec2 displaced = center + vec2(cos(newAngle), sin(newAngle)) * newRadius;
    return displaced;
}
```

### Alternative: Displacement Vector Approach

For smoother results, compute radial and tangential displacement vectors:

```glsl
vec2 radialDir = normalize(delta);
vec2 tangentDir = vec2(-radialDir.y, radialDir.x);

float radialDisp = sin(radius * radialFreq + phase) * radialAmp;
float tangentDisp = sin(angle * float(segments) + spiralPhase) * angularAmp * radius;

vec2 displaced = uv + radialDir * radialDisp + tangentDir * tangentDisp;
```

Multiplying tangent displacement by radius prevents excessive distortion near center.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| radialFreq | float | 1.0 - 30.0 | Ring density (waves per unit radius) |
| radialAmp | float | 0.0 - 0.3 | Radial displacement strength |
| segments | int | 2 - 16 | Petal count (angular symmetry) |
| angularAmp | float | 0.0 - 0.5 | Angular displacement strength (radians) |
| phase | float | accumulated | Animation phase (radians) |
| phaseSpeed | float | 0.0 - 5.0 | Animation rate (radians/second) |
| spiralTwist | float | -10.0 - 10.0 | Spiral coupling (angular phase shift per radius) |
| centerX | float | 0.0 - 1.0 | Effect center X |
| centerY | float | 0.0 - 1.0 | Effect center Y |

### Modulatable Parameters

Register in `param_registry.cpp`: radialFreq, radialAmp, angularAmp, spiralTwist, centerX, centerY.

Phase accumulation happens on CPU (per angular field naming convention in CLAUDE.md).

## Audio Mapping Ideas

- **radialFreq** -> bass energy: more rings on kicks
- **radialAmp** -> mid-range RMS: breathing expansion
- **angularAmp** -> high-frequency energy: petal flutter
- **spiralTwist** -> beat detection: spiral spin on transients

## Distinction from Existing Effects

| Effect | Pattern | Mechanism |
|--------|---------|-----------|
| Wave Ripple | Concentric waves | Height field gradient (Cartesian derivative) |
| Kaleidoscope | Wedge reflection | Polar modulo/reflection |
| Sine Warp | Directional cascade | Cartesian sine with per-octave rotation |
| **Radial Pulse** | Rings + petals + spirals | Direct polar sine on radius AND angle |

Key difference from Wave Ripple: Radial Pulse operates natively in polar space and includes angular (petal) distortion. Wave Ripple computes a height field and displaces via gradient.

Key difference from Kaleidoscope: Kaleidoscope reflects/mirrors UV across angular boundaries. Radial Pulse distorts continuously via sine waves.

## Notes

- At high `radialFreq` (>20), rings become visually noisy; pair with lower `radialAmp`
- Integer `segments` values produce clean rotational symmetry
- Non-integer `segments` create asymmetric patterns (intentional artistic choice)
- Near-zero radius has undefined angle; mask center or clamp radius minimum
- `spiralTwist` sign determines spiral direction (positive = counterclockwise unwind)
- Consider aspect ratio correction if effect center isn't at screen center
