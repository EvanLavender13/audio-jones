# Glitch Shader Enhancements

Advanced glitch techniques extracted from a Shadertoy "soulstone glitch" shader. These extend the existing Glitch effect with temporal coherence breaking, spatial splitting, and variable-resolution pixelation.

## Classification

- **Category**: TRANSFORMS > Style (extends existing Glitch effect)
- **Pipeline Position**: Same as current Glitch—applies during transform pass

## References

- [Harry Alisavakis - Glitch Image Effect](https://halisavakis.com/my-take-on-shaders-glitch-image-effect/) - Stripe displacement, wavy distortion, chromatic aberration
- [Agate Dragon - Displacement Lines](https://agatedragon.blog/2023/12/20/glitch-shader-effect-with-displacement-lines/) - Time-gated row displacement
- [Agate Dragon - Block Glitches](https://agatedragon.blog/2023/12/21/glitch-shader-effect-using-blocks-part-2/) - Colored block overlays with timing
- [GitHub kuravih/gllock](https://github.com/kuravih/gllock/blob/master/shaders/glitch.fragment.glsl) - Per-row chromatic offset
- [GitHub cacheflowe/haxademic](https://github.com/cacheflowe/haxademic/blob/master/data/haxademic/shaders/textures/basic-diagonal-stripes.glsl) - Diagonal banding math
- Original Shadertoy shader (provided by user) - All techniques combined

---

## Technique 1: Variable Resolution Pixelation ("Datamosh")

### Visual Effect
Resolution randomly changes per-frame, creating blocky mosaic artifacts that flicker between fine and coarse detail. Different screen regions can have different block sizes simultaneously.

### Algorithm

```glsl
// Seed varies by frame and diagonal position
uint offset = uint(iFrame / 6) + uint((uv.x + uv.y) * 8.0);

// Random resolution between 6 and 64 cells
float res = mix(6.0, 64.0, random(offset));

// Snap UVs to grid
uv = floor(uv * res) / res;
```

**Key insight**: The `(uv.x + uv.y) * 8.0` term creates **diagonal bands**—pixels along the same diagonal share the same random seed, so they get the same resolution. This creates visible diagonal stripe artifacts.

### Integration into Existing Shader

**New UI subcategory**: "Datamosh" under Glitch

**Stage placement**: Before all other UV distortions (first thing in main())

```glsl
// Add after existing uniform declarations
uniform bool datamoshEnabled;
uniform float datamoshMin;      // Minimum resolution (default: 6.0)
uniform float datamoshMax;      // Maximum resolution (default: 64.0)
uniform float datamoshSpeed;    // Frame divisor (default: 6.0)
uniform float datamoshBands;    // Diagonal band count (default: 8.0)

// In main(), before CRT:
if (datamoshEnabled) {
    uint offset = uint(float(frame) / datamoshSpeed) + uint((uv.x + uv.y) * datamoshBands);
    float res = mix(datamoshMin, datamoshMax, hash11(float(offset)));
    uv = floor(uv * res) / res;
}
```

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| datamoshEnabled | bool | - | false | Master toggle |
| datamoshMin | float | 4-32 | 6.0 | Minimum cells (lower = blockier) |
| datamoshMax | float | 16-128 | 64.0 | Maximum cells (higher = finer) |
| datamoshSpeed | float | 1-30 | 6.0 | Frames between resolution changes |
| datamoshBands | float | 1-32 | 8.0 | Diagonal band count (higher = more stripes) |

### Modulation Candidates

- **datamoshMin/Max**: Audio-reactive resolution range creates pulsing blockiness
- **datamoshBands**: LFO creates rolling diagonal waves
- **datamoshSpeed**: Beat sync creates rhythmic resolution jumps

---

## Technique 2: Horizontal Row Displacement ("Row Slice")

### Visual Effect
Individual horizontal rows shift left or right randomly. Displacement bursts occur in time-gated pulses rather than continuously, creating sudden "slice" moments.

### Algorithm

```glsl
// Seed based on x-position, creating vertical column groups
uint seed = uint(fragCoord.x + time) / 32u;

// Time gate: high power of sin creates rare burst moments
float gate = step(random(seed), pow(sin(time * 4.0), 7.0));

// Random displacement when gate is open
float displacement = (random(seed) * 2.0 - 1.0) * gate * random(seed);

uv.x += displacement;
```

**Key insight**: `pow(sin(time * 4.0), 7.0)` creates a narrow pulse—sin^7 is only significant near sin=1, so displacement only triggers ~14% of the time, creating sporadic bursts.

### Integration into Existing Shader

**Extends**: VHS subcategory (similar to scanline noise but horizontal)

**Stage placement**: After CRT, before Analog

```glsl
// Add uniforms
uniform bool rowSliceEnabled;
uniform float rowSliceIntensity;  // Max displacement (default: 0.1)
uniform float rowSliceBurstFreq;  // Burst frequency Hz (default: 4.0)
uniform float rowSliceBurstPower; // Gate sharpness (default: 7.0)
uniform float rowSliceColumns;    // Column group width in pixels (default: 32.0)

// In main():
if (rowSliceEnabled) {
    uint seed = uint(gl_FragCoord.x + time * 60.0) / uint(rowSliceColumns);
    float gate = step(hash11(float(seed)), pow(abs(sin(time * rowSliceBurstFreq)), rowSliceBurstPower));
    float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * rowSliceIntensity;
    uv.x += offset;
}
```

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| rowSliceEnabled | bool | - | false | Master toggle |
| rowSliceIntensity | float | 0-0.5 | 0.1 | Max horizontal shift (fraction of screen) |
| rowSliceBurstFreq | float | 0.5-20 | 4.0 | Bursts per second |
| rowSliceBurstPower | float | 1-15 | 7.0 | Gate sharpness (higher = rarer, shorter bursts) |
| rowSliceColumns | float | 8-128 | 32.0 | Pixels per column group |

### Modulation Candidates

- **rowSliceIntensity**: Beat-reactive creates punchy glitch moments
- **rowSliceBurstFreq**: LFO varies burst rhythm
- **rowSliceBurstPower**: Audio envelope controls burst density

---

## Technique 3: Vertical Column Displacement ("Column Slice")

### Visual Effect
Vertical columns shift up or down independently. Combined with Row Slice, creates a grid of independently displaced regions.

### Algorithm

Same as Row Slice but transposed:

```glsl
uint seed = uint(fragCoord.y + time) / 32u;
float gate = step(random(seed), pow(sin(time * 4.0), 7.0));
float displacement = (random(seed) * 2.0 - 1.0) * gate * random(seed);
uv.y += displacement;
```

### Integration into Existing Shader

**Extends**: Same "Slice" subcategory as Row Slice

```glsl
uniform bool colSliceEnabled;
uniform float colSliceIntensity;
uniform float colSliceBurstFreq;
uniform float colSliceBurstPower;
uniform float colSliceRows;  // Row group height in pixels

if (colSliceEnabled) {
    uint seed = uint(gl_FragCoord.y + time * 60.0) / uint(colSliceRows);
    float gate = step(hash11(float(seed)), pow(abs(sin(time * colSliceBurstFreq)), colSliceBurstPower));
    float offset = (hash11(float(seed + 1u)) * 2.0 - 1.0) * gate * colSliceIntensity;
    uv.y += offset;
}
```

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| colSliceEnabled | bool | - | false | Master toggle |
| colSliceIntensity | float | 0-0.5 | 0.1 | Max vertical shift |
| colSliceBurstFreq | float | 0.5-20 | 4.0 | Bursts per second |
| colSliceBurstPower | float | 1-15 | 7.0 | Gate sharpness |
| colSliceRows | float | 8-128 | 32.0 | Pixels per row group |

---

## Technique 4: Diagonal Stripe Banding

### Visual Effect
Diagonal stripes across the screen, each with potentially different effects applied. Creates a "shattered glass" or "venetian blind" appearance along 45-degree angles.

### Algorithm

The key mathematical insight:

```glsl
// Points along the same diagonal have the same sum
float diagonal = uv.x + uv.y;

// Quantize into bands
float band = floor(diagonal * bandCount);

// Use band index as seed for per-band effects
float bandSeed = hash11(band + float(frame));
```

**Why this works**: The equation `x + y = c` defines a diagonal line. All pixels on that line share the same `c` value, so `floor((uv.x + uv.y) * n)` groups pixels into diagonal bands.

### Integration into Existing Shader

**New effect**: Can modulate any existing parameter per-band, or apply displacement

```glsl
uniform bool diagonalBandsEnabled;
uniform float diagonalBandCount;      // Number of bands (default: 8.0)
uniform float diagonalBandDisplace;   // Per-band UV offset (default: 0.02)
uniform float diagonalBandSpeed;      // Animation speed (default: 1.0)

if (diagonalBandsEnabled) {
    float diagonal = uv.x + uv.y;
    float band = floor(diagonal * diagonalBandCount);
    float bandOffset = hash11(band + floor(time * diagonalBandSpeed)) * 2.0 - 1.0;
    uv += bandOffset * diagonalBandDisplace;
}
```

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| diagonalBandsEnabled | bool | - | false | Master toggle |
| diagonalBandCount | float | 2-32 | 8.0 | Number of diagonal stripes |
| diagonalBandDisplace | float | 0-0.1 | 0.02 | UV offset per band |
| diagonalBandSpeed | float | 0-10 | 1.0 | Band animation rate |

---

## Technique 5: Radial Temporal Jitter

### Visual Effect
Different screen regions appear to be at different moments in time. Center and corners behave differently from edges, creating a "time ripple" where some areas show past or future frames.

### Algorithm

```glsl
// coord.x * coord.y creates radial symmetry
// Product is 0 on axes, maximum at corners, creating radial zones
float radialSeed = float((coord.x * coord.y) / 10024);

// Per-pixel time offset
float timeOffset = hash11(radialSeed) * 2.0 - 1.0;

// Gate: only apply when condition met
float gate = step(hash11(float(frame) * 0.1), sin(time));

// Effective time for this pixel
float pixelTime = time + timeOffset * gate * maxOffset;
```

**Key insight**: `coord.x * coord.y` equals zero along both axes (x=0 or y=0) and grows toward corners. This creates roughly radial zones of similar time offset.

### Integration into Existing Shader

**Requires**: Sampling from a previous frame buffer (feedback texture) or using time-offset noise

For a simpler approximation without temporal buffer:

```glsl
uniform bool temporalJitterEnabled;
uniform float temporalJitterAmount;  // Max time offset in seconds
uniform float temporalJitterGate;    // Gate threshold (0-1)

if (temporalJitterEnabled) {
    vec2 coord = gl_FragCoord.xy;
    float radial = (coord.x * coord.y) / (resolution.x * resolution.y * 0.25);
    float jitterSeed = hash11(radial + floor(time * 10.0));
    float gate = step(jitterSeed, temporalJitterGate);

    // Use jitter to offset UV sampling (simulates looking at different "time")
    vec2 jitterOffset = vec2(hash11(radial), hash11(radial + 1.0)) * 2.0 - 1.0;
    uv += jitterOffset * gate * temporalJitterAmount;
}
```

**Note**: True temporal jitter requires a frame history buffer. This approximation creates spatial jitter that visually resembles temporal discontinuity.

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| temporalJitterEnabled | bool | - | false | Master toggle |
| temporalJitterAmount | float | 0-0.1 | 0.02 | Jitter magnitude |
| temporalJitterGate | float | 0-1 | 0.3 | Probability of jitter per region |

---

## Technique 6: Block Color Masking

### Visual Effect
Random rectangular regions receive alternate color treatment—palette swap, tint overlay, or inverted colors. Blocks vary in size and position each frame.

### Algorithm

```glsl
// Vary grid size per frame
int gridSize = frame % 10 + 1;
ivec2 blockCoord = ivec2(gl_FragCoord.xy) / gridSize;

// Linearize block index
int index = blockCoord.y * int(resolution.y) + blockCoord.x;

// Vary block grouping
index /= (hash(frame) % 32) + 4;

// Pseudo-random mask
int pattern = hash(index + frame);
int checker = index % (pattern % 7 + 2);
float mask = (checker > 0) ? 0.0 : 1.0;

// Apply alternate color to masked regions
color = mix(originalColor, alternateColor, mask * intensity);
```

### Integration into Existing Shader

**Extends**: Digital subcategory (similar to block displacement but for color)

```glsl
uniform bool blockMaskEnabled;
uniform float blockMaskIntensity;  // Blend strength (default: 0.5)
uniform int blockMaskMinSize;      // Minimum block divisor (default: 1)
uniform int blockMaskMaxSize;      // Maximum block divisor (default: 10)
uniform vec3 blockMaskTint;        // Tint color (default: 1.0, 0.0, 0.5)

if (blockMaskEnabled) {
    int gridSize = (frame % (blockMaskMaxSize - blockMaskMinSize + 1)) + blockMaskMinSize;
    ivec2 blockCoord = ivec2(gl_FragCoord.xy) / gridSize;
    int index = blockCoord.y * int(resolution.x / float(gridSize)) + blockCoord.x;

    uint blockHash = hash(uint(index + frame / 6));
    int pattern = int(blockHash % 7u) + 2;
    float mask = (index % pattern == 0) ? 1.0 : 0.0;

    col = mix(col, col * blockMaskTint, mask * blockMaskIntensity);
}
```

### Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| blockMaskEnabled | bool | - | false | Master toggle |
| blockMaskIntensity | float | 0-1 | 0.5 | Blend strength with tint |
| blockMaskMinSize | int | 1-10 | 1 | Smallest block size |
| blockMaskMaxSize | int | 5-20 | 10 | Largest block size |
| blockMaskTint | vec3 | 0-1 each | (1,0,0.5) | Color multiplier for masked blocks |

---

## Proposed UI Structure

Extend the existing Glitch UI with new subcategories:

```
Glitch
├── CRT (existing)
├── Analog (existing)
├── Digital (existing)
│   └── Block Mask (NEW - color masking)
├── VHS (existing)
├── Slice (NEW)
│   ├── Row Slice
│   └── Column Slice
├── Datamosh (NEW)
│   └── Diagonal Bands
├── Temporal (NEW)
│   └── Radial Jitter
└── Overlay (existing)
```

**Alternative**: Group by visual similarity:

```
Glitch
├── CRT (existing)
├── Analog (existing)
├── Digital (existing)
├── VHS (existing)
├── Fragmentation (NEW)
│   ├── Datamosh (variable pixelation)
│   ├── Row Slice
│   ├── Column Slice
│   └── Diagonal Bands
├── Color Corruption (NEW)
│   ├── Block Mask
│   └── Radial Jitter
└── Overlay (existing)
```

---

## Shader Execution Order

Recommended order in `main()`:

1. **Datamosh** (resolution reduction first—affects all subsequent sampling)
2. **Diagonal Bands** (UV displacement)
3. **Row Slice** (horizontal UV displacement)
4. **Column Slice** (vertical UV displacement)
5. **CRT** (existing—barrel distortion)
6. **VHS** (existing—tracking bars, scanline noise)
7. **Analog** (existing—noise distortion + chromatic aberration)
8. **Digital** (existing—block displacement)
9. **Block Mask** (color modification on final samples)
10. **Temporal Jitter** (spatial approximation of temporal discontinuity)
11. **Overlay** (existing—scanlines, noise)

---

## Required Hash Function

The Shadertoy shader uses multiple hash functions. For consistency with your existing `hash33`, add:

```glsl
// Single float hash (for seeds)
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// Integer hash (for block indices)
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
```

---

## Notes

- **Performance**: All techniques are single-pass UV/color manipulation. No additional texture samples beyond existing chromatic aberration.
- **Interaction**: Datamosh + Diagonal Bands creates distinctive "shattered diagonal blocks." Row/Column Slice + Digital block displacement creates complex grid fragmentation.
- **Audio reactivity**: Burst parameters (`rowSliceBurstFreq`, `datamoshSpeed`) respond well to beat detection. Intensity parameters work with continuous audio envelope.
