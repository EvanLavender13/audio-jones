# Muons Distance Function Modes

Muons' visual character comes from its FBM-warped raymarching loop, not the specific distance function. By swapping the single line that computes `d` (the shell/surface distance in warped space), the same turbulence engine produces fundamentally different visuals — from thin filaments to swirling tubes to crystalline caverns. Each mode needs different default parameters to look good.

## Classification

- **Category**: GENERATORS > Filament (existing Muons effect)
- **Pipeline Position**: Generator stage (no change)
- **Scope**: Add a `mode` dropdown to Muons that selects which distance function the shader evaluates. One new uniform, one switch in the shader, one int in the config.

## Discovery

The distance function lives at line 61 of `shaders/muons.fs`:

```glsl
s = length(a);
d = ringThickness * abs(sin(s));
```

`a` is the sample point after Rodrigues rotation + FBM turbulence. `s` is its magnitude. `d` controls both the visual density (color accumulates where `d` is small) and the adaptive step size (ray crawls near surfaces, races through empty space).

The FBM warping is so aggressive that it dominates the visual character — but the distance function determines the *topology* of what gets warped:
- Concentric spheres → tangled filaments
- Axis-aligned tubes → swirling cosmic tendrils
- Diagonal planes → crystalline caverns
- Coordinate-product saddles → neon spirograph traces

Key finding: **periodic functions** (`sin`, `fract`, gyroid) all get warped into similar filaments. **Distance metrics** and **coordinate products** (non-periodic, broad value regions) resist the FBM and produce genuinely different visuals.

## Confirmed Modes

### Mode 0: Sine Shells (current default)

```glsl
d = ringThickness * abs(sin(s));
```

`abs(sin(length))` — periodic radial function. This is current Muons, no change needed.

### Mode 1: L1 Norm

```glsl
d = ringThickness * (abs(a.x - a.y) + abs(a.y - a.z) + abs(a.z - a.x));
```

Sum of absolute pairwise coordinate differences (L1 distance between components). Values can be much larger than mode 0, needs higher `ringThickness` (~0.008+) to get visible structure.

**Tested settings that worked**: ringThickness 0.008, turbulence 0.15, camera distance 5.0 (approximate from screenshot).

### Mode 2: Axis Distance

```glsl
d = ringThickness * min(length(a.xy), min(length(a.yz), length(a.xz)));
```

Minimum distance to the three coordinate axes (min of biaxial lengths).

**Tested settings that worked**: march steps 40, turbulence octaves 12, ringThickness 0.008, camera distance 5.0 (approximate from screenshot).

### Mode 3: Dot Product

```glsl
d = ringThickness * abs(a.x*a.y + a.y*a.z);
```

Partial dot product of shifted components (two coordinate-product terms summed).

**Tested and confirmed visually distinct.**

## Rejected / Too Similar

These were tested and looked too much like the original Muons filaments:

- **Grid planes** (`min(abs(fract(a)-0.5))`) — periodic thin crossings, warped into filaments
- **Gyroid** (`sin(x)*cos(y) + sin(y)*cos(z) + sin(z)*cos(x)`) — periodic, same filament look
- **Repeating cells product** (`fract(x)*fract(y)*fract(z)`) — near-zero everywhere, just bright Muons
- **Min of three sines** (`min(abs(sin(a.x)), ...)`) — periodic thin crossings, same filaments

**Pattern**: Periodic functions (`sin`, `fract`) all produce filaments because the FBM warps their thin zero-crossings identically. Only non-periodic metrics and coordinate products produce distinct visuals.

### Mode 4: Chebyshev Spread

```glsl
vec3 aa = abs(a);
d = ringThickness * abs(max(aa.x, max(aa.y, aa.z)) - min(aa.x, min(aa.y, aa.z)));
```

Difference between Chebyshev norm (max component) and minimum component. Subtle but distinct character.

**Tested and confirmed visually distinct.**

### Mode 5: Cone Metric

```glsl
d = ringThickness * abs(length(a.xy) - abs(a.z));
```

Biaxial length minus third axis — distance to a cone isosurface. Subtle but distinguishable.

**Tested and confirmed visually distinct.**

### Mode 6: Triple Product

```glsl
d = ringThickness * abs(a.x * a.y * a.z);
```

Product of all three coordinates — zero on the three coordinate planes. Most dramatically different mode; inverts the spatial distribution compared to all others.

**Tested and confirmed visually distinct. User favorite.**

## Implementation Approach

### Shader
- Add `uniform int mode;` to muons.fs
- Replace the single distance computation line with a switch on `mode`
- Each branch computes `d` differently; everything else (ray setup, FBM, color accumulation, trails) stays identical

### Config
- Add `int mode = 0;` to `MuonsConfig`
- Add to `MUONS_CONFIG_FIELDS` macro
- Register as non-modulatable (integer dropdown, not a float slider)

### UI
- Add `ImGui::Combo` dropdown before the raymarching params
- Labels TBD — need creative names, not visual descriptions

### Naming
Mode labels describe the math, not the visual. Names in the dropdown:
`Sine Shells`, `L1 Norm`, `Axis Distance`, `Dot Product`, `Chebyshev Spread`, `Cone Metric`, `Triple Product`

## Notes

- Each mode needs different default parameter sweet spots. Consider per-mode default presets, or at minimum document the tested settings.
- The mode is a shader branch, not a performance concern — only one branch executes per pixel.
- Some modes produce larger `d` values than mode 0, which means the ray takes bigger steps and accumulates less color. Modes with larger value ranges may need their own `ringThickness` scaling or a per-mode multiplier.
- The FBM warping, trail persistence, FFT mapping, and color system are mode-independent. Only the distance function changes.
