# Lens Space

Recursive crystal-ball warp — rays march through a 3D lens space L(p,q) where each bounce off the spherical boundary rotates by q/p revolutions, multiplying the scene into diminishing kaleidoscopic copies that shift between clean symmetry and chaotic overlap as P and Q morph.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Transform chain (user-reorderable)

## References

- User-provided Shadertoy shader implementing L(p,q) lens space raymarching
- [Lens space — Alternative definitions of three-dimensional lens spaces](https://en.wikipedia.org/wiki/Lens_space#Alternative_definitions_of_three-dimensional_lens_spaces) — mathematical definition: raymarch inside a ball, rotate polar coordinate by q/p revolutions at the boundary sphere

## Reference Code

Complete working Shadertoy shader (user-provided, unmodified):

```glsl
#define PI 3.14159265

/*
This visualization is my interpretation of the alternative definition of a three-dimensional lens space L(p, q) given on wikipedia:

https://en.wikipedia.org/wiki/Lens_space#Alternative_definitions_of_three-dimensional_lens_spaces

The idea is simple, you raymarch inside a ball and when you hit the boundary sphere, the polar coordinate of your position will
rotate by q/p revolutions, where q and p are coprime integers. I decided to reflect the ray on the boundary after applying the
rotation because it looked nice, but you could also simply invert the direction of the ray after applying the rotation. To see the
second option, change REFLECT to 0. you can also change the values of p and q.
*/

#define P 5.
#define Q 2.
#define REFLECT 1

// 2D matrix to rotate a vector by some angle th
mat2 Rot(float th){
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord*2. - iResolution.xy)/iResolution.y;

    // Set ray origin and ray direction, add rotation
    vec3 ro = vec3(0., .0, -.5);
    vec3 rd = vec3(uv, 1.);
    mat2 R = Rot(iTime*.5);
    rd.xz *= R;

    // Choose center of sphere inside our space based on mouse position
    vec2 m = (iMouse.xy*2. - iResolution.xy)/iResolution.y;
    if (iMouse.z == 0.){ m *= 0.; }
    vec3 c = vec3(m*.4, .5);

    // Initialization for raymarching, reps is for keeping track of number of reflections
    float t;
    vec3 col;
    float refs;

    for (int i=0; i<300; i++){
        // Update position
        vec3 pos = ro + t * rd;

        // Calculate distance to sphere (d) and edge of our space (e)
        float d = length(pos - c) - .3;
        float e = 1.0 - length(pos);

        if (d < 0.001){
            // If we hit the sphere, get color from cubemap based on reflected direction
            vec3 n = normalize(pos - c);
            vec3 r = reflect(rd, n);
            col = texture(iChannel0, r).rgb;
            break;
        }

        if (abs(e) < 0.001){
            // If we hit the edge, rotate position and ray direction and then either reflect or invert ray direction
            mat2 R = Rot(Q*PI/P);
            pos.xz = R * pos.xz;
            rd.xz = R * rd.xz;

            vec3 n = normalize(-pos);
            #if REFLECT == 1
            rd = reflect(rd, n);
            #else
            rd *= -1.;
            #endif

            // Change ray origin to new position
            ro = pos + 0.01 * n;
            t = 0.;

            // Update number of reflections and stop raymarching if we reach a maximum number of reflections
            if (refs == 12.){
                break;
            }
            refs += 1.;
        }

        // Add distance to travel next step,
        t += min(e*.8, d*.4);
    }

    // Dim color the more reflections we have
    col *= 1. - refs/15.;

    fragColor = vec4(col,1.0);
}
```

## Algorithm

Adaptation from Shadertoy reference to AudioJones warp transform:

**Keep verbatim from reference:**
- `Rot()` rotation matrix helper
- Core raymarching loop structure (ray origin, direction, step sizing)
- Sphere distance / boundary distance calculations
- Boundary hit logic: rotate position and direction in XZ by `q*PI/p`, then reflect off inward normal
- Reflection counting and early termination at max reflections
- Dimming formula: `col *= 1.0 - refs * dimming`

**Replace:**
- `iResolution` → `resolution` uniform
- `iTime * 0.5` → CPU-accumulated rotation angle uniform (`rotAngle`, accumulated via `rotationSpeed * deltaTime`)
- `iMouse` sphere center → `center` uniform XY mapped to sphere center XY (existing codebase pattern)
- `texture(iChannel0, r)` cubemap sampling → perspective-project final ray direction to 2D UV and sample `texture0`:
  - On sphere hit: `vec3 r = reflect(rd, n); finalUV = 0.5 + r.xy / max(abs(r.z), 0.001) * projScale;`
  - On max reflections or loop exit: `finalUV = 0.5 + rd.xy / max(abs(rd.z), 0.001) * projScale;`
  - `projScale` controls how much the UV displacement fans out (acts as a warp strength)
- Hardcoded `#define P 5.` / `#define Q 2.` → float uniforms
- Hardcoded sphere radius `.3` → float uniform
- Hardcoded boundary `1.0` → float uniform
- Hardcoded max reflections `12.` → int uniform (passed as float for GPU comparison)
- Hardcoded dimming `1./15.` → float uniform

**Coordinate convention:**
- Center UV coordinates relative to `center` uniform: `vec2 uv = (fragTexCoord - center) * resolution / resolution.y;`
- Sphere center XY derives from `center` uniform offset (not iMouse)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| p | float | 2.0-12.0 | 5.0 | Symmetry order — number of rotational copies at boundary |
| q | float | 1.0-11.0 | 2.0 | Rotation fraction — angle per boundary reflection is q/p revolutions |
| sphereRadius | float | 0.05-0.8 | 0.3 | Central mirror sphere size |
| boundaryRadius | float | 0.5-2.0 | 1.0 | Lens space boundary radius |
| rotationSpeed | float | -ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX | 0.5 | Camera rotation rate (accumulated on CPU) |
| maxReflections | float | 2.0-20.0 | 12.0 | Reflection depth — more copies with diminishing brightness |
| dimming | float | 0.01-0.15 | 0.067 | Per-reflection brightness decay rate |
| zoom | float | 0.5-3.0 | 1.0 | Ray spread / field of view |
| projScale | float | 0.1-1.0 | 0.4 | UV projection strength — how far reflected rays displace from center |

## Modulation Candidates

- **p**: morphs symmetry order — transitional states between integers create flowing, dissolving geometry
- **q**: shifts rotation angle per bounce — smooth sliding between symmetry classes
- **sphereRadius**: pulsing sphere size gates how much scene is reflected vs passed through
- **zoom**: breathing FOV creates push/pull depth oscillation
- **dimming**: controls copy fade rate — low = bright recursive infinity, high = central focus
- **projScale**: controls displacement intensity — how dramatically the warp fans out

### Interaction Patterns

**P × Q resonance**: When Q approaches clean integer ratios of P (e.g., Q/P = 1/2, 1/3), copies align into crisp symmetric patterns. Between these ratios, copies smear and overlap chaotically. Modulating both creates moments of crystallization (chorus) followed by dissolution (verse).

**sphereRadius × zoom competing forces**: Large sphere + narrow zoom fills the frame with the curved central reflection, hiding boundary copies. Small sphere + wide zoom emphasizes recursive boundary tiling. They compete between "crystal ball" and "infinite corridor" character.

**dimming × maxReflections cascading threshold**: High maxReflections only matters when dimming is low enough for deep copies to remain visible. With high dimming, copies beyond depth ~5 are already black — increasing maxReflections has no visual effect until dimming drops.

## Notes

- 300-iteration raymarching loop is the heaviest of any warp transform. Consider `EFFECT_FLAG_HALF_RES` if frame rate suffers at 1080p+. Most pixels exit early (sphere hit ~20-30 steps, or maxRefs × ~5-10 steps per bounce ≈ 60-120 worst case).
- Non-integer P and Q are mathematically "invalid" lens spaces but produce visually interesting transitional states — this is intentional.
- Reflect mode only (not invert) per design decision.
- `rotationSpeed` follows codebase accumulation convention: `rotAngle += rotationSpeed * deltaTime` on CPU, shader receives `rotAngle`.
- `maxReflections` stored as float in config for modulation compatibility but used as integer comparison in shader.
