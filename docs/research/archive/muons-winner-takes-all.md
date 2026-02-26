# Muons Winner-Takes-All Color & Audio

Replace additive color accumulation with a winner-takes-all approach where the closest shell crossing determines both the pixel's color and its audio-reactive frequency band.

## Classification

- **Category**: GENERATORS > Filament (existing effect extension)
- **Pipeline Position**: No change — same generator pass
- **Scope**: Shader logic change + possible config field for blend between old/new

## References

- Existing: `shaders/motherboard.fs` lines 57-103 — `minit` orbit trap pattern, winning iteration drives LUT color and FFT band lookup
- Existing: `shaders/muons.fs` lines 96-112 — current per-step additive accumulation

## Reference Code

Motherboard's winner-takes-all pattern:

```glsl
// Track winning iteration (closest orbit trap)
int minit = 0;
float ot1 = 1000.0;
for (int i = 0; i < iterations; i++) {
    // ... iteration logic ...
    float m = abs(p.x);
    if (m < ot1) {
        ot1 = m;
        minit = i;
    }
}

// Winner determines color
vec3 layerColor = texture(gradientLUT, vec2((float(minit) + 0.5) / float(iterations), 0.5)).rgb;

// Winner determines frequency band
float t0 = float(minit) / float(iterations);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
// ... multi-sample FFT band lookup ...
float brightness = baseBright + energy;

vec3 color = trace * layerColor * brightness;
```

Current Muons accumulation (the part being replaced):

```glsl
// Every step contributes additively — colors wash out, audio imperceptible
color += sampleColor * audio / max(d * s, 1e-6);
```

## Algorithm

Track which march step has the closest shell crossing (`d * s` minimum), then use that step's index for both LUT color and FFT frequency:

```glsl
// Inside march loop, after distance function:
float proximity = d * s;
if (proximity < closestHit) {
    closestHit = proximity;
    winnerStep = i;
    winnerGlow = 1.0 / max(proximity, 1e-6);  // glow intensity from proximity
}

// After loop:
// Color from winner
float lutCoord = fract(float(winnerStep) / float(max(marchSteps - 1, 1)) + time * colorSpeed);
vec3 sampleColor = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

// FFT from winner's frequency band (motherboard multi-sample pattern)
float t0 = float(winnerStep) / float(max(marchSteps - 1, 1));
float t1 = float(winnerStep + 1) / float(max(marchSteps - 1, 1));
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

const int BAND_SAMPLES = 4;
float energy = 0.0;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
}
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float audio = baseBright + energy;

// Final pixel
color = winnerGlow * sampleColor * audio * brightness;
```

Key differences from current approach:
- `closestHit` replaces additive `color +=` — only the best shell crossing counts
- `winnerStep` maps to ONE frequency band, not 10 averaged together
- LUT is sampled once at the winner's position — no additive secondary blending
- `colorFreq` may become unnecessary (winner index IS the LUT coordinate) — or repurposed as an offset/multiplier

## Parameters

No new config fields required — reuses existing `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `colorSpeed`, `brightness`, `exposure`.

`colorFreq` may need repurposing or removal since the winner's step index naturally maps to a LUT position.

## Modulation Candidates

Existing params gain new perceptual impact:
- **baseFreq / maxFreq**: Now visibly control which screen regions respond to which frequencies (because winner steps create spatially coherent zones)
- **gain / curve**: Brightness contrast between active and silent frequency bands becomes visible per-region

## Notes

- The glow intensity (`1.0 / max(d*s, 1e-6)`) preserves the bright-near-shells character
- Trail persistence (ping-pong feedback) still works — it composites the final pixel color, doesn't care how it was computed
- Tonemap (`tanh`) still applies after the winner color computation
- With 10 march steps and log frequency spread, each step covers roughly one octave — spatial regions will visibly separate by octave
- Risk: winner-takes-all may look harsher than the current soft additive glow. Could add a `winnerSharpness` param later to blend between additive and winner modes, but start with pure winner and evaluate
