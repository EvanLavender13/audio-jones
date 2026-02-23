# Prism Shatter

Dense crystalline geometry of sharp neon edges and dark faceted voids — like peering through a shattered prism refracting laser light. Camera drifts through infinite 3D crystal space while the structure animates independently.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after trail boost, before transforms)

## References

- User-provided Shadertoy shader (pasted in conversation)

## Reference Code

```glsl
#define IT 256
#define ITF float(IT)
#define STR 1.5/sqrt(ITF)
#define A 1.0

#define M_PI 3.14159254

vec3 colorFilter(vec3 c) {
    vec3 color;

    float g = (c.r+c.g+c.b)/3.0;
    float s = abs(c.r-g)+abs(c.g-g)+abs(c.b-g);

    color = c*s+(1.0-s)*(c-s);

    return color*color;
}

vec3 value(vec3 pos) {
    vec3 color;

    color.r = sin(pos.x);
    color.g = sin(pos.y);
    color.b = sin(pos.z);

    return color;
}

vec3 scan(vec3 pos, vec3 dir){
    vec3 c = vec3(0.5);
    for (int i=0; i<IT; i++) {
        float f = (1.0- float(i)/ITF)*STR;

        vec3 posMod = value(pos);
        vec3 newPos;
        newPos.x = posMod.y*posMod.z;
        newPos.y = posMod.x*posMod.z;
        newPos.z = posMod.x*posMod.y;

        c+=value(pos+newPos*4.0)*f;
        pos+=dir*2.0;
    }
    return colorFilter(c);
}

void mainImage(out vec4 fragColor,in vec2 uv){
    uv = (uv-iResolution.xy/2.0)/iResolution.y;
    float a = iTime*0.125;

    vec3 pos = vec3(cos(a/4.0+sin(a/2.0)),cos(a*0.2),sin(a*0.31)/4.5)*16.0;
    vec3 on = vec3(1.0,uv.x,uv.y);
    vec3 n;

    n = normalize(pos + vec3(cos(a*2.3),cos(a*2.61),cos(a*1.62)));
    vec3 crossRight = normalize( cross(n,vec3(0.0,0.0,1.0)));
    vec3 crossUp = normalize(cross(n, crossRight));
    n = n*1.5 + crossRight*uv.x + crossUp*uv.y;

    fragColor.rgb = scan(pos,normalize(n)).rgb;
}
```

## Algorithm

### Core Mechanism

Volumetric ray caster through a 3D sinusoidal color field with nonlinear displacement:

1. **Color field** — `value(pos)` maps 3D position to RGB via `sin(x), sin(y), sin(z)`. Infinite repeating color lattice.
2. **Displacement** — At each ray step, the sample position is displaced by cross-products of the sine values (`posMod.y * posMod.z`, etc.). Creates hard interference planes where sine axes intersect.
3. **Accumulation** — Ray steps accumulate color with linear falloff. Where interference aligns: bright saturated edges. Where it cancels: dark voids.
4. **Saturation filter** — Boosts saturated colors (edges), kills desaturated (voids), squares for contrast.

### AudioJones Adaptation

**Keep verbatim from reference:**
- `value()` function (sine color field)
- `scan()` ray march loop structure with cross-product displacement
- Camera orbit with Lissajous frequency ratios
- The core interference math that produces the crystalline edges

**Replace:**
- `colorFilter()` — replace with hue extraction + gradient LUT sampling + saturation-based brightness (see below)
- `iTime` — replace with CPU-accumulated `cameraTime` and `structureTime` uniforms
- `iResolution` — replace with `resolution` uniform
- Hardcoded constants (`IT`, `STR`, `4.0` displacement, `2.0` step size, `16.0` orbit radius, `1.5` focal length) — replace with config uniforms

**Gradient LUT coloring** — extract hue from accumulated RGB, use as gradient position:

```glsl
// After scan() accumulates raw c (BEFORE colorFilter):
float gray = (c.r + c.g + c.b) / 3.0;
float sat = abs(c.r - gray) + abs(c.g - gray) + abs(c.b - gray);
float hue = atan(c.g - c.b, c.r - (c.g + c.b) * 0.5) / (2.0 * PI) + 0.5;
float bright = pow(sat, saturationPower);
```

- `hue` (0-1) varies spatially — different facets land at different gradient positions, preserving multi-color stained-glass look
- `bright` from saturation^power — preserves sharp edges vs dark voids contrast
- `saturationPower` parameter controls edge-to-void contrast (higher = more extreme)

**FFT audio reactivity** — hue maps to frequency band so different facets react to different frequencies. Uses the exact codebase FFT bin pattern (from nebula.fs):

```glsl
float freq = baseFreq * pow(maxFreq / baseFreq, hue);
float bin = freq / (sampleRate * 0.5);
float energy = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
energy = pow(clamp(energy * gain, 0.0, 1.0), curve);
float audioBright = baseBright + energy;
```

**Final pixel:**
```glsl
vec3 color = texture(gradientLUT, vec2(hue, 0.5)).rgb * bright * audioBright * brightness;
```

**Structure animation** — add time offset to sine field for independent crystal animation:
```glsl
vec3 value(vec3 pos, float phase) {
    return vec3(sin(pos.x + phase), sin(pos.y + phase), sin(pos.z + phase));
}
```

**Camera and structure time** — both accumulated on CPU, sent as uniforms:
```cpp
e->cameraTime += cfg->cameraSpeed * deltaTime;
e->structureTime += cfg->structureSpeed * deltaTime;
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| displacementScale | float | 1.0-10.0 | 4.0 | Cross-product displacement magnitude. Higher = more angular/shattered |
| stepSize | float | 0.5-5.0 | 2.0 | Ray march step distance. Affects pattern density and depth |
| iterations | int | 64-256 | 128 | Ray march steps. Quality vs performance |
| cameraSpeed | float | -1.0-1.0 | 0.125 | Camera orbit speed through crystal space |
| structureSpeed | float | -1.0-1.0 | 0.0 | Crystal structure phase animation speed |
| orbitRadius | float | 4.0-32.0 | 16.0 | Camera distance from origin |
| fov | float | 0.5-3.0 | 1.5 | Field of view / focal length |
| brightness | float | 0.5-3.0 | 1.0 | Overall brightness multiplier |
| saturationPower | float | 1.0-4.0 | 2.0 | Gray suppression exponent. Higher = sharper edge/void contrast |
| baseFreq | float | 27.5-440 | 55.0 | Lowest FFT frequency mapped to hue |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT frequency mapped to hue |
| gain | float | 0.1-10.0 | 2.0 | FFT brightness amplification |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve exponent |
| baseBright | float | 0.0-1.0 | 0.3 | Minimum brightness without audio |

## Modulation Candidates

- **displacementScale**: Angular sharpness — low values smooth the facets into curves, high values create harder fractures
- **stepSize**: Pattern density — small steps = fine crystalline detail, large steps = coarser structure
- **cameraSpeed**: Orbit pacing — speed up during energetic passages, slow during calm
- **structureSpeed**: Crystal evolution — frozen geometry vs actively shifting planes
- **orbitRadius**: Zoom feel — closer = immersed in crystal detail, farther = wider view
- **fov**: Focal compression — narrow = telephoto flattening, wide = fisheye spread
- **brightness**: Global intensity
- **saturationPower**: Edge contrast ratio — low = softer transitions, high = stark edge/void separation

### Interaction Patterns

- **displacementScale x stepSize**: Displacement controls edge sharpness; step size controls how densely those edges pack. Low displacement + small steps = dense smooth mesh. High displacement + large steps = sparse jagged shards. Modulating both from different sources creates oscillation between dense-smooth and sparse-sharp.
- **cameraSpeed x structureSpeed**: Same direction = amplified apparent motion. Opposite directions = structure passing through the viewer. Both zero = frozen. Both peaking = maximum visual chaos.

## Notes

- 256 iterations per pixel is expensive. Default to 128. At 64 the geometry softens but remains recognizable. Consider half-resolution rendering (`EFFECT_FLAG_HALF_RES`) if performance is tight.
- The camera orbit uses irrational frequency ratios (`a/4+sin(a/2)`, `a*0.2`, `a*0.31`) so the path never exactly repeats.
- The effect is purely procedural — no input texture dependency.
- The `scan()` return must be captured BEFORE any colorFilter-like processing for hue extraction. The raw accumulated RGB has the spatial hue variation; processing it first would collapse the channel differences.
