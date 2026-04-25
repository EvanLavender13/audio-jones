# Butterflies

A scrolling field of procedurally unique butterflies tiled in a brick pattern. Each butterfly has Fourier-curve wing edges, voronoi detail texturing, and hash-seeded color variation sampled from the gradient LUT. Wings flap continuously, and patterns periodically mutate (shapeshift). Vertical scroll and horizontal drift animate the field. A static zoom parameter controls how many butterflies are visible.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Output stage, after trail boost, blended via blend compositor
- **Section Index**: 13 (Field)

## Attribution

- **Based on**: "The Butterfly Effect" by Martijn Steinrucken aka BigWings
- **Source**: https://www.shadertoy.com/view/XsVGRV
- **License**: CC BY-NC-SA 3.0

## References

- [The Butterfly Effect](https://www.shadertoy.com/view/XsVGRV) - Complete procedural butterfly field shader (user-pasted, code below)

## Reference Code

```glsl
// "The Butterfly Effect" by Martijn Steinrucken aka BigWings - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode

#define PI 3.141592653589793238
#define TWOPI 6.283185307179586
#define S01(x, offset, frequency) (sin((x+offset)*frequency*TWOPI)*.5+.5)
#define S(x, offset, frequency) sin((x+offset)*frequency*TWOPI)
#define B(x,y,z) S(x, x+fwidth(z), z)*S(y+fwidth(z), y, z)
#define saturate(x) clamp(x,0.,1.)
float dist2(vec2 P0, vec2 P1) { vec2 D=P1-P0; return dot(D,D); }

float SHAPESHIFT=0.;

float hash(vec2 seed) {
    seed *= 213234598766.2345768238784;
    return fract(sin(seed.x)*1234765.876 + cos(seed.y)*8764238764.98787687);
}

float smooth90(float x, float power) {
    // like smoothstep, only 90 degrees rotated
    
    return (1.-x)*pow(x, power) + x * (1.-pow(1.-x, power));
}

vec2 hash2( vec2 p ) { p=vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))); return fract(sin(p)*18.5453); }

vec4 hash4(float seed) {
    vec2 n1 = hash2(vec2(seed, sin(seed*123432.2345)));
    vec2 n2 = hash2(n1);
    
    return vec4(n1, n2);
}

// return distance, and cell id
vec2 voronoi( in vec2 x )
{
    vec2 n = floor( x );
    vec2 f = fract( x );

    vec3 m = vec3( 8.0 );
    for( float j=-1.; j<=1.; j++ )        // iterate cell neighbors
    for( float i=-1.; i<=1.; i++ )
    {
        vec2  g = vec2( i, j );            // vector holding offset to current cell
        vec2  o = hash2( n + g );        // unique random offset per cell
          o.y*=.1;
        vec2  r = g - f + o;            // current pixel pos in local coords
       
        float d = dot( r, r );            // squared dist from center of local coord system
        
        if( d<m.x )                        // if dist is smallest...
            m = vec3( d, o );            // .. save new smallest dist and offset
    }

    return vec2( sqrt(m.x), m.y+m.z );
}

float skewcirc(vec2 uv, vec2 p, float size, float skew, float blur) {
    uv -= p;
    
    uv.x *= 1.+uv.y*skew;
    
    float c = length(uv);
    c = smoothstep(size+blur, size-blur, c);
    return c;
}

float curve(float x, vec4 offs, vec4 amp, vec4 pulse) {
    // returns a fourier-synthesied signal followed by a band-filter
    x *= 3. * pulse.w;
    
    vec4 c = vec4(    S(x, offs.x, 1.),
                      S(x, offs.y, 2.),
                     S(x, offs.z, 4.),
                     S(x, offs.w, 8.));

    float v = dot(c, amp*vec4(1., .5, .25, .125));
    
    pulse.y/=2.;
    
    v *= smoothstep(pulse.x-pulse.y-pulse.z, pulse.x-pulse.y, x);
    v *= smoothstep(pulse.x+pulse.y+pulse.z, pulse.x+pulse.y, x); 
    return v;
}

vec4 Wing(vec2 st, vec2 id, float radius, vec2 center, vec4 misc, vec4 offs, vec4 amp, vec4 pattern1, vec4 global, vec4 detail) {
    // returns a wings shape in the lower right quadrant (.5<st.x<1)
    // we do this by drawing a circle... (white if st.y<radius, black otherwise)
    // ...and scaling the radius based on st.x 
    // when st.x<.5 or st.x>1 radius will be 0, inside of the interval it will be 
    // an upside down parabola with a maximum of 1
    
    vec2 o=vec2(0.);
    
    vec2 colId = hash2(id);
    
    colId.x *= mix( 1., floor(cos(iTime*.125)+.5)+.00015, SHAPESHIFT);
    colId.y *= mix(1., floor(cos(iTime*.25)+.5)+.0001, SHAPESHIFT);
    
    // use upsidedown parabola 1-((x - center)*4)^2
    float b = mix(center.x, center.y, st.x);    // change the center based on the angle to further control the wings shape
    float a = (st.x-b)*4.;            // *4 so curve crosses 0 at .5 and 1.
    a *= a;
    a = 1.-a;                        // flip curve upside down
    float f = max(0., a);            // make curve 0 outside of interval
    
    f = pow(f, mix(.5, 3., misc.x));
    
    o.x = st.x;
    
    float r = 0.;
    float x = st.x*2.;
    
    vec2 vor = voronoi(vec2(st.x, st.y*.1)*40.*detail.z);
    
    r = curve(x-b, offs, amp,vec4(global.x, global.y, max(.1, global.z), .333));

    r = (radius + r*.1)*f;
    
    float edge = 0.01;//max(.001, fwidth(r))*4.;
    
    o.x = smoothstep(r, r-edge, st.y);
    o.y=r;
    
    float t = floor(iTime*2.)*SHAPESHIFT;
    
    
    vec3 edgeCol = hash4(colId.x+t).rgb;
    vec3 mainCol = hash4(colId.y+t).rgb;
    vec3 detailCol = cross(edgeCol, mainCol);
    
    vec3 col = mainCol;
    
    misc = pow(misc, vec4(10.));
    
    r -= misc.y*curve(x-b, amp, offs, vec4(offs.xw, amp.wz));
    
    float edgeBand =  smoothstep(r-edge*3.*misc.w, r, st.y);
    col = mix(col, edgeCol, edgeBand);
    r = st.y-r;
    
    float clockValue = curve(r*.5+.5, pattern1, offs, amp)*global.x;
    
    float distValue = curve(length(st-offs.yx), pattern1.wzyx, amp, global);
    
    col += (clockValue+pow(distValue,3.))*detail.z;
    
    
    float d= distance(st, fract(st*20.*detail.x*detail.x));
    col += st.y*st.y*smoothstep(.1, .0, d)*detail.w*5.*curve(st.x,pattern1, offs, amp);
    
    col *= mix(1., st.y*st.y*(1.-vor.x*vor.x)*15., detail.x*detail.w);
    
    return vec4(col, o.x);
}

vec4 body(vec2 uv, vec4 n) {
    
    float eyes = skewcirc(uv, vec2(.005, .06), .01, 0., 0.001);
    
    uv.x+=.01;
    uv.x *= 3.;
    
    vec2 p = vec2(-.0, 0.);
    float size = .08;
    float skew = 2.1;
    float blur = .005;
    
    float v = skewcirc(uv, p, size, skew, blur);
    
    p.y -= .1;
    uv.x *= mix(.5, 1.5, n.x);
    v += skewcirc(uv, p, size, skew, blur);
    
    vec4 col = n.w*.1+ vec4(.1)* saturate(1.-uv.x*10.)*mix(.1, .6, S01(uv.y, 0., mix(20., 40., n.y)));
    col +=.1;
    col.a = saturate(v);
    
    
    col = mix(col, n*n, eyes);
    
    return col;
}

float BlockWave(float x, float b, float c) {
    // expects 0<x<1
    // returns a block wave where b is the high part and c is the transition width
    
    return smoothstep(b-c, b, x)*smoothstep(1., 1.-c, x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 m = iMouse.xy/iResolution.xy;
    
    float songLength=268.3;
    
    float loopNum = floor(iTime/songLength);
    float t=fract(iTime/songLength)*songLength;
    
    vec2 p = fragCoord.xy/iResolution.xy;
    vec2 uv = p;
    
    SHAPESHIFT = floor(fract(t/40.)+.5);        // turns the overall shapeshifting on or off
    
    p -=.5;     // origin to center
    
     float camT = PI*t/16.;
    float camDive = 1.-(cos(camT)*.5+.5);        // up and down camera motion
    
   
    p =vec2(p.x*1.777, mix(p.y, smooth90(p.y+.5, .7)-.5, camDive));            // aspect ratio fix + a barrel distortion                        

   p*=mix(.85, 5., camDive);
    
    p.y += floor(p.x+.5)*.5+.05;                    // brick pattern = offset every other column
    
    p.y -= 8.*(camT+sin(camT+PI))/TWOPI;        // stop-go camera  motion
    
    vec2 id = floor(p+.5);                        // tile-id
    p=fract(p+.5)-.5;                            // p in tile-space
    
    p.x = abs(p.x);                                // mirror wings

    float shapeShifter = floor(t/8.+.5)*floor(t/2.+.5)*SHAPESHIFT;            // only shapeshift when zoomed in
    shapeShifter += loopNum;
    
    float it = hash2(id+shapeShifter).x*10.+.25; // the only seed that will feed the entire thing
    
    vec4 pattern1 = hash4(it+.345);            // get a whole bunch of random numbers 
    vec4 n1 = hash4(it);
    vec4 n2 = hash4(it+.3);
    vec4 n3 = hash4(n1.x);
    vec4 global = hash4(it*12.);
    vec4 detail = hash4(it*-12.);
    vec4 nBody = hash4(it*.1425);
    
     p.x-=.01*n1.x;                            // distance between wings
    
    vec4 col = vec4(1.);
    vec4 bodyCol = body(p, nBody);
    
    float wingFlap = pow(S01(t+hash2(id.xy).x*20., 10., .05), 60.)*camDive; 
    
     p.x *= mix(1.,20., wingFlap);
    
    vec2 st = vec2(atan(p.x, p.y), length(p));
   st.x /= PI;
   
    
    vec4 top = vec4(0.);
    if(st.x<.6)
        top = Wing(st, id, .5, vec2(.25, .4), n1, n2, n3, pattern1, global, detail);
    vec4 bottom = vec4(0.);
    if(st.x>.4)
        bottom = Wing(st, id, .4, vec2(.5, .75), n2, n3, n1, pattern1, global, detail); 
    
    wingFlap = (1.-wingFlap*.9);
    
    vec4 wings = mix(bottom, top, top.a);
    wings.rgb *= wingFlap;                            // darken wings when they are back-to-back
    
      col = mix(bodyCol*bodyCol.a, wings, wings.a);    // composite wings and body
    
    col *= smoothstep(0., 3., t)*smoothstep(268., 250., t);        // fade in at the start
   
    fragColor = vec4( col );
}
```

## Algorithm

### Keep verbatim
- `hash2()`, `hash4()` hash functions
- `voronoi()` function (cell noise for wing detail texture)
- `skewcirc()` function (body/eye shapes)
- `curve()` function (Fourier synthesis with band-pass filter)
- `Wing()` core logic: parabolic radius envelope, Fourier edge modulation, voronoi detail, edge band coloring, clock/distance pattern overlays
- `body()` function (thorax + eye rendering)
- Wing mirroring via `abs(p.x)`
- Brick pattern tiling: `p.y += floor(p.x + 0.5) * 0.5`
- Polar coordinate conversion: `st = vec2(atan(p.x, p.y), length(p)); st.x /= PI;`
- Top/bottom wing compositing with overlap region (0.4-0.6 blend)
- Hash-seeded per-tile randomness: single `it` seed drives all `hash4()` calls

### Replace

| Original | Replacement | Reason |
|----------|------------|--------|
| `iTime` | `uniform float time` | CPU-accumulated time uniform |
| `iResolution` | `uniform vec2 resolution` | Standard generator uniform |
| `iMouse` | Remove | No mouse interaction |
| Camera dive system (`camDive`, `camT`, `smooth90` barrel distortion, stop-go scroll) | `uniform float zoom` | Static configurable zoom replaces animated dive |
| `p *= mix(.85, 5., camDive)` | `p *= zoom` | Zoom uniform directly scales tile space |
| `SHAPESHIFT` toggle + `songLength` timing | Always 1.0; `shiftSpeed` uniform controls `floor(time * shiftSpeed)` | Shapeshifting always active, user controls rate |
| `wingFlap = ... * camDive` | `pow(S01(time + hash2(id) * 20.0, 10.0, flapSpeed), 60.0)` | Continuous flap decoupled from camera, `flapSpeed` uniform |
| `p.y -= 8.*(camT+sin(camT+PI))/TWOPI` (stop-go scroll) | `p.y -= scrollPhase` | CPU-accumulated scroll from `scrollSpeed` |
| No horizontal drift | `p.x += driftPhase` | CPU-accumulated drift from `driftSpeed` |
| `edgeCol = hash4(colId.x+t).rgb` / `mainCol = hash4(colId.y+t).rgb` | Sample `gradientLUT` at hash-derived positions with per-butterfly offset | Gradient LUT + hash variation for user-controlled palette |
| Aspect ratio fix `p.x*1.777` | `p.x *= resolution.x / resolution.y` | Actual aspect ratio |
| Fade in/out `smoothstep(0., 3., t)*smoothstep(268., 250., t)` | Remove | No song-timed fades |

### Add
- `uniform sampler2D gradientLUT` for palette coloring
- `uniform sampler2D fftTexture` + standard FFT audio uniforms for audio reactivity
- Per-butterfly FFT brightness: tile ID hash produces index `t`, same `t` samples gradient and FFT frequency band. Each butterfly glows independently based on its frequency.
- `uniform float patternDetail` to scale voronoi density (multiplier on `40.0 * detail.z`)
- `uniform float wingShape` to bias the curvature exponent (replaces `mix(.5, 3., misc.x)`)

### Coloring adaptation
The original picks `edgeCol` and `mainCol` from `hash4()` giving fully random RGB per butterfly. The adaptation:
1. Each butterfly's tile ID hash produces a base gradient position `t_base = hash2(id).x`
2. `mainCol = texture(gradientLUT, vec2(t_base, 0.5)).rgb`
3. `edgeCol = texture(gradientLUT, vec2(fract(t_base + 0.3), 0.5)).rgb` (offset along gradient for contrast)
4. Shapeshifting varies `t_base` over time: `t_base = hash2(id + floor(time * shiftSpeed)).x`
5. The same `t_base` drives FFT lookup so color and audio reactivity are linked per butterfly

### FFT audio integration
Each butterfly reacts to its own frequency band:
```
float t_base = hash2(id + floor(time * shiftSpeed)).x;
float freq = baseFreq * pow(maxFreq / baseFreq, t_base);
float bin = freq / (sampleRate * 0.5);
float energy = 0.0;
if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```
Apply `brightness` as a multiplier on final butterfly color before output.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoom | float | 0.5-5.0 | 1.5 | Field scale - low shows many small butterflies, high shows few large ones |
| scrollSpeed | float | -2.0-2.0 | 0.3 | Vertical scroll rate through the field |
| driftSpeed | float | -1.0-1.0 | 0.0 | Horizontal drift rate |
| flapSpeed | float | 0.0-1.0 | 0.05 | Wing flap frequency |
| shiftSpeed | float | 0.1-4.0 | 0.5 | How quickly butterfly patterns mutate |
| patternDetail | float | 0.1-3.0 | 1.0 | Voronoi detail density on wings |
| wingShape | float | 0.2-3.0 | 1.0 | Wing silhouette curvature - low is broad/round, high is narrow/pointed |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency mapped to butterflies |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT frequency mapped to butterflies |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness without audio |

## Modulation Candidates

- **zoom**: Breathing field density, zooming in reveals individual wing detail
- **scrollSpeed**: Varying traverse speed through the field
- **driftSpeed**: Adding lateral sway to the scroll direction
- **flapSpeed**: Synchronized or chaotic wing motion across the field
- **shiftSpeed**: Rate of pattern mutation - fast creates kaleidoscopic chaos
- **patternDetail**: Voronoi density shifting from smooth to granular wings
- **wingShape**: Wing silhouette morphing between broad and narrow

### Interaction Patterns

**zoom x scrollSpeed (competing forces):** High zoom makes individual butterflies fill the frame while fast scroll whips them past - the two fight for visual dominance. Modulating scroll with bass energy while zoom follows a slow LFO creates verse/chorus dynamics where quiet sections hold still on one butterfly and loud sections rush through the field.

**flapSpeed x shiftSpeed (resonance):** Both control temporal chaos on different axes - flapping is spatial (wing position) while shifting is identity (pattern seed). When both spike simultaneously, butterflies become frantic and unrecognizable. When both are low, the field is serene. Mapping these to different frequency bands creates moments of rare full-chaos alignment on broadband hits.

## Notes

- The `curve()` function is the heart of the visual variety - it's a 4-harmonic Fourier synthesizer with band-pass filtering. All wing edge shapes, internal patterns, and detail overlays derive from it with different hash-seeded parameters.
- The voronoi call in `Wing()` is the most expensive per-pixel operation (nested loops). `patternDetail` scaling the density means lower values are cheaper.
- `smooth90()` and `BlockWave()` from the original are only used for camera motion. They can be omitted since the camera dive is removed.
- The `B()` and `dist2()` macros in the original are unused dead code.
- Wing flap works by scaling `p.x` by up to 20x, which collapses the wing to a thin line in polar space. This is cheap and effective.
