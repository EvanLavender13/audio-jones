# Plasma Surge & Sway

Adds two behaviors from the "Thunder Dance" shader to the existing Plasma generator: a power surge oscillator that makes bolt displacement pulse irregularly, and coherent directional sway that replaces chaotic random wiggle with wind-like sweeping motion.

## Classification

- **Category**: GENERATORS > Filament (existing effect enhancement)
- **Pipeline Position**: Same as Plasma (section 11)

## Attribution

- **Based on**: "thunder dance by sin" by yufengjie
- **Source**: https://www.shadertoy.com/view/3cdSzl
- **License**: CC BY-NC-SA 3.0

## Reference Code

```glsl
#define S smoothstep
#define T iTime
#define PI 3.141596


mat2 rotate(float a){
  float s = sin(a);
  float c = cos(a);
  return mat2(c,-s,s,c);
}


void mainImage(out vec4 O, in vec2 I){
  vec2 R = iResolution.xy;
  vec2 uv = (I*2.-R) / R.y;
  O.rgb *= 0.;
  O.a = 1.;

  float total = 3.;
  float a = T*0.1;
  vec2 v = normalize(vec2(cos(a),sin(a)));

  // v1 v2 Perpendicular to the rotation angle
  vec2 v1 = v.yx*vec2(-1,1)*sin(T);
  vec2 v2 = v.yx*vec2(1,-1)*sin(T);

  vec2 p = uv*rotate(a);
  float n1 = fbm(uv+v1);
  float n2 = fbm(uv+v2);
  float n = (n1 + n2)/2.;

  float power = (sin(10.*T+sin(20.*T)))*.8;
  float range = S(1.2,0.,abs(p.x));

  float d = abs(p.y+n*range*power);
  d = pow(.01/d,2.);
  O.rgb += d;
}

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec2 p) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2
    const float K2 = 0.211324865; // (3-sqrt(3))/6

    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;

    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);

    vec3 n = h * h * h * h * vec3(
        dot(a, hash(i + 0.0)),
        dot(b, hash(i + o)),
        dot(c, hash(i + 1.0))
    );

    return dot(n, vec3(70.0));
}


float fbm(vec2 p){
  float a = .5;
  float n = 0.;

  for(float i=0.;i<8.;i++){
    n += a * noise(p);
    p *= 2.;
    a *= .5;
  }
  return n;
}
```

## Algorithm

Enhancement to existing `shaders/plasma.fs` and `src/effects/plasma.cpp`.

### Power Surge

The reference uses `(sin(10*T + sin(20*T))) * 0.8` to create an irregular pulsing multiplier on displacement. The nested sinusoid structure means peaks cluster unpredictably rather than metronomically.

Adaptation:
- Keep: the nested `sin(A*T + sin(B*T))` structure for irregular pulsing
- Replace: hardcoded `0.8` amplitude with `surgeAmount` config field
- Replace: hardcoded `10.0`/`20.0` frequency constants with fixed internal ratios derived from `animPhase` (already accumulated on CPU)
- The surge multiplies `displacement` in the shader: `displacement * mix(1.0, surge, surgeAmount)`
- When `surgeAmount` is 0, displacement is unmodified (current behavior)

### Perpendicular Sway

The reference constructs two perpendicular offset vectors from a rotating angle, scales them by `sin(T)`, and samples FBM at `uv + v1` and `uv + v2` (averaged).

Adaptation:
- Keep: the perpendicular vector construction `v.yx * vec2(-1,1)` and `v.yx * vec2(1,-1)`
- Keep: averaging two FBM samples for the coherent displacement
- Replace: `T * 0.1` rotation speed with `swayRotationPhase` (accumulated on CPU from `swayRotation` speed param)
- Replace: `sin(T)` oscillation with `sin(swayPhase)` (accumulated on CPU from `swaySpeed` param)
- Blend: `mix(randomDisplacement, swayDisplacement, sway)` where `sway` is 0.0-1.0
- When `sway` is 0, existing random X/Y displacement is used (current behavior)

### CPU-side phase accumulation (plasma.cpp)

Three new phase accumulators in `PlasmaEffect`:
- `surgePhase += animSpeed * deltaTime` (reuses animSpeed to keep surge tied to noise evolution rate)
- `swayPhase += swaySpeed * deltaTime`
- `swayRotationPhase += swayRotation * deltaTime`

### Shader changes (plasma.fs)

In the displacement section of the bolt loop, after computing `dx`/`dy`:
1. Compute surge: `float surge = sin(10.0 * surgePhase + sin(20.0 * surgePhase));`
2. Compute sway vectors from `swayRotationPhase` and `swayPhase`
3. Sample two FBM values at `uv + v1` and `uv + v2`, average for sway displacement
4. Blend: `vec2 finalDisp = mix(vec2(dx, dy) - 0.5, swayDisp, sway);`
5. Apply surge: `displaced += finalDisp * displacement * mix(1.0, surge, surgeAmount);`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| surgeAmount | float | 0.0-1.0 | 0.0 | Blend from steady displacement (0) to pulsing surge (1) |
| sway | float | 0.0-1.0 | 0.0 | Blend from random displacement (0) to coherent directional sway (1) |
| swaySpeed | float | 0.0-5.0 | 1.0 | How fast the bolt swings back and forth along the sway direction |
| swayRotation | float | 0.0-2.0 | 0.1 | How fast the sway direction itself rotates |

All defaults are 0.0 for `surgeAmount` and `sway` so existing presets are unaffected.

## Modulation Candidates

- **surgeAmount**: modulating creates sections where bolts breathe vs sections where they hold steady
- **sway**: modulating transitions between chaotic sparking and flowing wind motion
- **swaySpeed**: modulating varies the gusting rate - slow dreamy sway vs rapid whipping
- **swayRotation**: modulating varies how quickly the wind direction shifts

### Interaction Patterns

- **Surge gates sway visibility (cascading threshold)**: Surge multiplies displacement intensity. When surge dips near zero, even high sway produces barely visible motion. When surge peaks, sway creates dramatic directional sweeps. Different audio bands on each create moments where direction and intensity align for bright flares.
- **Sway speed vs sway rotation (competing forces)**: High speed + low rotation = sustained directional whipping. High rotation + low speed = gentle lean that orbits. Both high = chaotic again, undermining the coherence that sway provides. The ratio determines whether the effect reads as "wind" or "turbulence."

## Notes

- All new defaults are zero, so existing presets load unchanged
- Surge reuses `animPhase` accumulator rate to stay musically coherent with noise evolution
- The perpendicular FBM samples add two extra `fbm()` calls per bolt when `sway > 0`; at 8 octaves and 8 bolts this could be expensive. Consider short-circuiting the sway FBM path when `sway == 0.0` in the shader.
