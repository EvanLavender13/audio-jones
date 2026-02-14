# Kernel Shapes

Adds shaped aperture kernels to both Bokeh and Phi Blur. Five shapes — disc (current default), box, hexagon, star, diamond — all built from a single polar radius function that reshapes the golden-angle Vogel spiral. Configurable star point count (3-8) with inner radius ratio. Shape rotation parameter on both effects.

## Classification

- **Category**: TRANSFORMS > Optical (augments existing Bokeh + planned Phi Blur)
- **Pipeline Position**: No change — both effects keep their current pipeline slots
- **Scope**: Shader-only change to both effects, plus new config fields and UI controls

## References

- [Inigo Quilez: 2D SDF Functions](https://iquilezles.org/articles/distfunctions2d/) — Exact polygon and star SDFs. Source of the `sdStar` function with configurable point count and inner ratio.
- [GM Shaders: Phi Tutorial](https://mini.gmshaders.com/p/phi) — Golden-ratio sampling foundation (already referenced by Phi Blur research).
- [Bart Wronski: Golden Ratio Sequence](https://bartwronski.com/2016/10/30/dithering-part-two-golden-ratio-sequence-blue-noise-and-highpass-and-remap/) — Low-discrepancy properties of phi-based distributions.

## Reference Code

### Inigo Quilez — sdStar (exact)

```glsl
// r = outer radius, n = number of points, m = inner ratio (2.0 = deep valleys)
float sdStar(in vec2 p, in float r, in int n, in float m) {
    float an = 3.141593 / float(n);
    float en = 3.141593 / m;
    vec2 acs = vec2(cos(an), sin(an));
    vec2 ecs = vec2(cos(en), sin(en));
    float bn = mod(atan(p.x, p.y), 2.0 * an) - an;
    p = length(p) * vec2(cos(bn), abs(sin(bn)));
    p -= r * acs;
    p += ecs * clamp(-dot(p, ecs), 0.0, r * acs.y / ecs.y);
    return length(p) * sign(p.x);
}
```

### Existing Bokeh sampling (from `shaders/bokeh.fs`)

```glsl
#define GOLDEN_ANGLE 2.39996323
#define HALF_PI 1.5707963

for (int i = 0; i < iterations; i++) {
    vec2 dir = cos(float(i) * GOLDEN_ANGLE + vec2(0.0, HALF_PI));
    float r = sqrt(float(i) / float(iterations)) * radius;
    vec2 offset = dir * r;
    offset.x *= aspect;
    vec3 col = texture(texture0, uv + offset).rgb;
    // ... accumulate
}
```

## Algorithm

### Core Approach: Polar Radius Scaling

Both effects already distribute samples using golden-angle Vogel spirals. Each sample has a natural angle `theta = i * GOLDEN_ANGLE` and radius `r = sqrt(i/N) * radius`. To reshape from disc to any convex shape, multiply `r` by a shape factor `S(theta)` that traces the shape boundary as a function of angle.

### Shape Radius Functions

All functions return a scale factor normalized so the maximum output is 1.0 (preserving the blur radius parameter's meaning).

**Regular N-gon** (used for Box n=4, Hex n=6, Diamond n=4 rotated):

```glsl
float ngonRadius(float theta, int n, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle) - halfAngle;
    return cos(halfAngle) / cos(sector);
}
```

This traces the apothem-normalized boundary of a regular polygon. `cos(halfAngle)` normalizes so the inscribed circle radius = 1.0.

**Shapes via ngonRadius:**
- **Box**: `ngonRadius(theta, 4, shapeAngle)`
- **Hex**: `ngonRadius(theta, 6, shapeAngle)`
- **Diamond**: `ngonRadius(theta, 4, shapeAngle + PI/4)` — a box rotated 45 degrees

**Star** (configurable points and depth):

```glsl
float starRadius(float theta, int n, float innerRatio, float rotation) {
    float halfAngle = PI / float(n);
    float sector = mod(theta + rotation, 2.0 * halfAngle);
    // Triangle wave: 1.0 at tip (sector=halfAngle), innerRatio at valley (sector=0 or 2*halfAngle)
    float t = abs(sector - halfAngle) / halfAngle;
    return mix(innerRatio, 1.0, t);
}
```

Linear interpolation between inner and outer radii creates straight-edged star arms. `innerRatio` controls how deep the valleys cut (0.1 = deep spiky star, 0.9 = barely indented polygon).

**Disc**: scale factor = 1.0 (no-op, current behavior).

### Integration into Sampling Loop

For both shaders, the shape function modifies `r` before computing the UV offset:

```glsl
float theta = float(i) * GOLDEN_ANGLE;
float r = sqrt(float(i) / float(samples)) * radius;

// Apply shape
if (shape == 1)      r *= ngonRadius(theta, 4, shapeAngle);           // Box
else if (shape == 2)  r *= ngonRadius(theta, 6, shapeAngle);           // Hex
else if (shape == 3)  r *= starRadius(theta, starPoints, starInner, shapeAngle); // Star
else if (shape == 4)  r *= ngonRadius(theta, 4, shapeAngle + PI/4.0);  // Diamond

vec2 dir = cos(theta + vec2(0.0, HALF_PI));
vec2 offset = dir * r;
```

The shape function runs per-sample inside the loop — negligible cost since it's a few trig ops vs. the texture fetch that dominates.

### Phi Blur Specifics

Phi Blur has two modes: Rect (phi-sequence sampling) and Disc (golden-angle spiral). The shape parameter only applies in Disc mode. In Rect mode the kernel is inherently rectangular and shaped by `angle` + `aspectRatio`, so the shape enum is hidden/ignored.

## Parameters

Shared parameters added to both BokehConfig and PhiBlurConfig:

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| shape | int | 0-4 | 0 | 0=Disc, 1=Box, 2=Hex, 3=Star, 4=Diamond |
| shapeAngle | float | 0-2pi | 0.0 | Rotation of the kernel shape (radians, displayed as degrees) |
| starPoints | int | 3-8 | 5 | Number of star points (only visible when shape=Star) |
| starInnerRadius | float | 0.1-0.9 | 0.4 | Valley depth of star shape (only visible when shape=Star) |

### Conditional Visibility

- `shapeAngle` hidden when shape == 0 (Disc) — rotation of a circle is meaningless
- `starPoints` and `starInnerRadius` only visible when shape == 3 (Star)
- For Phi Blur: entire shape section hidden when mode == Rect

## Modulation Candidates

- **shapeAngle**: Rotating the kernel shape — hex bokeh slowly spinning creates a kaleidoscopic shimmer
- **starInnerRadius**: Morphing between polygon and deep star — smooth transitions between soft and spiky bokeh

### Interaction Patterns

- **shapeAngle + radius** (resonance): At small radii the shape is invisible; at large radii the shape dominates. Modulating both means the shape "breathes" in and out of visibility with the blur intensity.
- **starInnerRadius + brightnessPower** (Bokeh only, competing forces): High brightness power makes bright spots pop through the star tips while inner radius controls how much dark space separates them. Deep inner ratio + high brightness = isolated bright spikes; shallow inner ratio + low brightness = soft polygon.

## Notes

- **No performance impact**: Shape functions add ~3 trig operations per sample. The texture fetch per sample (~100+ cycles) dominates.
- **Disc is the identity case**: `shape == 0` multiplies by 1.0, so existing behavior is preserved with zero cost.
- **Star linearity**: Linear interpolation between inner/outer radii creates straight star edges. A smoother `smoothstep` variant could be offered later but straight edges match real aperture blade geometry.
- **Aspect correction**: Both shaders already handle aspect ratio. The shape function operates in the pre-aspect-correction space, so shapes appear correct regardless of resolution.
