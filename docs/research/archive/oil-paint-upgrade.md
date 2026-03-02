# Oil Paint Upgrade

Faithful port of flockaroo's multi-scale brush stroke algorithm. Fixes per-layer randomness, adds detail-aware stroke culling, corrects paint thickness distribution, and introduces canvas texture background using the shared noise texture utility.

## Classification

- **Category**: TRANSFORMS > Painterly (upgrade to existing `TRANSFORM_OIL_PAINT`)
- **Pipeline Position**: Same as current oil paint
- **Dependency**: Shared noise texture utility from pencil sketch upgrade (`src/render/noise_texture.h`)

## Attribution

- **Based on**: "oil paint brush" by flockaroo (Florian Berger)
- **Source**: https://www.shadertoy.com/view/MtKcDG
- **License**: CC BY-NC-SA 3.0

## References

- User-provided Shadertoy source code (pasted in conversation)

## Reference Code

### Buffer A — Stroke Pass

```glsl
// created by florian berger (flockaroo) - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// oil paint brush drawing

// calculating and drawing drawing the brush strokes
// ...reimplementation of shaderoo.org geometry version, but purely in fragment shader
// original geometry version of this: https://shaderoo.org/?shader=N6DFZT

#define COLORKEY_BG
#define QUALITY_PERCENT 85
//#define CANVAS

#define Res (iResolution.xy)
#define Res0 vec2(textureSize(iChannel0,0))
#define Res1 vec2(textureSize(iChannel1,0))

#define PI 3.1415927

#define N(v) (v.yx*vec2(1,-1))

vec4 getRand(vec2 pos)
{
    return textureLod(iChannel1,pos/Res1,0.);
}

vec4 getRand(int idx)
{
    ivec2 rres=textureSize(iChannel1,0);
    idx=idx%(rres.x*rres.y);
    return texelFetch(iChannel1,ivec2(idx%rres.x,idx/rres.x),0);
}

float SrcContrast = 1.4;
float SrcBright = 1.;

vec4 getCol(vec2 pos, float lod)
{
    // use max(...) for fitting full image or min(...) for fitting only one dir
    vec2 uv = (pos-.5*Res)*min(Res0.y/Res.y,Res0.x/Res.x)/Res0+.5;
    vec2 mask = step(vec2(-.5),-abs(uv-.5));
    vec4 col = clamp(((textureLod(iChannel0,uv,lod)-.5)*SrcContrast+.5*SrcBright),0.,1.)/**mask.x*mask.y*/;
    #ifdef COLORKEY_BG
    vec4 bg=textureLod(iChannel2,uv,lod+.7);
    col = mix(col,bg,dot(col.xyz,vec3(-.6,1.3,-.6)));
    #endif
    return col;
}

uniform float FlickerStrength;

vec3 getValCol(vec2 pos, float lod)
{
    return getCol(pos,1.5+log2(Res0.x/600.)).xyz;
    return getCol(pos,1.5+log2(Res0.x/600.)).xyz*.7+getCol(pos,3.5+log2(Res0.x/600.)).xyz*.3;
}

float compsignedmax(vec3 c)
{
    vec3 s=sign(c);
    vec3 a=abs(c);
    if (a.x>a.y && a.x>a.z) return c.x;
    if (a.y>a.x && a.y>a.z) return c.y;
    return c.z;
}

vec2 getGradMax(vec2 pos, float eps)
{
    vec2 d=vec2(eps,0);
    // calc lod according to step size
    float lod = log2(2.*eps*Res0.x/Res.x);
    //lod=0.;
    return vec2(
        compsignedmax(getValCol(pos+d.xy,lod)-getValCol(pos-d.xy,lod)),
        compsignedmax(getValCol(pos+d.yx,lod)-getValCol(pos-d.yx,lod))
        )/eps/2.;
}

vec2 quad(vec2 p1, vec2 p2, vec2 p3, vec2 p4, int idx)
{
    vec2 p[6] = vec2[](p1,p2,p3,p2,p4,p3);
    return p[idx%6];
}

float BrushDetail = 0.1;

float StrokeBend=-1.;
float BrushSize = 1.;

void mainImage( out vec4 fragColor, vec2 fragCoord )
{
    vec2 pos = fragCoord.xy;
    pos += 4.0*sin(iTime*.5*vec2(1,1.7))*iResolution.y/400.;

    float canv=0.;
    canv=max(canv,(getRand(pos*vec2(.7,.03).xy)).x);
    canv=max(canv,(getRand(pos*vec2(.7,.03).yx)).x);
    fragColor=vec4(vec3(.93+.07*canv),1);
    canv-=.5;

    int pidx0 = 0;

    vec3 brushPos;
    //int layerScalePercent = QUALITY_PERCENT;
    float layerScaleFact=float(QUALITY_PERCENT)/100.;
    float ls = layerScaleFact*layerScaleFact;
    //number of grid positions on highest detail level
    int NumGrid=int(float(0x10000/2)*min(pow(Res.x/1920.,.5),1.)*(1.-ls));
    //int NumGrid=10000;
    float aspect=Res.x/Res.y;
    int NumX = int(sqrt(float(NumGrid)*aspect)+.5);
    int NumY = int(sqrt(float(NumGrid)/aspect)+.5);
    //int pidx2 = NumX*NumY*4/3-pidx;
    int pidx2 /*= NumTriangles/2-pidx*/;
    // calc max layer NumY*layerScaleFact^maxLayer==10. - so min-scale layer has at least 10 strokes in y
    int maxLayer=int(log2(10./float(NumY))/log2(layerScaleFact));
    //maxLayer=8;
    for(int layer = min(maxLayer,11); layer>=0; layer--) // min(...) at beginning - possible crash cause on some systems?
    {
    int NumX2 = int(float(NumX) * pow(layerScaleFact,float(layer))+.5);
    int NumY2 = int(float(NumY) * pow(layerScaleFact,float(layer))+.5);

    // actually -2..2 would be needed, but very slow then...
    //for(int nx=-1;nx<=1;nx++)
    //for(int ny=-1;ny<=1;ny++)
    // replaced the 2 loops above by 1 loop and some modulo magic (possible crash cause on some systems?)
    for(int ni=0;ni<9;ni++)
    {
        int nx=ni%3-1;
        int ny=ni/3-1;
    // index centerd in cell
    int n0 = int(dot(floor(vec2(pos/Res.xy*vec2(NumX2,NumY2))),vec2(1,NumX2)));
    pidx2=n0+NumX2*ny+nx;
    int pidx=pidx0+pidx2;
    brushPos.xy = (vec2(pidx2%NumX2,pidx2/NumX2)+.5)/vec2(NumX2,NumY2)*Res;
    float gridW = Res.x/float(NumX2);
    float gridW0 = Res.x/float(NumX);
    // add some noise to grid pos
    brushPos.xy += gridW*(getRand(pidx+iFrame*123*0).xy-.5);
    // more trigonal grid by displacing every 2nd line
    brushPos.x += gridW*.5*(float((pidx2/NumX2)%2)-.5);

    vec2 g;
    g = getGradMax(brushPos.xy,gridW*1.)*.5+getGradMax(brushPos.xy,gridW*.12)*.5
        +.0003*sin(pos/Res*20.); // add some gradient to plain areas
    float gl=length(g);
    vec2 n = normalize(g);
    vec2 t = N(n);

    brushPos.z = .5;

    // width and length of brush stroke
    float wh = (gridW-.6*gridW0)*1.2;
    float lh = wh;
    float stretch=sqrt(1.5*pow(3.,1./float(layer+1)));
    wh*=BrushSize*(.8+.4*getRand(pidx).y)/stretch;
    lh*=BrushSize*(.8+.4*getRand(pidx).z)*stretch;
    float wh0=wh;

    wh/=1.-.25*abs(StrokeBend);

    wh = (gl*BrushDetail<.003/wh0 && wh0<Res.x*.02 && layer!=maxLayer) ? 0. : wh;

    vec2 uv=vec2(dot(pos-brushPos.xy,n),dot(pos-brushPos.xy,t))/vec2(wh,lh)*.5;
    // bending the brush stroke
    uv.x-=.125*StrokeBend;
    uv.x+=uv.y*uv.y*StrokeBend;
    uv.x/=1.-.25*abs(StrokeBend);
    uv+=.5;
    //float s=mix((uv.x-.4)/.6,1.-uv.x,step(.5,uv.x))*5.;
    float s=1.;
    s*=uv.x*(1.-uv.x)*6.;
    s*=uv.y*(1.-uv.y)*6.;
    float s0=s;
    s=clamp((s-.5)*2.,0.,1.);
    vec2 uv0=uv;

    // brush hair noise
    float pat = textureLod(iChannel1,uv*1.5*sqrt(Res.x/600.)*vec2(.06,.006),1.).x+textureLod(iChannel1,uv*3.*sqrt(Res.x/600.)*vec2(.06,.006),1.).x;
    vec4 rnd = getRand(pidx);

    s0=s;
    s*=.7*pat;
    uv0.y=1.-uv0.y;
    float smask=clamp(max(cos(uv0.x*PI*2.+1.5*(rnd.x-.5)),(1.5*exp(-uv0.y*uv0.y/.15/.15)+.2)*(1.-uv0.y))+.1,0.,1.);
    s+=s0*smask;
    s-=.5*uv0.y;
#ifdef CANVAS
    s+=(1.-smask)*canv*1.;
    s+=(1.-smask)*(getRand(pos*.7).z-.5)*.5;
#endif

    vec4 dfragColor;
    dfragColor.xyz = getCol(brushPos.xy,1.).xyz*mix(s*.13+.87,1.,smask)/**(.975+.025*s)*/;
    s=clamp(s,0.,1.);
    dfragColor.w = s * step(-0.5,-abs(uv0.x-.5)) * step(-0.5,-abs(uv0.y-.5));
    // do alpha blending
    fragColor = mix(fragColor,dfragColor,dfragColor.w);
    }
    pidx0+=NumX2*NumY2;
    }
}
```

### Image Pass — Relief Lighting

```glsl
// created by florian berger (flockaroo) - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// oil paint brush drawing

// original geometry version of this: https://shaderoo.org/?shader=N6DFZT

// some relief lighting

#define ImageTex iChannel0

#define Res  iResolution.xy
#define Res0 vec2(textureSize(iChannel0,0))
#define Res1 vec2(textureSize(iChannel1,0))
#define Res2 vec2(textureSize(iChannel2,0))
#define Res3 vec2(textureSize(iChannel3,0))

float getVal(vec2 uv)
{
    return length(textureLod(ImageTex,uv,.5+.5*log2(Res.x/1920.)).xyz)*1.;
}

vec2 getGrad(vec2 uv,float delta)
{
    vec2 d=vec2(delta,0);
    return vec2(
        getVal(uv+d.xy)-getVal(uv-d.xy),
        getVal(uv+d.yx)-getVal(uv-d.yx)
    )/delta;
}

float PaintSpec = .15;

float Vignette = 1.5;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/Res;
    vec3 n = vec3(getGrad(uv,1.0/iResolution.y),150.0);
    //n *= n;
    n=normalize(n);
    fragColor=vec4(n,1);
    vec3 light = normalize(vec3(-1,1,1.4));
    float diff=clamp(dot(n,light),0.,1.0);
    float spec=clamp(dot(reflect(light,n),vec3(0,0,-1)),0.0,1.0);
    spec=pow(spec,12.0)*PaintSpec;
    float sh=clamp(dot(reflect(light*vec3(-1,-1,1),n),vec3(0,0,-1)),0.0,1.0);
    sh=pow(sh,4.0)*.1;
    fragColor = texture(ImageTex,uv)*mix(diff,1.,.9)+spec*vec4(.85,1.,1.15,1.)+sh*vec4(.85,1.,1.15,1.);
    fragColor.w=1.;
    vec2 uv2 = (fragCoord-.5*Res)*min(Res2.y/Res.y,Res2.x/Res.x)/Res2+.5;
    vec4 col0 = texture(iChannel2,uv2);

    if(true)
    {
        vec2 scc=(fragCoord-.5*iResolution.xy)/iResolution.x;
        float vign = 1.1-Vignette*dot(scc,scc);
        vign*=1.-.7*Vignette*exp(-sin(fragCoord.x/iResolution.x*3.1416)*40.);
        vign*=1.-.7*Vignette*exp(-sin(fragCoord.y/iResolution.y*3.1416)*20.);
        fragColor.xyz *= vign;
    }
}
```

## Algorithm

Upgrade the existing two-pass oil paint effect to match the reference algorithm. Eight corrections, ordered by visual impact.

**Keep verbatim from reference:**
- Multi-scale grid layout with `layerScaleFact = 0.85`, `maxLayer` calculation, and `pidx0` accumulation
- `compSignedMax` gradient computation with dual-scale averaging (`gridW*1.0` and `gridW*0.12`)
- Stroke shape: width/length with `stretch = sqrt(1.5 * pow(3, 1/(layer+1)))` and random variation
- Bend math: `uv.x -= 0.125 * StrokeBend; uv.x += uv.y*uv.y * StrokeBend; uv.x /= 1.0 - 0.25*abs(StrokeBend)`
- `smask` with `uv0.y = 1.0 - uv0.y` flip, cosine + gaussian terms
- Alpha compositing with `step` mask and `mix`
- Relief lighting pass: gradient-derived normal, diffuse/specular/shadow with blue-tinted `vec3(0.85, 1.0, 1.15)` highlights

**Replace / adapt:**
- `iChannel0` → `texture0` (input texture)
- `iChannel1` → `texture1` (shared noise texture from `NoiseTextureGet()`, replacing the per-effect 256x256 noise)
- `Res0` → `textureSize(texture0, 0)` or passed as uniform where needed
- `Res1` → `textureSize(texture1, 0)` (noise texture size)
- `iTime` → `wobbleTime` uniform (CPU-accumulated, for optional canvas jitter)
- `getCol` contrast/brightness → expose `srcContrast` and `srcBright` as config params
- Reference `COLORKEY_BG` and `CANVAS` defines → omit (not applicable to this pipeline)
- Reference vignette in lighting pass → omit (project has its own vignette effect)
- Add attribution comment to shader header

### Fix 1: Per-layer random index offset

Current shader uses bare `pidx2` for `getRand()`. Reference accumulates `pidx0` across layers so each scale level gets unique random values:

```
int pidx0 = 0;
...
int pidx = pidx0 + pidx2;
getRand(pidx)    // unique per layer
...
pidx0 += numX2 * numY2;  // shift for next layer
```

All `getRand` calls (position jitter, stroke size randomization, smask phase) must use `pidx` not `pidx2`.

### Fix 2: Detail threshold for stroke culling

Add gradient-length gate to skip strokes in flat areas:

```
float gl = length(g);
...
float wh0 = wh;
wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer) ? 0.0 : wh;
```

Requires new `brushDetail` uniform. When gradient is too weak relative to stroke size, the stroke is culled. Avoids mechanical-looking strokes in uniform regions.

### Fix 3: UV y-flip before smask and thickness falloff

Reference copies UV to `uv0` and flips `uv0.y = 1.0 - uv0.y` before the smask calculation and the `s -= 0.5 * uv0.y` line. Current shader skips the flip, inverting paint thickness distribution along each stroke. The flip also affects the `step` mask which uses `uv0` coordinates.

### Fix 4: Width pre-compensation for bend

Reference adjusts `wh` before UV computation:

```
wh /= 1.0 - 0.25 * abs(StrokeBend);
```

This happens before `wh` is used in the UV calculation. Current shader only normalizes `uv.x` after bending but doesn't widen the stroke to compensate, making bent strokes too narrow.

### Fix 5: Gradient LOD resolution ratio

Change `log2(2.0 * eps)` to `log2(2.0 * eps * texRes.x / resolution.x)` where `texRes` is the source texture size. This selects the correct mip level for gradient computation when the source texture and screen resolutions differ.

### Fix 6: Color sampling at mip 1

Change stroke color sampling from `texture(texture0, brushPos / resolution)` to `textureLod(texture0, brushPos / resolution, 1.0)`. The slight pre-blur produces smoother, more painterly color dabs.

### Fix 7: Canvas background texture

Initialize output with a canvas texture pattern using the shared noise texture, sampled at two stretched scales:

```
float canv = 0.0;
canv = max(canv, textureLod(texture1, pos * vec2(0.7, 0.03) / noiseRes, 0.0).x);
canv = max(canv, textureLod(texture1, pos * vec2(0.03, 0.7) / noiseRes, 0.0).x);
fragOut = vec4(vec3(0.93 + 0.07 * canv), 1.0);
```

Gaps between strokes show canvas rather than the raw input image. Expose canvas visibility as a config param.

### Fix 8: Explicit mip 1 for brush hair noise

Change `texture(texture1, ...)` to `textureLod(texture1, ..., 1.0)` for the two bristle pattern samples. Mip 1 gives softer, more natural brush hair texture.

### Noise texture migration

Remove the per-effect 256x256 `GenImageColor` noise from `OilPaintEffectInit`. Replace with the shared `NoiseTextureGet()` from the noise texture utility (introduced by pencil sketch upgrade). Remove `noiseTex` field from `OilPaintEffect` struct — bind the shared texture in setup instead.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| brushSize | float | 0.5-3.0 | 1.0 | Stroke width relative to grid cell |
| strokeBend | float | -2.0-2.0 | -1.0 | Curvature bias follows or opposes gradient |
| brushDetail | float | 0.01-0.5 | 0.1 | Gradient threshold for stroke culling (lower = more strokes) |
| srcContrast | float | 0.5-3.0 | 1.4 | Source color contrast boost before painting |
| srcBright | float | 0.5-1.5 | 1.0 | Source brightness adjustment |
| canvasStrength | float | 0.0-1.0 | 0.5 | Canvas texture visibility in gaps between strokes |
| specular | float | 0.0-1.0 | 0.15 | Surface sheen from relief lighting |
| layers | int | 3-11 | 8 | Multi-scale layer count |

## Modulation Candidates

- **brushSize**: stroke scale reacts to audio energy
- **brushDetail**: threshold shift reveals/hides detail strokes
- **srcContrast**: dynamic color saturation of paint dabs
- **strokeBend**: curvature direction shifts with audio
- **canvasStrength**: canvas shows through more in quiet passages

### Interaction Patterns

**Cascading threshold — brushDetail gates stroke density**: High `brushDetail` culls all but the strongest-gradient strokes, leaving large gaps where canvas shows through. `canvasStrength` then determines whether those gaps feel like deliberate negative space or bare emptiness. One parameter controls how many strokes exist; the other controls what the absence looks like.

**Competing forces — srcContrast vs. brushDetail**: High `srcContrast` amplifies gradients, making more areas pass the detail threshold and produce strokes. High `brushDetail` raises the threshold, culling strokes. They push in opposite directions — modulating both creates a dynamic push-pull between dense impasto and sparse gestural painting.

## Notes

- The stroke pass is the performance bottleneck: `layers * 9` stroke evaluations per pixel, each with 4+ texture fetches for gradient computation. The `layers` param is the primary performance knob.
- Noise texture migration depends on the shared noise texture utility from the pencil sketch upgrade. If implementing oil paint first, the utility must be built first or the existing per-effect noise retained temporarily.
- Reference `COLORKEY_BG` (green-screen replacement) and `CANVAS` define (interleaved canvas texture within strokes) are omitted. Canvas background is implemented as a simpler gap-fill instead.
- Reference time-based jitter (`pos += sin(iTime*...)`) is omitted. The effect is applied as a post-process transform where temporal stability is preferred. Could be added as an optional `wobbleAmount` param later if desired.
