# Pillar Grid

The input scene is sampled on a square grid; each cell becomes a 3D rounded-box pillar standing on a flat floor. Pillar height comes from that cell's luminance and pillar color comes from that cell's RGB. A static camera looks at the field from a user-controllable pitch angle, with the camera distance auto-derived so the entire field always fills the frame. A fixed-step SDF raymarch in the fragment shader produces diffuse + white specular + AO shading, turning the input into a 3D voxel city of pulsing pillars whose silhouette pulses with whatever the visualizer below it is doing.

## Classification

- **Category**: TRANSFORMS > Cellular (section index 2)
- **Pipeline Position**: After Symmetry and Warp, alongside Voronoi / Lattice Fold / Multi-Scale Grid / Dot Matrix / Fracture Grid

## Attribution

- **Based on**: "Voxel Pillars webcam Y28" by Yusef28
- **Source**: https://www.shadertoy.com/view/s3XGWH
- **License**: CC BY-NC-SA 3.0 (Shadertoy default; not stated otherwise in source)

## References

- [Voxel Pillars webcam Y28](https://www.shadertoy.com/view/s3XGWH) - Source shader: per-cell rounded-box SDF heightfield raymarched in fragment shader, height driven by per-cell luminance of an input texture (originally a webcam)
- [iquilezles.org - distance functions](https://iquilezles.org/articles/distfunctions/) - `roundBox` SDF and palette technique referenced in the source

## Reference Code

```glsl
#define FAR 60.
//take in vec2 return random float 0 - 1
//take in vec2 return random float 0 - 1
float rnd(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

//rounded box from iq
float roundBox(vec3 p, vec3 b, float r)
{
    return length(max(abs(p)-b,0.0))-r;
}

float sdBoxFrame( vec3 p, vec3 b, float e )
{
       p = abs(p  )-b;
  vec3 q = abs(p+e)-e;
  return min(min(
      length(max(vec3(p.x,q.y,q.z),0.0))+min(max(p.x,max(q.y,q.z)),0.0),
      length(max(vec3(q.x,p.y,q.z),0.0))+min(max(q.x,max(p.y,q.z)),0.0)),
      length(max(vec3(q.x,q.y,p.z),0.0))+min(max(q.x,max(q.y,p.z)),0.0));
}

//rotation matrix (clockwise)
mat2 rot(float a)
{
     float c = cos(a),s = sin(a);
     return mat2(c, -s, s, c);
}

//makes the dna strands
vec2 helix(vec3 p )
{
    //repeat space on xz
    vec3 op = p;
    p.xz = mod(p.xz, 15.) -7.5;
    //p.x += float(int(op.x/15.) % 2 )*4. ;
    //rotate each cell based on y for helix shape
    p.xz*=rot(p.y*3.14159/10.);

    //create two cylinders which will be twisted
    vec2 t = vec2(length(p.xz + vec2(1.0,0.0)) - 0.2, 1.);
    t.x = min(t.x,length(p.xz - vec2(1.0, 0.0)) - 0.2);

    vec2 h = vec2(length(p.xz + vec2(1.3,0.0)) - 0.1, 2.);
    h.x = min(h.x,length(p.xz - vec2(1.3, 0.0)) - 0.1);
    h.x = min(h.x,length(p.xz + vec2(1.3, 0.0).yx) - 0.1);
    h.x = min(h.x,length(p.xz - vec2(1.3, 0.0).yx) - 0.1);

    t.y = t.x < h.x ? t.y : h.y; t.x = min(t.x,h.x);
    //mod space on y for bars
    p.y = mod(p.y,.4)-.2;
    //create y repeated cylinders cut at abs(p.x etc)
    h = vec2(max(length(p.yz) - 0.07, abs(p.x) - .9), 3.) ;
    t.y = t.x < h.x ? t.y : h.y;  t.x = min(t.x,h.x);
    //return helix (union of cylinders and bars)
    return t;
}

float tileHeight(vec2 id){
    //id = id*0.4;
    return (pow(rnd(id)*2.,2.2));//+iTime*rnd(id));//sin(id.x+iTime)+cos(id.y+iTime);
}
float getGrey(vec3 p)
{
    return p.x*0.299 + p.y*0.587 + p.z*0.114;
       }

float gh = 0.;

float tile(vec3 p){
     //repeat xz
     float N = 2.;
     vec2 id = floor(p.xz);
     p.xz = fract(p.xz)-0.5;// mod(p.xz, 1.)-0.5;
     //create rounded boxes for tiles and return
     float d = 100.;
     float ASP = iResolution.x/iResolution.y;
     gh = getGrey(texture(iChannel1, ((floor((id)*vec2(1.,ASP))+40.)/80.)).rgb)*5.;//tileHeight(id+n)

     for(float i = -N; i <= N; i++){
         for(float j = -N; j <= N; j++){
             vec2 n = vec2(i,j);
             float h = getGrey(textureLod(iChannel1, ((floor((id+n)*vec2(1.,ASP))+40.)/80.), 2.).rgb)*8.;//tileHeight(id+n)


             d = min(d, roundBox(p-vec3(n.x,-3.,n.y),vec3(0.27,h,0.27), 0.3));
     }
        }
     return d;
}

//map function
vec2 map(vec3 p)
{
    //vec2 h = helix(p);
    vec2 t = vec2(tile(p),0.);
  //  t.y = t.x < h.x ? t.y : h.y;  t.x = min(t.x,h.x);
     return t;
}

//basic raymarch
vec2 trace(vec3 ro, vec3 rd)
{
       vec2 t = vec2(0.),dist;
    for(int i=0; i<96; i++)
    {
         dist = map(ro + rd*t.x);
         if(dist.x<0.0001 || t.x > FAR){break;}
         t.x += dist.x*0.85;
         t.y = dist.y;
    }
 return t;
}

//reflection trace (see shanes reflection shader)
vec2 rtrace(vec3 ro, vec3 rd)
{
       vec2 t = vec2(0.),dist;
    for(int i=0; i<48; i++)
    {
         dist = map(ro + rd*t.x);
         if(dist.x<0.0001 || t.x > FAR){break;}
         t.x += dist.x;
    }
 return t;
}

//basic normal calculation
vec3 normal(vec3 p){
    mat3 k = mat3(p,p,p) - mat3(0.01);
    return normalize(map(p).x - vec3(map(k[0]).x, map(k[1]).x ,map(k[2]).x));
}
//ao from shane
float calculateAO(in vec3 pos, in vec3 nor)
{
    float sca = 2.0, occ = 0.0;
    for( int i=0; i<5; i++ ){

        float hr = 0.01 + float(i)*0.5/4.0;
        float dd = map(nor * hr + pos).x;
        occ += (hr - dd)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - occ, 0.0, 1.0 );
}
vec3 colors[6] = vec3[](
    vec3(0.0000, 0.0000, 0.0000), // #000000
    vec3(0.2000, 0.2000, 0.2000), // #333333
    vec3(1.0000, 1.0000, 1.0000), // #FFFFFF
    vec3(1.0000, 0.0000, 0.4000), // #FF0066
    vec3(0.0000, 0.8000, 1.0000), // #00CCFF
    vec3(1.0000, 0.8500, 0.000)
    );
//based on shanes lighting function
vec3 lighting(vec3 sp, vec3 sn, vec3 lp, vec3 rd, float id)
{
    vec3 color;
    //vector from hit position to light position
    vec3 lv = lp - sp;
    //length of that vector
    float ldist = max(length(lv), 0.001);
    //direction of that vector
    vec3 ldir = lv/ldist;
    //attenuation
    float atte = 2.0/(1.0 + 0.002*ldist*ldist );
    //diffuse color
    float diff = dot(ldir, sn);
    //specular reflection
    float spec = pow(max(dot(reflect(-ldir, sn), -rd), 0.0), 10.);
    //fresnel
    float fres = pow(max(dot(rd, sn) + 1., 0.0), 1.);
    //ambient occlusion
    float ao = calculateAO(sp, sn);
    //reflecton
    vec3 refl = reflect(rd, sn);
    //id for random tile color
    float rndTile = rnd(floor(sp.xz));
    float rndTileY = rnd(floor(sp.yy*0.5));
    //color options
    vec3 color2 =  0.5+0.5*sin(vec3(1.,3.,4.)/1. + gh*3.1415/2.+1.5);//mix(vec3(1.),colors[int(floor(mod(rndTile*50.,5.)))], 0.6);
   /* if(id > 0.){color2 = mix(colors[1],colors[3], float(mod(rndTileY, 2.) < 0.5));}
    if(id > 1.){color2 = colors[4];}
    if(id > 2.){color2 = colors[5];}*/
    //getting reflected and refracted color froma cubemap, only refl is used
    vec4 reflColor = texture(iChannel0, refl);
    //orage specular
    vec3 hotSpec = vec3(0.9,0.5, 0.2);
    //apply color options and add refl/refr options
    color = (diff*color2 + spec*hotSpec + reflColor.xyz*0.2 )*atte;
    //apply ambient occlusion and return.
    return clamp(color*ao, 0.,1.);
}

//rotation matrix
mat2 rot2( float a ){ vec2 v = sin(vec2(1.570796, 0) - a);    return mat2(v, -v.y, v.x); }

//path from shane's abstract plane shader
vec2 path(in float z){ float s = sin(z/36.)*cos(z/18.); return vec2(s*16., 0.); }

vec3 skyGradient(float t) {
    // t = 0.0 (horizon) -> 1.0 (zenith)

    vec3 horizon = vec3(0.95, 0.97, 1.00); // casi blanco frio
    vec3 mid     = vec3(0.20, 0.45, 0.75); // azul limpio
    vec3 zenith  = vec3(0.02, 0.05, 0.12); // azul muy oscuro

    // mezcla en dos etapas para mas control visual
    vec3 col = mix(horizon, mid, smoothstep(0.0, 0.5, t));
 col = mix(col, zenith, smoothstep(0.4, 1.0, t));

    return col;//vec3(0.);
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - iResolution.xy*.5)/iResolution.y;

    //fisheye
    //uv = normalize(uv) * tan(asin(length(uv) * 1.));

    // Camera Setup.
    vec3 lk = vec3(0, 0., 0.);
    lk.xy += path(lk.z);
    vec3 ro = lk + vec3(0, 25., -.25);
     vec3 lp = ro + vec3(0, 3.75, 10);


    //camera
    float FOV = 1.57;
    vec3 fwd = normalize(lk-ro);
    vec3 rgt = normalize(vec3(fwd.z, 0., -fwd.x ));
    vec3 up = cross(fwd, rgt);
    vec3 rd = normalize(fwd + FOV*uv.x*rgt + FOV*uv.y*up);
    //rd.zy *= rot( -0.6 );
    //rd.xy *= rot( path(lk.z).x/64. );


    //distance to closest hit
    vec2 t = trace(ro, rd);
    //normalized distance
    float far = smoothstep(0.0, 1.0, t.x/FAR);
    //hit point
    vec3 sp = ro + rd*t.x;
    //normal
    vec3 sn = normal(sp);

    vec4 cubeColor = texture(iChannel0, rd);
    //lighting
    vec3 color = lighting(sp, sn, lp, rd, t.y);
    //reflection based on shanes reflection shader
    vec3 refRay = reflect(rd, sn);

    //trace reflection
    vec2 rt = rtrace(sp+sn*0.005, refRay);
    //relection hit point
    vec3 rsp = (sp+refRay*0.005) + refRay*rt.x;
    //reflection surfact normal
    vec3 rsn = normal(rsp);
    //add reflection lighting
    //color += lighting(rsp, rsn, lp, refRay, t.y)*0.1;

    //accidental solar halo
    vec3 sky = vec3(0.);//skyGradient(rd.y);//mix(vec3(.3,0,.2), vec3(.9,.5,.2), rd.y+0.3) ;//mix(vec3(0.9, 0.5, 0.2)*4., vec3(0.0)-0.4, pow(abs(rd.y), 1./3.))*(1./pow(abs(length(rd.xy)-0.4), 1./3.))/8.;

    //add cube color
   // sky += cubeColor.xyz*0.1;
  //  color = mix(color, sky, far);

    //naive vignette
    float vig = 1.0-smoothstep(1.0,3.5, length(uv));
    color.xyz *= mix( 0.8, 1.0, vig);

    fragColor = vec4(color,1.0);
}
```

## Algorithm

This is a one-pass fragment-shader transform: per-pixel raymarch of an SDF whose heightfield is sampled from the input texture.

**Keep verbatim from reference:**
- `roundBox(p, b, r)` SDF
- `getGrey(rgb)` luminance helper
- `tile(p)` 5x5 neighborhood loop with `roundBox` per cell (the `N=2` window must stay so tall pillars can be SDF-evaluated from neighboring cells)
- `trace(ro, rd)` raymarch (96 steps, 0.85 step scale, FAR=60)
- `normal(p)` central-difference gradient
- `calculateAO(pos, nor)` 5-sample ambient occlusion
- `lighting()` core: diffuse `dot(ldir, sn)`, specular `pow(max(dot(reflect(-ldir, sn), -rd), 0.0), 10.)`, attenuation `2.0/(1.0 + 0.002*ldist*ldist)`
- Vignette: `1.0 - smoothstep(1.0, 3.5, length(uv))` mixed with `mix(0.8, 1.0, vig)`
- FOV = 1.57

**Replace from reference:**
- `tile()` cell sampling formula `((floor((id+n)*vec2(1.,ASP))+40.)/80.)` -> map cell id directly to input UV based on `density` uniform: `(id + n + density*0.5 + 0.5) / density`. The `+0.5` centers each cell on its corresponding input pixel. The `+40./80.` reference encodes "input image spans 80 cells across centered on origin"; we generalize to N cells.
- Pillar half-extent `vec3(0.27, h, 0.27)` -> `vec3(pillarFill, h, pillarFill)` uniform
- Corner radius `0.3` -> `cornerRadius` uniform
- Height multiplier `*8.` (per-cell) -> `heightScale` uniform
- `gh*5.` (color-input grey scaler) -> drop entirely; we sample input RGB directly instead of using `gh` for palette indexing
- Color line `vec3 color2 = 0.5+0.5*sin(vec3(1.,3.,4.)/1. + gh*3.1415/2.+1.5)` -> `vec3 color2 = texture(inputTex, cellUV).rgb` where `cellUV` is the same UV used to sample height for the hit cell
- Specular tint `hotSpec = vec3(0.9, 0.5, 0.2)` -> `vec3(1.0)` (white)
- `reflColor.xyz*0.2` cubemap term -> remove entirely (no cubemap asset available, and the reference's other reflection paths are already inactive)
- Camera setup `vec3 ro = lk + vec3(0, 25., -.25)` -> auto-derive from `pitch` and `density`: at distance R = density * framingMargin, `ro = vec3(0, R*sin(pitch), -R*cos(pitch) - 0.25)`. Keep `lk = vec3(0)` and the `-0.25` z offset (transcribed from reference - prevents the `rgt` basis singularity at pitch = PI/2 because `fwd.z` stays nonzero). `framingMargin` is a fixed scalar chosen so the floor extent fits inside the FOV (roughly 0.7 for FOV=1.57; tune in implementation).
- Light position `lp = ro + vec3(0, 3.75, 10)` -> keep this relative-to-camera offset; light tracks the camera so highlights stay consistent across pitch values.

**Drop entirely (already inactive in reference):**
- `helix()` function (defined, never called)
- `tileHeight()` function (defined, never called; height comes from texture)
- `sdBoxFrame()` function (defined, never called)
- `rtrace()` function and its caller-site lines (`rt`, `rsp`, `rsn`, the commented `color += lighting(rsp, ...)` block)
- `colors[6]` palette array
- `rnd()`, `rndTile`, `rndTileY` (only feed unused color paths)
- `fres` fresnel computation
- `path()` function and the `lk.xy += path(lk.z)` line
- `skyGradient()` function and the `sky` variable (sky stays vec3(0))
- `cubeColor` texture lookup
- `rot()`, `rot2()` functions (no rotation needed; camera math uses straight basis vectors)
- Fisheye line, the two `rd *= rot(...)` lines (already commented)

**Cell-to-UV mapping (the central adaptation):** The reference's `tile()` computes the SDF in world space where 1 unit = 1 cell. For our adaptation:
1. The visible field is `density x density` cells, world width = `density` units, centered on origin.
2. Cell id `(i, j)` (integer) maps to input UV `((i + density*0.5 + 0.5) / density, (j + density*0.5 + 0.5) / density)` so cell index 0 lives at UV (0.5/density, 0.5/density) and the field fills `[0, 1] x [0, 1]`.
3. Height for cell `(i, j)` = `getGrey(texture(inputTex, cellUV).rgb) * heightScale`.
4. Color for the hit cell = `texture(inputTex, cellUV).rgb` (no luminance conversion; preserve hue).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| density | int | 16-128 | 48 | Cells across the field; auto-frames the camera |
| pitch | float (radians) | 0-PI/2 | PI/3 | Camera tilt: 0=horizontal-across-field, PI/2=overhead |
| heightScale | float | 0-20 | 8 | Tallest possible pillar height in world units (world unit = 1 cell width) |
| pillarFill | float | 0.05-0.49 | 0.27 | Half-extent of each pillar inside its cell; gap between pillars = (1 - 2*pillarFill) cell-widths |
| cornerRadius | float | 0-0.5 | 0.3 | Rounded-box corner softness; 0=sharp boxes, high=bubbly tops |

## Modulation Candidates

- **density**: shifts visual scale - low values give chunky few-pillar looks, high values approach a continuous heightfield surface
- **pitch**: dramatic perspective shift between top-down map and low-angle skyline
- **heightScale**: pillars rise and fall together; the headline "pulse" parameter
- **pillarFill**: pillars expand and contract like the field is breathing
- **cornerRadius**: shifts the pillars between hard tile-floor and bubbly blob field

### Interaction Patterns

- **heightScale x pitch (cascading threshold)**: at near-overhead pitch (close to PI/2), pillar height is mostly invisible because we're looking down their long axis - height-mod produces almost no visible change. As pitch decreases toward horizontal, pillars start showing their silhouette and height-mod becomes dramatic. Pitch is effectively a gate on whether height changes are visible at all. Audio-driving heightScale while a slow LFO sweeps pitch produces a "the city rises into view as the camera tilts" choreography.
- **pillarFill x cornerRadius (competing forces)**: together they decide whether the field reads as discrete cells or a continuous surface. High fill + high corner radius -> pillars merge into a bumpy organic surface. Low fill + low corner radius -> sparse sharp spires with empty floor between them. Modulating one against the other shifts the surface character between cellular and continuous along an axis neither parameter could traverse alone.
- **density x heightScale (resonance)**: when density is low (large pillars), heightScale changes are visually dominant and individual pillars carry the silhouette. When density is high (tiny pillars), heightScale changes look like a noise floor instead of a city skyline. Both mapped to the same audio source produces matched pulses; mapped to different sources, they swap the field between "few large dramatic pulsing towers" and "fine vibrating texture."

## Notes

- **Performance**: 5x5 neighborhood lookup per ray-step (25 SDF calls) times up to 96 ray steps = up to 2400 SDF evaluations per pixel. At 1080p this is heavy. Plan accordingly - the descriptor likely needs `EFFECT_FLAG_HALF_RES`.
- **Neighborhood window**: `N=2` (5x5) is required because tall pillars from neighboring cells can stick into the current cell when measured by SDF. Reducing to `N=1` (3x3) saves 16/25 of the work but causes incorrect occlusion when `heightScale * pillarFill` exceeds ~1 cell-width. If perf becomes critical, consider scaling N from `pillarFill * heightScale` rather than hardcoding.
- **Frame clipping at low pitch**: auto-framing fits the floor extent in view, not the pillar tops. At low pitch with high heightScale, the tallest pillars will clip the top of the frame. This is intentional (sky-cropping is part of the look); if undesirable the framing math could include `heightScale` in the distance formula.
- **Zero-luminance cells**: produce zero-height pillars, which collapse to `roundBox(..., vec3(fill, 0, fill), cornerRadius)` - flat squares on the floor. Not a bug; this is what gives dark areas of the scene a flat tile-floor appearance.
- **Color sampled at hit cell**: when a ray hits a pillar from neighbor `(i, j)` of the floor cell, the surface point's `floor(p.xz)` lands inside the actual pillar's cell, so `cellUV` for color matches the pillar - no special bookkeeping needed.
- **Shader coordinate convention**: the source uses centered NDC `uv = (fragCoord - iResolution.xy*.5)/iResolution.y` which already matches this codebase's centered-coords rule.
