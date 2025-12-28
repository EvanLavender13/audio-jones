# MilkDrop Kaleidoscope, Fractal, and Triangle Effects

Research on techniques for creating kaleidoscope, fractal, and symmetry effects in MilkDrop-style visualizers.

---

## Textured Custom Shapes

The core mechanism for fractals in MilkDrop: shapes that sample the previous frame as a texture.

### Shape Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `textured` | 0 | 1 = map previous frame onto shape |
| `tex_zoom` | 1.0 | Portion of previous frame to sample (>1 = crop/zoom in) |
| `tex_ang` | 0.0 | Rotate the sampled texture (radians, 0..2π) |
| `sides` | 4 | Number of polygon vertices (3-100, up to 500 in MilkDrop3) |
| `x`, `y` | 0.5 | Shape center position (0..1) |
| `rad` | 0.1 | Shape radius |
| `ang` | 0.0 | Shape rotation angle |

### How Textured Shapes Create Fractals

When `textured=1`, the shape samples `prev_frame` texture and renders it at a different position/scale/rotation. The feedback loop compounds this each frame:

```
Frame N:
1. Warp shader renders feedback
2. Textured shape drawn at center, slightly zoomed in (tex_zoom=1.05)
3. Result feeds back to next frame
4. The shape now contains the shape from the previous frame...
   which contained the shape from the frame before...
   = recursive fractal zoom effect
```

The **"Geiss - Feedback"** preset demonstrates this: a textured triangle at screen center with `tex_zoom > 1` creates an infinite zoom tunnel as each frame's shape contains a smaller version of the previous frame.

### Triangle Fractals

Setting `sides=3` creates a triangular shape. With texturing enabled:
- The previous frame image maps onto the triangle
- Each frame, the triangle recursively displays itself
- Combined with `tex_zoom > 1` and slight `tex_ang` rotation: spiral fractal tunnel

---

## Kaleidoscope Effects

Two main approaches: **polar UV folding** or **multiple textured shapes**.

### Approach 1: Polar UV Folding

Transform UV coordinates to polar, fold angles into radial segments, convert back:

```glsl
// 1. Shift origin to center
vec2 uv = fragTexCoord - 0.5;

// 2. Convert to polar coordinates
float radius = length(uv);
float angle = atan(uv.y, uv.x);  // -π to π

// 3. Fold into N segments
float segmentCount = 6.0;
float segmentAngle = TAU / segmentCount;  // TAU = 2π
angle = mod(angle, segmentAngle);  // Reduce to one segment

// 4. Mirror within segment (creates symmetry)
angle = min(angle, segmentAngle - angle);

// 5. Convert back to Cartesian
uv = vec2(cos(angle), sin(angle)) * radius + 0.5;

// 6. Handle edge reflection (keeps pixels in bounds)
uv = max(min(uv, 2.0 - uv), -uv);

vec3 color = texture(prevFrame, uv).rgb;
```

**Key operations:**
- `mod(angle, segmentAngle)` — divides circle into N wedges
- `min(angle, segmentAngle - angle)` — mirrors each wedge, creating symmetry

### Approach 2: Iterative Mirror Function

Repeated rotation + mirroring creates more complex patterns:

```glsl
vec2 mirror(vec2 x) {
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / resolution * 2.0 - 1.0;  // -1 to 1
    float a = time * 0.2;

    for (float i = 1.0; i < 10.0; i += 1.0) {
        // Rotate
        uv = vec2(
            sin(a) * uv.y - cos(a) * uv.x,
            sin(a) * uv.x + cos(a) * uv.y
        );
        // Mirror
        uv = mirror(uv);
        a += i;
        a /= i;
    }

    fragColor = texture(prevFrame, mirror(uv * 2.0));
}
```

Each iteration adds another layer of folding symmetry. Fewer iterations = simpler pattern, more iterations = complex fractal kaleidoscope.

### Approach 3: Multiple Textured Shapes

Place several textured shapes around the screen, each sampling a rotated portion of `prev_frame`:

```
Shape 1: x=0.25, y=0.25, tex_ang=0
Shape 2: x=0.75, y=0.25, tex_ang=π/2
Shape 3: x=0.75, y=0.75, tex_ang=π
Shape 4: x=0.25, y=0.75, tex_ang=3π/2
```

Creates four-way symmetry as each quadrant mirrors the others with different rotations.

---

## Textured Shape Implementation

To add MilkDrop-style textured shapes to a visualizer:

### Shape Definition

```cpp
struct TexturedShape {
    int sides;          // 3 = triangle, 4 = quad, etc.
    float x, y;         // Center position (0..1)
    float radius;       // Size
    float angle;        // Rotation
    bool textured;      // Sample previous frame?
    float texZoom;      // Texture zoom (1.0 = exact, >1 = zoom in)
    float texAngle;     // Texture rotation
    Color color;        // Tint color
    float alpha;        // Opacity
    bool additive;      // Blend mode
};
```

### Render Order

Shapes draw after the warp/feedback pass, before display:

```
1. Feedback shader: accumTexture → tempTexture
2. Draw textured shapes onto tempTexture:
   - Generate n-gon vertices
   - If textured: bind accumTexture, compute tex coords with zoom/rotation
   - Draw with OpenGL blend mode (alpha or additive)
3. Copy tempTexture → accumTexture
4. Composite shader → screen
```

### Texture Coordinate Calculation

```cpp
// For each vertex of the shape
float vertexAngle = shape.angle + (TAU * i / shape.sides);
float vx = shape.x + cos(vertexAngle) * shape.radius;
float vy = shape.y + sin(vertexAngle) * shape.radius;

// Texture coordinates (centered, zoomed, rotated)
float tx = (vx - 0.5) * shape.texZoom;
float ty = (vy - 0.5) * shape.texZoom;

// Apply texAngle rotation
float c = cos(shape.texAngle);
float s = sin(shape.texAngle);
float rtx = tx * c - ty * s;
float rty = tx * s + ty * c;

tx = rtx + 0.5;
ty = rty + 0.5;
```

---

## Specific Effect Recipes

### Fractal Zoom Tunnel

```
textured=1, sides=3 (or 4), x=0.5, y=0.5
tex_zoom=1.05, tex_ang=0.01 (per frame)
rad=0.4, additive=0
```

The `tex_zoom > 1` causes each frame to sample a slightly larger area, creating apparent zoom-in. `tex_ang` adds spiral rotation.

### 6-Way Kaleidoscope Mirror

```glsl
float segments = 6.0;
float segAngle = TAU / segments;
float angle = atan(uv.y - 0.5, uv.x - 0.5);
angle = mod(angle, segAngle);
angle = min(angle, segAngle - angle);
float r = length(uv - 0.5);
uv = vec2(cos(angle), sin(angle)) * r + 0.5;
```

### Sierpinski Triangle (via Shapes)

Three textured triangles, each positioned at a corner, each sampling the full previous frame scaled to 0.5:

```
Shape 1: x=0.25, y=0.25, sides=3, tex_zoom=2.0
Shape 2: x=0.75, y=0.25, sides=3, tex_zoom=2.0
Shape 3: x=0.50, y=0.75, sides=3, tex_zoom=2.0
```

After many frames, the fractal pattern emerges through feedback accumulation.

---

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html) — Official documentation
- [Ultra Effects Part 8: Crazy Kaleidoscopes](https://danielilett.com/2020-02-19-tut3-8-crazy-kaleidoscopes/) — UV folding technique
- [Godot Simple Kaleidoscope](https://godotshaders.com/shader/simple-kaleidoscope/) — Clean shader implementation
- [KinoMirror](https://github.com/keijiro/KinoMirror) — Unity mirroring/kaleidoscope effect
- [GLMixer Kaleidoscope](https://sourceforge.net/p/glmixer/Source/1319/tree/trunk/shaders/effect/kaleidoscope.glsl) — Iterative mirror technique
- [MilkDrop3](https://github.com/milkdrop2077/MilkDrop3) — Extended shape system (16 shapes, 500 sides)
- [Butterchurn](https://github.com/jberg/butterchurn) — WebGL MilkDrop implementation
