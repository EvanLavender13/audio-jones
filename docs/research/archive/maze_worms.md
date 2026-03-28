# Maze Worms

Autonomous worm agents draw colored trails that fill the screen in maze-like patterns. Each worm advances one pixel per step, steering according to a configurable turning strategy — spiraling inward, following walls, or branching with mixed chirality. Trails decay over time so the screen never saturates, creating perpetual motion with breathing density. The visual ranges from meditative calligraphy (sparse, slow decay) to churning graffiti texture (dense, fast decay).

## Classification

- **Category**: SIMULATIONS
- **Pipeline Position**: Simulation compute → trail texture → blend composite
- **Compute Model**: Compute shader (agent SSBO) + TrailMap ping-pong (diffusion + decay)

## Attribution

- **Based on**: "maze worms / graffitis 3" series by FabriceNeyret2
- **Source**: https://www.shadertoy.com/view/XdjcRD (3), https://www.shadertoy.com/view/4djcRD (3b), https://www.shadertoy.com/view/Xs2cRD (3c), https://www.shadertoy.com/view/XdSczm (3d)
- **License**: CC BY-NC-SA 3.0

## References

- [maze worms / graffitis 3](https://www.shadertoy.com/view/XdjcRD) - Spiral turning variant (a += 1/stepCount)
- [maze worms / graffitis 3b](https://www.shadertoy.com/view/4djcRD) - Wall-following variant (while-loop collision avoidance)
- [maze worms / graffitis 3c](https://www.shadertoy.com/view/Xs2cRD) - Wall-hugging variant (seek nearest wall, follow edge)
- [maze worms / graffitis 3d](https://www.shadertoy.com/view/XdSczm) - Mixed chirality variant (CW/CCW per worm)

## Reference Code

### Variant 3 — Spiral turning (canonical)

```glsl
#define CS(a)  vec2(cos(a),sin(a))
#define rnd(x) ( 2.* fract(456.68*sin(1e4*x+mod(iDate.w,100.))) -1.) // NB: mod(t,1.) for less packed pattern
#define T(U) texture(iChannel0, (U)/R)
const float r = 1.5, N = 50., da = .5, // width , number of worms , turn angle at hit
            L = 10., l= 6.;            // sinusoidal path parameters: L straight + l turn

void mainImage( out vec4 O, vec2 U )
{
    vec2 R = iResolution.xy;

    if (T(R).x==0.) { U = abs(U/R*2.-1.); O  = vec4(max(U.x,U.y)>1.-r/R.y); O.w=0.; return; }

    if (U.y==.5 && T(U).w==0.) {                           // initialize heads state: P, a, t
        O = vec4( R/2. + R/2.4* vec2(rnd(U.x),rnd(U.x+.1)) , 3.14 * rnd(U.x+.2), 1);
        if (T(O.xy).x>0.) O.w = 0.;                        // invalid start position
        return;
    }

    O = T(U);

    for (float x=.5; x<N; x++) {                           // draw heads
        vec4 P = T(vec2(x,.5));                            // head state: P, a, t
        if (P.w>0.) O += smoothstep(r,0., length(P.xy-U))  // draw head if active
                         *(.5+.5*sin(.01*P.w+vec4(0,-2.1,2.1,1)));  // coloring scheme
    }

    if (U.y==.5) {                                         // head programms: worm strategy
        vec4 P = T(U);                                     // head state: P, a, t
        if (P.w>0.) {                                      // if active
            float a = P.z;
            a += 1./P.w;
            if  ( T(P.xy+(r+2.)*CS(a)).w > 0. )  { O.w = 0.; return; }
            O = vec4(P.xy+CS(a),mod(a,6.2832),P.w+1.);     // move head
        }
    }
}
```

### Variant 3b — Wall-following

```glsl
#define CS(a)  vec2(cos(a),sin(a))
#define rnd(x) ( 2.* fract(456.68*sin(1e3*x+mod(iDate.w,100.))) -1.)
#define T(U) textureLod(iChannel0, (U)/R, 0.)
const float r = 1.5, N = 50., da = .5, // width , number of worms , turn angle at hit
            L = 10., l= 6.;

void mainImage( out vec4 O, vec2 U )
{
    vec2 R = iResolution.xy;

    if (T(R).x==0.) { U = abs(U/R*2.-1.); O  = vec4(max(U.x,U.y)>1.-r/R.y); O.w=0.; return; }

    if (U.y==.5 && T(U).w==0.) {
        O = vec4( R/2. + R/2.4* vec2(rnd(U.x),rnd(U.x+.1)) , 3.14 * rnd(U.x+.2), 1);
        if (T(O.xy).x>0.) O.w = 0.;
        return;
    }

    O = T(U);

    for (float x=.5; x<N; x++) {
        vec4 P = T(vec2(x,.5));
        if (P.w>0.) O += smoothstep(r,0., length(P.xy-U))
                         *(.5+.5*sin(.01*P.w+vec4(0,-2.1,2.1,1)));
    }

    if (U.y==.5) {
        vec4 P = T(U);
        if (P.w>0.) {
            float a = P.z;
             a -= 1./sqrt(P.w);
            while ( T(P.xy+(r+2.)*CS(a)).w > 0. && a < 13. )  a += da; // hit: turn
            if (a>=13.) { O.w = 0.; return; }              // stop head
            O = vec4(P.xy+CS(a),mod(a,6.2832),P.w+1.);     // move head
        }
    }
}
```

### Variant 3c — Wall-hugging

```glsl
#define CS(a)  vec2(cos(a),sin(a))
#define rnd(x) ( 2.* fract(456.68*sin(1e3*x+mod(iDate.w,100.))) -1.)
#define T(U) textureLod(iChannel0, (U)/R, 0.)
const float r = 1.5, N = 100., da = .1; // width , number of worms , turn angle at hit

void mainImage( out vec4 O, vec2 U )
{
    vec2 R = iResolution.xy;

    if (T(R).x==0.) { U = abs(U/R*2.-1.); O  = vec4(max(U.x,U.y)>1.-r/R.y); O.w=0.; return; }

    if (U.y==.5 && T(U).w==0.) {
        O = vec4( R/2. + R/2.4* vec2(rnd(U.x),rnd(U.x+.1)) , 3.14 * rnd(U.x+.2), 1);
        if (T(O.xy).x>0.) O.w = 0.;
        return;
    }

    O = T(U);

    for (float x=.5; x<N; x++) {
        vec4 P = T(vec2(x,.5));
        if (P.w>0.) O += smoothstep(r,0., length(P.xy-U))
                         *(.5+.5*sin(6.3*x/N+vec4(0,-2.1,2.1,1)));
    }

    if (U.y==.5) {
        vec4 P = T(U);
        if (P.w>0.) {
            float a = P.z-1.6, a0=a;
#define next T(P.xy+(r+2.)*CS(a)).w
            while ( next == 0. && a < 13. )  a += da;      // seek for last angle before hit
            a = max(a0, a-4.*da);
            if ( next > 0.) { O.w = 0.; return; }          // stop head
            O = vec4(P.xy+CS(a),mod(a,6.2832),P.w+1.);     // move head
        }
    }
}
```

### Variant 3d — Mixed chirality

```glsl
#define CS(a)  vec2(cos(a),sin(a))
#define rnd(x) ( 2.* fract(456.68*sin(1e4*x+mod(iDate.w,100.))) -1.)
#define T(U) texture(iChannel0, (U)/R)
const float r = 1.5, N = 50., da = .5, // width , number of worms , turn angle at hit
            L = 10., l= 6.;

void mainImage( out vec4 O, vec2 U )
{
    vec2 R = iResolution.xy;

    if (T(R).x==0.) { U = abs(U/R*2.-1.); O  = vec4(max(U.x,U.y)>1.-r/R.y); O.w=0.; return; }

    if (U.y==.5 && T(U).w==0.) {
        O = vec4( R/2. + R/2.4* vec2(rnd(U.x),rnd(U.x+.1)) , 3.14 * rnd(U.x+.2), 1);
        if (T(O.xy).x>0.) O.w = 0.;
        return;
    }

    O = T(U);

    for (float x=.5; x<N; x++) {
        if (R.y < 200. && x>5.) break;
        vec4 P = T(vec2(x,.5));
        if (P.w>0.) O += smoothstep(r,0., length(P.xy-U))
                         *mix(vec4(0,.5,0,1),vec4(1,.5,1,1),.01*P.w);
    }

    if (U.y==.5) {
        vec4 P = T(U);
        if (P.w>0.) {
            float a = P.z;
            a += .001*P.w * sign(sin(U.x));
            if  ( T(P.xy+(r+2.)*CS(a)).w > 0. )  { O.w = 0.; return; }
            O = vec4(P.xy+CS(a),mod(a,6.2832),P.w+1.);     // move head
        }
    }
}
```

## Algorithm

### Adaptation from Shadertoy to AudioJones Simulation Pipeline

The original Shadertoy shaders use a single-buffer feedback trick: agent state is encoded in row 0 of the framebuffer, and the rest of the framebuffer holds trail colors. In AudioJones, this separates into the standard simulation architecture:

#### Keep verbatim
- Core turning formulas from each variant (the `if (U.y==.5)` block logic)
- `CS(a) = vec2(cos(a), sin(a))` helper
- `smoothstep(r, 0., length(P.xy - U))` head drawing
- Collision detection: sample trail texture at `position + (radius + gap) * CS(angle)` to check for occupied space
- `mod(a, 6.2832)` angle wrapping

#### Replace
| Shadertoy | AudioJones |
|-----------|------------|
| Agent state in texture row 0 (`T(vec2(x, .5))`) | SSBO with `MazeWormAgent` struct per agent |
| `texture(iChannel0, ...)` for collision check | `imageLoad(trailTexture, ivec2(...))` in compute shader |
| Head drawing in fragment shader (loop over all worms per pixel) | `imageStore()` in compute shader — each agent writes its own smoothstep circle to the trail texture |
| `iDate.w` for random seed | CPU-side seed uniform, set once at init |
| `P.w > 0.` active flag in alpha channel | `uint active` field in agent SSBO struct |
| Worm death (`O.w = 0.; return;`) | Set `active = 0` in SSBO; CPU respawn timer reinitializes dead agents to random open positions after cooldown |
| Border detection (`max(U.x,U.y) > 1 - r/R.y`) | Bounds check in compute shader: `if (newPos.x < margin \|\| newPos.x > width - margin \|\| ...)` |
| `.5 + .5 * sin(.01*P.w + vec4(0,-2.1,2.1,1))` coloring | `hue` field in agent struct → sample gradientLUT in trail shader |

#### Agent SSBO struct (32-byte aligned)

```
MazeWormAgent {
    float x, y;        // position in pixels
    float angle;        // heading in radians
    float age;          // step count (increments each step)
    uint  active;       // 1 = alive, 0 = dead (awaiting respawn)
    float hue;          // gradient LUT coordinate [0,1]
    float pad[2];       // padding to 32 bytes
}
```

#### Compute shader structure

```
layout(local_size_x = 1024) in;

// Per-agent update:
1. Read agent from SSBO
2. If inactive: skip (CPU handles respawn)
3. Compute new angle based on turning mode:
   - SPIRAL:      angle += curvature / age
   - WALL_FOLLOW: angle -= curvature / sqrt(age);
                   while (collision(angle) && attempts < maxAttempts)
                       angle += turnAngle;
                   if (attempts >= maxAttempts) deactivate;
   - WALL_HUG:    scan from angle-rearAngle to find nearest wall,
                   back off by gap*turnAngle;
                   if (collision) deactivate;
   - MIXED:       angle += curvature * age * chirality;  // chirality = ±1 per agent
                   if (collision) deactivate;
4. Bounds check: deactivate if new position exits margins
5. Collision check at new position: if trail alpha > threshold, deactivate
6. If still active: write smoothstep circle to trail texture via imageStore
7. Update position, angle, age in SSBO
```

#### Respawning (CPU side)

Dead agents respawn after a configurable cooldown. On respawn:
1. Pick random position within spawn radius of screen center (or random anywhere)
2. Sample trail texture at that position — if alpha > threshold, skip (position occupied)
3. Try up to N random positions; if all occupied, wait for next frame
4. Set random angle, reset age to 1, set active = 1
5. Assign hue via `ColorConfigAgentHue()`

#### Trail softness

The smoothstep circle radius in the compute shader controls trail width. Softness modulates the edge falloff:
- `softness = 0`: `step(radius, dist)` — hard edge
- `softness = 1`: `smoothstep(radius, 0., dist)` — full glow
- Between: `smoothstep(radius, radius * (1.0 - softness), dist)`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| wormCount | int | 4-200 | 50 | Number of simultaneous worm agents |
| turningMode | enum | SPIRAL/WALL_FOLLOW/WALL_HUG/MIXED | SPIRAL | Agent steering strategy |
| curvature | float | 0.1-10.0 | 1.0 | Turning strength — higher = tighter curves |
| turnAngle | float | 0.05-1.0 | 0.5 | Angle increment for wall-follow collision avoidance (radians) |
| trailWidth | float | 0.5-5.0 | 1.5 | Radius of the smoothstep circle drawn at each head |
| softness | float | 0.0-1.0 | 0.8 | Edge falloff — 0 = hard, 1 = full glow |
| decayHalfLife | float | 0.5-30.0 | 8.0 | Trail persistence — seconds to fade to 50% (via TrailMap) |
| diffusionScale | int | 0-5 | 1 | Trail blur radius — 0 = crisp, 5 = smeared |
| respawnCooldown | float | 0.0-5.0 | 0.5 | Seconds before a dead worm respawns |
| stepsPerFrame | int | 1-8 | 2 | Agent steps computed per frame — higher = faster growth |
| collisionGap | float | 1.0-5.0 | 2.0 | Lookahead distance beyond trail width for collision sensing |
| boostIntensity | float | 0.0-2.0 | 1.0 | Blend intensity onto scene (standard sim boost) |
| blendMode | enum | BlendMode | SCREEN | Compositing mode (standard sim boost) |

## Modulation Candidates

- **wormCount**: Population density — more worms = faster fill, more collisions, more chaotic
- **curvature**: Spiral tightness — low = sweeping arcs, high = tight rosettes
- **turnAngle**: Wall-follow agility — small = smooth corridor tracking, large = jagged right-angle turns
- **trailWidth**: Visual weight — thin = delicate threads, thick = bold strokes
- **softness**: Edge quality — modulating creates dynamic hard/soft transitions
- **decayHalfLife**: Persistence — fast decay = rapid turnover, slow = dense accumulation
- **stepsPerFrame**: Growth speed — modulating creates bursts of rapid growth then pauses
- **boostIntensity**: Overall brightness/presence

### Interaction Patterns

**Cascading threshold — decayHalfLife gates wormCount impact**: At low decay (long persistence), adding worms rapidly saturates the screen and most worms die on spawn. At high decay (fast fade), even many worms find open space. Audio-driven decay effectively gates whether population surges create chaos or merely fill gaps. Quiet passages with slow decay build dense texture; loud passages with fast decay create rapid churning renewal.

**Competing forces — curvature vs. trailWidth**: Tight curvature makes worms curve back toward their own trail. Wide trails make that self-intersection happen sooner (worm collides with its own fat tail). The tension between wanting to spiral and wanting to survive creates a natural lifespan — modulatable curvature against fixed width means audio can shorten or lengthen worm lifetimes dynamically.

**Resonance — stepsPerFrame × turnAngle**: When both are high simultaneously, wall-following worms navigate at high speed with coarse turning, creating distinctive jagged rapid-fire patterns that only appear when both values peak together. Low stepsPerFrame + fine turnAngle produces smooth slow wall-tracing. The visual character shifts dramatically at the coincidence point.

## Notes

- **Performance**: Compute shader dispatches at 1024 threads/group. At 200 worms the agent update is trivially cheap; the trail texture write (smoothstep circles) is the main cost, bounded by stepsPerFrame × wormCount imageStore calls per frame.
- **Multi-step per frame**: The original shaders advance one pixel per frame. At 60fps this is quite slow for large screens. `stepsPerFrame` loops the agent update N times per frame to reach comfortable growth speeds.
- **Trail texture collision**: Agents read the trail texture via `imageLoad()` for collision detection. Since agents write to the same texture, there's a theoretical race condition when two worms approach the same pixel simultaneously. In practice this is benign — worst case, both worms survive one extra step. No synchronization needed.
- **Wall-follow loop bound**: Variant 3b's `while (a < 13.)` loop is bounded to prevent infinite spinning. In the compute shader, cap at `maxAttempts = int(2π / turnAngle) + 1` iterations.
- **Respawn fairness**: CPU-side respawn with trail texture readback (once per frame, sample N candidate positions) avoids GPU-side random position generation complexity. At 200 worms this is negligible overhead.
