# Muons Turbulence Modes

Add a turbulence mode selector to the Muons effect, mirroring the existing distance function mode selector. Each mode replaces the FBM waveform function inside the turbulence loop, producing different spatial folding characters.

## Classification

- **Category**: GENERATORS > Filament (existing effect extension)
- **Pipeline Position**: No change — same generator pass
- **Scope**: Shader switch + config field + UI combo + serialization

## References

- Existing: `shaders/muons.fs` distance mode switch (lines 63-88) — pattern to mirror
- Existing: `src/effects/muons.cpp` mode combo UI (line 217) — pattern to mirror
- Tested live: all five turbulence functions confirmed visually distinct by user

## Reference Code

Current turbulence loop (the part that changes per mode):

```glsl
// Inside the march loop, after Rodrigues rotation, before distance function
d = 1.0;
for (int j = 1; j < turbulenceOctaves; j++) {
    d += 1.0;
    a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;  // <-- this line varies per mode
}
```

Five tested turbulence functions (all confirmed working):

```glsl
// Mode 0: Sine (original) — smooth swirling folds
a += sin(a * d + time * PHI).yzx / d * turbulenceStrength;

// Mode 1: Fract Fold — sharp sawtooth, crystalline/faceted filaments
a += (fract(a * d + time * PHI) * 2.0 - 1.0).yzx / d * turbulenceStrength;

// Mode 2: Abs-Sin Fold — sharp angular turns, always-positive displacement
a += abs(sin(a * d + time * PHI)).yzx / d * turbulenceStrength;

// Mode 3: Triangle Wave — smooth zigzag between sin and fract character
a += (abs(fract(a * d + time * PHI) * 2.0 - 1.0) * 2.0 - 1.0).yzx / d * turbulenceStrength;

// Mode 4: Squared Sin — soft peaks with flat valleys, organic feel
a += (sin(a * d + time * PHI) * abs(sin(a * d + time * PHI))).yzx / d * turbulenceStrength;

// Mode 5: Square Wave — hard binary on/off, blocky digital geometry
a += (step(0.5, fract(a * d + time * PHI)) * 2.0 - 1.0).yzx / d * turbulenceStrength;

// Mode 6: Quantized — grid-locked staircase structures
a += (floor(a * d + time * PHI + 0.5)).yzx / d * turbulenceStrength;
```

## Algorithm

Mirror the existing distance function `mode` switch pattern:
- Add `uniform int turbulenceMode` to the shader
- Replace the single turbulence line with a `switch (turbulenceMode)` block
- Each case contains one of the five tested functions above
- The loop structure (`d` counter, `.yzx` swizzle, `/ d * turbulenceStrength`) is identical across all modes — only the waveform function changes

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| turbulenceMode | int | 0-6 | 0 | Selects FBM waveform: 0=Sine, 1=Fract Fold, 2=Abs-Sin, 3=Triangle, 4=Squared Sin, 5=Square Wave, 6=Quantized |

## UI

Add a combo box labeled `"Turbulence Mode##muons"` directly below the existing `"Mode##muons"` combo (or next to the Octaves/Turbulence sliders — wherever feels natural in the Geometry section).

Mode labels: `"Sine"`, `"Fract Fold"`, `"Abs-Sin"`, `"Triangle"`, `"Squared Sin"`, `"Square Wave"`, `"Quantized"`

## Files Changed

| File | Change |
|------|--------|
| `src/effects/muons.h` | Add `int turbulenceMode = 0;` to `MuonsConfig`, add to `MUONS_CONFIG_FIELDS`, add `int turbulenceModeLoc;` to `MuonsEffect` |
| `src/effects/muons.cpp` | Cache `turbulenceModeLoc`, bind uniform in Setup, add combo in UI |
| `shaders/muons.fs` | Add `uniform int turbulenceMode;`, replace turbulence line with switch block |

## Notes

- No new shaders, no new files, no resize/init changes — this is purely additive
- The `.yzx` swizzle is shared across all modes (swizzle variants were tested but too subtle to justify the combinatorial explosion of swizzle × turbulence × distance)
- All five functions were tested live and confirmed visually distinct
