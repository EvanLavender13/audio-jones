# Oil Paint Upgrade

Upgrade the existing two-pass oil paint effect to match flockaroo's multi-scale brush stroke algorithm. Applies 8 targeted fixes (per-layer randomness, detail-aware culling, UV flip, width pre-compensation, gradient LOD, color mip sampling, canvas background, brush hair mip), migrates to the shared noise texture, and exposes 4 new parameters (brushDetail, srcContrast, srcBright, canvasStrength). Also adds mipmap generation to the shared noise texture so `textureLod` at mip 1 produces softer brush hair.

**Research**: `docs/research/oil-paint-upgrade.md`

## Design

### Types

**OilPaintConfig** (modify `src/effects/oil_paint.h`):

```
struct OilPaintConfig {
  bool enabled = false;
  float brushSize = 1.0f;       // Stroke width relative to grid cell (0.5-3.0)
  float strokeBend = -1.0f;     // Curvature bias follows/opposes gradient (-2.0-2.0)
  float brushDetail = 0.1f;     // Gradient threshold for stroke culling (0.01-0.5)
  float srcContrast = 1.4f;     // Source color contrast boost (0.5-3.0)
  float srcBright = 1.0f;       // Source brightness (0.5-1.5)
  float canvasStrength = 0.5f;  // Canvas texture visibility in gaps (0.0-1.0)
  float specular = 0.15f;       // Surface sheen from relief lighting (0.0-1.0)
  int layers = 8;               // Multi-scale layer count (3-11)
};

#define OIL_PAINT_CONFIG_FIELDS \
  enabled, brushSize, strokeBend, brushDetail, srcContrast, srcBright, \
  canvasStrength, specular, layers
```

**OilPaintEffect** (modify `src/effects/oil_paint.h`):

Remove `noiseTex` field. Add uniform locations for 4 new stroke shader uniforms.

```
typedef struct OilPaintEffect {
  Shader strokeShader;
  Shader compositeShader;
  RenderTexture2D intermediate;

  // Stroke shader uniform locations
  int strokeResolutionLoc;
  int brushSizeLoc;
  int strokeBendLoc;
  int brushDetailLoc;      // NEW
  int srcContrastLoc;      // NEW
  int srcBrightLoc;        // NEW
  int canvasStrengthLoc;   // NEW
  int layersLoc;
  int noiseTexLoc;

  // Composite shader uniform locations
  int compositeResolutionLoc;
  int specularLoc;
} OilPaintEffect;
```

### Algorithm

#### Stroke Shader (`oil_paint_stroke.fs`)

Attribution header (before `#version`):

```glsl
// Based on "oil paint brush" by flockaroo (Florian Berger)
// https://www.shadertoy.com/view/MtKcDG
// License: CC BY-NC-SA 3.0 Unported
// Modified: adapted uniforms, removed COLORKEY_BG/CANVAS defines, added canvasStrength param
```

**Uniforms** (stroke shader):

```glsl
uniform sampler2D texture0;    // source image
uniform sampler2D texture1;    // shared noise texture (1024x1024 with mipmaps)
uniform vec2 resolution;       // render resolution (half-res in half-res pass)
uniform float brushSize;       // 0.5-3.0, default 1.0
uniform float strokeBend;      // -2.0-2.0, default -1.0
uniform float brushDetail;     // 0.01-0.5, default 0.1
uniform float srcContrast;     // 0.5-3.0, default 1.4
uniform float srcBright;       // 0.5-1.5, default 1.0
uniform float canvasStrength;  // 0.0-1.0, default 0.5
uniform int layers;            // 3-11, default 8
```

Derived sizes (no uniforms needed):

```glsl
vec2 texRes = vec2(textureSize(texture0, 0));
vec2 noiseRes = vec2(textureSize(texture1, 0));
```

**getCol** — source color with contrast/brightness:

```glsl
vec4 getCol(vec2 pos, float lod) {
    vec2 uv = pos / resolution;
    return clamp(((textureLod(texture0, uv, lod) - 0.5) * srcContrast + 0.5 * srcBright), 0.0, 1.0);
}
```

**getValCol** — value sampling for gradient computation at resolution-adapted LOD:

```glsl
vec3 getValCol(vec2 pos) {
    return getCol(pos, 1.5 + log2(texRes.x / 600.0)).xyz;
}
```

**getRand** — index-based noise lookup (wraps around texture dimensions):

```glsl
vec4 getRand(int idx) {
    ivec2 texSize = textureSize(texture1, 0);
    idx = idx % (texSize.x * texSize.y);
    return texelFetch(texture1, ivec2(idx % texSize.x, idx / texSize.x), 0);
}
```

**compSignedMax** — returns the component with largest absolute value, preserving sign:

```glsl
float compSignedMax(vec3 c) {
    vec3 a = abs(c);
    if (a.x > a.y && a.x > a.z) return c.x;
    if (a.y > a.x && a.y > a.z) return c.y;
    return c.z;
}
```

**getGradMax** — dual-scale gradient computation:

```glsl
vec2 getGradMax(vec2 pos, float eps) {
    vec2 d = vec2(eps, 0.0);
    return vec2(
        compSignedMax(getValCol(pos + d.xy) - getValCol(pos - d.xy)),
        compSignedMax(getValCol(pos + d.yx) - getValCol(pos - d.yx))
    ) / eps / 2.0;
}
```

Note: The reference code computes a LOD in `getGradMax` and passes it to `getValCol`, but `getValCol` ignores the parameter (dead code in the reference). Our version omits the dead variable. `getValCol` uses its own fixed LOD formula based on `texRes.x / 600.0`.

**main function** — complete revised flow:

```glsl
void main() {
    vec2 pos = fragTexCoord * resolution;

    // Fix 7: Canvas background texture
    float canv = 0.0;
    canv = max(canv, textureLod(texture1, pos * vec2(0.7, 0.03) / noiseRes, 0.0).x);
    canv = max(canv, textureLod(texture1, pos * vec2(0.03, 0.7) / noiseRes, 0.0).x);
    vec4 fragOut = vec4(vec3(0.93 + 0.07 * canv * canvasStrength), 1.0);

    const float layerScaleFact = 0.85;
    float ls = layerScaleFact * layerScaleFact;
    int numGrid = int(float(0x8000) * min(pow(resolution.x / 1920.0, 0.5), 1.0) * (1.0 - ls));
    float aspect = resolution.x / resolution.y;
    int numX = int(sqrt(float(numGrid) * aspect) + 0.5);
    int numY = int(sqrt(float(numGrid) / aspect) + 0.5);
    int maxLayer = min(layers, int(log2(10.0 / float(numY)) / log2(layerScaleFact)));
    float gridW0 = resolution.x / float(numX);
    float resScale = sqrt(resolution.x / 600.0);

    // Fix 1: per-layer random index offset
    int pidx0 = 0;

    for (int layer = maxLayer; layer >= 0; layer--) {
        int numX2 = int(float(numX) * pow(layerScaleFact, float(layer)) + 0.5);
        int numY2 = int(float(numY) * pow(layerScaleFact, float(layer)) + 0.5);
        for (int ni = 0; ni < 9; ni++) {
            int nx = ni % 3 - 1;
            int ny = ni / 3 - 1;
            int n0 = int(dot(floor(pos / resolution * vec2(numX2, numY2)), vec2(1, numX2)));
            int pidx2 = n0 + numX2 * ny + nx;

            // Fix 1: unique random per layer
            int pidx = pidx0 + pidx2;

            vec2 brushPos = (vec2(pidx2 % numX2, pidx2 / numX2) + 0.5)
                            / vec2(numX2, numY2) * resolution;
            float gridW = resolution.x / float(numX2);

            // Fix 1: use pidx for all getRand calls
            vec4 rand = getRand(pidx);
            brushPos += gridW * (rand.xy - 0.5);
            brushPos.x += gridW * 0.5 * (float((pidx2 / numX2) % 2) - 0.5);

            // Gradient orientation
            vec2 g = getGradMax(brushPos, gridW * 1.0) * 0.5
                   + getGradMax(brushPos, gridW * 0.12) * 0.5
                   + 0.0003 * sin(pos / resolution * 20.0);
            float gl = length(g);
            vec2 n = normalize(g);
            vec2 t = n.yx * vec2(1, -1);

            // Stroke shape
            float stretch = sqrt(1.5 * pow(3.0, 1.0 / float(layer + 1)));
            float wh = (gridW - 0.6 * gridW0) * 1.2;
            float lh = wh;
            wh *= brushSize * (0.8 + 0.4 * rand.y) / stretch;
            lh *= brushSize * (0.8 + 0.4 * rand.z) * stretch;
            float wh0 = wh;

            // Fix 4: width pre-compensation for bend
            wh /= 1.0 - 0.25 * abs(strokeBend);

            // Fix 2: detail threshold for stroke culling
            wh = (gl * brushDetail < 0.003 / wh0 && wh0 < resolution.x * 0.02 && layer != maxLayer) ? 0.0 : wh;

            vec2 uv = vec2(dot(pos - brushPos, n), dot(pos - brushPos, t))
                       / vec2(wh, lh) * 0.5;

            // Stroke bending
            uv.x -= 0.125 * strokeBend;
            uv.x += uv.y * uv.y * strokeBend;
            uv.x /= 1.0 - 0.25 * abs(strokeBend);
            uv += 0.5;

            // Alpha mask
            float s = 1.0;
            s *= uv.x * (1.0 - uv.x) * 6.0;
            s *= uv.y * (1.0 - uv.y) * 6.0;
            float s0 = s;
            s = clamp((s - 0.5) * 2.0, 0.0, 1.0);

            // Fix 3: copy uv to uv0 and flip y
            vec2 uv0 = uv;

            // Fix 8: brush hair noise at mip 1
            float pat = textureLod(texture1, uv * 1.5 * resScale * vec2(0.06, 0.006), 1.0).x
                      + textureLod(texture1, uv * 3.0 * resScale * vec2(0.06, 0.006), 1.0).x;

            s0 = s;
            s *= 0.7 * pat;

            // Fix 3: y-flip before smask and thickness falloff
            uv0.y = 1.0 - uv0.y;
            float smask = clamp(
                max(cos(uv0.x * 3.14159265 * 2.0 + 1.5 * (rand.x - 0.5)),
                    (1.5 * exp(-uv0.y * uv0.y / 0.0225) + 0.2) * (1.0 - uv0.y)) + 0.1,
                0.0, 1.0);
            s += s0 * smask;
            s -= 0.5 * uv0.y;
            s = clamp(s, 0.0, 1.0);

            // Fix 6: color sampling at mip 1
            vec3 strokeColor = getCol(brushPos, 1.0).xyz
                             * mix(s * 0.13 + 0.87, 1.0, smask);

            // Compositing
            float mask = s * step(-0.5, -abs(uv0.x - 0.5)) * step(-0.5, -abs(uv0.y - 0.5));
            fragOut = mix(fragOut, vec4(strokeColor, 1.0), mask);
        }

        // Fix 1: accumulate offset for next layer
        pidx0 += numX2 * numY2;
    }

    finalColor = fragOut;
}
```

#### Composite Shader (`oil_paint.fs`)

Minor change: use `textureLod` with resolution-adapted LOD for gradient sampling, matching the reference's `getVal`:

```glsl
float lod = 0.5 + 0.5 * log2(resolution.x / 1920.0);
float val_l = length(textureLod(texture0, uv - d.xy, lod).rgb);
float val_r = length(textureLod(texture0, uv + d.xy, lod).rgb);
float val_d = length(textureLod(texture0, uv - d.yx, lod).rgb);
float val_u = length(textureLod(texture0, uv + d.yx, lod).rgb);
```

No other changes to the composite shader.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| brushSize | float | 0.5-3.0 | 1.0 | yes | Brush Size |
| strokeBend | float | -2.0-2.0 | -1.0 | yes | Stroke Bend |
| brushDetail | float | 0.01-0.5 | 0.1 | yes | Brush Detail |
| srcContrast | float | 0.5-3.0 | 1.4 | yes | Contrast |
| srcBright | float | 0.5-1.5 | 1.0 | yes | Brightness |
| canvasStrength | float | 0.0-1.0 | 0.5 | yes | Canvas |
| specular | float | 0.0-1.0 | 0.15 | yes | Specular |
| layers | int | 3-11 | 8 | no | Layers |

### Uniform Binding Split

**OilPaintEffectSetup** (called once per frame from descriptor dispatch):
- Stroke shader: brushSize, strokeBend, brushDetail, srcContrast, srcBright, canvasStrength, layers, texture1 (noise)
- Composite shader: specular

**ApplyHalfResOilPaint** (called during render pass):
- Stroke shader: resolution (half-res)
- Composite shader: resolution (half-res)
- After rendering, restores full-res resolution on both shaders.
- Removes: brushSize, strokeBend, layers, noiseTex bindings (now in Setup)

### Constants

No new enum entries. Existing `TRANSFORM_OIL_PAINT` is unchanged.

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Update oil paint header

**Files**: `src/effects/oil_paint.h`
**Creates**: Updated config struct and effect struct that Wave 2 depends on

**Do**:

1. Add 4 new fields to `OilPaintConfig` with defaults and range comments as specified in the Types section above
2. Update `OIL_PAINT_CONFIG_FIELDS` macro to include all 8 fields (the serialization file references this macro, so updating it here is sufficient)
3. In `OilPaintEffect`: remove `Texture2D noiseTex` field, add 4 new `int` uniform location fields (`brushDetailLoc`, `srcContrastLoc`, `srcBrightLoc`, `canvasStrengthLoc`)
4. Change `OilPaintEffectSetup` signature to add `float deltaTime` parameter (not currently used but matches the standard effect Setup signature): `void OilPaintEffectSetup(OilPaintEffect *e, const OilPaintConfig *cfg, float deltaTime);`

**Verify**: `cmake.exe --build build` compiles (expect warnings about unused deltaTime — that's fine).

---

#### Task 1.2: Add mipmap generation to shared noise texture

**Files**: `src/render/noise_texture.cpp`

**Do**:

After creating the texture and setting filter/wrap, generate mipmaps so `textureLod` at mip 1+ works:

1. After `SetTextureWrap(noiseTexture, TEXTURE_WRAP_REPEAT);`, call `GenTextureMipmaps(&noiseTexture);`
2. Change `SetTextureFilter` from `TEXTURE_FILTER_BILINEAR` to `TEXTURE_FILTER_TRILINEAR` (enables mip-level interpolation)
3. Remove the manual `noiseTexture.mipmaps = 1;` line (GenTextureMipmaps sets this)

Follow this order: upload texture, set wrap, gen mipmaps, set filter. GenTextureMipmaps needs the texture uploaded first.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Update oil paint C++ module

**Files**: `src/effects/oil_paint.cpp`
**Depends on**: Wave 1 complete

**Do**:

1. Add `#include "render/noise_texture.h"` to includes

2. **Init**: Remove all noise texture generation code (GenImageColor, pixel loop, LoadTextureFromImage, UnloadImage, SetTextureFilter, SetTextureWrap for noiseTex). Cache 4 new uniform locations on strokeShader: `brushDetailLoc` → `"brushDetail"`, `srcContrastLoc` → `"srcContrast"`, `srcBrightLoc` → `"srcBright"`, `canvasStrengthLoc` → `"canvasStrength"`. Remove the NOLINTBEGIN/END block entirely.

3. **Setup**: Add `float deltaTime` parameter (unused, add `(void)deltaTime;`). Bind ALL non-resolution stroke shader uniforms: brushSize, strokeBend, brushDetail, srcContrast, srcBright, canvasStrength, layers. Bind noise texture on stroke shader via `SetShaderValueTexture(e->strokeShader, e->noiseTexLoc, NoiseTextureGet())`. Keep composite shader specular binding.

4. **Uninit**: Remove `UnloadTexture(e->noiseTex)` line.

5. **RegisterParams**: Add 4 new registrations:
   - `"oilPaint.brushDetail"`, range 0.01-0.5
   - `"oilPaint.srcContrast"`, range 0.5-3.0
   - `"oilPaint.srcBright"`, range 0.5-1.5
   - `"oilPaint.canvasStrength"`, range 0.0-1.0

6. **SetupOilPaint bridge**: Update to pass `pe->currentDeltaTime` as the third argument.

7. **UI** (`DrawOilPaintParams`): Add 4 new `ModulatableSlider` calls after the existing ones, before the Layers slider:
   - `"Brush Detail##oilpaint"` for brushDetail, param `"oilPaint.brushDetail"`, format `"%.2f"`
   - `"Contrast##oilpaint"` for srcContrast, param `"oilPaint.srcContrast"`, format `"%.2f"`
   - `"Brightness##oilpaint"` for srcBright, param `"oilPaint.srcBright"`, format `"%.2f"`
   - `"Canvas##oilpaint"` for canvasStrength, param `"oilPaint.canvasStrength"`, format `"%.2f"`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Upgrade stroke shader

**Files**: `shaders/oil_paint_stroke.fs`
**Depends on**: Wave 1 complete

**Do**: Replace the entire shader with the stroke shader algorithm from the Algorithm section above. All helper functions (`getRand`, `compSignedMax`, `getCol`, `getValCol`, `getGradMax`) and the complete `main` function are provided there. Key changes from current:

1. Add attribution comment block before `#version 330` line
2. Add 4 new uniforms: `brushDetail`, `srcContrast`, `srcBright`, `canvasStrength`
3. Add `getCol` function with contrast/brightness
4. Replace `getVal` with `getValCol` (resolution-adapted LOD, no lod parameter)
5. Remove dead `lod` variable from `getGradMax` (reference code computes it but `getValCol` ignores it)
6. Fix 7: Canvas background initialization before layer loop
7. Fix 1: `pidx0` accumulator and `pidx = pidx0 + pidx2` for all `getRand` calls
8. Fix 2: Detail threshold after `wh0` save, before UV computation
9. Fix 4: `wh /= 1.0 - 0.25 * abs(strokeBend)` before UV computation
10. Fix 3: `uv0 = uv`, `uv0.y = 1.0 - uv0.y` before smask, use `uv0` for smask, thickness falloff, and step mask
11. Fix 8: `textureLod(..., 1.0)` for brush hair noise
12. Fix 6: Color sampling via `getCol(brushPos, 1.0)` instead of `texture(texture0, ...)`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Update composite shader

**Files**: `shaders/oil_paint.fs`
**Depends on**: Wave 1 complete

**Do**: Change the 4 gradient samples from `texture()` to `textureLod()` with resolution-adapted LOD:

1. Add `float lod = 0.5 + 0.5 * log2(resolution.x / 1920.0);` after the `delta`/`d` declarations
2. Replace each `texture(texture0, uv ± d.xy/yx)` with `textureLod(texture0, uv ± d.xy/yx, lod)`
3. Add attribution comment block before `#version 330` (same source as stroke shader)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Slim down ApplyHalfResOilPaint

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Do**: Since `OilPaintEffectSetup` now binds all non-resolution uniforms, strip `ApplyHalfResOilPaint` to resolution management only:

1. Remove these lines (Setup now handles them):
   - `SetShaderValue(...brushSizeLoc...)`
   - `SetShaderValue(...strokeBendLoc...)`
   - `SetShaderValue(...layersLoc...)`
   - `SetShaderValueTexture(...noiseTexLoc...noiseTex)`
   - `SetShaderValue(...specularLoc...)` (the one in the half-res block)

2. Keep these lines (resolution depends on half-res pass geometry):
   - `SetShaderValue(...strokeResolutionLoc, halfRes...)` — before stroke draw
   - `SetShaderValue(...compositeResolutionLoc, halfRes...)` — before composite draw
   - `SetShaderValue(...strokeResolutionLoc, fullRes...)` — after (restore)
   - `SetShaderValue(...compositeResolutionLoc, fullRes...)` — after (restore)

3. Remove the `const OilPaintConfig *op = &pe->effects.oilPaint;` local since it's no longer used.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Oil paint effect renders with visible brush strokes
- [x] Brush Detail slider culls strokes in flat areas at high values
- [x] Contrast and Brightness sliders visibly affect paint dab colors
- [x] Existing presets with oil paint load correctly (new fields get defaults)
- [x] Specular still produces surface sheen on relief lighting pass

---

## Implementation Notes

**canvasStrength parameter removed.** The canvas texture effect (0.93 + 0.07 range) was imperceptible at runtime — only 7% brightness variation, and only visible in gaps between strokes which are rare at default layer count. The canvas background is now hardcoded at full strength in the shader. The `canvasStrength` field, uniform, param registration, and UI slider were all removed.

**File-scope `textureSize()` is invalid in GLSL 330.** The plan placed `vec2 texRes = vec2(textureSize(texture0, 0))` at file scope. Sampler-dependent functions cannot be evaluated as global initializers — this caused a silent shader compilation failure (all-white output). Fix: `noiseRes` moved into `main()`, `texRes` computed locally inside `getValCol()`.

**`SetupOilPaint` must be called explicitly before `ApplyHalfResOilPaint`.** Oil paint bypasses `RenderPass` (which normally invokes the setup callback), so the render pipeline must call `entry.setup(pe)` before `ApplyHalfResOilPaint`. Without this, all non-resolution uniforms (brushSize, noise texture, etc.) are unbound and the shader produces white output.

**Full-res rendering tested, kept half-res.** `ApplyFullResOilPaint` was implemented and tested. Visual difference was marginal at the cost of significantly more GPU work. Half-res retained. The full-res function remains in `shader_setup.cpp` but is unused.
