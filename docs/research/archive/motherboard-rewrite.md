# Motherboard Rewrite

Complete rewrite of the motherboard generator. One unified Kali-family fractal algorithm where every difference between the three Circuits references is a continuous parameter. No mode combo, no branching between separate functions. Each reference shader is a specific point in the parameter space; you explore the space between them with sliders.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator (blend compositor)

## Attribution

- **Based on**: "Circuits" by Kali
- **Source**: https://www.shadertoy.com/view/XlX3Rj
- **License**: MIT

- **Based on**: "Circuits II" by Kali
- **Source**: https://www.shadertoy.com/view/wlBcDK
- **License**: CC BY-NC-SA 3.0

- **Based on**: "Circuits III" by Kali
- **Source**: https://www.shadertoy.com/view/WlXBWN
- **License**: CC BY-NC-SA 3.0

## References

- [Circuits](https://www.shadertoy.com/view/XlX3Rj) — Kali accumulation, compound orbit trap, width-per-depth
- [Circuits II](https://www.shadertoy.com/view/wlBcDK) — Anisotropic fold, stepping orbit trap animation
- [Circuits III](https://www.shadertoy.com/view/WlXBWN) — 90deg fold rotation, box-distance junctions, smoothstep edges

## Reference Code

### Circuits (Ref 1) — MIT License

```glsl
// This content is under the MIT License.

#define time iTime*.02


#define width .005
float zoom = .18;

float shape=0.;
vec3 color=vec3(0.),randcol;

void formula(vec2 z, float c) {
    float minit=0.;
    float o,ot2,ot=ot2=1000.;
    for (int i=0; i<9; i++) {
        z=abs(z)/clamp(dot(z,z),.1,.5)-c;
        float l=length(z);
        o=min(max(abs(min(z.x,z.y)),-l+.25),abs(l-.25));
        ot=min(ot,o);
        ot2=min(l*.1,ot2);
        minit=max(minit,float(i)*(1.-abs(sign(ot-o))));
    }
    minit+=1.;
    float w=width*minit*2.;
    float circ=pow(max(0.,w-ot2)/w,6.);
    shape+=max(pow(max(0.,w-ot)/w,.25),circ);
    vec3 col=normalize(.1+texture(iChannel1,vec2(minit*.1)).rgb);
    color+=col*(.4+mod(minit/9.-time*10.+ot2*2.,1.)*1.6);
    color+=vec3(1.,.7,.3)*circ*(10.-minit)*3.*smoothstep(0.,.5,.15+texture(iChannel0,vec2(.0,1.)).x-.5);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord.xy / iResolution.xy - .5;
    pos.x*=iResolution.x/iResolution.y;
    vec2 uv=pos;
    float sph = length(uv); sph = sqrt(1. - sph*sph)*1.5;
    uv=normalize(vec3(uv,sph)).xy;
    float a=time+mod(time,1.)*.5;
    vec2 luv=uv;
    float b=a*5.48535;
    uv*=mat2(cos(b),sin(b),-sin(b),cos(b));
    uv+=vec2(sin(a),cos(a*.5))*8.;
    uv*=zoom;
    float pix=.5/iResolution.x*zoom/sph;
    float dof=max(1.,(10.-mod(time,1.)/.01));
    float c=1.5+mod(floor(time),6.)*.125;
    for (int aa=0; aa<36; aa++) {
        vec2 aauv=floor(vec2(float(aa)/6.,mod(float(aa),6.)));
        formula(uv+aauv*pix*dof,c);
    }
    shape/=36.; color/=36.;
    vec3 colo=mix(vec3(.15),color,shape)*(1.-length(pos))*min(1.,abs(.5-mod(time+.5,1.))*10.);
    colo*=vec3(1.2,1.1,1.0);
    fragColor = vec4(colo,1.0);
}
```

### Circuits II (Ref 2)

```glsl
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

mat2 rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c,s,-s,c);
}

vec3 fractal(vec2 p)
{
       p=vec2(p.x/p.y,1./p.y);
    p.y+=iTime*sign(p.y);
    p.x+=sin(iTime*.1)*sign(p.y)*4.;
    p.y=fract(p.y*.05);
    float ot1=1000., ot2=ot1, it=0.;
    for (float i=0.; i<10.; i++) {
        p=abs(p);
        p=p/clamp(p.x*p.y,0.15,5.)-vec2(1.5,1.);
        float m=abs(p.x);
        if (m<ot1) {
            ot1=m+step(fract(iTime*.2+float(i)*.05),.5*abs(p.y));
            it=i;
        }
        ot2=min(ot2,length(p));
    }

    ot1=exp(-30.*ot1);
    ot2=exp(-30.*ot2);
    return hsv2rgb(vec3(it*.1+.5,.7,1.))*ot1+ot2;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy-.5;
    uv.x*=iResolution.x/iResolution.y;
    float aa=6.;
    uv*=rot(sin(iTime*.1)*.3);
    vec2 sc=1./iResolution.xy/(aa*2.);
    vec3 c=vec3(0.);
    for (float i=-aa; i<aa; i++) {
        for (float j=-aa; j<aa; j++) {
            vec2 p=uv+vec2(i,j)*sc;
            c+=fractal(p);
        }
    }
    fragColor = vec4(c/(aa*aa*4.)*(1.-exp(-20.*uv.y*uv.y)),1.);
}
```

### Circuits III (Ref 3)

```glsl
mat2 rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c,s,-s,c);
}


vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


vec3 fractal(vec2 p) {
    float o=100.,l=o;
    p*=rot(-.3);
    p.x*=1.+p.y*.7;
    p*=.5+sin(iTime*.1)*.3;
    p+=iTime*.02;
    p=fract(p);
    for (int i=0; i<10; i++) {
        p*=rot(radians(90.));
        p.y=abs(p.y-.25);
        p=p/clamp(abs(p.x*p.y),0.,3.)-1.;
        o=min(o,abs(p.y-1.5)-.2)+fract(p.x+iTime*.3+float(i)*.2)*.5;
        l=min(l,length(max(vec2(0.),abs(p)-.5)));

    }
    o=exp(-5.*o);
    l=smoothstep(.1,.11,l);
    return hsv2rgb(vec3(o*.5+.6,.8,o+.1))+l*vec3(.4,.3,.2);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    uv-=.5;
    uv.x*=iResolution.x/iResolution.y;
    vec3 col=vec3(0.);
    float aa=4.;
    vec2 eps=1./iResolution.xy/aa;
    for (float i=-aa; i<aa; i++){
        for (float j=-aa; j<aa; j++){
            col+=fractal(uv+vec2(i,j)*eps);
        }
    }
    col/=pow(aa*2.,2.);
    fragColor = vec4(col,1.0);
}
```

## Algorithm

### Core Loop

One fractal function. Every reference uses the same skeleton — `abs` fold, divide by clamped value, subtract constant, track orbit trap minimums. The differences are all in what values feed each step.

```
p = abs(p);
p *= iterationRotationMatrix;
p.y = abs(p.y - foldOffset);
divisor = mix(dot(p,p), abs(p.x*p.y), inversionBlend);
p = p / clamp(divisor, clampLo, clampHi) - vec2(foldConstantX, foldConstantY);
```

**Inversion blend**: `mix(dot(p,p), abs(p.x*p.y), inversionBlend)` — 0.0 = squared magnitude (Ref 1), 1.0 = axis product (Ref 2/3). Intermediate values produce hybrid topology.

**Per-iteration rotation**: `iterRotation` in radians. 0 = no rotation (Ref 1/2), PI/2 = 90deg (Ref 3). Any angle works.

**Y-fold offset**: `abs(p.y - foldOffset)` before inversion. 0.0 = symmetric (Ref 1/2), 0.25 = asymmetric (Ref 3).

**Fold subtraction**: `vec2(foldConstantX, foldConstantY)`. Ref 1 uses (c, c) where c~1.5. Ref 2 uses (1.5, 1.0). Ref 3 uses (1.0, 1.0). Two sliders.

### Orbit Traps

Primary trap (trace distance): `min(abs(p.x), abs(p.y - trapOffset))` with flow modulation `+ fract(p.x + flowAccum + float(i) * 0.2) * flowIntensity`.
- `trapOffset = 0`: traces follow axes (Ref 1/2 territory)
- `trapOffset = 1.5`: traces offset to Ref 3 territory

Ring trap: `abs(length(p) - ringRadius)`. When `ringRadius > 0`, combined with primary trap via `min(max(primaryTrap, -length(p) + ringRadius), ringTrap)` — this is Ref 1's compound axis+ring geometry. When `ringRadius = 0`, ring trap disabled.

Junction trap: `length(max(vec2(0.0), abs(p) - junctionBox))`.
- `junctionBox = 0.0`: collapses to `length(p)` = point distance (Ref 1/2)
- `junctionBox = 0.5`: box distance producing rectangular pads (Ref 3)

Winning iteration tracked via `minit` for FFT frequency mapping and gradient LUT color.

### Rendering

Trace glow: `exp(-ot / (traceWidth * (1.0 + float(minit) * depthScale)))`.
- `depthScale = 0`: uniform glow width
- `depthScale > 0`: deeper iterations get wider glow (Ref 1's depth perception)

Junction glow: always on. `exp(-junctionDist / traceWidth)` for soft glow, mixed with `smoothstep(junctionEdge, junctionEdge + 0.01, junctionDist)` controlled by `junctionHardness`.
- `junctionHardness = 0`: pure exp glow (Ref 1/2)
- `junctionHardness = 1`: hard-edged pads (Ref 3)

### Compositing

Shape+color accumulation model (from Ref 1), which subsumes the direct-color approach:
- Per AA sample: compute `shape` (trace + junction glow) and `color` (shape * layerColor * fftBrightness)
- After AA averaging: `mix(vec3(bgLevel), color, shape)`
- `bgLevel` param: 0.0 = pure additive (Ref 2/3 feel), 0.15 = Ref 1 background blend

### Anti-Aliasing

Grid-based supersampling: `aaSamples` controls NxN grid (1=off, 2=4 samples, 3=9, 4=16). Offset = `1.0 / resolution / float(aaSamples)`. Default 1 (off) for performance.

### Shared Adaptations

- Coordinates: `(fragTexCoord * resolution - resolution * 0.5) / resolution.y`
- Pan via CPU-accumulated `panAccum`
- Rotation via CPU-accumulated `rotationAccum`
- Flow via CPU-accumulated `flowAccum`
- FFT: winning iteration `minit` maps to frequency band in log space (baseFreq to maxFreq)
- Color: gradient LUT at `float(minit) / float(iterations)`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| iterations | int | 4-16 | 10 | Fold depth; each iteration = one frequency band |
| zoom | float | 0.1-4.0 | 0.5 | Scale factor before tiling |
| inversionBlend | float | 0.0-1.0 | 0.0 | Inversion formula: 0=dot(p,p), 1=abs(p.x*p.y) |
| clampLo | float | 0.0-1.0 | 0.1 | Inversion lower bound |
| clampHi | float | 0.5-5.0 | 3.0 | Inversion upper bound |
| foldConstantX | float | 0.5-2.0 | 1.0 | Post-inversion X translation |
| foldConstantY | float | 0.5-2.0 | 1.0 | Post-inversion Y translation |
| iterRotation | float | -PI..PI | 0.0 | Per-iteration fold rotation (radians) |
| foldOffset | float | 0.0-0.5 | 0.0 | Y-fold asymmetry offset |
| trapOffset | float | 0.0-2.0 | 0.0 | Primary orbit trap Y offset |
| ringRadius | float | 0.0-0.5 | 0.0 | Ring trap radius (0=disabled) |
| junctionBox | float | 0.0-1.0 | 0.0 | Junction shape: 0=point, higher=box |
| traceWidth | float | 0.001-0.05 | 0.005 | Trace glow thickness |
| depthScale | float | 0.0-2.0 | 0.0 | Width scaling by iteration depth |
| junctionHardness | float | 0.0-1.0 | 0.0 | Junction edge: 0=soft exp, 1=hard smoothstep |
| bgLevel | float | 0.0-0.3 | 0.0 | Background level for shape/color compositing |
| panSpeed | float | -2.0-2.0 | 0.3 | Drift speed through fractal space |
| flowSpeed | float | 0.0-2.0 | 0.3 | Data streaming animation speed |
| flowIntensity | float | 0.0-1.0 | 0.5 | Streaming line visibility |
| rotationSpeed | float | -PI..PI | 0.0 | Global pattern rotation (radians/second) |
| aaSamples | int | 1-4 | 1 | Supersampling grid NxN; 1=off |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest frequency band Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest frequency band Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 0.7 | Contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when silent |

**Removed from old effect**: `accentIntensity`, `rotAngle`, `glowIntensity`

### Reference Presets

Specific parameter combinations that recreate each Circuits shader:

**Circuits (Ref 1)**: inversionBlend=0, clampLo=0.1, clampHi=0.5, foldConstantX=1.5, foldConstantY=1.5, iterRotation=0, foldOffset=0, trapOffset=0, ringRadius=0.25, junctionBox=0, traceWidth=0.005, depthScale=1.0, junctionHardness=0, bgLevel=0.15

**Circuits II (Ref 2)**: inversionBlend=1, clampLo=0.15, clampHi=5.0, foldConstantX=1.5, foldConstantY=1.0, iterRotation=0, foldOffset=0, trapOffset=0, ringRadius=0, junctionBox=0, traceWidth=0.033, depthScale=0, junctionHardness=0, bgLevel=0

**Circuits III (Ref 3)**: inversionBlend=1, clampLo=0, clampHi=3.0, foldConstantX=1.0, foldConstantY=1.0, iterRotation=PI/2, foldOffset=0.25, trapOffset=1.5, ringRadius=0, junctionBox=0.5, traceWidth=0.2, depthScale=0, junctionHardness=1, bgLevel=0

## Modulation Candidates

- **inversionBlend**: Morph between fractal topologies — dot-product (rotational) vs product (axis-aligned)
- **zoom**: Pattern scale — in reveals nested detail, out shows tiling
- **clampLo/clampHi**: Inversion aggressiveness — tight = regular, wide = chaotic
- **foldConstantX/Y**: Fractal character — asymmetry creates directional bias
- **iterRotation**: Fold geometry — small angles produce subtle skew, PI/2 produces grid lock
- **traceWidth**: Trace visibility threshold — narrow = sparse, wide = everything glows
- **depthScale**: Depth perception — 0 = flat, higher = deep iterations dominate
- **junctionBox**: Junction geometry — morph from circular blobs to rectangular pads
- **flowIntensity**: Stream visibility — gates flowing data lines
- **gain**: Overall audio reactivity

### Interaction Patterns

- **traceWidth x gain (cascading threshold)**: Narrow traceWidth means only high-energy frequency bands produce visible traces. Quiet = sparse, loud = full density.
- **clampLo x clampHi (competing forces)**: Tight bounds = regular geometry, wide = chaotic tangles. Opposing modulation alternates between order and chaos.
- **inversionBlend x iterRotation (topology shift)**: Dot-product inversion with rotation produces spiraling structures. Product inversion with rotation produces grid-locked circuits. The combination determines the fundamental visual character.
- **depthScale x iterations (depth gating)**: High depthScale makes deep iterations visually dominant. With many iterations, the deepest frequency bands (highest frequencies) produce the widest glows, creating a treble-reactive depth effect.

## Notes

- AA is expensive: each level squares fragment cost. Default off (1); 2-3 for quality.
- `inversionBlend` intermediate values (0.3-0.7) may produce unexpected hybrid topologies — could be interesting or garbage. The extremes (0 and 1) are the tested reference points.
- The ring trap compound logic from Ref 1 only activates when `ringRadius > 0`. When disabled, the orbit trap is simpler and cheaper.
- Ref 1's time-varying fold constant (`1.5 + mod(floor(time), 6) * 0.125`) can be reproduced by modulating `foldConstantX/Y` via LFO.
- Ref 2's perspective transform (`p = vec2(p.x/p.y, 1./p.y)`) intentionally excluded.
- Ref 3's pre-loop skew (`p.x *= 1. + p.y * 0.7`) intentionally excluded — it's a cosmetic warp, not part of the fractal structure. Could be added as a `skew` param later if desired.
