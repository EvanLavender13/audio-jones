# Marble

A glowing glass marble floating in dark space, containing swirling luminous fractal filaments that breathe and morph over time. Raymarched 3D inversive fractal volume inside an analytical bounding sphere, with gradient LUT coloring and FFT-reactive depth layers.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator (same slot as Dream Fractal, Voxel March)

## Attribution

- **Based on**: "Playing marble" by guil (S. Guillitte 2015)
- **Source**: https://www.shadertoy.com/view/MtX3Ws
- **License**: CC BY-NC-SA 3.0

- **Based on**: "Nova Marble" by rwvens (Modified from S. Guillitte 2015)
- **Source**: https://www.shadertoy.com/view/MtdGD8
- **License**: CC BY-NC-SA 3.0

Playing Marble provides the base fractal geometry and raymarch structure. Nova Marble provides the time-animated fold perturbation that makes the internal structure breathe.

## References

- "Playing marble" by guil - Base fractal: inversive fold + csqr + axis permutation, volumetric raymarch with adaptive step size
- "Nova Marble" by rwvens - Animated fold perturbation technique: `cos(time)*amp` offsets in the fractal fold

## Reference Code

### Playing Marble (guil) - Primary reference

```glsl
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Created by S. Guillitte 2015

float zoom=1.;

vec2 cmul( vec2 a, vec2 b )  { return vec2( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x ); }
vec2 csqr( vec2 a )  { return vec2( a.x*a.x - a.y*a.y, 2.*a.x*a.y  ); }


mat2 rot(float a) {
    return mat2(cos(a),sin(a),-sin(a),cos(a));
}

vec2 iSphere( in vec3 ro, in vec3 rd, in vec4 sph )//from iq
{
    vec3 oc = ro - sph.xyz;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - sph.w*sph.w;
    float h = b*b - c;
    if( h<0.0 ) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b-h, -b+h );
}

float map(in vec3 p) {

    float res = 0.;

    vec3 c = p;
    for (int i = 0; i < 10; ++i) {
        p =.7*abs(p)/dot(p,p) -.7;
        p.yz= csqr(p.yz);
        p=p.zxy;
        res += exp(-19. * abs(dot(p,c)));

    }
    return res/2.;
}



vec3 raymarch( in vec3 ro, vec3 rd, vec2 tminmax )
{
    float t = tminmax.x;
    float dt = .02;
    //float dt = .2 - .195*cos(iTime*.05);//animated
    vec3 col= vec3(0.);
    float c = 0.;
    for( int i=0; i<64; i++ )
    {
        t+=dt*exp(-2.*c);
        if(t>tminmax.y)break;

        c = map(ro+t*rd);

        col = .99*col+ .08*vec3(c*c, c, c*c*c);//green
        //col = .99*col+ .08*vec3(c*c*c, c*c, c);//blue
    }
    return col;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime;
    vec2 q = fragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0 * q;
    p.x *= iResolution.x/iResolution.y;
    vec2 m = vec2(0.);
    if( iMouse.z>0.0 )m = iMouse.xy/iResolution.xy*3.14;
    m-=.5;

    // camera

    vec3 ro = zoom*vec3(4.);
    ro.yz*=rot(m.y);
    ro.xz*=rot(m.x+ 0.1*time);
    vec3 ta = vec3( 0.0 , 0.0, 0.0 );
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
    vec3 rd = normalize( p.x*uu + p.y*vv + 4.0*ww );


    vec2 tmm = iSphere( ro, rd, vec4(0.,0.,0.,2.) );

    // raymarch
    vec3 col = raymarch(ro,rd,tmm);
    if (tmm.x<0.)col = texture(iChannel0, rd).rgb;
    else {
        vec3 nor=(ro+tmm.x*rd)/2.;
        nor = reflect(rd, nor);
        float fre = pow(.5+ clamp(dot(nor,rd),0.0,1.0), 3. )*1.3;
        col += texture(iChannel0, nor).rgb * fre;

    }

    // shade

    col =  .5 *(log(1.+col));
    col = clamp(col,0.,1.);
    fragColor = vec4( col, 1.0 );

}
```

### Nova Marble (rwvens) - Animated fold perturbation

Only the `map()` function differs from Playing Marble. This is the source of the breathing animation:

```glsl
float map(in vec3 p) {
    float res = 0.;

    vec3 c = p;
    for (int i = 0; i < 10; ++i) {
        p =.7*abs(p+cos(iTime*0.15+1.6)*0.15)/dot(p,p) -.7+cos(iTime*0.15)*0.15;
        p.yz= csqr(p.yz);
        p=p.zxy;
        res += exp(-19. * abs(dot(p,c)));

    }
    return res/2.;
}
```

## Algorithm

### Substitution Table

Starting from Playing Marble as the base. Apply these substitutions:

| Reference | Keep / Replace | Our version |
|-----------|---------------|-------------|
| `float zoom=1.` | Replace | `uniform float zoom` |
| `cmul()`, `csqr()` | Keep verbatim | Same complex math helpers |
| `rot()` | Keep verbatim | Same 2D rotation matrix |
| `iSphere()` | Keep verbatim | Same ray-sphere intersection |
| `map()` fold line: `p =.7*abs(p)/dot(p,p) -.7` | Replace with Nova's animated version | `p = foldScale*abs(p + cos(perturbPhase + 1.6)*perturbAmp) / dot(p,p) + foldOffset + cos(perturbPhase)*perturbAmp` |
| `map()` iteration count: `i < 10` | Replace | `i < fractalIters` |
| `map()` trap exponent: `-19.` | Replace | `-trapSensitivity` |
| `map()` axis permutation: `p=p.zxy` | Keep verbatim | Same axis cycle |
| `raymarch()` step count: `i<64` | Replace | `i < marchSteps` |
| `raymarch()` base step: `.02` | Replace | `stepSize` uniform |
| `raymarch()` adaptive factor: `exp(-2.*c)` | Keep verbatim | Same adaptive slowdown in dense regions |
| `raymarch()` color: `.99*col+ .08*vec3(c*c, c, c*c*c)` | Replace with gradient LUT + FFT | See color accumulation below |
| `iTime` in camera: `0.1*time` | Replace | `orbitPhase` uniform (CPU-accumulated) |
| `iTime` in Nova's map: `iTime*0.15` | Replace | `perturbPhase` uniform (CPU-accumulated) |
| `zoom*vec3(4.)` | Replace | `zoom*vec3(4.)` with uniform zoom |
| `iMouse` rotation | Remove | Camera orbit is `orbitPhase` only |
| `iSphere(..., 2.)` | Replace | `iSphere(ro, rd, vec4(0.,0.,0., sphereRadius))` |
| `texture(iChannel0, ...)` cubemap | Remove | Dark void -- `col = vec3(0.)` when ray misses sphere |
| Fresnel reflection | Remove | No cubemap, no reflection |
| Tonemapping: `.5*(log(1.+col))` | Replace | `col = clamp(col, 0.0, 1.0)` -- no tonemap per project conventions |

### Color Accumulation (replaces the power-curve line)

At each raymarch step, compute normalized depth `tNorm` from ray parameter:

```
float tNorm = (t - tminmax.x) / (tminmax.y - tminmax.x);
```

Sample gradient LUT and FFT using shared `tNorm`:

```
vec3 lutColor = texture(gradientLUT, vec2(tNorm, 0.5)).rgb;
float freq = baseFreq * pow(maxFreq / baseFreq, tNorm);
float bin = freq / (sampleRate * 0.5);
float energy = 0.0;
if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

Accumulate with density and brightness:

```
col = 0.99 * col + 0.08 * c * lutColor * brightness;
```

The `0.99` decay and `0.08` accumulation rate match the reference. `c` is the fractal density from `map()`. Front-of-sphere samples (low `tNorm`) react to bass; core samples (high `tNorm`) react to treble.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| fractalIters | int | 4-12 | 10 | Fractal detail / iteration count |
| marchSteps | int | 32-128 | 64 | Raymarch sample count (quality vs perf) |
| stepSize | float | 0.005-0.1 | 0.02 | Base ray step size |
| foldScale | float | 0.3-1.2 | 0.7 | Inversive fold strength -- higher = denser filaments |
| foldOffset | float | -1.5-0.0 | -0.7 | Attractor shift -- changes filament arrangement |
| trapSensitivity | float | 5.0-40.0 | 19.0 | Orbit trap decay rate -- higher = sharper filaments |
| perturbAmp | float | 0.0-0.5 | 0.15 | Fold animation strength (0 = static marble) |
| perturbSpeed | float | 0.01-1.0 | 0.15 | Fold animation rate (rad/s) |
| orbitSpeed | float | -2.0-2.0 | 0.1 | Camera orbit rate (rad/s) |
| zoom | float | 0.5-3.0 | 1.0 | Camera distance multiplier |
| sphereRadius | float | 1.0-3.0 | 2.0 | Bounding sphere radius |
| baseFreq | float | 27.5-440 | 55.0 | FFT low frequency bound (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Floor brightness |

## Modulation Candidates

- **perturbAmp**: Drives how much the fractal morphs. Higher values reshape filaments more dramatically.
- **foldScale**: Changes the fundamental fractal geometry. Subtle shifts create large structural changes.
- **trapSensitivity**: Sharp razor filaments vs soft diffuse fog.
- **zoom**: Camera breathes closer/further.
- **foldOffset**: Shifts the attractor center. Combined with foldScale, reorganizes the entire structure.
- **stepSize**: Affects accumulation density -- smaller steps = brighter, denser glow.

### Interaction Patterns

**perturbAmp vs trapSensitivity** (competing forces): High perturbation spreads filaments into new positions as the fractal morphs. High sensitivity tries to keep them razor-sharp. The tension creates flickering firefly-like points during active morphs that settle into clean structures in between. Low sensitivity + high perturbation = smooth flowing fog. When both are modulated by different audio bands, the marble alternates between electric crackling (both peak) and soft nebula (both trough).

**foldScale vs foldOffset** (resonance): These jointly define the attractor geometry. Modulating both by different bands causes the marble's internal structure to reorganize -- one band might stretch the filament web while another shifts its center. Rare alignment (both peaking) briefly resolves the marble into striking crystalline clarity before dissolving back into chaos.

## Notes

- **Performance**: 64 march steps x 10 fractal iterations = 640 map evaluations per pixel. At 1080p this is heavy. The `marchSteps` param lets users trade quality for framerate. Consider half-res rendering (EFFECT_FLAG_HALF_RES).
- **Adaptive step size**: `exp(-2.*c)` slows rays in dense regions. This is critical for quality -- without it, rays skip through filaments and produce noisy results.
- **Phase accumulation**: `orbitPhase` and `perturbPhase` must be accumulated on CPU (`+= speed * deltaTime`) per project conventions. Never accumulate time in the shader.
- **No tonemap**: The reference uses `log(1+col)` tonemapping. Per project conventions, no tonemap in shaders -- just clamp.
- **Sphere miss**: When `iSphere` returns negative (ray misses the sphere), output `vec3(0.)` for dark void background.
