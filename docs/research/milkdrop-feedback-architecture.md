# MilkDrop Feedback Architecture

## How MilkDrop Works

MilkDrop's visuals emerge from a **persistent texture feedback loop**:

1. Each frame samples the previous frame's image at **offset UV coordinates**
2. Small UV offsets compound over hundreds of frames into complex motion
3. The waveform is a **tiny perturbation** - the feedback loop amplifies it

### Rendering Pipeline (per frame)

| Stage | Purpose |
|-------|---------|
| Per-frame equations | Compute global params (zoom, rot, warp) from audio/time |
| Per-vertex equations | Compute spatially-varying UV offsets across mesh grid |
| Warp shader | Sample previous frame at warped UVs → write back to canvas |
| Composite shader | Final effects → display |

### Key Insight: Spatial UV Variation

MilkDrop's mesh is a grid of vertices. Each vertex computes its own UV offset. This creates **flow fields** where different screen regions move differently (center zooms in, edges spiral, etc).

Parameters like `zoom`, `rot`, `warp`, `dx`, `dy` all vary per-vertex based on position (`x`, `y`, `rad`, `ang`).

### Parameter Sensitivity

Feedback parameters compound exponentially:
- `zoom = 0.99` → 60 frames → `0.99^60 = 0.55` (half size)
- `zoom = 0.97` → 60 frames → `0.97^60 = 0.16` (tiny)

MilkDrop presets use **extremely narrow ranges** (zoom 0.99-1.01, rot ±0.005) because small changes create large visual velocity differences.

## Current AudioJones Approach

- Waveform is the **primary visual** drawn at full brightness
- Feedback creates trails/bloom **behind** the waveform
- Feedback applies **uniform** UV transform (no spatial variation)
- Modulating feedback params is jarring due to exponential sensitivity

## Proposed Experimental Approach

### Goal
Waveform as **seed** that generates effects, not as dominant foreground element. Waveform remains visible but feedback-derived visuals dominate.

### Architecture
1. Draw waveform to **injection texture** (not main accumulator)
2. Blend injection **subtly** into feedback buffer (5-20% opacity)
3. Feedback loop with spatial UV variation creates flowing motion
4. Fresh waveform injection each frame keeps it visible amid the flow

### Minimal Starting Point
1. Feedback + blur/decay only (no UV motion initially)
2. Waveform injection at low opacity
3. Validate the seed concept before adding spatial warping

### Success Criteria
- Waveform visible but integrated into flowing background
- Visual motion feels organic, not jarring
- Parameter modulation doesn't cause photosensitive-level changes

---

## Spatial UV Variation: Deep Dive

### MilkDrop's Mesh-Based Approach

MilkDrop uses a **CPU-evaluated mesh grid** (typically 48x36 or 64x48 vertices) where per-vertex equations run at each grid point. The GPU then **interpolates** results across triangle faces, creating smooth spatial variation without computing at every pixel.

**Per-vertex inputs (read-only):**
| Variable | Range | Description |
|----------|-------|-------------|
| `x` | 0..1 | Horizontal position (left to right) |
| `y` | 0..1 | Vertical position (bottom to top) |
| `rad` | 0..1 | Distance from screen center (0 at center, ~1 at corners) |
| `ang` | 0..2π | Angle from center in radians |

**Per-vertex outputs (UV displacement):**
| Variable | Default | Description |
|----------|---------|-------------|
| `zoom` | 1.0 | 0.9 = zoom out 10%, 1.1 = zoom in 10% |
| `rot` | 0.0 | Rotation in radians |
| `dx` | 0.0 | Horizontal displacement (-0.01 = move left 1%) |
| `dy` | 0.0 | Vertical displacement (-0.01 = move up 1%) |
| `warp` | 1.0 | Amplitude of a procedural warping function |

**Example per-vertex equations:**
```
// Edges zoom faster than center (perspective tunnel)
zoom = 0.99 + rad * 0.02;

// Rotate based on angle (spiral motion)
rot = 0.01 * sin(ang * 2 + time);

// Push left-side content rightward, right-side leftward (breathing)
dx = (x - 0.5) * 0.01 * sin(time);
```

### Why Mesh vs Per-Pixel?

MilkDrop was designed for late-90s hardware. Per-vertex equations run on CPU, mesh is ~2000 vertices. Evaluating complex math per-pixel (2M+ pixels) was infeasible. GPU interpolation provides smooth gradients between sparse evaluation points.

Modern GPUs can evaluate complex expressions per-pixel. **However**, the mesh approach has a benefit: it creates **inherently smooth** flow fields. Per-pixel evaluation with discontinuous functions can create harsh visual seams.

### Shader-Based Equivalent (AudioJones Approach)

In a pure fragment shader, compute position-dependent UV offsets directly:

```glsl
// Position inputs (equivalent to MilkDrop per-vertex inputs)
vec2 center = vec2(0.5);
vec2 pos = fragTexCoord - center;
float rad = length(pos) * 2.0;  // 0 at center, ~1 at edges
float ang = atan(pos.y, pos.x);

// Compute spatially-varying zoom
float zoomLocal = zoomBase + rad * zoomRadial;

// Compute spatially-varying rotation
float rotLocal = rotBase + sin(ang * 2.0 + time) * rotAmount;

// Apply transforms
vec2 uv = pos;
uv = mat2(cos(rotLocal), -sin(rotLocal), sin(rotLocal), cos(rotLocal)) * uv;
uv *= zoomLocal;
uv += center;
```

### Key Parameters for Flow Fields

**Radial variation** (depends on `rad`):
- `zoom = 0.99 + rad * 0.01` → Center stays still, edges zoom in
- `zoom = 1.01 - rad * 0.02` → Center zooms in, edges zoom out (tunnel)

**Angular variation** (depends on `ang`):
- `rot = 0.01 * sin(ang * 2)` → Two-fold spiral symmetry
- `dx = 0.005 * cos(ang)` → Circular flow around center

**Cartesian variation** (depends on `x`, `y`):
- `dy = 0.01 * sin(x * 6.28)` → Vertical waves across screen
- `dx = (x - 0.5) * 0.02` → Horizontal expansion from center

### Parameter Ranges (Critical)

From MilkDrop documentation and preset analysis:

| Parameter | Safe Range | Effect at Boundary |
|-----------|------------|-------------------|
| `zoom` | 0.98 - 1.02 | 0.98 → 60fps → 30% size in 1 second |
| `rot` | -0.01 - 0.01 | ±0.01 → full rotation every 10 seconds |
| `dx/dy` | -0.02 - 0.02 | ±0.02 → traverses screen in 1 second |
| `warp` | 0.0 - 2.0 | Procedural effect, less sensitive |

These ranges are **per-frame**. At 60fps, effects compound 60x per second.

---

## Next Steps: Spatial UV for AudioJones

### Is Spatial UV Variation the Minimum Next Step?

**Yes**, spatial UV variation is the key missing piece for MilkDrop-style generative visuals. The current experimental pipeline has:
- ✓ Feedback loop (persistent texture)
- ✓ Blur (softens harsh edges)
- ✓ Decay (prevents infinite accumulation)
- ✓ Uniform zoom (basic motion)
- ✓ Low-opacity waveform injection (seed concept)
- ✗ **Spatial UV variation** (flow fields)

Without spatial variation, all screen regions move identically. This produces **radially symmetric** effects (everything flows to center). Spatial variation creates **asymmetric flow** where different regions exhibit different motion characteristics.

### Proposed Implementation: Phase 7

**Goal**: Add position-dependent UV displacement to `feedback_exp.fs`.

**New parameters:**
```c
struct ExperimentalConfig {
    float halfLife = 0.5f;
    float zoomFactor = 0.995f;
    float injectionOpacity = 0.3f;

    // Spatial variation (Phase 7)
    float zoomRadial = 0.005f;    // Additional zoom at edges (0 = uniform)
    float rotBase = 0.0f;         // Global rotation
    float rotRadial = 0.002f;     // Rotation variation by radius
    float dxWave = 0.0f;          // Horizontal wave amplitude
    float dyWave = 0.0f;          // Vertical wave amplitude
};
```

**Shader changes:**
1. Compute `rad` and `ang` from fragment position
2. Modulate zoom by `rad`: `zoom = zoomFactor + rad * zoomRadial`
3. Compute rotation: `rot = rotBase + rad * rotRadial`
4. Apply rotation matrix before zoom
5. Add optional dx/dy waves

**UI additions:**
- Collapsible "Flow Field" section in Experimental panel
- Sliders for radial zoom, rotation, wave parameters

### Alternative: Start with Rotation Only

Even simpler first step: add just `rotBase` parameter (global rotation). This creates **spiral** effects from the combination of zoom + rotation. Spatial variation can come later.

```glsl
// Minimal addition to feedback_exp.fs
uniform float rotation;

// In main():
vec2 uv = fragTexCoord - center;
float s = sin(rotation);
float c = cos(rotation);
uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
uv *= zoomFactor;
uv += center;
```

This validates the concept with minimal code before adding the full spatial variation system.

---

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html) - Official documentation
- [projectM](https://github.com/projectM-visualizer/projectm) - Open-source cross-platform MilkDrop-compatible visualizer
- [Butterchurn](https://github.com/jberg/butterchurn) - WebGL MilkDrop implementation
- [milkdrop-shader-converter](https://github.com/jberg/milkdrop-shader-converter) - HLSL to GLSL conversion tool