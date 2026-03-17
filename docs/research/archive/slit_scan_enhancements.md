# Slit Scan Enhancements

Three changes to the existing Slit Scan Corridor effect: rename to "Slit Scan", add a flat (single-wall) mode, and add a center glow parameter.

## Classification

- **Category**: TRANSFORMS > Motion (existing effect, section 3)
- **Pipeline Position**: Same as current Slit Scan Corridor
- **Type**: Enhancement to existing effect

## References

- Existing implementation: `src/effects/slit_scan_corridor.*`, `shaders/slit_scan_corridor*.fs`
- Visual reference: Kubrick's *2001: A Space Odyssey* Stargate sequence (1.png, 2.png in project root)

## Changes

### 1. Rename: "Slit Scan Corridor" → "Slit Scan"

Full rename across all identifiers:
- Files: `slit_scan_corridor.*` → `slit_scan.*` (`.h`, `.cpp`, `.fs`, `_display.fs`)
- Structs: `SlitScanCorridorConfig` → `SlitScanConfig`, `SlitScanCorridorEffect` → `SlitScanEffect`
- Functions: `SlitScanCorridorEffect*()` → `SlitScanEffect*()`
- Config field: `EffectConfig::slitScanCorridor` → `EffectConfig::slitScan`
- Param IDs: `"slitScanCorridor.*"` → `"slitScan.*"`
- PostEffect field: `PostEffect::slitScanCorridor` → `PostEffect::slitScan`
- Enum value: `TRANSFORM_SLIT_SCAN_CORRIDOR` → `TRANSFORM_SLIT_SCAN`
- Display name: `"Slit Scan Corridor"` → `"Slit Scan"`
- Serialization key in `effect_serialization.cpp`
- CMakeLists.txt source entry
- No backwards-compatibility shims.

### 2. Flat Mode

Add a `mode` field (int, combo box): 0 = Corridor (default), 1 = Flat.

**Accumulation shader (`slit_scan.fs`):**

Currently hardcodes center at 0.5:
```glsl
float dx = uv.x - 0.5;
```

Replace with a `center` uniform:
```glsl
float dx = uv.x - center;
```

- Corridor mode: `center = 0.5` (pushes outward both directions, current behavior)
- Flat mode: `center = 0.0` (slit at left edge, pushes rightward across full frame)

The rest of the accumulation math generalizes naturally — `sign(dx)` is always positive when center = 0.0, so all pixels push in one direction. The `halfPixel` minimum prevents sampling across the boundary.

**Display shader (`slit_scan_display.fs`):**

Add `center` uniform. Replace hardcoded 0.5 references:
```glsl
// Current
float dist = max(abs(uv.x - 0.5), 0.001);
float fog = pow(clamp(dist * 2.0, 0.0, 1.0), fogStrength);

// Generalized
float dist = max(abs(uv.x - center), 0.001);
float maxDist = max(center, 1.0 - center);  // 0.5 for corridor, 1.0 for flat
float normDist = clamp(dist / maxDist, 0.0, 1.0);
float fog = pow(normDist, fogStrength);
```

Perspective uses the same normalization:
```glsl
float scale = 1.0 + perspective * normDist;
```

Rotation center also shifts with `center` uniform for consistency.

**Flat mode + rotation**: 180° rotation visually flips the slit to the right edge, so no separate left/right option needed.

### 3. Center Glow

Add a `glow` parameter (float, 0.0–5.0, default 0.0).

In the display shader, after fog:
```glsl
// Center glow: brighten near slit, falloff outward
if (glow > 0.0) {
    float g = pow(1.0 - normDist, glow);
    color *= 1.0 + g;
}
```

- `glow = 0`: no effect (current behavior)
- `glow = 1`: gentle center brightness boost
- `glow = 5`: tight, intense center glow (Stargate look)

Works in both corridor and flat modes via the same `normDist` value.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| mode | int | 0-1 | 0 | 0 = Corridor (symmetric), 1 = Flat (single wall) |
| glow | float | 0.0-5.0 | 0.0 | Center brightness falloff steepness |

All existing parameters remain unchanged.

## Modulation Candidates

- **glow**: Modulating glow intensity makes the center seam pulse — dramatic at high values, subtle at low
- **glow + brightness**: Brightness controls slit injection intensity, glow controls display-time center emphasis. High brightness + high glow = blindingly hot seam. Low brightness + high glow = warm but subdued center. Cascading threshold — glow amplifies whatever brightness provides.

## Notes

- `mode` is an int combo box, not modulatable
- Flat mode uses full texture resolution (no wasted half like a UV crop would)
- Existing presets will load with `mode = 0` and `glow = 0.0` via default initialization — no migration needed
- File rename means old preset JSON keys (`slitScanCorridor`) won't deserialize into new keys (`slitScan`) — this is intentional per project conventions (no compat shims)
