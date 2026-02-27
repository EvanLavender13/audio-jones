# Prism Shatter — Displacement Modes

Add a switchable displacement operation to the Prism Shatter raymarcher, like Muons' distance metric modes.

## Classification

- **Category**: GENERATORS > Geometric (existing effect)
- **Pipeline Position**: Generator stage
- **Scope**: Shader mode switch + config/UI plumbing

## Tested Modes

Six displacement operations tested individually against the original baseline. Each replaces the `newPos` computation in the `scan()` loop.

### Mode 0: Triple Product Gradient (default)

The original. Gradient of the scalar field f(x,y,z) = xyz.

```glsl
newPos.x = posMod.y * posMod.z;
newPos.y = posMod.x * posMod.z;
newPos.z = posMod.x * posMod.y;
```

### Mode 1: Absolute Fold

IFS-style mirror folding across each axis, offset by 1.

```glsl
newPos = abs(posMod) - 1.0;
```

### Mode 2: Mandelbox Fold

Box fold (clamp and reflect) followed by conditional spherical fold. The two-step operation from the Mandelbox fractal: values inside inner radius get scaled up, values between inner and outer get inverted.

```glsl
newPos = clamp(posMod, -1.0, 1.0) * 2.0 - posMod;
float r2 = dot(newPos, newPos);
if (r2 < 0.25) newPos *= 4.0;
else if (r2 < 1.0) newPos /= r2;
```

### Mode 3: Sierpinski Fold

Three reflections across tetrahedral symmetry planes. The standard fold sequence for Sierpinski tetrahedron IFS fractals.

```glsl
newPos = posMod;
newPos.xy -= 2.0 * min(newPos.x + newPos.y, 0.0) * vec2(0.5);
newPos.xz -= 2.0 * min(newPos.x + newPos.z, 0.0) * vec2(0.5);
newPos.yz -= 2.0 * min(newPos.y + newPos.z, 0.0) * vec2(0.5);
```

### Mode 4: Menger Fold

Absolute value then sort components descending. The fold operation from Menger sponge IFS — forces a canonical ordering that creates cubic symmetry.

```glsl
newPos = abs(posMod);
if (newPos.x < newPos.y) newPos.xy = newPos.yx;
if (newPos.x < newPos.z) newPos.xz = newPos.zx;
if (newPos.y < newPos.z) newPos.yz = newPos.zy;
```

### Mode 5: Burning Ship Fold

The original triple product gradient applied to `abs(v)`. Same relationship as Burning Ship fractal to Mandelbrot: absolute value before the core operation distorts fractal boundaries.

```glsl
vec3 a = abs(posMod);
newPos.x = a.y * a.z;
newPos.y = a.x * a.z;
newPos.z = a.x * a.y;
```

## Rejected Modes

| Mode | Why |
|------|-----|
| Cross Product: `cross(v, v.yzx)` | Not different enough from original |
| Rodrigues Rotation: `v*dot(v,v.yzx) - cross(v,v.yzx)` | Looked like a noise field |
| Box Fold alone: `clamp(v,-1,1)*2 - v` | Absolute Fold looked better; Box Fold kept as part of Mandelbox |
| Spherical Inversion: `v / dot(v,v)` | Inconclusive |
| Octahedral Fold | Indistinguishable from earlier keepers |

## Implementation

- Add `int displacementMode = 0` to `PrismShatterConfig` (range 0-5)
- Add `uniform int displacementMode` to shader, `switch` statement in `scan()` loop
- Add `int displacementModeLoc` to `PrismShatterEffect`, wire in Init/Setup
- Add `ImGui::Combo` in UI with mode names
- Add field to `PRISM_SHATTER_CONFIG_FIELDS` for serialization

### Mode Names (for UI combo)

```
"Triple Product\0Absolute Fold\0Mandelbox\0Sierpinski\0Menger\0Burning Ship\0"
```

## Parameters

No new float parameters. Single int mode selector.

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| displacementMode | int | 0-5 | 0 | Selects displacement operation |
