# Pencil Sketch Upgrade

Full-fidelity port of flockaroo's notebook drawings shader: gradient-aligned hatching strokes with colored tinting, noise-modulated pencil pressure, cubic contrast curve, and paper texture. Requires a new shared noise texture utility.

## Classification

- **Category**: TRANSFORMS > Painterly (upgrade to existing `TRANSFORM_PENCIL_SKETCH`)
- **Pipeline Position**: Same as current pencil sketch
- **New Shared Resource**: `src/render/noise_texture.h/.cpp` — 1024x1024 RGBA white noise, loaded once, available to any effect

## Attribution

- **Based on**: "notebook drawings" by flockaroo (Florian Berger)
- **Source**: https://www.shadertoy.com/view/XtVGD1
- **License**: CC BY-NC-SA 3.0

## References

- User-provided Shadertoy source code (pasted in conversation)

## Reference Code

```glsl
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// trying to resemle some hand drawing style


#define SHADERTOY
#ifdef SHADERTOY
#define Res0 iChannelResolution[0].xy
#define Res1 iChannelResolution[1].xy
#else
#define Res0 textureSize(iChannel0,0)
#define Res1 textureSize(iChannel1,0)
#define iResolution Res0
#endif

#define Res  iResolution.xy

#define randSamp iChannel1
#define colorSamp iChannel0


vec4 getRand(vec2 pos)
{
    return textureLod(iChannel1,pos/Res1/iResolution.y*1080., 0.0);
}

vec4 getCol(vec2 pos)
{
    // take aspect ratio into account
    vec2 uv=((pos-Res.xy*.5)/Res.y*Res0.y)/Res0.xy+.5;
    vec4 c1=texture(iChannel0,uv);
    vec4 e=smoothstep(vec4(-0.05),vec4(-0.0),vec4(uv,vec2(1)-uv));
    c1=mix(vec4(1,1,1,0),c1,e.x*e.y*e.z*e.w);
    float d=clamp(dot(c1.xyz,vec3(-.5,1.,-.5)),0.0,1.0);
    vec4 c2=vec4(.7);
    return min(mix(c1,c2,1.8*d),.7);
}

vec4 getColHT(vec2 pos)
{
     return smoothstep(.95,1.05,getCol(pos)*.8+.2+getRand(pos*.7));
}

float getVal(vec2 pos)
{
    vec4 c=getCol(pos);
     return pow(dot(c.xyz,vec3(.333)),1.)*1.;
}

vec2 getGrad(vec2 pos, float eps)
{
       vec2 d=vec2(eps,0);
    return vec2(
        getVal(pos+d.xy)-getVal(pos-d.xy),
        getVal(pos+d.yx)-getVal(pos-d.yx)
    )/eps/2.;
}

#define AngleNum 3

#define SampNum 16
#define PI2 6.28318530717959

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord+4.0*sin(iTime*1.*vec2(1,1.7))*iResolution.y/400.;
    vec3 col = vec3(0);
    vec3 col2 = vec3(0);
    float sum=0.;
    for(int i=0;i<AngleNum;i++)
    {
        float ang=PI2/float(AngleNum)*(float(i)+.8);
        vec2 v=vec2(cos(ang),sin(ang));
        for(int j=0;j<SampNum;j++)
        {
            vec2 dpos  = v.yx*vec2(1,-1)*float(j)*iResolution.y/400.;
            vec2 dpos2 = v.xy*float(j*j)/float(SampNum)*.5*iResolution.y/400.;
            vec2 g;
            float fact;
            float fact2;

            for(float s=-1.;s<=1.;s+=2.)
            {
                vec2 pos2=pos+s*dpos+dpos2;
                vec2 pos3=pos+(s*dpos+dpos2).yx*vec2(1,-1)*2.;
                g=getGrad(pos2,.4);
                fact=dot(g,v)-.5*abs(dot(g,v.yx*vec2(1,-1)))/**(1.-getVal(pos2))*/;
                fact2=dot(normalize(g+vec2(.0001)),v.yx*vec2(1,-1));

                fact=clamp(fact,0.,.05);
                fact2=abs(fact2);

                fact*=1.-float(j)/float(SampNum);
                col += fact;
                col2 += fact2*getColHT(pos3).xyz;
                sum+=fact2;
            }
        }
    }
    col/=float(SampNum*AngleNum)*.75/sqrt(iResolution.y);
    col2/=sum;
    col.x*=(.6+.8*getRand(pos*.7).x);
    col.x=1.-col.x;
    col.x*=col.x*col.x;

    vec2 s=sin(pos.xy*.1/sqrt(iResolution.y/400.));
    vec3 karo=vec3(1);
    karo-=.5*vec3(.25,.1,.1)*dot(exp(-s*s*80.),vec2(1));
    float r=length(pos-iResolution.xy*.5)/iResolution.x;
    float vign=1.-r*r*r;
    fragColor = vec4(vec3(col.x*col2*karo*vign),1);
    //fragColor=getCol(fragCoord);
}
```

## Algorithm

### Part 1: Shared Noise Texture Utility

**New files**: `src/render/noise_texture.h`, `src/render/noise_texture.cpp`

Generate a 1024x1024 RGBA white noise texture at startup using a PCG hash function instead of `rand()`. PCG hash produces high-quality uncorrelated values with no visible stripe artifacts:

```c
// PCG hash: fast, high-quality, no visible patterns
static uint32_t pcg_hash(uint32_t input) {
    uint32_t state = input * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
```

Each pixel seeded with `(y * 1024 + x)`, hashed 4 times with different seeds for independent R/G/B/A channels. Texture set to bilinear + wrap-repeat.

API:
- `NoiseTextureInit()` — generate and upload the texture. Called once at startup.
- `NoiseTextureGet()` — return the `Texture2D`. Any effect can grab it.
- `NoiseTextureUninit()` — unload at shutdown.

### Part 2: Pencil Sketch Shader Upgrade

Port the full flockaroo algorithm. Key adaptations from reference:

**Keep verbatim from reference:**
- Dual accumulation loop (`col` for darkness, `col2` for tinted color)
- `fact` / `fact2` gradient alignment math
- `getColHT` halftone thresholding with noise dithering
- Cubic contrast curve: `col.x = 1.0 - col.x; col.x *= col.x * col.x;`
- Noise-modulated stroke intensity: `col.x *= (0.6 + 0.8 * getRand(pos * 0.7).x)`
- Paper grid (`karo`) and vignette
- Normalization: `col /= float(sampleCount * angleCount) * 0.75 / sqrt(resolution.y)`
- Color sampling at `pos3` (perpendicular offset, 2x distance from gradient sample)

**Replace / adapt:**
- `iChannel0` → `texture0` (input texture, already bound by pipeline)
- `iChannel1` → `texture1` (shared noise texture, bound via `SetShaderValueTexture`)
- `Res0` / `Res1` → `resolution` uniform + `textureSize(texture1, 0)` for noise resolution
- `iTime` → `wobbleTime` (CPU-accumulated)
- `iResolution.y/400.0` → `resolution.y / 400.0` (same scale factor)
- Reference `getCol` desaturation/clamping: keep the algorithm but expose `desaturation` and `toneCap` as config params so the muting intensity is controllable
- `AngleNum` / `SampNum` → `angleCount` / `sampleCount` uniforms (already exist)

**New config fields (additions to `PencilSketchConfig`):**
- `float desaturation = 1.8f;` — strength of green-channel muting in getCol (0.0-3.0)
- `float toneCap = 0.7f;` — max brightness clamp, pencil tonal range (0.3-1.0)
- `float noiseInfluence = 0.8f;` — how much noise texture modulates stroke intensity (0.0-1.0)
- `float colorStrength = 1.0f;` — blend between monochrome (0) and tinted strokes (1) (0.0-1.0)

**Remove config fields:**
- `strokeFalloff` — reference uses fixed linear falloff `1.0 - j/sampleCount`; our parameterized version diverges from the algorithm. Remove it.

**New shader uniform:**
- `sampler2D texture1` — noise texture

**New effect struct field:**
- `int noiseTexLoc;` — uniform location for noise texture binding

**New C++ setup logic:**
- In `PencilSketchEffectSetup`: call `SetShaderValueTexture(e->shader, e->noiseTexLoc, NoiseTextureGet())` to bind the shared noise texture each frame

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| angleCount | int | 2-6 | 3 | Number of hatching directions |
| sampleCount | int | 8-24 | 16 | Samples per stroke / stroke length |
| gradientEps | float | 0.2-1.0 | 0.4 | Edge detection sensitivity |
| paperStrength | float | 0.0-1.0 | 0.5 | Grid paper texture visibility |
| vignetteStrength | float | 0.0-1.0 | 1.0 | Edge darkening |
| wobbleSpeed | float | 0.0-2.0 | 1.0 | Animation rate (0 = static) |
| wobbleAmount | float | 0.0-8.0 | 4.0 | Pixel displacement magnitude |
| desaturation | float | 0.0-3.0 | 1.8 | Color muting toward gray |
| toneCap | float | 0.3-1.0 | 0.7 | Max brightness (pencil tonal range) |
| noiseInfluence | float | 0.0-1.0 | 0.8 | Noise modulation of stroke intensity |
| colorStrength | float | 0.0-1.0 | 1.0 | Monochrome (0) to tinted strokes (1) |

## Modulation Candidates

- **wobbleAmount**: stroke jitter reacts to audio energy
- **noiseInfluence**: dynamic pencil pressure variation
- **desaturation**: shift between colorful and muted
- **toneCap**: dynamic tonal range compression
- **colorStrength**: fade between monochrome and color with audio
- **gradientEps**: edge sensitivity shift — tight edges vs. soft regions

### Interaction Patterns

**Cascading threshold — desaturation gates colorStrength**: When `desaturation` is high, the input is nearly gray, so `colorStrength` has little color to reveal. Modulating desaturation down opens up the color palette that colorStrength can then amplify. One parameter unlocks what the other controls.

**Competing forces — noiseInfluence vs. toneCap**: High `noiseInfluence` introduces bright random variation into strokes. Low `toneCap` compresses everything dark. They push in opposite directions — the visual tension determines whether the sketch feels clean or gritty.

## Notes

- The inner loop is `angleCount * sampleCount * 2` gradient evaluations per pixel, each requiring 5 texture samples (getVal calls getCol). At 3 angles x 16 samples = 96 gradient evals = 480 texture fetches per pixel. This is expensive. Consider documenting as a heavy effect.
- The noise texture is 1024x1024 RGBA = 4 MB VRAM. Negligible.
- PCG hash is deterministic — same noise every launch. This is fine (matches Shadertoy behavior).
- The shared noise texture utility is designed for future consumers (oil paint refactor, any stippling/dithering effects).
