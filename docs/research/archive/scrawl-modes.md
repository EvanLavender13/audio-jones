# Scrawl Fractal Fold Modes

Scrawl's visual character comes from the IFS fold formula inside its iteration loop. By swapping the single fold line, the same rendering framework (min-distance tracking, sinusoidal warp, smoothstep stroke, gradient LUT coloring, per-cell rotation) produces fundamentally different curve topologies — from smooth marker swooshes to sharp crystalline branches to spiraling tangles.

## Classification

- **Category**: GENERATORS > Texture (existing Scrawl effect)
- **Pipeline Position**: Generator stage (no change)
- **Scope**: Add a `mode` int to Scrawl that selects which fold formula the shader evaluates. One new uniform, one switch in the shader, one int in the config.

## Discovery

The fold formula lives at line 47 of `shaders/scrawl.fs`:

```glsl
uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
```

Everything else in the loop — the sinusoidal distance warp (`asin(0.9 * sin(length(uv) * warpFreq)) * warpAmp`), min-distance tracking, and winning-iteration recording — stays identical across all modes. Only the fold line changes.

Also discovered during testing: per-cell rotation (using `floor()` cell index to derive a pseudo-random angle before `fract()` tiling) breaks the wallpaper repetition and is already in the shader.

## Confirmed Modes

### Mode 0: IFS Fold (current default)

```glsl
uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
```

The `x*y` product in the divisor creates smooth curving strokes. This is current Scrawl, no change needed.

### Mode 1: Kali Set

```glsl
uv = abs(uv) / dot(uv, uv) - foldOffset;
```

Inversion by squared magnitude instead of clamped product. Sharp crystalline branches like frost on glass.

**User reaction**: "awesome"

### Mode 2: Burning Ship

```glsl
uv = vec2(uv.x * uv.x - uv.y * uv.y, 2.0 * abs(uv.x * uv.y)) - foldOffset;
```

Mandelbrot-like squaring with abs before the cross term. Asymmetric, flame-like branching — jagged and directional.

**User reaction**: "interesting"

### Mode 3: Menger Fold

```glsl
uv = abs(uv) - foldOffset; if (uv.x < uv.y) uv = uv.yx;
```

Abs-fold with conditional axis swap. Angular cross/sponge patterns with sharp right-angle structure.

**User reaction**: "pretty cool"

### Mode 4: Box Fold

```glsl
uv = clamp(uv, -foldOffset, foldOffset) * 2.0 - uv;
uv /= clamp(dot(uv, uv), 0.25, 1.0);
```

Mandelbox-style box fold + sphere inversion. Blocky recursive structures with organic curves at edges. The sphere inversion step is critical — without it the fold is too flat.

**User reaction**: "a lot cooler"

### Mode 5: Spiral IFS

```glsl
uv = abs(uv) / clamp(abs(uv.x * uv.y), 0.5, 1.0) - 1.0 - foldOffset;
float ca = 0.714, sa = 0.700;
uv *= mat2(ca, -sa, sa, ca);
```

Same fold as mode 0 but with a ~40-degree rotation per iteration. Curves spiral into each other instead of repeating straight.

**User reaction**: "nice nice"

### Mode 6: Power Fold

```glsl
uv = pow(abs(uv), vec2(1.5)) * sign(uv) - foldOffset;
```

Nonlinear exponent before offset. Soft organic blobby curves — less angular than the other modes.

**User reaction**: "not bad"

## Rejected

- **Box fold without sphere inversion** (`clamp * 2.0 - uv` alone) — too flat, "not cool"
- **Mirror tiling** (triangle wave instead of `fract()`) — "too symmetrical"
- **No tiling** (removed `fract()` entirely) — user preferred the cell structure

## Implementation Approach

### Shader
- Add `uniform int mode;` to scrawl.fs
- Replace the single fold line with a switch on `mode`
- Mode 4 (Box Fold) is two lines; Mode 5 (Spiral IFS) adds a rotation after the fold. All others are single-line replacements.
- Everything else (distance warp, min tracking, stroke rendering, LUT coloring, cell rotation) stays identical.

### Config
- Add `int mode = 0;` to `ScrawlConfig`
- Add to `SCRAWL_CONFIG_FIELDS` macro
- Register as non-modulatable (integer dropdown, not a float slider)

### UI
- Add `ImGui::Combo` dropdown before the geometry params in the Scrawl section
- Labels: `IFS Fold`, `Kali Set`, `Burning Ship`, `Menger Fold`, `Box Fold`, `Spiral IFS`, `Power Fold`

## Notes

- All modes share the same parameter set (`foldOffset`, `warpFreq`, `warpAmp`, `thickness`, etc.). Different modes may have different sweet spots but no per-mode defaults are needed — the existing defaults work acceptably across all modes.
- The mode is a shader branch, not a performance concern — only one branch executes per pixel.
- The per-cell rotation (`cellAngle` derived from `floor()` cell index) is already in the shader and mode-independent.
- More modes can be added later by extending the switch — the framework is open-ended.
