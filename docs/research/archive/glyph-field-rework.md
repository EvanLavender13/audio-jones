# Glyph Field Rework

Clean up the glyph field generator: remove features not in the reference, fix band distortion to match the reference's step-based UV scaling, rename confusing parameters, and reorganize UI sections.

## Classification

- **Category**: Rework of existing GENERATORS > Texture > Glyph Field
- **Pipeline Position**: No change — same generator slot
- **Scope**: Shader edits, config struct edits, UI reorganization. No new files.

## Attribution

- **Based on**: "Infoline" by totetmatt
- **Source**: https://www.shadertoy.com/view/l3XGDX
- **License**: CC BY-NC-SA 3.0
- **Influences**: Necromurlok, Xor, Byte_mechanic (credited in the original shader)

## Reference Code

The original Infoline shader — the source of truth for what the base effect should do:

```glsl
/*
Heavily influenced by :
 Necromurlok  - https://www.shadertoy.com/view/ssf3zX
 Xor - https://www.shadertoy.com/view/sd3czM
     - https://www.shadertoy.com/view/NdtyzM
 Byte_mechanic - https://www.shadertoy.com/view/4fB3RR
*/
uvec3 pcg3d(uvec3 p){
    p=p*1234567u+1234567890u;
    p.x*=p.y+p.z;p.y*=p.x+p.z;p.z*=p.y+p.x;
    p^=p>>16;
    p.x*=p.y+p.z;p.y*=p.x+p.z;p.z*=p.y+p.x;
    return p;
}
vec3 pcg3df(vec3 p){
return vec3(pcg3d(floatBitsToUint(p)))/float(0xFFFFFFFFu);
}
float char(vec2 uv,vec2 idx){

  idx = floor(idx)*1./16.;

  uv+=idx;
  return texture(iChannel0,clamp(uv,0.+idx,1./16.+idx)).r;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    uv *= 1.+step(8./16.,abs(uv.y));
    uv *= 1.+step(6./16.,abs(uv.y));
     uv *= 1.+step(2./16.,abs(uv.y));
    vec3 noise = pcg3df(uv.xyy);

    // Time varying pixel color
    vec3 col =vec3(0.);
    vec2 fuv = floor(uv*16.);
    vec3 rnd = pcg3df(fuv.yyy);
    uv.x +=iTime*(rnd.x-.5)*.5;
    fuv = floor(uv*16.);
    rnd = pcg3df(.0*floor(iTime+noise.x*.1+rnd.z)+fuv.xyy+rnd);

    uv = mod(uv,1./16.)-(1./256.);
    col+=char(uv,
    vec2(rnd.xy*32.));
    col*=rnd.xzz;
    if(fract(rnd.y+rnd.x+rnd.y+iTime*.1+noise.z*.01)<.1)col=(1.-col)*rnd.xzz;
    col *= length(rnd);
    // Output to screen
    fragColor = vec4(col,1.0);
}
```

## Algorithm

### Removals

**Warp (wave displacement)** — `waveAmplitude`, `waveFreq`, `waveSpeed`, `waveTime` and all corresponding shader uniforms/locs. Not present in the reference. The sine-based UV displacement creates a wobbly grid that doesn't match the visual language.

**LCD mode** — `lcdMode`, `lcdFreq` and corresponding shader uniforms/locs. Cut entirely.

### Fix: Band Distortion → Band Strength

**Problem**: Current implementation uses continuous linear scaling: `bandScale = 1.0 + distFromCenter * bandDistortion * 2.0`. This makes cells uniformly smaller toward the edges — a smooth gradient, not a distortion.

**Reference behavior**: Three `step()` multipliers at fixed Y thresholds. Each step doubles the UV scale when its threshold is exceeded, creating discrete bands of progressively denser cells:

```glsl
uv *= 1.+step(8./16.,abs(uv.y));   // |y| >= 0.5:   2x
uv *= 1.+step(6./16.,abs(uv.y));   // |y| >= 0.375: 2x (cumulative 4x)
uv *= 1.+step(2./16.,abs(uv.y));   // |y| >= 0.125: 2x (cumulative 8x)
```

**Fix**: Replace with a single `bandStrength` float (0-1). At 0, no banding. At 1, full reference behavior. Values in between give partial compression per step:

```glsl
// Apply before grid quantization, after centering UV
uv *= 1.0 + bandStrength * step(0.5, abs(uv.y));
uv *= 1.0 + bandStrength * step(0.375, abs(uv.y));
uv *= 1.0 + bandStrength * step(0.125, abs(uv.y));
```

The 3 thresholds are hardcoded from the reference. `bandStrength` controls how much each step multiplies (0 = multiply by 1, 1 = multiply by 2).

### Rename: Flutter → Char

- `flutterAmount` → `charAmount`
- `flutterSpeed` → `charSpeed`
- `flutterTime` → `charTime` (internal accumulator)
- Shader uniform names: `flutterAmount` → `charAmount`, `flutterTime` → `charTime`
- Modulation param IDs: `glyphField.flutterAmount` → `glyphField.charAmount`, `glyphField.flutterSpeed` → `glyphField.charSpeed`

### UI Section Reorganization

**Before:**
| Section | Params |
|---------|--------|
| Audio | baseFreq, maxFreq, freqBins, gain, curve, baseBright |
| Grid | gridSize, layerCount, layerScaleSpread, layerSpeedSpread, layerOpacity |
| Scroll | scrollDirection, scrollSpeed |
| Stutter | stutterAmount, stutterSpeed, stutterDiscrete |
| Motion | ~~flutterAmount, flutterSpeed, waveAmplitude, waveFreq, waveSpeed~~, driftAmount, driftSpeed |
| Distortion | ~~bandDistortion~~, inversionRate, inversionSpeed |
| LCD | ~~lcdMode, lcdFreq~~ |

**After:**
| Section | Params |
|---------|--------|
| Audio | baseFreq, maxFreq, freqBins, gain, curve, baseBright |
| Grid | gridSize, layerCount, layerScaleSpread, layerSpeedSpread, layerOpacity, bandStrength |
| Scroll | scrollDirection, scrollSpeed |
| Stutter | stutterAmount, stutterSpeed, stutterDiscrete |
| Character | charAmount, charSpeed, inversionRate, inversionSpeed |
| Drift | driftAmount, driftSpeed |

- `bandStrength` moves to Grid — it controls grid cell sizing/layout
- Renamed char params + inversion group under "Character" — both control per-cell visual state
- Drift gets its own small section (or could merge into Scroll)
- Motion, Distortion, LCD sections eliminated

## Parameters

Changed params only:

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `bandStrength` | float | 0.0-1.0 | 0.3 | Step-based edge compression; 0 = flat, 1 = reference (8x at edges) |
| `charAmount` | float | 0.0-1.0 | 0.3 | Per-cell character cycling intensity (was flutterAmount) |
| `charSpeed` | float | 0.1-10.0 | 2.0 | Character cycling rate (was flutterSpeed) |

## Modulation Candidates

- **bandStrength**: Edge compression pulses — bass hits compress edges, silence relaxes to flat
- **charAmount**: Character cycling intensity — drives visual activity/chaos
- **inversionRate**: Fraction of inverted cells — bright flashes of inverted blocks on transients

### Interaction Patterns

**bandStrength + gridSize (competing forces)**: Higher bandStrength compresses edge cells into denser bands while gridSize sets overall density. Modulating both creates a breathing effect where the center stays stable while edges pulse between sparse and packed.

## Notes

- Removing wave/LCD/renaming flutter is a breaking change for existing presets. Old presets with `flutterAmount` will lose that field on load (falls back to default). Consider this acceptable since the effect is new.
- The 3 step thresholds (0.125, 0.375, 0.5) match the reference exactly. They create 4 bands: center, inner, outer, edge.
- Band strength applies before grid quantization in the shader, same position as the current `bandDistortion` computation — just different math.
