# Plaid

Woven tartan fabric with colored bands crossing at right angles, visible twill thread texture at the intersections, and FFT-driven per-band glow. The pattern breathes as band widths morph continuously over time — wide ground bands pulse to bass, thin accent stripes flicker to treble.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after trail boost, before transforms)
- **Files**: `src/effects/plaid.cpp`, `src/effects/plaid.h`, `shaders/plaid.fs`, `src/ui/imgui_effects_gen_texture.cpp`

## References

- User-provided "Plaid forever" Shadertoy — full tartan weave with band generation, warp/weft interleaving, and twill pattern (primary reference)
- User-provided "Simple Plaid" Shadertoy — smoothstep-based band functions with stipple texture
- User-provided "Tartan Plaid" Shadertoy — coordinate grid overlay approach with layered mix blending

## Reference Code

### "Plaid forever" (primary — complete tartan generation)

```glsl
// smaller means more zoomed in
#define pattern_scale 1.5

// how detailed the threading is.
#define thread_detail 256.0

float hash(float x) {
    return fract(sin(x * 91.3458) * 47453.5453);
}

// --- Dynamic plaid generation ---
#define MAX_BANDS 16
void generatePlaidBands(float seed,
                        out vec3 bandColors[MAX_BANDS],
                        out float bandWidths[MAX_BANDS],
                        out int bandCount) {
    // Realistic tartan base palette (darker tones)
    vec3 palette[6];
    palette[0] = vec3(0.1, 0.1, 0.2);   // Dark navy
    palette[1] = vec3(0.3, 0.2, 0.1);   // Brown
    palette[2] = vec3(0.2, 0.3, 0.2);   // Forest green
    palette[3] = vec3(0.5, 0.1, 0.1);   // Maroon
    palette[4] = vec3(0.4, 0.4, 0.6);   // Muted purple-blue
    palette[5] = vec3(0.6, 0.6, 0.6);   // Gray

    // Bright accent colors (thin only)
    vec3 accentPalette[3];
    accentPalette[0] = vec3(1.0, 0.9, 0.2);   // Light yellow
    accentPalette[1] = vec3(0.9, 0.8, 0.6);   // Beige
    accentPalette[2] = vec3(0.7, 0.7, 0.7);   // Light gray

    int baseCount = 4 + int(mod(floor(seed * 10.0), 3.0));

    bool hasThinBand = false;
    bool hasAccent = false;

    for (int i = 0; i < baseCount; i++) {
        float rand = fract(sin(seed * 57.0 + float(i) * 4.5) * 43758.5453);

        vec3 baseColor;
        float width;

        bool injectAccent = !hasAccent && rand < 0.4;

        if (injectAccent) {
            int accentIndex = int(mod(floor(seed * 123.4 + float(i) * 6.7), 3.0));
            baseColor = accentPalette[accentIndex];
            width = rand < 0.2 ? 0.5 : 1.0;

            hasAccent = true;
            hasThinBand = true;
        } else {
            int colorIndex = int(mod(floor(seed * 100.0 + float(i) * 7.3), 6.0));
            vec3 darkBase = palette[colorIndex];

            float tint = 0.92 + 0.16 * fract(sin(seed * 99.0 + float(i) * 8.1) * 54321.123);
            baseColor = clamp(darkBase * tint, 0.0, 1.0);

            if (!hasThinBand && rand < 0.4) {
                width = rand < 0.2 ? 0.5 : 1.0;
                hasThinBand = true;
            } else {
                width = 2.5 + rand * 3.5;
            }
        }

        bandColors[i] = baseColor;
        bandWidths[i] = width;
    }

    if (!hasThinBand) {
        int thinIndex = int(mod(seed * 13.3, float(baseCount)));
        bandWidths[thinIndex] = 0.5;
    }

    for (int i = 0; i < baseCount; i++) {
        bandColors[baseCount + i] = bandColors[baseCount - 1 - i];
        bandWidths[baseCount + i] = bandWidths[baseCount - 1 - i];
    }

    bandCount = baseCount * 2;
}

vec3 getBandColor(float pos,
                  vec3 colors[MAX_BANDS],
                  float widths[MAX_BANDS],
                  int count) {
    float total = 0.0;
    for (int i = 0; i < count; i++) {
        total += widths[i];
    }

    float localPos = mod(pos, total);
    float cursor = 0.0;

    for (int i = 0; i < count; i++) {
        float w = widths[i];
        if (localPos < cursor + w) return colors[i];
        cursor += w;
    }

    return vec3(0., 0.0, 0.);
}

vec3 blendThreads(vec3 top, vec3 bottom, vec2 uv, int index) {
    return mix(bottom, top, 0.75);
}

vec3 generatePlaid(vec2 uv, int plaidIndex) {
    uv *= pattern_scale;

    float rawSeed = float(plaidIndex + 1) * 3.14159 * 193.1337 + 49.0;
    float seed = fract(sin(rawSeed) * 43758.5453);

    vec3 bandColors[MAX_BANDS];
    float bandWidths[MAX_BANDS];
    int bandCount;

    generatePlaidBands(seed, bandColors, bandWidths, bandCount);

    float totalWidth = 0.0;
    for (int i = 0; i < bandCount; i++) {
        totalWidth += bandWidths[i];
    }

    float warpPos = mod(uv.x * totalWidth, totalWidth);
    float weftPos = mod(uv.y * totalWidth, totalWidth);

    vec3 warpColor = getBandColor(warpPos, bandColors, bandWidths, bandCount);
    vec3 weftColor = getBandColor(weftPos, bandColors, bandWidths, bandCount);

    uv *= thread_detail;

    float twillRepeat = 4.0;
    float flipDiag = hash(float(plaidIndex)) < 0.5 ? 1.0 : -1.0;
    float i = mod(floor(uv.x), twillRepeat);
    float j = mod(floor(uv.y*flipDiag), twillRepeat);
    bool warpOnTop = mod(i + j, twillRepeat) < 2.0;

    return warpOnTop
        ? blendThreads(warpColor, weftColor, uv, plaidIndex)
        : blendThreads(weftColor, warpColor, uv, plaidIndex);
}
```

### "Simple Plaid" (smoothstep band functions)

```glsl
float stipple(float c) {
   return clamp(0.0, 1.0, sin(c * 3.14 * 190.0)+.7);
}

float twoband(vec2 p, float s, float width, float gap) {
    float hw = width / 2.0;
    float hg = gap / 2.0;
    float c = abs(p.x) - hg;
    return smoothstep( hw+s, hw, abs(c) );
}

float threeband(vec2 p, float s, float width, float gap1, float gap2) {
    float hw = width / 2.0;
    float hg1 = gap1 / 2.0;
    float hg2 = gap2 / 2.0;
    float c = abs(p.x) - hg1;
    c = abs(c) - hg2;
    return smoothstep( hw+s, hw, abs(c) );
}
```

## Algorithm

### Adaptation from reference

**Keep from "Plaid forever":**
- Band-based sett structure with mirrored symmetry (bands go 0,1,...,N-1,N-1,...,1,0)
- Warp (vertical) / weft (horizontal) color lookup via cumulative band widths
- Twill thread-over-thread pattern: `mod(floor(uv.x) + floor(uv.y), twillRepeat) < twillRepeat/2`
- `blendThreads()` mix: top thread dominates at 75%, bottom thread shows through at 25%
- `getBandColor()` cursor walk to find which band a position falls in

**Replace:**
- Hardcoded color palettes and seed-based random generation → gradient LUT sampling. Each band `i` samples `texture(gradientLUT, vec2(float(i) / float(bandCount - 1), 0.5)).rgb`
- Static seed-generated widths → continuous morphing. Each band's width oscillates: `baseWidth + sin(time * morphSpeed + float(i) * phaseSpread) * morphAmount`. Ground bands use wider baseWidth, accent bands use narrow baseWidth.
- `iTime`-based pattern transitions → continuous parameter animation
- Fixed `pattern_scale` and `thread_detail` defines → uniform params

**Add:**
- FFT per-band brightness: each band maps to a frequency in `baseFreq → maxFreq` log space. Band brightness = `baseBright + pow(fftEnergy * gain, curve)`.
- Band width classes: alternating wide/thin pattern controlled by `accentWidth` ratio. Every Nth band is thin (accent), rest are wide (ground). This creates the classic tartan look of a few dominant colors separated by thin pinstripes.

### Per-pixel evaluation

1. Center UV on screen center, apply aspect correction and scale
2. Compute band widths array: N bands with time-morphing widths, mirrored into 2N total
3. Cumulative sum for total sett width
4. For X position: `mod(uv.x, totalWidth)` → cursor walk → warp band index + color from LUT
5. For Y position: `mod(uv.y, totalWidth)` → cursor walk → weft band index + color from LUT
6. Twill: `floor(uv * threadDetail)` → `mod(ix + iy, twillRepeat) < twillRepeat/2` determines warp-on-top
7. Blend: `mix(bottomColor, topColor, 0.75)`
8. FFT brightness: multiply final color by per-band brightness (use the band that's on top)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| scale | float | 0.5-8.0 | 2.0 | Tiles per screen — zoom level |
| bandCount | int | 3-8 | 5 | Unique bands per sett half (doubled by mirror) |
| accentWidth | float | 0.05-0.5 | 0.15 | Thin accent stripe width relative to wide bands |
| threadDetail | float | 16.0-512.0 | 128.0 | Twill texture fineness (thread density) |
| twillRepeat | int | 2-8 | 4 | Twill over/under repeat count |
| morphSpeed | float | 0.0-2.0 | 0.3 | Band width oscillation speed |
| morphAmount | float | 0.0-1.0 | 0.3 | Strength of band width morphing |
| glowIntensity | float | 0.0-2.0 | 1.0 | Overall output brightness |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.3 | Minimum band brightness without audio |

## Modulation Candidates

- **scale**: zooming in/out shifts between seeing individual threads and seeing the overall plaid pattern
- **bandCount**: changing band count restructures the entire plaid (integer steps create dramatic shifts)
- **accentWidth**: thin→thick accent stripes changes the pattern from pinstripe tartan to bold checker
- **threadDetail**: controls how visible the weave texture is vs smooth flat bands
- **morphSpeed**: controls breathing rate of the band width oscillation
- **morphAmount**: 0 = static traditional tartan, 1 = constantly shifting widths
- **glowIntensity**: overall brightness control

### Interaction Patterns

**Cascading threshold — accentWidth x FFT energy**: Thin accent stripes map to treble frequencies. At low `accentWidth` (thin), they're barely visible even at full brightness. Widening `accentWidth` while treble energy is high creates bright pinstripe flashes that disappear during quiet passages. Wide ground bands only glow during bass-heavy sections.

**Competing forces — morphAmount x scale**: At large scale (zoomed out) with high morphAmount, the bands visibly breathe and shift. At small scale (zoomed in), the same morphing is invisible because individual threads dominate the visual. Modulating both creates tension between macro-pattern dynamics and micro-texture stability.

**Resonance — threadDetail x twillRepeat**: Certain combinations of thread density and twill repeat create resonant moire patterns at the weave intersections. Low twillRepeat with high threadDetail produces fine diagonal hatching; high twillRepeat with moderate threadDetail creates bold chevron-like weave texture.

## Notes

- Band count is an int param (use `ModulatableSliderInt` in UI) — fractional values would create partial bands
- twillRepeat is also int — the weave pattern must tile cleanly
- At very low threadDetail (16), the twill texture looks like large diagonal blocks rather than fabric threads — this is a valid artistic look but not realistic tartan
- The mirrored sett structure means total visible bands = `bandCount * 2`, so bandCount=5 produces a 10-band repeating unit
- Performance: the cursor walk loop in `getBandColor` runs per-pixel but is bounded by MAX_BANDS (16 after mirroring) — negligible cost
