# Iris Rings

Concentric colored rings where each ring maps to a frequency band and shows a partial arc proportional to that band's energy — quiet frequencies shrink to thin slivers, loud ones bloom into full circles. A twist parameter fans successive rings into a pinwheel spiral, and a snap slider transitions the arc gating from smooth proportional opening to hard on/off threshold behavior.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator stage (after trail boost, before transforms)

## References

- User-provided Shadertoy shader (CC BY-SA 4.0) — concentric arc rings with cumulative rotation and angular gating

## Reference Code

```glsl
/*
Code Under Attribution-ShareAlike 4.0 International (CC BY-SA 4.0) Licence
https://creativecommons.org/licenses/by-sa/4.0/legalcode
**/
#define PI 3.141592
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}
vec3 p(float t){
return palette( t, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.33,0.67) );
}
mat2 r(float a){
    float c=cos(a),s=sin(a);
    return mat2(c,s,-s,c);
}
vec3 fig(vec2 uv){
vec3 d  =vec3(0.);
    float n = 10.;+cos(iTime*2.)*10.;
    for(float i=0.;i<=n;i++){
        uv*=r(iTime*.2+PI/n);
     float l = length(uv)-.07*i;

    float q = step(cos(iTime+i)*.25+.75,(atan(uv.y,uv.x)+PI)/(2.*PI));
    l = abs(l)-.001;
    d+= vec3(p(i/n+fract(iTime*.75))*smoothstep(.02,.015,l)*q);
    }
    return d ;
}
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord.xy-.5* iResolution.xy)/iResolution.y;
    uv*=2.;
    if(mod(iTime,15.)<10.){
    uv = abs(uv)-.75;
    uv*=r(PI/4.);
    if(mod(iTime,15.)<5.){

    uv = abs(uv)-.125;
   }
   }
    vec3 d = fig(uv);

    vec3 col = vec3(d);


    fragColor = vec4(col,1.0);
}
```

## Algorithm

Adaptation from reference to AudioJones generator conventions:

**Keep from reference:**
- Core loop structure: iterate over rings, compute SDF distance, apply angular gate, accumulate glow
- Cumulative per-ring rotation via mat2 multiply (the twist mechanic)
- Ring SDF: `abs(length(uv) - radius) - thickness`
- Angular gating via `atan(uv.y, uv.x)` normalized to 0-1

**Replace:**
- `palette()` function → `texture(gradientLUT, vec2(float(i)/float(layers), 0.5)).rgb`
- `iTime` arc gating → FFT energy lookup per ring: `freq = baseFreq * pow(maxFreq/baseFreq, float(i)/float(layers-1))`
- Fixed `n = 10` → `layers` uniform
- Fixed `0.07` spacing → `ringSpacing` uniform
- `smoothstep(.02,.015,l)` → standard generator glow: `smoothstep(thickness, 0.0, dist) * glowIntensity / max(abs(dist), 0.001)`

**Arc gating logic:**
- Compute normalized angle: `a = (atan(uv.y, uv.x) + PI) / TWO_PI`
- Compute FFT energy for this ring's frequency
- Proportional mode: `gate = step(a, energy)` — arc opens from 0 to energy fraction of full circle
- Snap mode: `gate = step(threshold, energy)` — ring fully on or fully off
- Blend: `gate = mix(proportionalGate, snapGate, snap)`

**Rotation:**
- Accumulate `rotAngle += rotationSpeed * deltaTime` on CPU, pass as uniform
- Per-ring twist applied in shader loop: multiply UV by `r(rotAngle + twist * float(i) / float(layers))`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| layers | int | 4-96 | 24 | Number of concentric rings |
| ringSpacing | float | 0.01-0.15 | 0.05 | Distance between successive rings |
| thickness | float | 0.001-0.02 | 0.005 | Ring line width |
| twist | float | -PI to PI | 0.3 | Cumulative rotation offset per successive ring |
| rotationSpeed | float | -PI to PI | 0.2 | Global rotation rate (rad/s) |
| snap | float | 0-1 | 0.0 | Blend: proportional arc (0) vs threshold on/off (1) |
| snapThreshold | float | 0-1 | 0.5 | Energy cutoff when snap > 0 |
| glowIntensity | float | 0.1-3.0 | 1.0 | Ring glow brightness |
| baseFreq | float | 27.5-440 | 55.0 | Lowest mapped frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest mapped frequency (Hz) |
| gain | float | 0.1-10 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.0 | FFT contrast exponent |
| baseBright | float | 0-1 | 0.05 | Minimum ring brightness when no audio |

## Modulation Candidates

- **twist**: fans rings between concentric and pinwheel — dramatic shape change
- **snap**: transitions between fluid proportional arcs and percussive on/off
- **ringSpacing**: breathing ring density, rings spread and compress
- **glowIntensity**: pulsing overall brightness
- **rotationSpeed**: spin rate changes
- **snapThreshold**: shifts the energy cutoff, revealing or hiding rings

### Interaction Patterns

**snap + snapThreshold (cascading threshold):** At snap=0, threshold is irrelevant — arcs open proportionally. As snap increases toward 1, threshold becomes a hard gate that determines which rings appear at all. Modulating both together creates verse/chorus dynamics: quiet sections show only the loudest frequencies as isolated rings, while choruses bloom all rings into full circles.

**twist + rotationSpeed (competing forces):** Twist fans rings outward into a pinwheel while rotation spins the whole figure. When both are modulated by different sources, the visual oscillates between tight concentric circles and loose spiraling fans — neither parameter dominates.

## Notes

- Ring count is purely visual density; frequency coverage is always baseFreq → maxFreq (new paradigm per `docs/research/fft-frequency-spread.md`)
- At high layer counts (60+) the ring loop is the performance bottleneck — consider documenting a recommended range of 12-48
- The reference shader's `abs(uv)` symmetry folding is intentionally omitted — existing transform effects (Kaleidoscope, KIFS) handle symmetry
- Arc gating direction: arcs open counterclockwise from the right (+x axis) matching standard `atan2` convention
