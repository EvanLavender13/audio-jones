# Nebula FFT Fix: BAND_SAMPLES Multi-Sample

Replace nebula's single-bin FFT star sampling with the standard BAND_SAMPLES multi-sample pattern used by galaxy, fireworks, filaments, etc. Currently each star samples one FFT texel; the fix samples 4 texels across the frequency band for smoother, more accurate reactivity.

## Design

### Algorithm

In `starLayer()`, replace the current single-bin lookup:

```glsl
// CURRENT (remove this)
float sBin = baseFreq * pow(maxFreq / baseFreq, semi / float(totalStarBins)) / (sampleRate * 0.5);
float sMag = (sBin <= 1.0) ? texture(fftTexture, vec2(sBin, 0.5)).r : 0.0;
```

With the standard BAND_SAMPLES pattern:

```glsl
// NEW (replace with this)
float freqLo = baseFreq * pow(maxFreq / baseFreq, semi / float(totalStarBins));
float freqHi = baseFreq * pow(maxFreq / baseFreq, (semi + 1.0) / float(totalStarBins));
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) {
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
}
float sMag = energy / float(BAND_SAMPLES);
```

The downstream usage of `sMag` stays identical — `react`, `twinkle`, `glow`, `spike` all remain unchanged.

No C++ changes required. All uniforms (`baseFreq`, `maxFreq`, `sampleRate`, `starBins`) are already bound.

---

## Tasks

### Wave 1: Shader Fix

#### Task 1.1: Replace single-bin FFT with BAND_SAMPLES in nebula.fs

**Files**: `shaders/nebula.fs`

**Do**: In the `starLayer()` function (around line 112-113), replace the two-line single-bin FFT lookup with the BAND_SAMPLES pattern from the Algorithm section above. Keep everything else in the function unchanged — `react`, `twinkle`, `glow`, `core`, `spike` all use `sMag` the same way.

**Verify**: `cmake.exe --build build` compiles. Launch app, enable Nebula, confirm stars still react to audio.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Nebula stars react to audio with same visual quality (smoother response expected)
- [ ] No regression in star brightness or color mapping
