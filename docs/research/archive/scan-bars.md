# Scan Bars

Variable-width colored bars that scroll and converge toward a focal point, creating an optical illusion where bar geometry and color cycle independently. Bars bunch together via `tan()` distortion and snap between vivid palette colors. Supports linear (vertical/angled), spoke (radial fan), and ring (concentric circle) layouts.

## Classification

- **Category**: GENERATORS (alongside Plasma, Constellation, Interference)
- **Pipeline Position**: Generator stage, before transforms
- **Chosen Approach**: Full — the visual is geometrically simple (bars), so more parameters yield more variety

## References

- Source Shadertoy shader (pasted by user) — core `tan(abs(uv.x))` convergence, `fract()` bar generation, dual-rate color/position scrolling
- Existing scan-lines branch (git reflog `5a11dcc`) — prior attempt at scan-line effect with FBM wave displacement, provides reference for angle rotation and phase accumulation patterns

## Algorithm

### Bar Generation (Linear Mode)

UV rotation for arbitrary bar angle:
```
rotated = rotate2D(uv - pivot, angle) + pivot
```

Convergence distortion bunches bars toward a focal point:
```
coord = rotated.x + convergence * tan(abs(rotated.x - offset) * convergenceFreq)
```
Where `offset` shifts the convergence center away from screen middle.

Bar mask from repeating stripe pattern:
```
d = fract(barDensity * coord + scrollPhase)
mask = smoothstep(0.5 - sharpness, 0.5, d) * smoothstep(0.5 + sharpness, 0.5, d)
```

### Bar Generation (Spoke Mode)

Replace linear coordinate with angular:
```
coord = atan(uv.y, uv.x) / TAU + 0.5    // 0-1 range around circle
```
Convergence distortion still applies — bunches spokes toward an angular focal point.

### Bar Generation (Ring Mode)

Replace linear coordinate with radial distance:
```
coord = length(uv)
```
Convergence distortion bunches rings toward a specific radius.

### Color via LUT

The chaos math produces a float `t` that indexes into the ColorLUT:
```
chaos = tan(coord * chaosFreq + colorPhase)
t = fract(chaos * chaosIntensity)
color = texture(colorLUT, vec2(t, 0.5))
```

`chaosIntensity` controls how wildly `t` jumps between palette positions. At 0, bars sample the LUT sequentially. At high values, adjacent bars get distant palette colors — recreating the original's vivid multicolor snapping.

### Time / Scrolling

Two independent phase accumulators on CPU:
- `scrollPhase += scrollSpeed * deltaTime` — drives bar position
- `colorPhase += colorSpeed * deltaTime` — drives LUT index drift

The independent rates create the optical illusion: bars appear to move one direction while colors drift another.

Optional snap/lurch via quantized time:
```
snapPhase = floor(phase) + pow(fract(phase), snapAmount)
```
At `snapAmount = 0`, smooth scrolling. Higher values create lurching/stuttering motion.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| mode | int (enum) | 0=Linear, 1=Spokes, 2=Rings | 0 | Bar layout geometry |
| angle | float | 0 – 2π | 0 | Bar orientation (linear mode) |
| barDensity | float | 1 – 100 | 10 | Number of bars across viewport |
| convergence | float | 0 – 2 | 0.5 | Strength of tan() bunching distortion |
| convergenceFreq | float | 1 – 20 | 5 | Spatial frequency of convergence warping |
| convergenceOffset | float | -1 – 1 | 0 | Focal point offset from center |
| sharpness | float | 0.01 – 0.5 | 0.1 | Bar edge hardness (smoothstep width) |
| scrollSpeed | float | 0 – 5 | 0.2 | Bar position scroll rate |
| colorSpeed | float | 0 – 5 | 1.0 | LUT index drift rate (independent from scroll) |
| chaosIntensity | float | 0 – 5 | 1.0 | How wildly adjacent bars jump across the palette |
| snapAmount | float | 0 – 2 | 0 | Time-lurching intensity (0 = smooth, higher = stutter) |
| gradient | ColorConfig | — | Rainbow | Color palette via LUT system |

## Modulation Candidates

- **barDensity**: bar count pulses with energy
- **convergence**: bunching intensity shifts with dynamics
- **sharpness**: edges soften and harden
- **scrollSpeed**: bar drift rate responds to tempo
- **chaosIntensity**: color chaos ramps with intensity
- **snapAmount**: lurching syncs with transients
- **convergenceOffset**: focal point wanders

## Notes

- `tan()` produces asymptotes — clamp or wrap the distorted coordinate to prevent NaN/Inf artifacts near poles
- Spoke mode needs `atan()` discontinuity handling at ±π boundary (wrap seam)
- Ring mode convergence may need radius clamping to avoid degenerate bunching at origin
- The old scan-lines branch (`5a11dcc`) used FBM wave displacement on scan lines — that could be a future addition but is out of scope for initial implementation
