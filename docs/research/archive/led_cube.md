# LED Cube

A rotating 3D grid of glowing LED points floating in space. A hot spot traces the cube's 12 edges in sequence, bringing nearby LEDs to life from FFT audio data -- low frequencies light up close LEDs, high frequencies reach further out. The grid breathes: tight crystalline lattice when quiet, loosening into organic drift when loud. Each point's glow spread is adjustable from sharp pinprick to soft halo.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (after drawables, before transforms)
- **Section Index**: 13 (Field)

## Attribution

- **Based on**: "LED Cube Visualizer" by orblivius
- **Source**: https://www.shadertoy.com/view/7fsXWj
- **License**: CC BY-NC-SA 3.0

## References

- Shadertoy "LED Cube Visualizer" by orblivius - complete working implementation of 3D LED grid with perspective projection, FFT audio reactivity, and edge-tracing hot spot

## Reference Code

```glsl
// LED Cube - Inside The Room + Volumetric Fog
// Fog march lifted from Eminance visualizer by Orblivius

const float PI  = acos(-1.);
const float side = 5.;
#define iBeatNum floor(iTime)

vec3 getCubeEdgePosition(float beat) {
    float e=mod(beat,12.),t=fract(beat);
    if(e<1.)  return vec3(t,0.,0.);  if(e<2.)  return vec3(1.,t,0.);
    if(e<3.)  return vec3(1.-t,1.,0.); if(e<4.)  return vec3(0.,1.-t,0.);
    if(e<5.)  return vec3(0.,0.,t);  if(e<6.)  return vec3(t,0.,1.);
    if(e<7.)  return vec3(1.,t,1.);  if(e<8.)  return vec3(1.-t,1.,1.);
    if(e<9.)  return vec3(0.,1.-t,1.); if(e<10.) return vec3(1.,0.,1.-t);
    if(e<11.) return vec3(1.,1.,t);  return vec3(0.,1.,1.-t);
}

void faceBasis(vec3 fwd, out vec3 rgt, out vec3 up) {
    vec3 tmp = abs(fwd.y)<0.9 ? vec3(0.,1.,0.) : vec3(1.,0.,0.);
    rgt = normalize(cross(tmp,fwd));
    up  = cross(fwd,rgt);
}

float gridMask(vec2 uv, float freq, float width) {
    vec2 f = abs(fract(uv*freq)-0.5);
    return 1.-smoothstep(0.,width,min(f.x,f.y));
}

vec3 retroPalette(float y, float h) {
    float t = clamp((y+h)/(2.*h),0.,1.);
    vec3 bot=vec3(0.85,0.0,0.9), mid=vec3(0.1,0.2,1.0), top=vec3(0.0,1.0,0.75);
    return t<0.5 ? mix(bot,mid,t*2.) : mix(mid,top,(t-.5)*2.);
}

// -- Fog helpers (from Eminance) --
float tri2(float x){ return abs(fract(x)-.5); }
vec3  tri32(vec3 p){ return abs(fract(p.zzy+abs(fract(p.yxx)-.5))-.5); }

float triNoise3D(vec3 p, float spd) {
    float z=1.4, rz=0.;
    vec3 bp=p;
    for(float i=0.;i<=2.;i++){
        vec3 dg=tri32(bp*2.);
        p+=dg+iTime*.3*spd;
        bp*=1.8; z*=1.5; p*=1.2;
        rz+=tri2(p.z+tri2(p.x+tri2(p.y)))/z;
        bp+=.14;
    }
    return rz;
}

vec3 tanh_approx(vec3 x){
    vec3 x2=x*x;
    return clamp(x*(27.+x2)/(27.+9.*x2),-1.,1.);
}

vec4 marchFog(vec3 ro, vec3 rd, float tStart, float tEnd, vec3 ledColor) {
    const int   FOG_STEPS = 32;
    const float FOG_SPEED = 0.02;
    const float NOISE_SCALE1 = 0.1;
    const float NOISE_SCALE2 = 0.3;
    const float THRESHOLD    = 0.35;
    const float DENSITY_POW  = 0.2;
    const float DENSITY_MUL  = 1.0;
    const float EDGE_SCALE   = 6.00;
    const float INT_SCALE    = 0.2;
    const float EDGE_FALLOFF = 0.9;
    const float EXTINCT      = 4.0;
    const float ACCUM_MUL    = 1.0;

    float stepSize = (tEnd - tStart) / float(FOG_STEPS);
    float vis      = 1.0;
    vec3  fogAccum = vec3(0.);

    for(int i=0; i<FOG_STEPS; i++) {
        float ft = tStart + (float(i)+0.5)*stepSize;
        vec3  fp = ro + rd*ft;
        vec3  np = fp*NOISE_SCALE1 + vec3(0.3,0.1,0.7)*iTime*FOG_SPEED;
        float n  = triNoise3D(np, 0.3);
        float n2 = tri2(n*3.1+fp.x*NOISE_SCALE2)*0.3;
        float nf = n + n2;
        float density = pow(max(nf - THRESHOLD, 0.), DENSITY_POW) * DENSITY_MUL;
        if(density > 0.001) {
            float edgeGlow = exp(-density * EDGE_FALLOFF) * density;
            float interior = smoothstep(0., 1.5, density);
            vec3 fogBase = vec3(0.02, 0.02, 0.05);
            vec3 fogCol  = fogBase + ledColor * (INT_SCALE * interior + EDGE_SCALE * edgeGlow);
            fogAccum += vis * fogCol * density * stepSize * ACCUM_MUL;
        }
        vis *= exp(-density * stepSize * EXTINCT);
        if(vis < 0.005) break;
    }
    return vec4(fogAccum, vis);
}

void mainImage(out vec4 o, vec2 fragCoord) {
    vec4  O    = vec4(0.,0.,0.,1.);
    float st   = 1./side;
    vec3  r    = iResolution;
    vec3  trot = iTime*.25 + vec3(0.,11.,33.);
    vec3  sp   = getCubeEdgePosition(iBeatNum/4.);

    const float BOX_HALF = 2.0;
    const float CAM_DIST = 1.8;
    const float CAM_Z    = -(BOX_HALF + CAM_DIST);

    vec2  uv    = (2.*fragCoord - r.xy)/r.x;
    vec3  camRo = vec3(0., 0., CAM_Z);
    const float FOV_DIV = 2.2;
    vec3  camRd = normalize(vec3(uv / FOV_DIV, 1.0));

    mat2 R1 = mat2(cos(trot.xyzx));
    mat2 R2 = mat2(sin(trot.xyzx + iTime*.5));

    // Box intersection
    vec3 t0   = (-BOX_HALF - camRo) / camRd;
    vec3 t1   = ( BOX_HALF - camRo) / camRd;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);
    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar  = min(min(tMax.x, tMax.y), tMax.z);

    if (tNear > tFar || tFar < 0.0) { o = vec4(0.,0.,0.,1.); return; }

    vec3 wallPt = camRo + camRd * tFar;

    vec3 absW = abs(wallPt) / BOX_HALF;
    vec3 lFaceN;
    if      (absW.x >= absW.y && absW.x >= absW.z) lFaceN = vec3(sign(wallPt.x),0.,0.);
    else if (absW.y >= absW.x && absW.y >= absW.z) lFaceN = vec3(0.,sign(wallPt.y),0.);
    else                                            lFaceN = vec3(0.,0.,sign(wallPt.z));

    vec3 lRgt, lUp;
    faceBasis(lFaceN, lRgt, lUp);

    vec2 wallCoord = vec2(dot(wallPt, lRgt), dot(wallPt, lUp));
    float gMask    = gridMask(wallCoord, 3.0, 0.03);
    vec3  baseColor= retroPalette(wallPt.y, BOX_HALF);

    const float DC_NEAR = BOX_HALF + CAM_DIST;
    const float DC_FAR  = BOX_HALF + CAM_DIST + BOX_HALF*2.;
    const float DC_DIM  = 0.12;
    float wallFade = mix(1.0, DC_DIM,
                    clamp((tFar - DC_NEAR) / (DC_FAR - DC_NEAR), 0., 1.));

    vec3 wFaceN = lFaceN;
    vec3 wRgt   = lRgt;
    vec3 wUp2   = lUp;
    float faceDepth = dot(wallPt, wFaceN);

    vec3 ledProjection = vec3(0.);

    vec3  toCenter  = normalize(vec3(.5) - sp);
    float invSqrt3  = 0.57735;

    for (float z=0.; z<1.; z+=st)
    for (float y=0.; y<1.; y+=st)
    for (float x=0.; x<1.; x+=st)
    {
        vec3 xyz = vec3(x,y,z);

        vec3  diff = xyz - sp;
        float d0   = length(diff);
        float df   = max(0., dot(diff/(d0+1e-5), toCenter));
        float ap   = clamp(d0*invSqrt3, 0., 1.);
        float as_= texture(iChannel0,vec2(ap,0.)).r;
        float s  = pow(2.*as_*(1.-ap)*4.,3.5)*4.5;
        s = s*(.5+(1.-df)); s=min(s,30.);
        if(d0<.25) s=max(s,37.);

        float ss = s*sin((x-.5)*(y-.5)*(z-.5))*.001;
        vec3  p  = xyz + vec3(ss-2./side);
        p.yx *= R1; p.zy *= R2;

        const float CUBE_SCALE = 4.5;
        p *= CUBE_SCALE;

        // Direct LED view
        vec3  pRel  = p - camRo;
        vec2  pProj = pRel.xy / (pRel.z * FOV_DIV);
        float d     = length(uv - pProj);
        vec3  c     = (xyz*s) / (2050.*d);
        c = c*pow(c,vec3(100.5*side/(s*s)));
        O.rgb += c;

        // Wall projection
        if(s > 0.8) {
        float pDepth    = dot(p, wFaceN);
        float pDepthAbs = max(abs(pDepth), 0.05);
        vec2  pScreen   = vec2(dot(p,wRgt), dot(p,wUp2));
        vec2  ledOnWall = pScreen * (faceDepth / pDepthAbs);
        vec2  delta     = ledOnWall - wallCoord;
        const float BARREL = 0.5;
        float r2 = dot(wallCoord,wallCoord) / (BOX_HALF*BOX_HALF);
        delta *= 1.0 + BARREL * r2;
        float dW   = length(delta);
        const float SPREAD = 0.5;
        float glow = s * exp(-(dW*dW) / (SPREAD*SPREAD));
        ledProjection += xyz * glow * glow * glow * xyz * .006;
        }
    }

    vec3 gridLayer = baseColor * 0.05 * gMask * wallFade;
    vec3 projLayer = ledProjection / (side*side*side) * 14.0 * wallFade;
    vec3 wallColor = gridLayer + .3*projLayer;

    vec4 fog = marchFog(camRo, camRd, tNear, tFar, O.rgb);
    float transmit = fog.a;

    vec3 sceneColor = wallColor * transmit + fog.rgb + O.rgb;
    o = vec4(1.0 - exp(-sceneColor * 1.1), 1.);
}
```

## Algorithm

### Keep verbatim

- `getCubeEdgePosition()` - the 12-edge tracing function. Maps a phase value to a position on the unit cube skeleton. Unchanged.
- Triple nested loop structure: `for z, for y, for x` iterating grid points
- Hot-spot distance mapping: `diff = xyz - sp; d0 = length(diff); ap = clamp(d0 * invSqrt3, 0.0, 1.0)` - `ap` is the normalized distance from hot spot, used as the shared `t` index for both gradient LUT and FFT frequency
- Directional emphasis: `df = max(0, dot(diff/(d0+1e-5), toCenter)); s = s * (0.5 + (1.0 - df))` - dims LEDs along the hot-spot-to-center axis, brightens LEDs off to the sides
- Perspective projection core: `pRel = p - camRo; pProj = pRel.xy / (pRel.z * fovDiv); d = length(uv - pProj)` - projects each 3D point to screen space
- `invSqrt3 = 0.57735` constant for normalizing distance within unit cube

### Replace

| Reference | Replacement | Reason |
|-----------|-------------|--------|
| `iTime` | `time` uniform | Standard codebase convention |
| `iBeatNum / 4.0` (beat-quantized tracer input) | `tracerPhase` uniform (CPU-accumulated from `tracerSpeed * deltaTime`) | Speed accumulated on CPU per conventions; smooth continuous tracer instead of beat-quantized |
| `iResolution` | `resolution` uniform | Standard |
| `side = 5.0` (hardcoded) | `gridSize` uniform (int, 3-10) | Adjustable density |
| `st = 1.0 / side` | `st = 1.0 / float(gridSize)` | Dynamic step from gridSize |
| `texture(iChannel0, vec2(ap, 0.0)).r` | Standard FFT lookup: `freq = baseFreq * pow(maxFreq / baseFreq, ap); bin = freq / (sampleRate * 0.5); energy = texture(fftTexture, vec2(bin, 0.5)).r` | Standard FFT frequency spread. `ap` (distance from hot spot) naturally maps near LEDs to low frequencies, far LEDs to high frequencies |
| `pow(2.0 * as_ * (1.0 - ap) * 4.0, 3.5) * 4.5` (brightness formula) | `brightness = baseBright + pow(clamp(energy * gain, 0.0, 1.0), curve) * (1.0 - ap)` | Standard audio gain/curve pipeline. The `(1.0 - ap)` distance falloff is kept from the original |
| `xyz * s` (position-based RGB color) | `texture(gradientLUT, vec2(ap, 0.5)).rgb * brightness` | Standard gradient LUT. Shared `t = ap` index links color and frequency |
| `(xyz*s) / (2050.*d); c*pow(c, vec3(100.5*side/(s*s)))` (glow formula) | `color * brightness * exp(-d * d / (glowFalloff * glowFalloff))` | Gaussian falloff with controllable spread. Cleaner and modulatable |
| `trot = iTime * 0.25 + vec3(0,11,33)` and `R1 = mat2(cos(...)); R2 = mat2(sin(...))` | Two CPU-accumulated phase uniforms `rotPhaseA`, `rotPhaseB`. In shader: `mat2 R1 = mat2(cos(rotPhaseA), -sin(rotPhaseA), sin(rotPhaseA), cos(rotPhaseA))` (and R2 similarly) | Rotation speed accumulated on CPU per conventions. Two independent rotation axes |
| `ss = s * sin((x-.5)*(y-.5)*(z-.5)) * .001` (subtle position wobble) | `vec3 disp = vec3(sin(xyz.y * 6.28 + time), cos(xyz.x * 6.28 + time * 0.7), sin(xyz.z * 6.28 + time * 1.3)) * displacement` | Grid displacement - coherent sin-wave offset per point. `displacement` parameter controls amplitude (0 = rigid lattice, 1 = loose drift) |
| `if(d0 < 0.25) s = max(s, 37.0)` (hotspot brightness override) | Remove. `baseBright` provides minimum LED visibility; the hot spot emphasis comes naturally from distance falloff | Simplifies; baseBright handles the same role |
| `CUBE_SCALE = 4.5` | `cubeScale` config field (float, 2.0-8.0, default 4.5) | Controls apparent size of the cube in the viewport |
| `FOV_DIV = 2.2` | `fovDiv` config field (float, 1.0-4.0, default 2.2) | Controls perspective distortion |
| `CAM_Z = -(BOX_HALF + CAM_DIST)` | Derive from `cubeScale` and `fovDiv`: camera positioned to frame the grid | Keep the framing relationship, derive from parameters |

### Remove

- Wall/room rendering: box intersection, `faceBasis`, `gridMask`, `retroPalette`, wall projection, barrel distortion, `wallFade`, `wallColor` - not part of the LED point cloud concept
- Volumetric fog: `marchFog`, `triNoise3D`, `tri2`, `tri32`, `tanh_approx` - separate concept, not adapted
- `1.0 - exp(-sceneColor * 1.1)` final tonemap - no tonemap in shaders per codebase conventions
- `min(s, 30.0)` brightness clamp - not needed with standard gain/curve pipeline

### Add

- Standard generator output: `gradientLUT` sampler, blend mode, blend intensity via ColorConfig
- `displacement` uniform (float, 0.0-1.0) for grid loosening
- `glowFalloff` uniform (float, 0.01-1.0) for adjustable point spread
- Behind-camera culling: skip points where `pRel.z < 0.1` (point behind camera plane)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gridSize | int | 3-10 | 5 | Points per axis (gridSize^3 total LEDs) |
| tracerSpeed | float | 0.1-4.0 | 1.0 | Edge tracer traversal speed (radians/sec, accumulated on CPU) |
| glowFalloff | float | 0.01-1.0 | 0.15 | Point glow spread - lower = sharper pinpricks, higher = soft halos |
| displacement | float | 0.0-1.0 | 0.0 | Grid loosening - 0 = rigid lattice, 1 = organic drift |
| cubeScale | float | 2.0-8.0 | 4.5 | Apparent size of the cube in viewport |
| fovDiv | float | 1.0-4.0 | 2.2 | Perspective field of view divisor (higher = flatter) |
| rotSpeedA | float | -PI..PI | 0.25 | Rotation speed on first axis (accumulated on CPU) |
| rotSpeedB | float | -PI..PI | 0.15 | Rotation speed on second axis (accumulated on CPU) |
| baseFreq | float | 27.5-440 | 55.0 | FFT low frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Audio gain |
| curve | float | 0.1-3.0 | 1.5 | Audio contrast curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum LED brightness |

## Modulation Candidates

- **glowFalloff**: Sharp pinpoints on quiet passages, blooming halos on choruses
- **displacement**: Tight crystalline grid when calm, loose organic drift when loud
- **tracerSpeed**: Slow contemplative sweep vs. fast rhythmic traversal
- **rotSpeedA / rotSpeedB**: Independent axis rotation responds to different energy bands
- **cubeScale**: Breathing cube that grows and shrinks with audio dynamics
- **gain**: Dynamic sensitivity response

### Interaction Patterns

- **displacement + glowFalloff** (character spectrum): These two axes define the entire visual identity. Tight grid + sharp glow = clean hardware LED cube installation. Loose grid + wide glow = amorphous firefly cloud. Modulating both in the same direction amplifies the transformation; opposing directions create tension (sharp points drifting loosely, or soft halos locked to a rigid grid).

- **tracerSpeed + displacement** (traveling wave): The edge tracer activates LEDs spatially. When displacement is also audio-driven, the "alive" region physically moves too -- bright LEDs displace more, creating a traveling wave of both light and motion that sweeps across the cube. Slow tracer + high displacement = a slow-motion explosion. Fast tracer + low displacement = a clean scanning beam.

- **gridSize + glowFalloff** (cascading threshold): At low density, individual LEDs are always readable regardless of glow width. Past a density threshold (~7-8), wide glow merges adjacent points into a continuous glowing volume -- the discrete lattice dissolves into a solid. This is a one-way gate: once density crosses the threshold, glow controls whether the volume is visible or not.

## Notes

- **Performance**: The triple loop runs `gridSize^3` iterations per pixel. At gridSize=5 (125 iterations), comparable to other generators. At gridSize=10 (1000 iterations), each iteration is lightweight (one projection + one Gaussian) but GPU occupancy may suffer. Consider capping at 8 in the UI tooltip.
- **Behind-camera points**: Points with `pRel.z < 0.1` are behind the camera or at the projection singularity. Must be culled to avoid artifacts.
- **Rotation matrix construction**: The reference uses `mat2(cos(trot.xyzx))` which is a GLSL shorthand constructing a 2x2 matrix from a swizzled vec4. The adaptation should use explicit `mat2(cos(a), -sin(a), sin(a), cos(a))` for clarity.
- **Edge tracer cycle**: `getCubeEdgePosition` traces 12 edges, so one full cycle = tracerPhase advancing by 12.0. At default tracerSpeed=1.0, one full cycle takes 12 seconds.
