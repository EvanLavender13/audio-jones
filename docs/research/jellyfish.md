# Jellyfish

A drifting swarm of glowing jellyfish suspended in a deep blue water column. Bell bodies pulse in slow rhythm, ringed by soft halos and trailing a fan of tapered tentacles whose tips wave in seeded sine drift. Marine snow particles twinkle and rise upward through the column. A caustic shimmer plays across mid-depths like sunlight refracted through unseen ripples. Each jellyfish samples its hue from the active gradient at a hashed seed, so the swarm reads as a varied species rather than a uniform color.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Output stage > Generators (renders before transforms, blends via standard compositor)

## Attribution

- **Based on**: "Bioluminescent Deep Ocean" by hagy
- **Source**: https://www.shadertoy.com/view/7c2Xz3
- **License**: CC BY-NC-SA 3.0 (Shadertoy default)

## References

- [Bioluminescent Deep Ocean by hagy](https://www.shadertoy.com/view/7c2Xz3) - Complete reference shader; jellyfish SDF construction, marine snow cell drift, Worley caustics, depth-gradient backdrop.

## Reference Code

```glsl
float hash21(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

mat2 rot2D(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}


float valueNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash21(i),                hash21(i + vec2(1.0, 0.0)), u.x),
        mix(hash21(i + vec2(0.0,1.0)),hash21(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 r = rot2D(0.5);
    for (int i = 0; i < 5; i++) {
        v += a * valueNoise(p);
        p = r * p * 2.0;
        a *= 0.5;
    }
    return v;
}

vec2 worley(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    float d1 = 1e10, d2 = 1e10;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 nb = vec2(float(x), float(y));
            vec2 df = nb + hash22(i + nb) - f;
            float d = dot(df, df);
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
    }
    return sqrt(vec2(d1, d2));
}

float gerstnerH(vec2 p, vec2 dir, float amp, float freq, float spd, float t) {
    return amp * sin(freq * dot(dir, p) - spd * t);
}

float surfaceH(vec2 p, float t) {
    float h = 0.0;
    h += gerstnerH(p, normalize(vec2( 1.0,  0.8)), 0.06, 2.0, 1.2, t);
    h += gerstnerH(p, normalize(vec2(-0.5,  1.0)), 0.04, 3.2, 1.5, t);
    h += gerstnerH(p, normalize(vec2( 0.3, -0.7)), 0.03, 4.8, 1.8, t);
    h += gerstnerH(p, normalize(vec2(-0.8, -0.3)), 0.02, 6.0, 2.0, t);
    return h;
}


float caustics(vec2 p, float t) {
    vec2 w1 = worley(p * 3.2 + vec2( 0.7, 0.3) + t * 0.15);
    vec2 w2 = worley(p * 2.1 + vec2( 1.4, 0.9) - t * 0.09);
    return pow(1.0 - w1.x, 4.0) * 0.55 + pow(1.0 - w2.x, 3.0) * 0.45;
}


float sdEllipse(vec2 p, vec2 ab) {
    float k = length(p / ab);
    return (k - 1.0) * min(ab.x, ab.y);
}

float sdSeg(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}


float smax(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return max(a, b) + h * h * k * 0.25;
}


vec2 jellyPath(float s, float t) {
    float ax = 0.09 + hash21(vec2(s, 1.5)) * 0.07;
    float ay = 0.06 + hash21(vec2(s, 1.6)) * 0.05;
    float fx = 0.22 + hash21(vec2(s, 1.1)) * 0.16;
    float fy = 0.17 + hash21(vec2(s, 1.2)) * 0.12;
    float px = hash21(vec2(s, 1.3)) * 6.283;
    float py = hash21(vec2(s, 1.4)) * 6.283;
    return vec2(ax * sin(t * fx + px), ay * sin(t * fy + py));
}

vec3 jellyfish(vec2 uv, vec2 ctr, float s, vec3 hue, float phase, float t) {
    float sz    = 0.052 + hash21(vec2(s, 9.2)) * 0.026;
    float pSpd  = 0.55  + hash21(vec2(s, 9.1)) * 0.35;

    float pulse = 1.0 + 0.14 * sin(t * pSpd * 3.14159 + phase);
    float pz    = sz * pulse;
    vec2  pos   = ctr + jellyPath(s, t);
    vec2  lp    = uv - pos;


    float bellBody = sdEllipse(lp, vec2(pz * 0.76, pz * 0.38));
    float bellD    = smax(bellBody, lp.y + pz * 0.07, pz * 0.10);

    float cavD = sdEllipse(lp - vec2(0.0, pz * 0.10), vec2(pz * 0.46, pz * 0.29));


    float tentD = 100.0;
    for (int i = 0; i < 8; i++) {
        float fi   = float(i);
        float ang  = (fi / 7.0 - 0.5) * 1.5;
        float tLen = (0.05 + hash21(vec2(s, fi + 20.0)) * 0.04) * 1.5;
        vec2  prev = pos + vec2(sin(ang) * pz * 0.65, -pz * 0.06);
        for (int j = 0; j < 4; j++) {
            float fj  = float(j);
            float wX  = sin(t * 1.1 + fi * 1.4 + fj * 0.9 + s * 4.7) * 0.007
                      + sin(t * 2.3 + fi * 0.8 + fj * 1.5 + s * 2.1) * 0.004;
            vec2  nxt = prev + vec2(wX, -tLen * 0.25);
            float taper = mix(0.0045, 0.0012, fj / 3.0); 
            tentD = min(tentD, sdSeg(uv, prev, nxt) - taper);
            prev  = nxt;
        }
    }


    float rimD    = abs(sdEllipse(lp + vec2(0.0, pz * 0.07), vec2(pz * 0.68, pz * 0.265)));
    float rimGlow = exp(-rimD * rimD * 5500.0) * 0.45;

    float cavMask = smoothstep(-pz * 0.02, pz * 0.04, cavD); 
    float cavRim  = exp(-cavD  * cavD  * 3500.0) * 0.12;     
    float interior = smoothstep(0.0, -pz * 0.42, bellD) * (0.04 + 0.10 * cavMask);

    float bellRim  = exp(-bellD * bellD * 300.0);
    float bellHalo = exp(-max(bellD, 0.0) * 14.0) * 0.40;
    float tentGlow = exp(-max(tentD, 0.0) * 60.0) * 0.55;

    float nVar = valueNoise(uv * 11.0 + vec2(s * 2.3)) * 0.3 + 0.7;

    vec3 glow  = hue * (bellRim + bellHalo + interior + cavRim + tentGlow) * nVar;
    glow      += mix(hue, vec3(1.0), 0.7) * rimGlow; 
    return glow;
}


vec3 marineSnow(vec2 uv, float t) {
    vec3  col = vec3(.0);
    float cs  = 0.09;
    vec2  cb  = floor(uv / cs);

    for (int cy = -1; cy <= 1; cy++) {
        for (int cx = -1; cx <= 1; cx++) {
            vec2  ci  = cb + vec2(float(cx), float(cy));
            vec2  rng = hash22(ci);
            float spd = 0.018 + hash21(ci + vec2(3.17, 1.43)) * 0.012;

            // Drift upward at spd UV/sec, wraps seamlessly within cell column
            vec2  pt = vec2(
                (ci.x + rng.x) * cs,
                (ci.y + fract(rng.y + t * spd / cs)) * cs
            );

            float twk = 0.45 + 0.55 * sin(t * (0.35 + rng.x * 0.7) + rng.y * 6.283);
            float d   = length(uv - pt);
            float brt = 0.55 + 0.45 * smoothstep(-0.45, 0.40, pt.y); // brighter near surface

            col += vec3(0.62, 0.82, 1.0) * exp(-d * d * 55000.0) * twk * 0.14 * brt;
        }
    }
    return col;
}



void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    float t = iTime;

    float depth = clamp(uv.y + 0.5, 0.0, 1.0);

    vec3  abyssCol = vec3(0.005, 0.010, 0.040);
    vec3  deepCol  = vec3(0.012, 0.035, 0.130);
    vec3  surfCol  = vec3(0.030, 0.080, 0.250);
    float dAbove   = smoothstep(0.0, 0.30, depth);
    vec3  col      = mix(abyssCol, mix(deepCol, surfCol, pow(depth, 1.3)), dAbove);
    col += vec3(0.0, 0.007, 0.022) * fbm(uv * 2.8 + vec2(t * 0.07, t * 0.04));

    float surfY     = 0.43 + surfaceH(uv * 1.8 + vec2(t * 0.22, 0.0), t) * 0.155;
    float belowSurf = 1.0 - smoothstep(surfY - 0.005, surfY + 0.005, uv.y);

    col += marineSnow(uv, t) * belowSurf;

    col += jellyfish(uv, vec2(-0.27,  0.01), 1.0, vec3(0.15, 0.65, 1.00), 0.000, t) * belowSurf;
    col += jellyfish(uv, vec2( 0.21,  0.09), 2.0, vec3(0.70, 0.20, 1.00), 1.257, t) * belowSurf;
    col += jellyfish(uv, vec2( 0.03, -0.13), 3.0, vec3(0.10, 0.90, 0.45), 2.513, t) * belowSurf;
    col += jellyfish(uv, vec2(-0.17,  0.17), 4.0, vec3(1.00, 0.45, 0.65), 3.770, t) * belowSurf;
    col += jellyfish(uv, vec2( 0.37, -0.09), 5.0, vec3(0.25, 0.80, 0.90), 5.027, t) * belowSurf;

    float causticMask = smoothstep(0.12, 0.85, depth);
    float caus = caustics(uv * 2.3 + vec2(t * 0.07), t * 0.22);
    col += vec3(0.6, 0.8, 1.0) * caus * causticMask * 0.26 * belowSurf;

    float surfBand = exp(-abs(uv.y - surfY) * 38.0);
    col += vec3(0.04, 0.10, 0.24) * surfBand * 0.45;

    float aboveMask  = smoothstep(surfY - 0.01, surfY + 0.012, uv.y);
    float canopyCaus = caustics(uv * 4.2, t * 0.35);

    vec2  vUV = fragCoord / iResolution.xy;
    vec2  vq  = vUV * (1.0 - vUV);
    col *= mix(0.22, 1.0, pow(vq.x * vq.y * 15.0, 0.32));

    col = 1.0 - exp(-col * 1.3);

    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
```

## Algorithm

The reference is a complete procedural scene. Adaptation is mostly substitution, not restructuring. The fragment shader stays single-pass with the same compositing order.

**Keep verbatim:**
- All hash / noise / Worley / smax / SDF helpers (`hash21`, `hash22`, `rot2D`, `valueNoise`, `fbm`, `worley`, `sdEllipse`, `sdSeg`, `smax`)
- The `jellyPath()` seeded-drift function (each jellyfish gets independent random sinusoidal drift driven by its seed)
- The `jellyfish()` body: bell SDF, cavity SDF, 8x4 tentacle loop, rim/halo/interior glow stacking, valueNoise variation modulator
- The `marineSnow()` 3x3 cell scan with upward drift wrap
- The `caustics()` Worley double-scale shimmer
- The depth gradient blend `mix(abyssCol, mix(deepCol, surfCol, pow(depth, 1.3)), dAbove)` and the fbm depth-color tint
- Vignette term `mix(0.22, 1.0, pow(vq.x * vq.y * 15.0, 0.32))`
- UV centering `uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y` (this is the project's standard centered-coords convention)

**Replace:**
- `iTime` -> `time` uniform; `iResolution` -> `resolution` uniform.
- The five hardcoded `jellyfish(...)` calls -> a `for (int s = 0; s < jellyCount; s++)` loop. Per iteration:
  - `ctr` is hashed from the seed across the safe interior box (e.g. `(hash22(vec2(fs, 7.7)) - 0.5) * vec2(0.8, 0.6)`)
  - `phase` is hashed from the seed (`hash21(vec2(fs, 8.3)) * 6.283`)
  - `hue` samples the gradient LUT at a seeded position: `texture(gradientLUT, vec2(hash21(vec2(fs, 5.5)), 0.5)).rgb`
  - Loop bound is the dynamic uniform `jellyCount` (GLSL 330 supports this directly per project conventions)
- The `surfY` / `surfaceH` Gerstner wave term, the `belowSurf` mask, the `surfBand` line, the `aboveMask`/`canopyCaus` calc, and the `gerstnerH`/`surfaceH` helper functions: **delete entirely**. The user does not want a water surface band. Without `belowSurf` masking, jellyfish/snow/caustics simply render across the full frame.
- The `1.0 - exp(-col * 1.3)` Reinhard-ish tonemap on the final line: **delete**. Project rule: no tonemap in shaders.
- Hardcoded brightness constants (`0.45`, `0.40`, `0.55`, `0.26`, `0.14`, etc.) for `rimGlow`, `bellHalo`, `tentGlow`, `causticIntensity`, `marineSnow` brightness become uniforms so they can be tuned and modulated.
- Per-jellyfish FFT brightness boost: when sampling the FFT texture, use a per-jellyfish log-spaced band (seed maps to one band between `baseFreq` and `maxFreq`). The band amplitude adds an additive boost on top of `baseBright` to that jellyfish's `glow` accumulator. This is the per-unit log-spaced-bands pattern used by other generators (Plasma, Constellation, Pitch Spiral) - never sum the FFT to a global brightness.

**Parameter mapping:**

| Reference quantity | Maps to config field |
|--------------------|----------------------|
| 5 hardcoded jellyfish calls | `jellyCount` (int 1-16) |
| `0.052` size base in `jellyfish()` | `sizeBase` |
| `0.026` size variance | `sizeVariance` |
| `0.14` pulse depth in `jellyfish()` | `pulseDepth` |
| `0.55..0.90` random pulse speed range | `pulseSpeed` (single scalar; multiply the seeded random factor) |
| `0.007` and `0.004` tentacle wave amplitudes | `tentacleWave` (scales both) |
| Outer constants `0.06, 0.04, 0.03, 0.02` and frequencies in `jellyPath` amplitude | `driftAmp`, `driftSpeed` (scales seeded amp / freq across the whole swarm) |
| `0.40` bellHalo, `0.45` rimGlow, `0.55` tentGlow | `bellGlow`, `rimGlow`, `tentacleGlow` |
| `0.26` caustics scalar in scene composite | `causticIntensity` |
| `0.14` marineSnow brightness | `snowBrightness` |
| `0.09` cell size in `marineSnow` | `snowDensity` (smaller = denser; expose inverted so larger = denser) |
| 5 hardcoded `vec3` hues | Seeded gradient LUT samples per jellyfish |
| `1.3` Reinhard tonemap exposure | Removed |

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| jellyCount | int | 1-16 | 8 | Number of drifting jellyfish |
| sizeBase | float | 0.02-0.10 | 0.052 | Bell size baseline before per-seed variance |
| sizeVariance | float | 0.0-0.06 | 0.026 | Per-seed bell size jitter |
| pulseDepth | float | 0.0-0.4 | 0.14 | Bell expand/contract amplitude during pulse |
| pulseSpeed | float | 0.1-3.0 | 1.0 | Bell pulse rate scalar |
| driftAmp | float | 0.0-0.30 | 0.10 | Drift path radius scalar |
| driftSpeed | float | 0.0-3.0 | 1.0 | Drift path frequency scalar |
| tentacleWave | float | 0.0-0.025 | 0.007 | Tentacle sine wave horizontal displacement |
| bellGlow | float | 0.0-1.0 | 0.40 | Bell halo brightness |
| rimGlow | float | 0.0-1.0 | 0.45 | Bell rim white-tinted highlight brightness |
| tentacleGlow | float | 0.0-1.0 | 0.55 | Tentacle glow falloff brightness |
| snowDensity | float | 0.0-1.0 | 0.5 | Marine snow particle density (cell size inverse) |
| snowBrightness | float | 0.0-0.4 | 0.14 | Marine snow particle brightness |
| causticIntensity | float | 0.0-0.6 | 0.26 | Worley caustic shimmer brightness |
| backdropDepth | float | 0.0-2.0 | 1.0 | Vertical depth gradient strength multiplier (abyss-to-surf mix) |
| baseFreq | float | 27.5-440 | 55 | Lowest FFT band (Hz) for per-jellyfish sampling |
| maxFreq | float | 1000-16000 | 14000 | Highest FFT band (Hz) for per-jellyfish sampling |
| gain | float | 0.1-10 | 2.0 | FFT amplitude gain |
| curve | float | 0.1-3 | 1.5 | FFT response curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Base brightness floor before FFT boost per jellyfish |

## Modulation Candidates

- **pulseDepth**: Bell breathing intensity. Low = static, high = dramatic kettle-drum thump on each pulse.
- **pulseSpeed**: How fast bells pulse. Modulating with tempo locks the swarm to song rhythm.
- **driftSpeed**: How quickly jellyfish wander. Slow = serene; fast = chaotic schooling.
- **tentacleWave**: Tentacle ripple amplitude. Low = stiff trails, high = whipping streamers.
- **causticIntensity**: Worley shimmer brightness. Bright pulse on transients reads as shafts of light catching the swarm.
- **snowBrightness**: Marine snow sparkle. Driving with high frequencies makes the column twinkle on hi-hats.
- **snowDensity**: Particle count. Modulation here scales how busy the column feels.
- **bellGlow / tentacleGlow / rimGlow**: Per-element brightness pieces of the jellyfish silhouette.
- **jellyCount**: Population. Stepwise modulation can pop more creatures into existence on builds.

### Interaction Patterns

- **Cascading threshold (per-jellyfish band gating)**: Each jellyfish samples a unique log-spaced FFT band. Quiet bands stay near `baseBright`; loud bands flare additive glow. The swarm reads as a moving spectral display - low jellyfish breathe with bass, mid jellyfish flicker with vocals, high jellyfish twinkle with cymbals. Different sections of a track light up different members of the swarm.
- **Resonance (pulse + glow alignment)**: When `pulseDepth` is driven by an envelope follower on the kick, peak bell expansion lines up with peak per-jellyfish band glow. Low-band jellyfish hit a brief synchronized maximum on every kick - they grow AND brighten together for one frame, becoming the focal point.
- **Competing forces (atmosphere balance)**: Driving `causticIntensity` from low bands and `snowBrightness` from high bands creates inverse-correlated atmosphere. Bass-heavy passages produce deep shafts of caustic light through quiet snow; treble passages produce sparkly snow storms with calm caustics. The water column itself appears to react to the music's spectral balance.

## Notes

- **Performance**: Inner tentacle loop is 32 iterations per jellyfish (8 outer * 4 segments), each doing a `sdSeg` + 2 `sin` calls. At 16 jellyfish that's 512 segment SDFs per pixel before the bell/cavity/rim work. Modern GPUs handle this at 1080p comfortably; 4K with high counts may need to drop `jellyCount`.
- **Tonemap removal**: Without the Reinhard `1.0 - exp(-col * 1.3)` line, the raw additive `col` can exceed 1.0 in bright regions (bell halos, overlapping caustics). The blend compositor downstream handles output range; do not re-add tonemap.
- **Surface band removed**: All `surfY`, `surfaceH`, `gerstnerH`, `belowSurf`, `surfBand`, `aboveMask`, `canopyCaus`, and the `* belowSurf` masks on snow/jellyfish/caustic terms are deleted. Without those, scene contents fill the frame top-to-bottom with the depth gradient running from `abyssCol` at the bottom to `surfCol` at the top edge.
- **Hue derivation**: Gradient LUT is sampled per jellyfish at hashed `s` to pick stable colors that vary across the swarm and follow whichever gradient preset the user has loaded. The `mix(hue, vec3(1.0), 0.7)` rim-color shift in the reference is preserved (rim still goes mostly white) so silhouettes stay legible.
- **Vignette**: The reference vignette is a simple `vUV * (1-vUV)` term. Project-wide bloom and gamma stages downstream may double up on it; if the result feels too dim at corners, scale or remove the vignette term during tuning.
