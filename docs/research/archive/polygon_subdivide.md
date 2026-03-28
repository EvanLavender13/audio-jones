# Polygon Subdivide

Recursive convex polygon subdivision via arbitrary-angle line cuts. Starts with a screen-filling quad and repeatedly slices polygons with lines through their bounding-box center, producing irregular stained-glass / geological fault mosaics. Each cell gets a unique hash ID for gradient LUT coloring and per-cell FFT brightness. Sibling to the existing Subdivide generator (same author, shared utility functions), but produces arbitrary convex N-gons instead of axis-aligned quads.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator layer (section 10)

## Attribution

- **Based on**: "Convex Polygon Subdivision" by SnoopethDuckDuck
- **Source**: https://www.shadertoy.com/view/NfBGzd
- **License**: CC BY-NC-SA 3.0

## References

- [Convex Polygon Subdivision](https://www.shadertoy.com/view/NfBGzd) - Primary reference, full algorithm
- [Always Watching](https://www.shadertoy.com/view/ffB3D1) - Existing Subdivide generator (same author), visual treatment reference
- [2D distance functions](https://iquilezles.org/articles/distfunctions2d/) - sdPoly SDF used in reference

## Reference Code

```glsl
#define C(a,b) ((a).x*(b).y-(a).y*(b).x)
#define S(a) smoothstep(2./R.y, -2./R.y, a)

const int maxN = 15;

vec2 center(vec2[maxN] P, int N)
{
    vec2 bl = vec2(1e5), tr = vec2(-1e5);
    for (int j = 0; j < N; j++)
    {
        bl = min(bl, P[j]);
        tr = max(tr, P[j]);
    }
    return (bl+tr)/2.;
}

// https://www.shadertoy.com/view/4djSRW
float h11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec2 h21(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

// https://www.shadertoy.com/view/MsXGDj
float catrom1(float t)
{
    float f = floor(t),
          x = t - f;
    float v0 = h11(f), v1 = h11(f+1.), v2 = h11(f+2.), v3 = h11(f+3.);
    float c2 = -.5 * v0    + 0.5*v2;
    float c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    float c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

vec2 catrom2(float t)
{
    float f = floor(t),
          x = t - f;
    vec2 v0 = h21(f), v1 = h21(f+1.), v2 = h21(f+2.), v3 = h21(f+3.);
    vec2 c2 = -.5 * v0    + 0.5*v2;
    vec2 c3 = v0        - 2.5*v1 + 2.0*v2 - 0.5*v3;
    vec2 c4 = -.5 * v0    + 1.5*v1 - 1.5*v2 + 0.5*v3;
    return(((c4 * x + c3) * x + c2) * x + v1);
}

// https://iquilezles.org/articles/distfunctions2d/
float sdPoly( in vec2[maxN] v, int N, in vec2 p )
{
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for( int i=0, j=N-1; i<N; j=i, i++ )
    {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if( all(c) || all(not(c)) ) s*=-1.0;
    }
    return s*sqrt(d);
}


void mainImage( out vec4 o, vec2 u )
{
    vec2 R = iResolution.xy;
    u = (u+u-R)/R.y;

    float t = iTime/6.;
    float c = tanh(iTime);

    vec2 e;
    vec2[maxN] P = vec2[]( vec2(-c, 1)*R/R.y + vec2( .12,-.12),
                           vec2( c, 1)*R/R.y + vec2(-.12,-.12),
                           vec2( c,-1)*R/R.y + vec2(-.12, .12),
                           vec2(-c,-1)*R/R.y + vec2( .12, .12),
                           e,e,e,e,e,e,e,e,e,e,e);
    int N = 4;
    float id;

    for (int i = 0; i < 8; i++)
    {
        // Center of bounding box (incircle may be better but costly)
        vec2 c = center(P, N);

        // Two points for subdividing line
        // (prefer p0 inside polygon, p1 outside polygon)
        vec2 cr = catrom2(t);
        float d = sdPoly(P, N, c);
        vec2 p0 = c + (cr-.5)*d;

        float a = 2.*6.28*catrom1(t/5.);
        vec2 p1 = p0 + 2.*vec2(cos(a),sin(a));

        // Line-polygon intersections (intersect at a + s*(b-a))
        int i0; vec2 q0;
        for (int j = 0; j < N; j++) {
            vec2 a = P[j], b = P[j+1];
            float s = C(p1-p0,p0-a) / C(p1-p0,b-a);
            if (s >= 0. && s < 1.) { i0 = j; q0 = mix(a,b,s); break; }
        }

        int i1; vec2 q1;
        for (int j = i0 + 1; j < N; j++) {
            vec2 a = P[j], b = P[(j+1)%N];
            float s = C(p1-p0,p0-a) / C(p1-p0,b-a);
            if (s >= 0. && s < 1.) { i1 = j; q1 = mix(a,b,s); break; }
        }

        // Ensure p1 is closer to q0 than q1
        // (otherwise q0,q1 swap places as p1 rotates around p0)
        int ti; vec2 tq;
        if (dot(p1-q0,p1-q0) > dot(p1-q1,p1-q1))
            ti = i0, i0 = i1, i1 = ti,
            tq = q0, q0 = q1, q1 = tq;

        // Find new polygon
        vec2[maxN] Q;
        if (C(u-p0,p1-p0) > 0.)
        {
            int M = i0 < i1 ? (i1-i0)+2 : N-(i0-i1)+2;
            Q[0] = q0;
            for (int j = 1; j < M-1; j++) Q[j] = P[(i0+j)%N];
            Q[M-1] = q1;
            N = M;
            id += 1./float(i+1);
        }
        else
        {
            int M = i1 < i0 ? (i0-i1)+2 : N-(i1-i0)+2;
            Q[0] = q1;
            for (int j = 1; j < M-1; j++) Q[j] = P[(i1+j)%N];
            Q[M-1] = q0;
            N = M;
            id -= 1./float(i+1);
        }
        P = Q;

        t += 1e2*(h11(id)-.5);

        float tx = texture(iChannel0, u/9.).x;
        if (h11(id + floor(t/1.5 - tx/8.)) < .1) break;
    }

    float d1 = sdPoly(P,N,u);
    float d2 = sdPoly(P,N,u+.08);

    float h = h11(id);
    vec4 col = h < .25 ? vec4( 70, 83, 98,0)
             : h < .50 ? vec4(130,163,162,0)
             : h < .75 ? vec4(159,196,144,0)
             :           vec4(192,223,161,0);
    col /= 255.;

    o = vec4(1,25,54,0)/255.;
    o = mix(o, mix(col,vec4(1),h11(id+1e2)), S(abs(d1+.015) - .0075));
    o = mix(o, col, S(d1+.03));
    o *= .95 + .1 * S(d2);
    o = mix(vec4(.6,.83,1,0), o,
        pow(tanh(5.*(u.x-c*R.x/R.y+.12)*(u.x+c*R.x/R.y-.12)
                   *(u.y-1.+.12)*(u.y+1.-.12)),.03));
}
```

## Algorithm

### Keep verbatim
- `C(a,b)` cross product macro
- `S(a)` smoothstep AA macro (reference `R` from `resolution` uniform)
- `h11()` 1D hash
- `h21()` 1D-to-2D hash
- `catrom1()` 1D Catmull-Rom interpolation
- `catrom2()` 2D Catmull-Rom interpolation
- `center()` bounding box center
- `sdPoly()` generalized N-gon signed distance
- Core subdivision loop: center computation, p0/p1 line construction, line-polygon intersection, polygon split, ID accumulation

### Replace

| Reference | Ours | Reason |
|-----------|------|--------|
| `iResolution.xy` | `resolution` uniform | Standard convention |
| `iTime/6.` | `time` uniform (CPU-accumulated with `speed`) | Speed accumulation on CPU per conventions |
| `tanh(iTime)` intro animation | Remove - initial quad covers full screen | No intro animation needed |
| Initial quad `+ vec2(.12,-.12)` margins | Remove margins, use `vec2(-1,1)*R/R.y` directly | Full screen coverage |
| `for (int i = 0; i < 8; i++)` | `for (int i = 0; i < maxIterations; i++)` | Expose iteration cap |
| `texture(iChannel0, u/9.).x` noise | `h12(u * 0.11)` procedural hash | No noise texture needed; stable spatial variation |
| `h11(id + floor(t/1.5 - tx/8.)) < .1` | `h11(id + floor(t/1.5 - tx/8.)) < threshold` | Expose exit probability |
| Fixed 4-color palette | `texture(gradientLUT, vec2(h, 0.5)).rgb * brightness` | Gradient LUT coloring |
| Background `vec4(1,25,54,0)/255.` | `vec3(0.0)` black | Blend compositing background |
| Bottom vignette `mix(vec4(.6,.83,1,...))` | Remove entirely | No vignette for generator |
| `u+.08` fixed shadow offset | `u + 0.07 * l` where `l` = bounding box diagonal | Scale shadow with cell size (matches Subdivide) |

### Add (not in reference)
- `h12()` 2D-to-1D hash (from existing Subdivide) for spatial noise and cell ID hashing
- FFT per-cell brightness: same code block as existing `subdivide.fs` lines 122-140
- Desaturation gate: same code block as existing `subdivide.fs` lines 151-154
- Area fade: `mix(bg, col, exp(-areaFade / area))` - compute area from bounding box `(tr.x-bl.x)*(tr.y-bl.y)` using the center() min/max values
- Edge darkening: two-pass sdPoly treatment matching existing Subdivide style

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest visible frequency in Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest visible frequency in Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on magnitude |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor |
| speed | float | 0.1-2.0 | 0.45 | Animation rate (CPU-accumulated) |
| threshold | float | 0.01-0.9 | 0.15 | Subdivision probability cutoff - lower = deeper |
| maxIterations | int | 2-20 | 8 | Maximum subdivision depth |
| edgeDarken | float | 0.0-1.0 | 0.75 | SDF edge/shadow darkening intensity |
| areaFade | float | 0.0001-0.005 | 0.0007 | Area below which cells dissolve |
| desatThreshold | float | 0.0-1.0 | 0.5 | Hash above which desaturation applies |
| desatAmount | float | 0.0-1.0 | 0.9 | Strength of desaturation on gated cells |

## Modulation Candidates

- **threshold**: Global subdivision depth control. Lower values = deeper, more fragmented. Primary audio-reactive knob.
- **speed**: Animation rate. Modulation creates tempo-sync pulsing of the pattern evolution.
- **gain**: FFT brightness amplifier. Louder = brighter cells.
- **baseBright**: Floor brightness. Modulating creates blackout/reveal dynamics.
- **edgeDarken**: Edge emphasis. Modulating shifts between flat fills and outlined polygons.
- **areaFade**: Cell dissolution threshold. Higher values dissolve more small cells.
- **desatAmount**: Color saturation gate. Modulating shifts cells between vivid and monochrome.

### Interaction Patterns

- **threshold + areaFade** (competing forces): Lower threshold pushes subdivision deeper, creating smaller cells. Higher areaFade dissolves small cells. The tension creates a visual boundary where new cells appear at the edges of visibility - like crystallization at a phase boundary. Modulating both from different audio bands makes verse/chorus shift between bold large polygons and shimmering fine detail.
- **threshold + maxIterations** (cascading threshold): maxIterations sets the hard ceiling; threshold determines what fraction of that ceiling is reached. With high maxIterations but high threshold, most cells exit early. Only when threshold is modulated low (e.g., on kick hits) do the deep subdivisions briefly appear.

## Notes

- The `maxN = 15` constant limits polygon vertex count. Each subdivision can add at most one vertex (split edge creates new point on each side), so 15 is generous for 8 iterations starting from a quad.
- The `sdPoly()` function loops over all N vertices twice per pixel (once for d1, once for d2 shadow), so maxIterations and vertex count directly affect performance. The existing Subdivide uses fixed N=4, so this will be somewhat heavier.
- The line-polygon intersection assumes convex polygons. Non-convex splits would break the algorithm, but the recursive subdivision of convex polygons always produces convex sub-polygons, so this is safe.
- Area computation uses bounding box area `(tr.x-bl.x)*(tr.y-bl.y)` from the `center()` function's min/max tracking, rather than the shoelace formula (which would need the full vertex array).
