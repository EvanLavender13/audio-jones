# Fracture Grid

A shifting mosaic that shatters the image into a grid of tiles, each showing the scene from a different offset, rotation, and zoom. The grid density is a live parameter — at zero the image is whole, cranked up it fragments into a staggered kaleidoscope. Tessellation mode switches between rectangular, hexagonal, and triangular grids.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: User-ordered transform chain
- **Compute Model**: Single-pass fragment shader

## References

- poshbrolly.net — Original Shadertoy shader (user-provided), source of the rectangular grid subdivision + per-cell time offset technique

## Reference Code

```glsl
//originally coded in poshbrolly.net
#define _t (iTime+14.)
#define _res iResolution.xy

const float PI=3.14159265359;
float maxdist=50.;
float det=.001;
float t;
vec3 objcol;

mat2 rot(float a){
    float s=sin(a),c=cos(a);
    return mat2(c,s,-s,c);
}

float sabs(float x, float s) {
    return sqrt(x*x+s);
}

float fractal(vec3 p) {
    p*=.1;
    float m=100.;
    for (int i=0; i<6; i++) {
        p=abs(p)/clamp(abs(p.x*p.y),.3, 2.)-1.;
        m=min(m,abs(p.x));
    }
    m=exp(-3.*m);
    return m;
}

float de(vec3 p){
    p.xz*=rot(smoothstep(-.5,.5,sin(t))*PI);
    float f=fractal(p);
    objcol=vec3(0.4,0,1)*f;
    objcol.rb*=rot(f*3.+t);
    objcol+=.4;
    float l=length(p)-3.;
    p.x=sabs(p.x,2.)-smoothstep(-.5,.5,sin(t*2.))*5.;
    p.y=sabs(p.y,2.)-smoothstep(-.5,.5,cos(t*2.))*4.;
    p.z=sabs(p.z,2.)-smoothstep(-.5,.5,cos(t*2.))*4.;
    float d= length(p)-2.5;
    d=min(d,l);
    return d*.5-f*.5;
}

vec3 normal(vec3 p) {
    vec2 e=vec2(0,det);
    return normalize(vec3(de(p+e.yxx),de(p+e.xyx),de(p+e.xxy))-de(p));
}


vec3 march(vec3 from, vec3 dir){
    vec3 p=from, col=vec3(0), g=col;
    float d, td=0.;
    for (int i=0;i<100;i++){
        p+=dir*d;
        d=de(p);
        if (d<det||td>maxdist) break;
        td+=d;
        g+=.1/(.1+d*.5)*objcol;
    }
    vec3 n=normal(p);
    if (d<det) {
        col=objcol*n.z*n.z;
    }
    return col+g*.12;
}
void mainImage(out vec4 fragColor,in vec2 fragCoord){
    t=_t*2.;
    vec2 uv=(fragCoord/_res-0.5)/vec2(_res.y/_res.x,1);  //2d uvs
    float h=asin(sin(t*.1))*5.;
    vec2 id=floor(uv*h);
    if (sin(t*.1)>.3) uv=fract(uv*h)-.5;
    t+=(id.x*3.+id.y*2.)*.1;
    vec3 from=vec3(0,0,min(-20.,sin(t*1.5)*15.));
    vec3 dir=normalize(vec3(uv,1.));
    vec3 col=march(from,dir);
    fragColor=vec4(col,1);
}
```

## Algorithm

### What to extract from the reference

The grid technique lives entirely in `mainImage()`. The relevant lines:

```glsl
float h = asin(sin(t*.1)) * 5.;           // grid density (triangle wave)
vec2 id = floor(uv * h);                   // cell ID
if (sin(t*.1) > .3) uv = fract(uv*h)-.5;  // subdivide UVs into cells
t += (id.x*3. + id.y*2.) * .1;            // per-cell phase offset
```

Everything else (raymarching, fractal, distance estimation) is discarded — this is a texture-reading transform, not a generator.

### Adaptation to AudioJones transform

The reference uses per-cell time offset to stagger a raymarched animation. As a texture transform, the per-cell variation becomes **spatial**: each tile samples the input texture at a shifted/rotated/zoomed UV coordinate.

**Per-cell variation via hash:**
- Hash the cell ID (`id`) to get 3 pseudo-random values per tile
- Map those to: UV offset amount, rotation angle, zoom factor
- Scale all three by a single `stagger` intensity parameter

**Tessellation modes** (int enum, 0/1/2):

1. **Rectangular** — direct from reference: `id = floor(p * subdivision)`, `cell_uv = fract(p * subdivision)`
2. **Hexagonal** — axial hex coordinates: offset odd rows by 0.5, compute nearest hex center from 3 candidates, cell UV is distance from center
3. **Triangular** — equilateral triangle grid: divide into rows, alternate row offset, determine which triangle half the point falls in via diagonal test

All three produce the same two outputs: `vec2 id` (cell identifier for hashing) and `vec2 cell_uv` (local coordinate within the cell, centered at 0).

**UV remapping per tile:**
```
hash3 = hash(id)  // 3 pseudo-random values in [0,1]
offset = (hash3.xy - 0.5) * stagger * offsetScale
angle  = (hash3.z - 0.5) * stagger * rotationScale
zoom   = 1.0 + (hash3.x - 0.5) * stagger * zoomScale
```

The final texture sample coordinate:
```
sample_uv = center + rot2d(angle) * (cell_uv / zoom) + offset
```
Where `center` is the cell's position in the original UV space (so the tile shows roughly the right part of the image, shifted/rotated/zoomed by the stagger).

**Subdivision as continuous float:**
- At 0.0: bypass the effect entirely (pass through)
- At low values (0.5–2.0): a few large tiles with subtle stagger
- At high values (5.0+): many small tiles, stagger very visible
- Non-integer values produce partial tile rows at edges — this is fine, creates organic feel

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| subdivision | float | 0.0–20.0 | 4.0 | Grid density — number of tiles across the screen |
| stagger | float | 0.0–1.0 | 0.5 | Per-tile variation intensity (scales offset, rotation, zoom together) |
| offsetScale | float | 0.0–1.0 | 0.3 | Maximum UV offset per tile |
| rotationScale | float | 0.0–π | 0.5 | Maximum rotation per tile (radians) |
| zoomScale | float | 0.0–1.0 | 0.3 | Maximum zoom deviation per tile |
| tessellation | int | 0–2 | 0 | Grid type: 0=rectangular, 1=hexagonal, 2=triangular |
| seed | float | 0.0–100.0 | 0.0 | Hash seed — different values reshuffle the per-tile assignments |

## Modulation Candidates

- **subdivision**: pulsing grid density — tiles appear/dissolve with beat energy
- **stagger**: calm passages show coherent tiles, loud passages shatter into chaos
- **offsetScale / rotationScale / zoomScale**: individually target which type of per-tile variation reacts to audio
- **seed**: slow drift creates gradual tile reassignment over time

### Interaction Patterns

**Cascading threshold (subdivision × stagger):** At low subdivision (1–2 tiles), even max stagger produces minimal visual change because there are so few cells to differentiate. As subdivision rises, stagger multiplies across many tiles — the fragmentation becomes exponentially more chaotic. Modulating subdivision with beat energy while stagger sits at moderate creates verse/chorus contrast: verse = few calm tiles, chorus = shattered mosaic.

**Competing forces (offsetScale vs zoomScale):** High offset pulls tiles apart spatially while high zoom pulls them inward. The tension between "spread out" and "zoom in" per tile creates unpredictable compositions where some tiles show wide context and neighbors show tight crops.

## Notes

- Hex and triangular tessellations require slightly more ALU than rectangular but are single-pass with no branching concerns
- The `seed` parameter should be added to the hash input so the same cell IDs produce different variation when seed changes
- At very high subdivision (15+), tiles become small enough that per-tile rotation/zoom may alias — consider clamping effective stagger at high subdivision
- No extra textures or render targets needed — pure coordinate remapping
