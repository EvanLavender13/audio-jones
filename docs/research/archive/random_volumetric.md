# Random Volumetric

A stochastic generative art engine where every seed produces a unique visual universe. A volumetric camera flies through a raymarched tube while a random expression tree chains mathematical basis functions (polynomials, trig, noise, volumetric hit coordinates) through randomly-selected operations (sin, sqrt, add, divide, multiply, noise-sample) for 4-40 iterations. Each seed also selects from 9 color palette strategies (HSL variants, hex-float, channel swizzle, double-HSL). Feedback trails with directional drift create smoky persistence. The result is a slot machine of mathematical art - every seed is a different creature.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (section 13)
- **Compute Model**: Texture ping-pong (feedback trails)

## Attribution

- **Based on**: "Random Volumetric V2" by Cotterzz
- **Source**: https://www.shadertoy.com/view/3XGXRW
- **License**: CC BY-NC-SA 3.0

## References

- "Random Volumetric V2" (Shadertoy) - Complete reference implementation with volumetric raymarching, stochastic expression tree, 9 palette modes, and feedback trail system

## Reference Code

### Common

```glsl
float hash1(vec2 x)
{
    uvec2 t = floatBitsToUint(x);
    uint h = 0xc2b2ae3du * t.x + 0x165667b9u;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h += 0xc2b2ae3du * t.y;
    h = (h << 17u | h >> 15u) * 0x27d4eb2fu;
    h ^= h >> 15u;
    h *= 0x85ebca77u;
    h ^= h >> 13u;
    h *= 0xc2b2ae3du;
    h ^= h >> 16u;
    return uintBitsToFloat(h >> 9u | 0x3f800000u) - 1.0;
}
vec3 contrast(vec3 color, float value) {
  return 0.5 + value * (color - 0.5);
}
vec2 hash2(vec2 x) // improved hash using xxhash
{
    float k = 6.283185307 * hash1(x);
    return vec2(cos(k), sin(k));
}

float noise2( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

    vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
    vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
    vec3  n = h*h*h*vec3( dot(a,hash2(i+0.0)), dot(b,hash2(i+o)), dot(c,hash2(i+1.0)));
    return dot( n, vec3(32.99) );
}

float pnoise (vec2 uv){
    float f = 0.0;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise2( uv ); uv = m*uv;
    f += 0.2500*noise2( uv ); uv = m*uv;
    f += 0.1250*noise2( uv ); uv = m*uv;
    f += 0.0625*noise2( uv ); uv = m*uv;
    f = 0.5 + 0.5*f;
    return f;
}
```

### Buffer A (main computation)

```glsl
#define NEWVALUE values[int(floor(float(v)*rand(seed+float(i))))] * (sin(iTime*rand(seed+float(i)))*rand(seed+float(i)))
#define NEWVALUE2 values[int(floor(float(v)*rand(seed+float(i+5))))] * (sin(iTime*rand(seed+float(i)))*rand(seed+float(i+5)))
#define T (iTime*4.)
#define rot(a) mat2(cos(a+vec4(0,33,11,0)))
#define P(z) (vec3(tanh(cos((z) * .15) * 1.) * 8., \
                   tanh(cos((z) * .12) * 1.) * 8., (z)))

#define PREVIEW 0
#define SHOWITER 0
#define VOLUMETRIC 1

float displaytime = 2.5;

int PALETTE = 9;

vec3 p = vec3(0);

float rand(float n){
     return fract(cos(n*89.42)*343.42);
}
float map(vec3 p) {
    return 1.5 - length(p - P(p.z));
}
float rMix(float a, float b, float s){
    s = rand(s);
    return s>0.9?sin(a):s>0.8?sqrt(abs(a)):s>0.7?a+b:s>0.6?a-b:s>0.5?b-a:s>0.4?b/(a==0.?0.01:a):s>0.3?pnoise(vec2(a,b)):s>0.2?a/(b==0.?0.01:b):s>0.1?a*b:cos(a);
}
vec3 hsl2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}


vec3 gmc(vec3 colour, float gamma) {
  return pow(colour, vec3(1. / gamma));
}
vec3 fhexRGB(float fh){
    if(isinf(fh)||fh>100000.){fh = 0.;}
    fh = abs(fh*10000000.);
    float r = fract(fh/65536.);
    float g = fract(fh/256.);
    float b= fract(fh/16777216.);
    return hsl2rgb(vec3(r,g,b));
}

vec3 addColor(float num, float seed, float alt){
    if(isinf(num)){num = alt * seed;}
    if(PALETTE == 7){
        vec3 col = fhexRGB(num);
        return col;} else if(PALETTE > 2 || (PALETTE == 1 && rand(seed+19.)>0.3)){
    
        float sat = 1.;
        if(num<0.){sat = 1.-(1./(abs(num)+1.));}
        float light = 1.0-(1./(abs(num)+1.));

        vec3 col = hsl2rgb(vec3(fract(abs(num)), sat, light));
        if(PALETTE == 1){col *= 2.;}
       
        return col;
       
    } else {

        vec3 col = vec3(fract(abs(num)), 1./num, 1.-fract(abs(num)));
        if(rand(seed*2.)>0.5){col = col.gbr;}
        if(rand(seed*3.)>0.5){col = col.gbr;}
        if(PALETTE == 1){col += (1.+cos(rand(num)+vec3(4,2,1))) / 2.;}
        return col;
    
    }
}

vec3 sanitize(vec3 dc){
    dc.r = min(1., abs(dc.r));
    dc.g = min(1., abs(dc.g));
    dc.b = min(1., abs(dc.b));
    
    if(!(dc.r>=0.) && !(dc.r<0.)){
        return vec3(1,0,0);
    } else if(!(dc.g>=0.) && !(dc.g<0.)){
        return vec3(1,0,0);
    } else if(!(dc.b>=0.) && !(dc.b<0.)){
        return vec3(1,0,0);
    } else {
        return dc;
    }

}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

     vec3  r = iResolution;
    vec2 u = fragCoord;
    
        
        float s=.002,d,i, l;

        u = (u-r.xy/2.)/r.y;
    
        vec3  p = P(T),ro=p,q,
              Z = normalize( P(T+1.)  - p),
              X = normalize(vec3(Z.z,0,-Z)),
              D = vec3(rot(tanh(sin(p.z*.03)*8.)*3.)*u, 1)* mat3(-X, cross(X, Z), Z);
       
        for(; i++<80. && s > .001; ) {
    
            p = ro + D * d,
            s = map(p)*.8,
            d += s;
        }
        p = ro + D * d;
        
    vec2 uv = fragCoord/iResolution.y;
    vec2 uvd = fragCoord/iResolution.xy;
    float ar = iResolution.x/iResolution.y;
    uv.x-=0.5*iResolution.x/iResolution.y;
    uv.y-=0.5;
    float zoom = 4.;
    if(VOLUMETRIC == 1 ){ zoom+= (1.5*(sin(iTime)+1.)); } else { zoom+= (3.*(sin(iTime)+1.));}
    vec2 guv = (uv*zoom);
    float x = guv.x;
    float y = guv.y;
    
    float ix = 468.;
    float iy = 330.;
    if(iMouse.x>0.&&iMouse.y>0.){
    ix = iMouse.x; iy = iMouse.y;}
    
    float thumb = 5.+sin((iTime/2.)-1.62)*2.;
    
    if(!(iMouse.z>0.)&& bool(PREVIEW)){
        float pantime = (iTime)+50.;

        ix = floor(pantime+(uv.x*thumb/(iResolution.x/iResolution.y)));
        iy = floor((pantime+(uv.y*thumb)));

        uv.y*=thumb;
        uv.x*=thumb/(iResolution.x/iResolution.y);
        uv+=pantime;
        uv=fract(uv);
        uv.x-=0.5;
        uv.y-=0.5;
        guv = (uv*zoom);
        x = guv.x;
        y = guv.y;
    }
    
    float seed = (ix + (iy*iResolution.x))/iResolution.x;
    
    if(!(iMouse.z>0.) && iMouse.x==0. && iMouse.y==0.){
        seed = floor(iTime+50./displaytime);
    }
    
    if(PALETTE == 9){
        PALETTE = int(floor(float(8)*rand(seed+66.)));
    }

    if(PALETTE == 8){
        PALETTE = int(floor(fragCoord.x/iResolution.x * 8.));
    }
    
    const int v = 24;
    
    vec3 col = vec3(0);
    float cn = 1.;
    
    float values[v];
    
    values[0] = 1.0;
    values[1] = p.x;
    values[2] = x;
    values[3] = y;
    values[4] = x*x;
    values[5] = y*y;
    values[6] = x*x*x;
    values[7] = y*y*y;
    values[8] = x*x*x*x;
    values[9] = y*y*y*y;
   values[10] = x*y*x;
   values[11] = y*y*x;
   values[12] = sin(y);
   values[13] = cos(y);    
   values[14] = sin(x);
   values[15] = cos(x);   
   values[16] = sin(y)*sin(y);
   values[17] = cos(y)*cos(y);
   values[16] = sin(x)*sin(x);
   values[17] = cos(x)*cos(x);
   values[18] = p.y;
   values[19] = distance(vec2(x,y), vec2(0));
   values[20] = p.z;
   values[21] = atan(x, y)*4.;
   values[22] = pnoise(vec2(x,y)/2.);
   values[23] = pnoise(vec2(y,x)*10.);
   
    float total = 0.;
    float sub = 0.;
    int maxi = 40; int mini = 4;
    int iterations = min(maxi,mini + int(floor(rand(seed*6.6)*float(maxi-mini))));
    
    for(int i = 0; i<iterations; i++){
        if(rand(seed+float(i+3))>rand(seed)){
            sub = sub==0. ? rMix(NEWVALUE, NEWVALUE2, seed+float(i+4)) : rMix(sub, rMix(NEWVALUE, NEWVALUE2, seed+float(i+4)), seed+float(i));

        } else {
            sub = sub==0. ? NEWVALUE : rMix(sub, NEWVALUE, seed+float(i));
      
        }
        if(rand(seed+float(i))>rand(seed)/2.){
            total = total==0. ? sub : rMix(total, sub,seed+float(i*2));
            sub = 0.;
            if(rand(seed+float(i+30))>rand(seed)){
                col += addColor(total, seed+float(i), values[21]);
                cn+=1.;
            }
        }
    }
    total = sub==0. ? total : rMix(total, sub, seed);
    col += addColor(total, seed, values[21]);
    col /=cn;
    if(PALETTE<3){col/=(3.* (0.5 + rand(seed+13.)));}
    if(PALETTE == 4){col = pow(col, 1./col)*1.5;}
    if(PALETTE == 2 || PALETTE == 5 ){col = hsl2rgb(col);}
    
    if(PALETTE == 6){
        col = hsl2rgb(hsl2rgb(col));
        if(rand(seed+17.)>0.5){col = col.gbr;}
        if(rand(seed+19.)>0.5){col = col.gbr;}
    }

    col = sanitize(col);
    if(VOLUMETRIC == 1 ) { 
    uv+=vec2(0.25,0.15);
    float xspeed = ar*4.*(iMouse.x/iResolution.x);
    float yspeed = 4.*(iMouse.y/iResolution.y);
    vec3 old = textureLod(iChannel0, (fragCoord+vec2(-xspeed,-yspeed))/iResolution.xy, 0.).rgb;
    float alph = (col.r+col.g+col.b)/5.;
    fragColor = vec4(mix(old, col, alph), 1.);
    } else {
    fragColor = vec4(col, 1.);
    }

}
```

### Image (final output)

```glsl
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture(iChannel0, fragCoord/iResolution.xy, 0.);
    fragColor.rgb = contrast(fragColor.rgb, 1.5);
}
```

## Algorithm

### Keep verbatim
- `hash1()` xxhash function
- `hash2()` directional hash
- `noise2()` simplex noise
- `pnoise()` 4-octave FBM noise
- `rand()` simple cosine hash
- `rMix()` random operation selector (10 branches: sin, sqrt, add, sub, reverse-sub, div, noise, reverse-div, mul, cos)
- `hsl2rgb()` HSL to RGB conversion
- `fhexRGB()` hex-float to RGB extraction
- `addColor()` palette dispatch with PALETTE branching
- `sanitize()` NaN/Inf guard
- `rot(a)` rotation macro
- Basis values array structure (24 original slots)
- Expression tree loop logic (sub-expression accumulation, color contribution)
- All palette post-processing (PALETTE < 3 darkening, PALETTE 4 pow, PALETTE 2/5 double HSL, PALETTE 6 swizzle)

### Replace

| Original | Replacement | Reason |
|----------|-------------|--------|
| `iTime` | `time` uniform | CPU-accumulated phase |
| `iResolution` | `resolution` uniform | Standard naming |
| `iMouse` | Remove | Not applicable |
| `iChannel0` | `prevFrame` sampler2D | Ping-pong texture input |
| `fragCoord` | `gl_FragCoord.xy` | GLSL 330 built-in |
| `PREVIEW` block | Remove entirely | Shadertoy gallery feature |
| `SHOWITER` block | Remove entirely | Debug feature |
| `VOLUMETRIC` toggle | Remove (always on) | User decision |
| `T = iTime*4.` | `time * tubeSpeed` | Parameterized camera speed |
| `1.5` in `map()` | `tubeRadius` uniform | Parameterized tube width |
| `8.` in `P(z)` | `tubeAmplitude` uniform | Parameterized path swing |
| `3.` roll multiplier | `rollAmount` uniform | Parameterized camera roll |
| `4.` base zoom | `zoom` uniform | Parameterized magnification |
| `1.5` zoom pulse amp | `zoomPulse` uniform | Parameterized breathing |
| `mini=4, maxi=40` | `iterMin`, `iterMax` uniforms | Parameterized complexity |
| `displaytime = 2.5` | `cycleSpeed` uniform (= 1/displaytime) | Seeds per second |
| Seed from mouse/time | `seed + floor(time * cycleSpeed)` | Base seed + auto-advance |
| `PALETTE = 9` hardcode | `PALETTE = int(floor(8. * rand(effectiveSeed + 66.)))` always randomized | Stochastic palette is always active internally |
| `PALETTE == 8` banded mode | Remove | Screen-position palette is not useful here |
| Alpha divisor `5.` | `(col.r+col.g+col.b) / trailOpacity` | Adjustable via `decay` interaction |
| Mouse-driven drift | `driftX`, `driftY` uniforms | Modulatable trail direction |
| Feedback blend | `old * decay` before mixing | Explicit decay control |
| Contrast in Image pass | Apply `contrast()` in main shader before output | Single-pass design |
| `v = 24` array size | `v = 28` | Expanded with 4 FFT band values |
| `NEWVALUE`/`NEWVALUE2` `v` reference | Update to `28` | Matches expanded array |

### Add

| Addition | Purpose |
|----------|---------|
| `gradientLUT` sampler2D | Gradient LUT color source (when paletteRandomness < 1) |
| `fftTexture` sampler2D | FFT spectral data |
| `paletteRandomness` uniform | Blend 0=gradient LUT, 1=stochastic palette |
| `driftX`, `driftY` uniforms | Feedback trail drift direction |
| `decay` uniform | Feedback persistence multiplier |
| `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` uniforms | Standard FFT audio params |
| `sampleRate` uniform | FFT frequency calculation |
| values[24-27]: 4 FFT band energies | Audio-reactive basis values |

### FFT basis value injection

Expand the values array from 24 to 28. The 4 new slots sample FFT energy at logarithmically-spaced bands from `baseFreq` to `maxFreq`:

```
for (int b = 0; b < 4; b++) {
    float t = float(b) / 3.0;
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = 0.0;
    if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
    values[24 + b] = baseBright + pow(clamp(energy * gain, 0.0, 1.0), curve);
}
```

The stochastic expression tree randomly selects from all 28 basis values. Different seeds incorporate audio data differently (or not at all). This makes each seed's audio reactivity unique - some patterns pulse with bass, others shimmer with treble, others ignore audio entirely.

### Gradient LUT blending

When `paletteRandomness < 1.0`, blend the stochastic palette result with gradient LUT sampling:

```
vec3 stochasticCol = <result from addColor/palette system>;
float lutT = fract(abs(total));
vec3 lutCol = texture(gradientLUT, vec2(lutT, 0.5)).rgb;
col = mix(lutCol, stochasticCol, paletteRandomness);
```

### Seed computation

```
float effectiveSeed = seed + floor(time * cycleSpeed);
```

`seed` is the base (user-set, modulatable). `cycleSpeed` is seeds per second (default 0.4 = new pattern every 2.5 seconds). When `cycleSpeed = 0`, the pattern is static at the base seed.

### Note on values[16-17] bug

The original code writes `values[16] = sin(y)*sin(y)` and `values[17] = cos(y)*cos(y)`, then immediately overwrites them with `sin(x)*sin(x)` and `cos(x)*cos(x)`. This is preserved verbatim - the effective basis does not include sin^2(y) or cos^2(y).

## Parameters

### Camera

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| tubeSpeed | float | 0.5-10.0 | 4.0 | Camera traversal speed along tube path |
| tubeRadius | float | 0.5-5.0 | 1.5 | Tube cross-section radius |
| tubeAmplitude | float | 1.0-20.0 | 8.0 | Lateral swing amplitude of camera path |
| rollAmount | float | 0.0-6.0 | 3.0 | Camera barrel roll intensity |
| zoom | float | 1.0-20.0 | 4.0 | Pattern magnification |
| zoomPulse | float | 0.0-5.0 | 1.5 | Amplitude of automatic zoom breathing |

### Pattern

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| seed | float | 0.0-1000.0 | 0.0 | Base seed for expression tree and palette selection |
| cycleSpeed | float | 0.0-10.0 | 0.4 | Seeds per second auto-advance rate (0=static) |
| iterMin | int(float) | 1-20 | 4 | Minimum expression tree iterations |
| iterMax | int(float) | 5-80 | 40 | Maximum expression tree iterations |

### Color

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| paletteRandomness | float | 0.0-1.0 | 1.0 | 0=gradient LUT only, 1=stochastic palette only |
| contrast | float | 0.5-3.0 | 1.5 | Output contrast enhancement |

### Feedback

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| driftX | float | -5.0-5.0 | 0.0 | Horizontal trail drift direction |
| driftY | float | -5.0-5.0 | 0.0 | Vertical trail drift direction |
| decay | float | 0.8-1.0 | 0.95 | Trail persistence (0.8=fast fade, 1.0=infinite) |

### Audio

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT band frequency |
| maxFreq | float | 1000.0-16000.0 | 14000.0 | Highest FFT band frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT energy amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **seed**: Jumping between mathematical universes. Discrete steps create dramatic visual shifts.
- **cycleSpeed**: Universe change rate. Slow = contemplative dwelling, fast = frenetic slot machine.
- **zoom**: Pattern scale. Low = abstract field, high = microscopic detail.
- **zoomPulse**: Breathing intensity. Creates pulsing depth effect.
- **tubeSpeed**: Camera velocity. Speed changes create acceleration/deceleration.
- **rollAmount**: Camera rotation intensity. Creates spinning disorientation when high.
- **paletteRandomness**: Color character shift between ordered (gradient) and chaotic (stochastic).
- **contrast**: Visual punch. Low = washed out, high = stark.
- **driftX/driftY**: Trail direction. Creates swirling trail motion when modulated.
- **decay**: Trail length. Low = crisp edges, high = smoky history.

### Interaction Patterns

**seed x cycleSpeed (cascading threshold)**: `cycleSpeed` determines whether the universe changes at all. When `cycleSpeed` is near zero (e.g., modulated by a quiet audio passage), the current pattern holds steady. When energy spikes boost `cycleSpeed`, the visual rapid-fires through universes. Verses hold one pattern; choruses become a slot machine.

**decay x drift (competing forces)**: High `decay` preserves old patterns, but `driftX`/`driftY` smear them directionally. These compete: high decay wants to keep detail, but strong drift destroys it through motion blur. Modulating them from different audio sources creates tension - bass drives drift while treble drives decay, making bass-heavy moments smoky/smeared and treble-bright moments crisp/detailed.

**zoom x iterMax (resonance)**: More iterations produce finer mathematical structure. Higher zoom magnifies that structure. When both peak together (rare coincidence from independent modulation sources), the visual suddenly reveals intricate micro-detail invisible at any other moment.

## Notes

- **Performance**: This is a heavy generator. 80-step raymarching + up to 40 expression tree iterations (each with potential `pnoise` calls = 4 octaves of simplex noise) + 4 additional FFT noise samples. Expect significant GPU cost. The `iterMax` param is the primary performance knob.
- **NaN/Inf safety**: The `sanitize()` function and `isinf()` checks in `addColor` are critical. The `rMix` function includes division operations that can produce Inf, and chained operations on Inf produce NaN. Keep all guards verbatim.
- **Feedback stability**: With `decay = 1.0` and no drift, old content persists indefinitely. Combined with the alpha blend (brighter = more opaque), this can converge to a saturated mess over time. Default decay of 0.95 provides slow fade.
- **GLSL 330 compatibility**: All features used (bitwise ops, `floatBitsToUint`/`uintBitsToFloat`, `tanh`, `isinf`, `textureLod`) are available in GLSL 330.
