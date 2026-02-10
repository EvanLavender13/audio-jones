# MilkDrop Feedback Architecture

## How MilkDrop Works

MilkDrop's visuals emerge from a **persistent texture feedback loop**:

1. Each frame samples the previous frame's image at **offset UV coordinates**
2. Small UV offsets compound over hundreds of frames into complex motion
3. The waveform is a **tiny perturbation** - the feedback loop amplifies it

### Rendering Pipeline (per frame)

```
prevTexture
    ↓
[Warp Shader] ← samples prevTexture at displaced UVs, applies blur/decay
    ↓
targetTexture
    ↓
[Draw Shapes/Waves] ← drawn directly onto targetTexture with blend modes
    ↓
[Composite Shader] ← display-only effects (gamma, echo, etc.)
    ↓
Screen

(swap: prevTexture ↔ targetTexture)
```

| Stage | Purpose |
|-------|---------|
| Per-frame equations | Compute global params (zoom, rot, warp) from audio/time |
| Per-vertex equations | Compute spatially-varying UV offsets across mesh grid |
| Warp shader | Sample previous frame at warped UVs, apply blur/decay |
| Draw shapes/waves | Render visual elements directly onto warp output |
| Composite shader | Final effects → display only (does not feed back) |

### Parameter Sensitivity

Feedback parameters compound exponentially:
- `zoom = 0.99` → 60 frames → `0.99^60 = 0.55` (half size)
- `zoom = 0.97` → 60 frames → `0.97^60 = 0.16` (tiny)

MilkDrop presets use **extremely narrow ranges** (zoom 0.99-1.01, rot ±0.005) because small changes create large visual velocity differences.

---

## Drawing Into the Feedback Texture

Visual elements (waveforms, shapes, borders) are drawn **directly onto the warp shader output** using OpenGL blend modes. They are NOT composited through a separate blending pass.

### Draw Order

From [butterchurn's renderer](https://github.com/jberg/butterchurn):

```
1. Warp shader renders (samples previous frame)
2. Motion vectors
3. Custom shapes (up to 4, or 16 in MilkDrop3)
4. Custom waveforms (up to 4, or 16 in MilkDrop3)
5. Basic waveform (modes 0-7)
6. Darken center
7. Borders
   ↓
Result becomes prevTexture for next frame
```

All elements above participate in feedback - they persist and get warped in subsequent frames.

### Blend Modes

Elements use standard OpenGL blending, not a separate mix pass:

| Element | Blend Control | Effect |
|---------|---------------|--------|
| Basic waveform | `wave_additive` (0/1) | 0 = alpha blend, 1 = additive |
| Custom waveforms | Per-wave `additive` flag | Same as above |
| Custom shapes | Per-shape `additive` flag | Same as above |

**Alpha blend** (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`):
- `final = dest * (1 - src.a) + src * src.a`
- Waveform replaces/mixes based on its alpha
- Where no waveform drawn, feedback is **untouched**

**Additive blend** (`GL_ONE, GL_ONE`):
- `final = dest + src`
- Saturates toward white
- Good for glowing/bright effects

### Why This Matters

The waveform's own alpha (`wave_a`) controls how much it affects the feedback. A waveform with `wave_a = 0.3` blends 30% waveform + 70% feedback **only where the waveform is drawn**. The rest of the frame passes through unchanged.

---

## AudioJones Implementation

### Current Pipeline

```
Frame N:
1. feedback_exp.fs: expAccumTexture → expTempTexture     [WARP]
   - Spatial flow field (zoom, rotation, dx/dy)
   - 3x3 Gaussian blur
   - Framerate-independent half-life decay

2. Waveform drawn to injectionTexture (cleared to BLACK each frame)

3. blend_inject.fs: mix(expTempTexture, injectionTexture, opacity) → expAccumTexture
   - Result becomes input for next frame

4. composite.fs: expAccumTexture → screen               [COMPOSITE]
   - Gamma correction (display-only, does not feed back)
```

### Mapping to MilkDrop

| MilkDrop | AudioJones | Notes |
|----------|------------|-------|
| Warp shader | `feedback_exp.fs` | UV displacement, blur, decay |
| Direct drawing onto warp output | `injectionTexture` + `blend_inject.fs` | Extra indirection |
| Composite shader | `composite.fs` | Gamma implemented |

### Current Issue: blend_inject Approach

The `blend_inject.fs` shader uses a global mix:

```glsl
vec3 blended = mix(feedback.rgb, injection.rgb, injectionOpacity);
```

With `injectionTexture` cleared to BLACK and opacity at 0.3:
- **Where waveform exists:** `feedback * 0.7 + waveform * 0.3` ✓
- **Where NO waveform:** `feedback * 0.7 + BLACK * 0.3 = feedback * 0.7` ✗

This dims the **entire frame** by `(1 - injectionOpacity)` each frame, on top of the decay already in `feedback_exp.fs`. Double-dimming.

### Correct Approach

**Option A: MilkDrop style (recommended)**
Draw waveforms directly onto `expTempTexture` after the warp pass using OpenGL blend states:

```
1. feedback_exp.fs: expAccumTexture → expTempTexture
2. BeginTextureMode(expTempTexture)
   - Set blend mode (alpha or additive)
   - Draw waveforms/shapes directly
   EndTextureMode()
3. Copy expTempTexture → expAccumTexture
4. composite.fs → screen
```

No blend_inject shader needed. OpenGL's blend modes handle compositing correctly.

**Option B: Fix blend_inject to use alpha**

Clear injection to transparent `(0,0,0,0)` and use per-pixel alpha:

```glsl
vec3 blended = mix(feedback.rgb, injection.rgb, injection.a * injectionOpacity);
```

Where injection is transparent (no waveform), `injection.a = 0`, so feedback passes through unchanged.

---

## Spatial UV Variation

### MilkDrop's Mesh-Based Approach

MilkDrop uses a **CPU-evaluated mesh grid** (typically 48x36 or 64x48 vertices) where per-vertex equations run at each grid point. The GPU then **interpolates** results across triangle faces.

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
zoom = 0.99 + rad * 0.02;           // Edges zoom faster (tunnel)
rot = 0.01 * sin(ang * 2 + time);   // Spiral motion
dx = (x - 0.5) * 0.01 * sin(time);  // Breathing
```

### AudioJones Shader-Based Approach

AudioJones implements spatial UV variation per-pixel in `feedback_exp.fs`:

```glsl
vec2 uv = fragTexCoord - center;
float rad = length(normalized) * 2.0;  // 0 at center, ~1 at edges

float zoom = zoomBase + rad * zoomRadial;
float rot = rotBase + rad * rotRadial;
float dx = dxBase + rad * dxRadial;
float dy = dyBase + rad * dyRadial;

uv *= zoom;
uv = vec2(uv.x * cos(rot) - uv.y * sin(rot),
          uv.x * sin(rot) + uv.y * cos(rot));
uv += vec2(dx, dy);
uv += center;
```

**Current limitation**: All coefficients are radial only. MilkDrop also uses angular variation (`+ sin(ang * N) * coefficient`).

### Flow Field Patterns

**Radial variation** (depends on `rad`):
- `zoom = 0.99 + rad * 0.01` → Center stays still, edges zoom in
- `zoom = 1.01 - rad * 0.02` → Center zooms in, edges zoom out (tunnel)

**Angular variation** (depends on `ang`):
- `rot = 0.01 * sin(ang * 2)` → Two-fold spiral symmetry
- `dx = 0.005 * cos(ang)` → Circular flow around center

**Cartesian variation** (depends on `x`, `y`):
- `dy = 0.01 * sin(x * 6.28)` → Vertical waves across screen
- `dx = (x - 0.5) * 0.02` → Horizontal expansion from center

### Parameter Ranges

| Parameter | Safe Range | Effect at Boundary |
|-----------|------------|-------------------|
| `zoom` | 0.98 - 1.02 | 0.98 → 60fps → 30% size in 1 second |
| `rot` | -0.01 - 0.01 | ±0.01 → full rotation every 10 seconds |
| `dx/dy` | -0.02 - 0.02 | ±0.02 → traverses screen in 1 second |
| `warp` | 0.0 - 2.0 | Procedural effect, less sensitive |

These ranges are **per-frame**. At 60fps, effects compound 60x per second.

---

## MilkDrop Visual Elements

### Built-in Waveform Modes

MilkDrop provides 8 waveform drawing modes (`wave_mode` 0-7):

| Mode | Name | Description |
|------|------|-------------|
| 0 | Circular/Radial | Circle with radius varying by amplitude |
| 1 | Polar Angle | Left channel controls angle, right controls radius |
| 2 | XY Scatter | Classic oscilloscope lissajous |
| 3 | XY Scatter + Treble | Mode 2 with alpha multiplied by treble² |
| 4 | Linear Momentum | Horizontal line with momentum smoothing |
| 5 | Rotated Cross-Product | Complex interlocking patterns |
| 6 | Directional Line | Single line oriented by `wave_mystery` |
| 7 | Dual Lines | Two independent parallel lines |

**Common parameters:**
- `wave_x`, `wave_y` (0..1): Position
- `wave_r`, `wave_g`, `wave_b` (0..1): Color
- `wave_a` (0..1): Opacity
- `wave_additive` (0/1): Additive vs alpha blending
- `wave_usedots`: Draw as points instead of lines
- `wave_thick`: Thicker line rendering

### Custom Waves

Up to 4 custom waves (16 in MilkDrop3) with per-point control:

**Per-point inputs:** `sample` (0..1), `value1` (left), `value2` (right)
**Per-point outputs:** `x`, `y`, `r`, `g`, `b`, `a`

### Custom Shapes

Up to 4 custom shapes (16 in MilkDrop3). Each is an n-sided polygon:

- `sides`, `x`, `y`, `rad`, `ang`
- `r/g/b/a` (center), `r2/g2/b2/a2` (edge gradient)
- `textured`: Sample feedback texture onto shape (enables kaleidoscope, fractal zoom)
- `tex_zoom`, `tex_ang`: Control texture mapping when `textured` is enabled
- `additive`: Blend mode

### Shape Instancing

MilkDrop supports drawing up to 1024 instances of a single shape definition per frame.

**Core mechanism:**
- `num_inst` (1-1024): Number of instances to draw
- `instance` (0 to num_inst-1): Current instance index, available in per-frame code
- Per-frame equations execute once per instance, not once per frame

**Execution model (CPU-side):**
```
for instance = 0 to num_inst-1:
    run per-frame equations (can read 'instance' variable)
    draw shape with resulting x, y, rad, ang, colors, etc.
```

This is CPU looping, not GPU instancing. Each iteration produces different property values based on `instance`.

**Properties controllable per-instance:**

| Property | Description |
|----------|-------------|
| `x`, `y` | Position (0..1) |
| `rad` | Size |
| `ang` | Rotation |
| `sides` | Polygon sides (3-100) |
| `r/g/b/a` | Center color |
| `r2/g2/b2/a2` | Edge color |
| `tex_zoom`, `tex_ang` | Texture mapping |
| `additive`, `thick` | Render flags |

**Common patterns:**

```c
// Circle arrangement
x = 0.5 + 0.3 * cos(instance * 6.28 / num_inst);
y = 0.5 + 0.3 * sin(instance * 6.28 / num_inst);
ang = instance * 6.28 / num_inst;

// Color gradient across instances
r = instance / num_inst;
g = 1.0 - instance / num_inst;

// Audio-reactive size variation
rad = 0.1 + bass * 0.05 * sin(instance * 0.5 + time);

// Spiral arrangement
ang_offset = instance * 6.28 / num_inst + time;
x = 0.5 + (0.1 + instance * 0.02) * cos(ang_offset);
y = 0.5 + (0.1 + instance * 0.02) * sin(ang_offset);
```

**Variable naming note:** Use `num_inst`, not `num_instance`. MilkDrop's expression evaluator changed behavior for variables longer than 8 characters.

### Audio Data

| Variable | Description |
|----------|-------------|
| `bass`, `mid`, `treb` | Frequency bands (instant) |
| `bass_att`, `mid_att`, `treb_att` | Smoothed versions (attack/release) |
| `time`, `fps`, `frame` | Timing |

### Darken Center

Continuously dims the center point, preventing over-accumulation when zoom < 1 (content flowing inward).

---

## Composite Shader (Display-Only)

The composite shader applies effects you want to *see* but not *compound*:

| Pass | Writes To | Persists | Purpose |
|------|-----------|----------|---------|
| Warp + drawing | Internal texture | Yes | Feedback effects |
| Composite | Screen | No | Final styling |

### Effects

**Color manipulation:** `gamma`, `invert`, `brighten`, `darken`, `solarize`

**Video echo:** Trails at output stage (separate from warp trails)
- `echo_zoom`, `echo_alpha`, `echo_orient`

**Other:** Edge detection, blur/glow, film grain, vignette, color grading

### Why Separation Matters

Without composite pass, gamma correction would compound:
- Frame 1: Apply gamma
- Frame 2: Apply gamma to already-gamma'd content
- Frame 60: Completely blown out

With composite pass: warp behaves normally, gamma applies only to final output.

---

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html) - Official documentation
- [projectM](https://github.com/projectM-visualizer/projectm) - Open-source MilkDrop-compatible visualizer
- [Butterchurn](https://github.com/jberg/butterchurn) - WebGL MilkDrop implementation
- [milkdrop-shader-converter](https://github.com/jberg/milkdrop-shader-converter) - HLSL to GLSL conversion tool
