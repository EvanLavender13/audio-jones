# Hex Rush

Super Hexagon-inspired procedural geometry generator: sharp radial wall segments rush inward toward a pulsing center polygon over alternating colored background wedges, all rotating with configurable snap and intensity. A "difficulty" meta-parameter simultaneously scales speed, density, and gap tightness for one-slider intensity control.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator stage (after trail boost, before transforms)
- **Compute Model**: Fragment shader (no compute needed)

## References

- [Super-Haxagon](https://github.com/RedTopper/Super-Haxagon) - Open source C++ clone with complete wall geometry, background rendering, rotation, color cycling
- "Duper Hexagon" (Shadertoy, user-pasted) - Simplest 2D fragment shader recreation with hash-based wall gaps and perspective distortion
- "Pseudo Hyper Hexagonest" (Shadertoy, user-pasted) - Most complete recreation with bitmask wall patterns, section detection, depth scrolling
- "Super Hexagon - Colored" (Shadertoy, user-pasted) - Clean N-gon SHAPE macro and sine-hash wall generation
- "N-gon prism distance field" (Shadertoy, user-pasted) - N-gon SDF function and beat-timed animation

## Reference Code

### Duper Hexagon (complete shader)

```glsl
#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define TPIS 1.0471975512

float hash(float seed) { return fract(sin(seed) * 43758.5453); }
mat2 rot(float th) { return mat2(cos(th), -sin(th), sin(th), cos(th)); }
float angle(vec2 uv) { return mod(atan(uv.y, uv.x)+TWO_PI, TWO_PI); }

float line(vec2 r, float dist, float thickness) {
    float ret = 0.;
    float a = angle(r);
    if( a > TWO_PI - TPIS*0.5 || a < TPIS*0.5 ) {
        ret = smoothstep(dist, dist+0.002, r.x)*(1.0-smoothstep(dist+thickness, dist+thickness+0.002, r.x));
    }
    return ret;
}

float hexagonRadius = 0.1;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy - 0.5*iResolution.xy) / iResolution.y;

    vec2 uv3 = uv;

    float t = pow(iTime*0.1 + 10.,1.5) + 0.04*sin(10.*iTime);
    uv /= dot(uv, vec2(sin(t * 0.34), sin(t * 0.53))) * 0.6 + 1.0; // perspective distortion
    uv *= 0.1*sin(iTime*4.13)+1.0;
    float r = length(uv);
    float theta = angle(uv) + t;

    int region = int(6.*theta/TWO_PI);

    vec3 color = vec3(0.,0.,0.);

    // background
    float col1 = mod(float(region+int(iTime*0.9)),2.0);
    color += vec3(col1*0.2+0.2,col1*0.2+0.2,0.);

    // hexagon
    vec2 uv2;
    for(int i=0; i<6; i++) {
        uv2 = rot(float(i)*TPIS + TPIS*0.5 - t)*uv;
        color += line(uv2, hexagonRadius, 0.015)*vec3(1.,1.,0.);
        color -= line(uv2, 0., hexagonRadius)*vec3(col1*0.2+0.2,col1*0.2+0.2,0.);
    }

    // obstacles
    float seed2 = floor(t*0.5)+1424.0;
    float missingSegment = hash(seed2);
    float dist = mod(2.0 - t, 2.0);
    if(hexagonRadius<dist+0.01) {
        for(int i=0; i<5; i++) {
            uv2 = rot(float(i+int(missingSegment*6.))*TPIS + TPIS*0.5 - t)*uv;
            float obs = line(uv2, dist, 0.05);
            color += obs*vec3(1.,1.,0.);
        }
    }

    color *= smoothstep(1.8, 0.5, length(uv3));
    fragColor = vec4(color,1.0);
}
```

### Pseudo Hyper Hexagonest (key rendering logic)

```glsl
#define TAU 6.283185307179586
#define NSEC 6
#define SPINRATE 3.0
#define DEPTHRATE 10.0
#define DEPTHSCALE 10.0
#define INNERWALLSIZE 0.01
#define INNERRADIUS 0.2
#define INNERBEATSIZE 0.01
#define INNERBEATRATE 6.0
#define COLORSPEED 1.8

#define COLOR_1_FG  vec3(0.96, 0.96, 0.96)
#define COLOR_1_BG1 vec3(0.67, 0.67, 0.67)
#define COLOR_1_BG2 vec3(0.53, 0.53, 0.53)
#define COLOR_2_FG  vec3(0.93, 0.93, 0.93)
#define COLOR_2_BG1 vec3(0.44, 0.44, 0.44)
#define COLOR_2_BG2 vec3(0.35, 0.35, 0.35)

#define nsin(x) ((sin(x) + 1.0) / 2.0)

vec2 rotate(in vec2 point, in float rads)
{
    float cs = cos(rads);
    float sn = sin(rads);
    return point * mat2(cs, -sn, sn, cs);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    float px = 1.0/iResolution.y;
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 position = (uv * 2.0) - 1.0;
    float aspect = iResolution.x / iResolution.y;
    position.x *= aspect;

    // Variable spin rate with wobble
    float spintime = iTime * SPINRATE;
    spintime = (0.4 * spintime) + sin(0.7 * spintime) + sin(0.2 * spintime);
    float spin = mod(spintime, TAU);

    float realdepth = iTime * DEPTHRATE;

    vec2 rposition = rotate(position, spin);

    float r = length(rposition);
    float theta = atan(rposition.y, rposition.x);
    theta += TAU/2.0;
    float section = 6.0 * (theta/TAU);
    float zone = floor(section);
    float zonefrag = mod(section, 1.0);
    float odd = mod(zone, 2.0);

    // Color cycling between two palettes
    float color_fade = nsin(iTime * COLORSPEED);
    vec3 color_fg  = mix(COLOR_1_FG,  COLOR_2_FG,  color_fade);
    vec3 color_bg1 = mix(COLOR_1_BG1, COLOR_2_BG1, color_fade);
    vec3 color_bg2 = mix(COLOR_1_BG2, COLOR_2_BG2, color_fade);

    // Alternating background segments
    vec3 color = mix(color_bg1, color_bg2, odd);

    // Hex-corrected radial distance (perpendicular to polygon edge)
    float dist_to_edge = abs(zonefrag - 0.5) * 2.0;
    float angle_to_edge = dist_to_edge * (TAU / 12.0);
    float hexradius = cos(TAU - angle_to_edge) * r;
    float depth = (hexradius * DEPTHSCALE) + realdepth;

    // Inner polygon with beat pulse
    float ib = INNERBEATRATE * iTime;
    float ir = 2.1 +
        (3.0 * sin(ib)) +
        (4.0 * cos(ib)) -
        (      sin(ib * 2.0)) -
        (2.1 * cos(ib * 2.0));
    ir = INNERRADIUS - (INNERBEATSIZE * abs(ir));

    if (hexradius < ir) {
        if ((ir - hexradius) < INNERWALLSIZE) {
            color = color_fg;
        }
    } else {
        // Wall lookup: bitmask per ring, each bit = one angular segment
        float moddepth = floor(mod(depth, 128.0));
        int wallidx = int(moddepth);

        // [hardcoded bitmask table omitted — replaced by hash-based generation]
        // Original uses: wall(1, 0x2a) wall(4, 0x15) etc.
        // where 0x2a = 101010 binary = walls on segments 1,3,5

        int mask = 1;
        int izone = int(zone);
        // mask = 1 << izone (computed via if-chain in original)

        // curwall = and(curwall, mask);
        // if (curwall > 0) { color = color_fg; }
    }

    fragColor = vec4(color, 1.0);
}
```

### Super Hexagon - Colored (SHAPE macro + wall generation)

```glsl
#define SHAPE(N) cos(round(a / (pi * 2. / float(N))) * (pi * 2. / float(N)) - a) * dist

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    float pi = 3.1415926535;
    float rot = mod(iTime, 5.) > 4.5 ? (pi * 4. * (mod(iTime, 5.) - 4.5)) : 0.;

    vec2 uv = fragCoord / iResolution.xy;
    vec2 coord = 3. * 2. * (uv - vec2(.5, .5));
    coord.x *= iResolution.x / iResolution.y;
    float a = mod(atan(coord.y, coord.x) + rot, pi * 2.);
    float dist = length(coord);
    float r = pi * 2. / float(6);

    vec3 col = vec3(0.);

    float cOff = 0.1 * sin(iTime * pi * 2.);

    // Wall rings: 5 concurrent rings at different distances
    for (int i = 0; i < 5; i++)
    {
        float spd = 3.;
        float spawn_spd = .6;
        // SHAPE(6) gives hex-corrected radial distance
        if (SHAPE(6) > .8 + spd - spd / spawn_spd * mod(iTime, spawn_spd) + float(i) * spd &&
            SHAPE(6) < 1. + spd - spd / spawn_spd * mod(iTime, spawn_spd) + float(i) * spd)
        {
            int num = i + int(floor(iTime / spawn_spd));
            for (int j = 1; j <= 6; j++)
            {
                // Sine-hash determines wall presence per segment
                float sine = sin(float(num) * float(j) * 100.);

                int seg = int(floor((a + r * 0.5) / r));
                if (a < r / 2.) seg = 6;
                int clear_seg = 1 + int(round(2.5 + 2.5 * sin(float(num) * 100.)));
                if (seg == j && sine < .5)
                {
                    if (j != clear_seg)
                    {
                        col = vec3(1.0); // wall color
                    }
                }
            }
        }
    }

    // Center hexagon outline
    if (SHAPE(6) > .8 + cOff && SHAPE(6) < 1. + cOff)
    {
        col = vec3(1.0);
    }

    fragColor = vec4(col, 1.0);
}
```

### N-gon SDF (from N-gon prism distance field)

```glsl
float sdfPolyPrism (vec3 p, float sides, vec2 size)
{
    float gonSlice = 6.28319 / sides;
    float deltaAngle = atan(p.x, p.y);
    float dist = cos(floor(0.5 + deltaAngle / gonSlice) * gonSlice - deltaAngle) * length(p.xy) - size.x;

    vec2 d = vec2(dist, abs(p.z) - size.y);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}
```

### Super-Haxagon C++ wall geometry (fetched from GitHub)

```cpp
// Wall quad: 4 corners in polar space spanning one angular segment
Vec2f Wall::calcPoint(const float overflow, const float distance,
                      const float sides, const int side) {
    Vec2f point = {0, 0};
    auto width = static_cast<float>(side) * TAU / sides + overflow;
    if (width > TAU + WALL_OVERFLOW) width = TAU + WALL_OVERFLOW;
    point.x = distance * std::cos(width);
    point.y = distance * std::sin(width + PI);
    return point;
}

// Background: alternating colored triangles from center to edge
void SurfaceGame::drawBackground(const Color& color1, const Color& color2,
                                  const float sides) {
    const auto exactSides = static_cast<size_t>(std::ceil(sides));
    _screen.clear(color1);

    std::vector<Vec2f> edges;
    edges.resize(exactSides);
    for (size_t i = 0; i < exactSides; i++) {
        edges[i].x = 2.0f * std::cos(static_cast<float>(i) * TAU / sides);
        edges[i].y = 2.0f * std::sin(static_cast<float>(i) * TAU / sides + PI);
    }

    // Draw alternating colored triangles
    for (size_t i = 0; i < exactSides - 1; i = i + 2) {
        triangle[0] = {0, 0};
        triangle[1] = edges[i];
        triangle[2] = edges[i + 1];
        this->project(color2, triangle);
    }
}
```

## Algorithm

Polar-coordinate fragment shader with hash-based procedural wall patterns.

### Core rendering structure

1. Center UV relative to screen center, apply aspect ratio correction
2. Apply perspective distortion: `uv /= dot(uv, wobbleVec) * perspective + 1.0`
3. Convert to polar: `r = length(uv)`, `theta = atan(uv.y, uv.x) + rotation`
4. Compute section index: `zone = floor(sides * theta / TAU)`, `odd = mod(zone, 2.0)`
5. Compute N-gon corrected radius: `hexR = cos(floor(0.5 + theta/slice) * slice - theta) * r`
6. Draw alternating background segments using `odd`
7. Compute wall depth: `depth = hexR * depthScale + time * wallSpeed`
8. Hash-based wall lookup per ring to determine wall presence
9. Draw center N-gon outline using hexR distance
10. Color everything through gradient LUT

### Adaptation from references

**Keep verbatim:**
- Polar coordinate setup and section detection pattern (Pseudo Hyper Hexagonest)
- N-gon corrected radius via `SHAPE(N)` macro approach (Super Hexagon - Colored)
- Perspective distortion formula (Duper Hexagon)
- Variable spin rate with sinusoidal wobble (Pseudo Hyper Hexagonest)

**Replace:**
- Hardcoded colors and color cycling -> `texture(gradientLUT, vec2(t, 0.5)).rgb`
- `iTime` -> `time` uniform accumulated in C++ Setup function
- Cursor/player rendering -> remove entirely (game element, not visualizer)
- Hardcoded bitmask wall tables -> hash-based procedural generation (see below)
- Fixed 6 sides -> configurable `sides` uniform
- Mouse input -> remove

### Procedural wall generation

Replace the hardcoded wall table from Pseudo Hyper Hexagonest with hash-based generation:

```
ringIndex = floor(depth / wallSpacing)
segIndex = floor(theta * sides / TAU)
wallHash = fract(sin(ringIndex * 127.1 + segIndex * 311.7 + patternSeed) * 43758.5453)
hasWall = step(gapChance, wallHash)

// Guarantee at least one gap per ring
gapSeg = floor(fract(sin(ringIndex * 269.3 + patternSeed) * 43758.5453) * sides)
if segIndex == gapSeg then hasWall = 0
```

Wall rendering uses smoothstep for glow edges rather than hard step.

### Difficulty meta-parameter

A single 0-1 float that scales multiple shader-internal values:
- `effectiveSpeed = wallSpeed * (1.0 + difficulty * 2.0)`
- `effectiveGap = gapChance * (1.0 - difficulty * 0.5)`
- `effectiveSpacing = wallSpacing * (1.0 - difficulty * 0.3)`
- `effectiveRotSpeed = rotationSpeed * (1.0 + difficulty * 1.5)`

Applied in the shader so it responds to per-frame modulation.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| sides | int | 3-12 | 6 | Number of angular segments |
| difficulty | float | 0.0-1.0 | 0.5 | Meta-parameter scaling speed, density, gaps |
| wallSpeed | float | 0.5-10.0 | 3.0 | Base inward rush speed |
| wallThickness | float | 0.02-0.3 | 0.1 | Radial thickness of wall bands |
| wallSpacing | float | 0.2-2.0 | 0.5 | Distance between wall rings |
| gapChance | float | 0.1-0.8 | 0.35 | Probability a segment is open per ring |
| centerSize | float | 0.05-0.5 | 0.15 | Center polygon radius |
| rotationSpeed | float | -PI..PI | 0.5 | Global rotation rate (rad/s) |
| pulseAmount | float | 0.0-0.5 | 0.1 | Center polygon pulse intensity |
| perspective | float | 0.0-1.0 | 0.3 | Pseudo-3D perspective distortion |
| bgContrast | float | 0.0-1.0 | 0.3 | Brightness diff between alternating segments |
| wallGlow | float | 0.0-2.0 | 0.5 | Soft glow width on wall edges |
| glowIntensity | float | 0.1-3.0 | 1.0 | Overall brightness multiplier |
| patternSeed | float | 0.0-100.0 | 0.0 | Seed for wall pattern hash |
| flipRate | float | 0.0-5.0 | 1.0 | Rotation direction reversal frequency (Hz) |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT lower bound (Hz) |
| maxFreq | float | 1000-16000 | 14000 | FFT upper bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.0 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.1 | Minimum brightness floor |

## Modulation Candidates

- **difficulty**: Dynamic intensity — verse relaxed, chorus intense
- **wallSpeed**: Faster = frantic rushing; slower = contemplative
- **gapChance**: More gaps = airy/open; fewer = dense/aggressive
- **rotationSpeed**: Spin intensity, dramatic when beat-modulated
- **pulseAmount**: Center polygon breathing
- **perspective**: Depth wobble
- **sides**: Shifting geometry (integer jumps most striking)
- **patternSeed**: Slowly incrementing = evolving patterns; sudden jumps = pattern resets
- **bgContrast**: Segment visibility, subtle mood shifts
- **flipRate**: Disorientation level
- **wallGlow**: Edge softness/intensity
- **glowIntensity**: Overall brightness punch

### Interaction Patterns

**difficulty x audio energy** (Cascading threshold): Low difficulty keeps visuals calm even with loud audio. High difficulty means even moderate audio energy creates overwhelming wall density. Difficulty sets the "sensitivity floor" for when audio begins to affect the visual intensity.

**gapChance x sides** (Competing forces): Few sides (3-4) with low gapChance = claustrophobic narrow passages. Many sides (10-12) with high gapChance = open lattice. Perceived density depends on their ratio, not either value alone.

**rotationSpeed x flipRate** (Resonance): Fast rotation + frequent flips = jittery disorientation (both spike together). Fast rotation + rare flips = smooth hypnotic spinning. The "snappy Super Hexagon feel" emerges when flipRate is beat-triggered.

## Notes

- Wall patterns are purely procedural via hash — no data textures needed
- The N-gon distance function handles non-integer side counts with smooth morphing
- Difficulty is applied in the shader, not C++ Setup, so it responds to per-frame modulation
- Perspective distortion from Duper Hexagon adds visual depth cheaply
- The `flipRate` parameter could be implemented as accumulated time since last flip, with flip triggered when `fract(time * flipRate)` crosses 0.5, reversing a rotation direction multiplier in the Setup function
- Wall glow via smoothstep on wall distance creates neon-like soft edges matching the project's aesthetic
