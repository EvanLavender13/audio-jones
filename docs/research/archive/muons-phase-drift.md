# Muons Phase & Drift Controls

Expose the hardcoded rotation axis phase vector and add a drift rate parameter so users can tune the spatial orientation and break the ~6.3s temporal cycle.

## Classification

- **Category**: GENERATORS > Filament (existing effect extension)
- **Pipeline Position**: No change — same generator pass
- **Scope**: Shader uniforms + config fields + UI sliders + serialization

## References

- Existing: `shaders/muons.fs` line 50 — current hardcoded phase + drift
- Original: XorDev's "Muons" Shadertoy — `vec3(7,1,0)` was arbitrary code golf constant
- Golden ratio aperiodicity — PHI-scaled per-axis speeds ensure incommensurable periods

## Reference Code

Current shader line (combined from two incremental changes):

```glsl
const float PHI = 1.6180339887;
// ...
vec3 a = normalize(cos(vec3(7.0, 1.0, 0.0) + time * 0.05 * vec3(1.0, PHI, PHI * PHI) + time - s));
```

This is mathematically equivalent to:

```glsl
vec3 a = normalize(cos(phase + time * (1.0 + drift * vec3(1.0, PHI, PHI * PHI)) - s));
```

Where `phase = vec3(7,1,0)` and `drift = 0.05`. At `drift = 0`, all axes animate at speed 1.0 and the structure cycles every 2π seconds (vanilla behavior). At `drift > 0`, per-axis speeds diverge and the cycle never repeats.

## Algorithm

Replace the hardcoded line with:

```glsl
uniform vec3 phase;     // was vec3(7.0, 1.0, 0.0)
uniform float drift;    // was 0.05

// ...
vec3 a = normalize(cos(phase + time * (1.0 + drift * vec3(1.0, PHI, PHI * PHI)) - s));
```

- `phase` controls the starting spatial orientation — which regions of the volume are dense vs sparse
- `drift` controls how fast the bias migrates — 0 = static (vanilla cycling), higher = faster migration
- The `vec3(1, PHI, PHI²)` ratio is fixed (not user-facing) — it's the mathematical structure that guarantees aperiodicity

C++ side: three float config fields for phase (`phaseX`, `phaseY`, `phaseZ`) and one float for `drift`. Bound as `vec3` uniform via `SetShaderValue` with `SHADER_UNIFORM_VEC3` for phase, `SHADER_UNIFORM_FLOAT` for drift. No CPU accumulation needed — these are direct uniforms.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| phaseX | float | 0.0-6.28 | 7.0 | Rotation axis X phase offset |
| phaseY | float | 0.0-6.28 | 1.0 | Rotation axis Y phase offset |
| phaseZ | float | 0.0-6.28 | 0.0 | Rotation axis Z phase offset |
| drift | float | 0.0-0.5 | 0.05 | Per-axis speed divergence — 0 = vanilla cycling |

## Modulation Candidates

- **phaseX/Y/Z**: Shifts which screen regions are dense vs sparse — modulating creates wandering focal areas
- **drift**: Controls how quickly the spatial bias moves — higher values during energetic passages prevent stale patterns

## UI

Add to the Geometry section, after the existing Mode combo:

- `"Phase X"`, `"Phase Y"`, `"Phase Z"` — three `ModulatableSliderAngleDeg` sliders (values are cosine input, so angle display is natural)
- `"Drift"` — one `ModulatableSlider` with `"%.3f"` format

## Files Changed

| File | Change |
|------|--------|
| `src/effects/muons.h` | Add `phaseX`, `phaseY`, `phaseZ`, `drift` to `MuonsConfig`; add `phaseLoc`, `driftLoc` to `MuonsEffect`; update `MUONS_CONFIG_FIELDS` |
| `src/effects/muons.cpp` | Cache uniform locations in Init, bind `vec3(phaseX, phaseY, phaseZ)` and `drift` in Setup, add 4 sliders in UI |
| `shaders/muons.fs` | Replace hardcoded phase/drift with `uniform vec3 phase` and `uniform float drift`; remove redundant double-time term |

## Notes

- Phase range 0-2π covers the full cosine period — values beyond wrap identically
- Default `drift = 0.05` matches current behavior; `drift = 0` restores vanilla XorDev cycling for users who want it
- The PHI ratio `vec3(1, PHI, PHI²)` stays hardcoded in the shader — it's structural, not a creative knob
- Serialization uses three separate floats (`phaseX/Y/Z`) to avoid vec3 serialization complexity
