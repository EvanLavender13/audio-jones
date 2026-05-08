# Puzzle

The input image becomes a tiled jigsaw puzzle. Each cell holds a single locked piece with deterministic tabs and blanks that interlock with its neighbors, the input texture sampled across the piece face, and SDF-derived edge lighting that gives the pieces a 3D pop. A random seed shuffles the tab/blank pattern; a fill-mode toggle switches between sampling the input per-pixel (texture mode) or per-cell (solid color mode, like Voronoi cell fill).

## Classification

- **Category**: TRANSFORMS > Novelty
- **Pipeline Position**: Transform stage (reorderable, between previous transforms and output stage). Sibling of LEGO Bricks and Disco Ball.

## Attribution

- **Based on**: "Endless Puzzle" by Bingle
- **Source**: https://www.shadertoy.com/view/N3sGD4
- **License**: CC BY-NC-SA 3.0

## References

- "Endless Puzzle" by Bingle (https://www.shadertoy.com/view/N3sGD4) - source shader providing the jigsaw piece SDF (sdBox + tab/blank smooth union/subtraction), per-edge hash-driven tab orientation, and SDF-normal edge lighting.

## Reference Code

```glsl
#define EDGELIGHT 1.0
#define RAINBOW 0.6

// https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3){
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}

float hash12(vec2 p){
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

mat3 getPieceMat(vec2 cell,float cellSize,vec2 cam){
    float time = iTime*hash12(cell);
    vec3 rnd = mix(hash33(vec3(cell,floor(time))),hash33(vec3(cell,ceil(time))),smoothstep(0.0,1.0,fract(time)))-0.5;
    vec2 pos = (cell+cam)*cellSize-cam+0.5+rnd.xy*(cellSize-1.0);
    float rot = (rnd.z)*(cellSize-1.0);
    vec2 X = vec2(cos(rot),sin(rot));

    return mat3(
        vec3(X.x,X.y,0),
        vec3(-X.y,X.x,0),
        vec3(pos,1)
    );
}

float sdBox( in vec2 p, in vec2 b ){
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float opSmoothUnion( float a, float b, float k ){
    k *= 4.0;
    float h = max(k-abs(a-b),0.0);
    return min(a, b) - h*h*0.25/k;
}

float opSmoothSubtraction( float a, float b, float k ){
    return -opSmoothUnion(a,-b,k);
}

float pieceSDF(vec2 cell,vec2 p){
    float dist = sdBox(p,vec2(0.5));
    const vec2[4] dirs = vec2[](vec2(1,0),vec2(0,1),vec2(-1,0),vec2(0,-1));
    for (int i=0;i<4;i++){ // Add tabs and blanks
        bool dir = hash12(cell+dirs[i]*0.5)>0.5;
        if (dir == (dirs[i].y<dirs[i].x)){
            // Tab (the one that sticks out)
            dist = opSmoothUnion(dist,distance(p,dirs[i]*0.65)-0.14,0.025);
            dist = opSmoothUnion(dist,distance(p,dirs[i]*0.5)-0.1,0.025); // Makes the connection slightly smoother
        }else{
            // Blank (the one that sticks in)
            dist = opSmoothSubtraction(distance(p,dirs[i]*0.35)-0.15,dist,0.02);
        }
    }
    return dist;
}

vec2 pieceSDFnorm(vec2 cell,vec2 p){
    float e = 0.0001;
    float d = pieceSDF(cell,p);
    return vec2(d-pieceSDF(cell,p-vec2(e,0)),d-pieceSDF(cell,p-vec2(0,e)))/e;
}

float stepUpDown(float x,float period,float slope){
    return 1.0-clamp(slope*(abs(mod(x,2.0*period)-0.5*(period+1.0/slope))-0.5*(period-1.0/slope)),0.0,1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    float viewHeight = 8.5+sin(iTime*0.15)*3.0+10.0*smoothstep(0.0,1.0,stepUpDown(iTime-5.0,10.0,0.5));
    vec2 UV = (fragCoord/iResolution.y-0.5)*viewHeight;
    vec2 cam = vec2(-2.0*iTime,cos(iTime));
    float camTilt = (iTime*0.15-sin(iTime*0.2))*0.5;
    vec2 camX = vec2(cos(camTilt),sin(camTilt));

    UV *= mat2(camX,-camX.y,camX.x);
    UV -= cam;


    float cellSize = 1.0+smoothstep(0.0,1.0,stepUpDown(iTime+0.05*(UV.x+UV.y),7.0,0.2));
    vec2 cell = floor((UV+cam)/cellSize-cam);

    // Table texture
    vec3 col = texture(iChannel2,UV*0.2-0.13*cam).rgb*0.75;

    for (int i=0;i<9;i++){ // Draw all pieces in the current cell and surrounding cells
        vec2 testCell = cell + vec2(i%3-1,i/3-1);
        mat3 mat = getPieceMat(testCell,cellSize,cam);
        mat3 inv = inverse(mat);
        vec2 local = (inv*vec3(UV,1)).xy;
        float dist = pieceSDF(testCell,local);
        if (dist<0.0){
            vec2 norm = (vec3(pieceSDFnorm(testCell,local),0)*inv).xy;

            float edge = smoothstep(-0.075,0.0,dist);

            vec3 pieceCol = texture(iChannel0,0.25*(testCell+local)).rgb; // Brick texture
            pieceCol *= mix(vec3(0.5),texture(iChannel1,0.0015*(testCell+local)).rgb,RAINBOW); // Rainbow tint
            pieceCol *= mix(0.5,dot(norm*edge*edge,vec2(1,1))*0.5+0.5,EDGELIGHT); // Edge lighting
            pieceCol *= 4.5;

            col = mix(col,pieceCol,min(-iResolution.y*dist/viewHeight,1.0));
        }
    }

    // Vignette
    col /= dot(fragCoord/iResolution.xy-0.5,fragCoord/iResolution.xy-0.5)*1.5+1.0;

    fragColor = vec4(col,1.0);
}
```

## Algorithm

Adapt as a transform fragment shader that reads `texture0` (the upstream pipeline image) and outputs the puzzle-tiled version. The reference's piece SDF, normal, and edge lighting math transcribe verbatim; everything camera-related collapses to constants since pieces are locked.

**Keep verbatim:**
- `hash12(vec2)` and `hash33(vec3)` helpers
- `sdBox`, `opSmoothUnion`, `opSmoothSubtraction`
- `pieceSDF(cell, p)` - the tab/blank assembly (the heart of the effect)
- `pieceSDFnorm(cell, p)` - finite-difference normal
- The 9-cell loop (`i = 0..8`) covering current cell + 8 neighbors
- Edge fade `smoothstep(-0.075, 0.0, dist)` for the per-piece anti-alias mask
- Edge-lighting math: `dot(norm*edge*edge, vec2(1,1))*0.5+0.5`
- Final blend `mix(bg, pieceCol, min(-iResolution.y*dist/viewHeight, 1.0))` for sub-pixel piece edge

**Replace:**
- `iTime` time animations of `viewHeight`, `cam`, `camTilt`, `cellSize` -> all gone. `cellSize` becomes a uniform constant derived from `pieceCount`. There is no camera, no tilt, no view animation.
- `getPieceMat` matrix building -> deleted. With locked pieces (`cellSize - 1.0 = 0` in the original math), `pos = cell + 0.5` and `rot = 0`. Replace `local = (inv * vec3(UV, 1)).xy` with `local = UV - (cell + 0.5)`. Replace the inverse-transformed normal with the raw `pieceSDFnorm` output (no rotation to undo).
- `iChannel0` brick texture sampling -> sample input `texture0` instead. Two modes:
  - **Texture mode (fillMode == 0)**: `pieceCol = texture(texture0, fragTexCoord).rgb`. The input image shows through each piece at its actual screen position.
  - **Solid mode (fillMode == 1)**: `pieceCol = texture(texture0, cellCenterUV).rgb` where `cellCenterUV = ((cell + 0.5) / pieceWorldHeight + 0.5*aspectFix) * scaleBack` - sample input at the cell-center world position mapped back to UV. All pixels in a cell share one color (Voronoi-style flat fill).
- `iChannel1` rainbow noise sampling and the `mix(vec3(0.5), ..., RAINBOW)` line -> deleted. No tint at all.
- `iChannel2` table texture -> deleted. Background color is black (where pieces don't tile, which with locked interlocking pieces is hairline seams only).
- `pieceCol *= 4.5` brightness boost -> deleted. The input texture is already full-range; no scaling needed.
- Final vignette divide -> deleted. The codebase has a separate Vignette transform.
- `EDGELIGHT` define (1.0) -> uniform `edgeLight` (0.0-1.0), default 1.0. Same math: `mix(0.5, edgeLightTerm, edgeLight)`.
- The smoothing `k` constants in `pieceSDF` (0.025, 0.025, 0.02) -> a single uniform `smoothness`, default 0.025, range 0.005-0.1. Apply the same value at all three call sites.
- Hash inputs in tab/blank direction picking and SDF (`hash12(cell + dirs[i]*0.5)`) -> add a `seed` uniform (float). Replace with `hash12(cell + dirs[i]*0.5 + vec2(seed))`. Modulating or changing `seed` shuffles which edges are tabs vs blanks across the entire grid deterministically.

**Coordinate space (centered, per conventions):**
- Compute centered pixel coordinate: `vec2 centered = (fragTexCoord - 0.5) * vec2(resolution.x/resolution.y, 1.0)` (aspect-corrected so puzzle pieces are square).
- World UV scale: `vec2 worldUV = centered * pieceCount`. With `pieceCount = 12`, the screen is 12 cells tall; `worldUV` plays the role of the reference's `UV` after `viewHeight` scaling.
- `cell = floor(worldUV)`. The 9-cell neighbor loop uses `testCell = cell + vec2(i%3-1, i/3-1)`. Local coord is `local = worldUV - (testCell + 0.5)`.
- For solid-fill mode, the cell center world position is `testCell + 0.5`. Convert back to UV: `cellCenterUV = (testCell + 0.5) / pieceCount * vec2(resolution.y/resolution.x, 1.0) + 0.5`.

**Final blend (per-piece anti-alias):**
- The reference's `min(-iResolution.y*dist/viewHeight, 1.0)` is dist measured in pixels (since `iResolution.y / viewHeight` is the pixels-per-world-unit ratio). In our adaptation: pixels per world unit = `resolution.y / pieceCount`. Use `min(-resolution.y * dist / pieceCount, 1.0)` as the mix factor.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `pieceCount` | float (int slider) | 4 - 40 | 12 | Number of pieces across screen height. Smaller = chunkier pieces, larger = finer mosaic. |
| `edgeLight` | float | 0.0 - 1.0 | 1.0 | Strength of SDF-normal edge lighting. 0 = flat pieces, 1 = full reference shading. |
| `smoothness` | float | 0.005 - 0.1 | 0.025 | SDF smooth-union k for tab/blank junctions. Smaller = sharper crisp corners; larger = blobbier rounded pieces. |
| `seed` | float | 0.0 - 100.0 | 0.0 | Random offset to the tab/blank direction hash. Changing it reshuffles the puzzle layout. |
| `fillMode` | int (combo) | 0 - 1 | 0 | 0 = Texture (sample input per-pixel through each piece), 1 = Solid (sample input once per cell center, flat-fill). |

## Modulation Candidates

- **pieceCount**: changes piece density - small modulations breathe the puzzle in and out (more pieces -> finer mosaic).
- **edgeLight**: changes 3D pop - low values flatten pieces toward a flat-color collage; high values exaggerate the bevel.
- **smoothness**: changes corner geometry - low values give crisp interlocking; high values make pieces blobby and almost organic.
- **seed**: when modulated continuously, the tab/blank pattern shuffles, since the hash is deterministic per integer-step seed value the visual "snaps" between layouts; with smooth modulation the layout flickers between configurations.

### Interaction Patterns

- **pieceCount + smoothness (cascading thresholds)**: at high `pieceCount` (small pieces) the smoothness must shrink proportionally or pieces lose their identifiable tab/blank shape and the SDF blobs into an undifferentiated grid. Modulating `smoothness` upward without scaling it to `pieceCount` produces a threshold where pieces visibly merge - useful as a song-climax dissolve effect.
- **edgeLight + fillMode (resonance)**: in solid-fill mode, edge lighting becomes the *only* way to read piece geometry (since the face is a single flat color). High `edgeLight` + solid mode = sharply-defined cell-shaded pieces; low `edgeLight` + solid mode = barely-visible flat color blobs; low `edgeLight` + texture mode = puzzle vanishes into the input image. The two parameters resonate to control how strongly the puzzle is "felt" against the input.

## Notes

- **Locked pieces only**: the design strips the reference's camera and cellSize-pulse animations, which together drive all per-piece displacement and rotation. With `cellSize == 1.0` constant, the original math yields zero displacement (`rnd.xy * (cellSize - 1.0) == 0`) and zero rotation (`rnd.z * (cellSize - 1.0) == 0`). The `getPieceMat` matrix collapses to identity-plus-translation, which is why we drop it entirely and use `local = worldUV - (cell + 0.5)`.
- **Hash determinism for interlocking**: tab/blank direction is decided by `hash12(cell + dirs[i]*0.5)`. The argument is the midpoint of the shared edge between two cells, so neighbors agree on the hash. The condition `dir == (dirs[i].y < dirs[i].x)` flips polarity for the shared edge, ensuring one side is a tab when the other is a blank. Adding `seed` to the hash input preserves this property as long as the same `seed` is added on both sides (it is, since both cells compute the same midpoint).
- **No FFT band per piece**: pieces are too small/numerous and too uniform to make a per-cell FFT mapping meaningful here. Modulation flows through the high-level params (`pieceCount`, `edgeLight`, `smoothness`, `seed`) rather than per-piece audio reactivity.
- **Performance**: 9 SDF evaluations per pixel, each with a 4-iter inner loop containing a hash and 1-2 distance evaluations. Comparable to LEGO Bricks' inner cost; should run real-time at 1080p on integrated GPUs.
- **Edge cases at screen border**: pieces extending past the screen are still evaluated by the 9-cell loop because we always check the cell containing `worldUV` plus its 8 neighbors. No special handling needed.
