# General Warp

A unified UV distortion effect with multiple warp algorithms. Replaces the need for many single-purpose warp effects by exposing the underlying distortion type as a parameter.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: UV displacement based on selected algorithm
- **Pipeline Position**: Transform stage (user-ordered)

## Design Philosophy

**Orthogonal to Depth Layers:** This effect handles UV distortion only. No layering, blending, or depth cycling—that's Depth Layers' job. Chain them for combined results.

**One warp type per instance:** Keep it simple. Users add multiple Warp effects to the chain if they want to combine distortions (e.g., Noise → Twirl).

**Consistent interface:** All warp types share core parameters (strength, speed, center). Type-specific parameters show/hide based on selection.

## Warp Categories

### 1. Noise-Based
Displace UV by sampling noise functions.

| Type | Character | Key Params |
|------|-----------|------------|
| Simplex | Smooth organic flow | freq, speed |
| Perlin | Classic gradient noise | freq, speed |
| Worley | Cellular/bubble distortion | freq, speed |
| fBM | Layered fractal detail | freq, octaves, lacunarity, gain |
| Turbulence | Sharp valleys/creases | freq, octaves |
| Ridged | Sharp peaks/ridges | freq, octaves |

### 2. Recursive/Domain
Feed warped coordinates back into the warp function.

| Type | Character | Key Params |
|------|-----------|------------|
| Domain Warp | Flowing marble/terrain | iterations, warpScale, octaves |

### 3. Geometric
Mathematical UV transforms.

| Type | Character | Key Params |
|------|-----------|------------|
| Sine | Layered wave distortion | freqX, freqY, phaseOffset |
| Twirl | Rotation that varies with radius | radius, falloff |
| Pinch | Radial scale distortion | radius, falloff |
| Bulge | Inverse pinch (expand from center) | radius, falloff |
| Spherize | Map onto sphere surface | radius, curvature |

### 4. Flow-Based
Divergence-free or flow-field distortion.

| Type | Character | Key Params |
|------|-----------|------------|
| Curl | Smoke-like swirling | freq, speed |
| Vortex | Angle-dependent spiral | foldCount, tightness |

### 5. Radial
Center-relative distortions.

| Type | Character | Key Params |
|------|-----------|------------|
| Radial Push | Expand/contract from center | falloff |
| Tangent Swirl | Circular motion around center | falloff |
| Ripple | Concentric ring displacement | freq, decay |

## Shared Parameters

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `warpType` | enum | - | Algorithm selection |
| `strength` | float | 0.0-2.0 | Displacement amplitude |
| `speed` | float | 0.0-2.0 | Animation rate (some types) |
| `centerX` | float | 0.0-1.0 | Focal center X (radial/geometric types) |
| `centerY` | float | 0.0-1.0 | Focal center Y (radial/geometric types) |

## Type-Specific Parameters

### Noise Types (Simplex, Perlin, Worley, Curl)
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `freq` | float | 0.5-20.0 | 4.0 |

### fBM Types (fBM, Turbulence, Ridged)
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `freq` | float | 0.5-20.0 | 4.0 |
| `octaves` | int | 1-8 | 4 |
| `lacunarity` | float | 1.5-3.0 | 2.0 |
| `gain` | float | 0.3-0.7 | 0.5 |

### Domain Warp
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `iterations` | int | 1-4 | 2 |
| `warpScale` | float | 0.5-4.0 | 1.5 |
| `octaves` | int | 1-6 | 4 |
| `lacunarity` | float | 1.5-3.0 | 2.0 |
| `gain` | float | 0.3-0.7 | 0.5 |

### Sine
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `freqX` | float | 0.5-20.0 | 4.0 |
| `freqY` | float | 0.5-20.0 | 4.0 |
| `phaseOffset` | float | 0.0-TAU | 0.0 |

### Geometric (Twirl, Pinch, Bulge, Spherize)
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `radius` | float | 0.1-2.0 | 0.5 |
| `falloff` | enum | Linear/Smooth/Exp | Smooth |

### Vortex
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `foldCount` | int | 1-12 | 3 |
| `tightness` | float | 0.0-2.0 | 1.0 |

### Ripple
| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `freq` | float | 1.0-30.0 | 10.0 |
| `decay` | float | 0.0-5.0 | 2.0 |

## Algorithm Sketches

### Noise-Based (General Pattern)
```glsl
vec2 noiseWarp(vec2 uv, float freq, float strength, float time) {
    vec2 offset = vec2(
        noise(uv * freq + time),
        noise(uv * freq + time + 100.0)
    );
    return uv + (offset - 0.5) * strength;
}
```

### Domain Warp (Recursive)
```glsl
vec2 domainWarp(vec2 uv, int iterations, float scale, ...) {
    for (int i = 0; i < iterations; i++) {
        vec2 q = vec2(
            fbm(uv + vec2(0.0, 0.0)),
            fbm(uv + vec2(5.2, 1.3))
        );
        uv = uv + q * scale;
    }
    return uv;
}
```

### Twirl
```glsl
vec2 twirl(vec2 uv, vec2 center, float strength, float radius) {
    vec2 d = uv - center;
    float r = length(d);
    float falloff = smoothstep(radius, 0.0, r);
    float angle = strength * falloff;
    return center + rotate(d, angle);
}
```

### Curl Noise
```glsl
vec2 curlWarp(vec2 uv, float freq, float strength, float time) {
    float eps = 0.001;
    float n1 = noise(uv * freq + vec2(0, eps) + time);
    float n2 = noise(uv * freq - vec2(0, eps) + time);
    float n3 = noise(uv * freq + vec2(eps, 0) + time);
    float n4 = noise(uv * freq - vec2(eps, 0) + time);
    vec2 curl = vec2((n1 - n2), -(n3 - n4)) / (2.0 * eps);
    return uv + curl * strength;
}
```

### Vortex (from Shadertoy analysis)
```glsl
vec2 vortex(vec2 uv, vec2 center, float strength, int folds, float tightness) {
    vec2 d = uv - center;
    float a = atan(d.y, d.x) * float(folds);
    float r = length(d);
    float angle = a * tightness + strength;
    return center + vec2(cos(angle), sin(angle)) * r;
}
```

## Open Questions

1. **Bounds handling:** When UV goes out of [0,1], clamp/wrap/mirror/discard? Probably a shared param across all types.

2. **Aspect ratio:** Should warp account for non-square aspect? Noise looks stretched otherwise.

3. **Pre-baked noise textures:** For performance, sample textures instead of computing noise. Worth the texture slots?

4. **Combining warps:** Allow two warp types in one effect (Type A + Type B with blend), or keep it pure single-type?

5. **Modulation:** Which params are high-value modulation targets?
   - `strength` - obvious choice for beat reactivity
   - `speed` - less useful if we use CPU accumulation
   - `centerX/Y` - interesting for drifting focal point
   - `freq` - breathing/pulsing texture scale

## Overlap with Existing Effects

| Existing Effect | Covered By | Notes |
|-----------------|------------|-------|
| Sine Warp | Sine type | Direct replacement |
| Texture Warp | Domain Warp type | Direct replacement |
| Wave Ripple | Ripple type | Direct replacement |
| Domain Warp | Domain Warp type | Already exists, subsume it |
| Chladni Warp | ? | Might need its own type or stay separate |
| Mobius | ? | Complex plane math—might stay separate |
| Gradient Flow | ? | Edge-based, different paradigm |

## UI Organization

```
[Warp Type ▾] ← dropdown, all types

── Strength ─────────────────○──
── Speed ────────────○──────────

[Center] (X, Y) ← draggable handle, show for radial/geometric

── Type-Specific ──────────────
   (parameters change based on type)

[Bounds ▾] Clamp / Wrap / Mirror
```

## Next Steps

1. Decide which existing effects to deprecate vs keep separate
2. Finalize parameter set
3. Prototype shader with 2-3 warp types to validate architecture
4. Full implementation via /feature-plan
