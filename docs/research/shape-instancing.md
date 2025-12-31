# Shape Instancing Research

## Problem Statement

AudioJones creates shapes through a GUI with no programming. MilkDrop achieves instancing via per-frame equations that reference an `instance` variable. How do we expose similar power without requiring users to write code?

---

## MilkDrop's Approach

MilkDrop runs per-frame equations N times per shape, incrementing `instance` each iteration:

```
for instance = 0 to num_inst-1:
    run per-frame equations (can reference 'instance')
    draw shape with computed x, y, rad, ang, colors
```

**Key insight:** Each property can be an arbitrary function of `instance`. Examples:

```c
// Circle arrangement
x = 0.5 + 0.3 * cos(instance * 6.28 / num_inst);
y = 0.5 + 0.3 * sin(instance * 6.28 / num_inst);

// Color gradient
r = instance / num_inst;

// Audio-reactive with instance variation
rad = 0.1 + bass * 0.05 * sin(instance * 0.5);
```

This is CPU-side looping, not GPU instancing.

---

## AudioJones Constraints

1. **No code editing** - Users configure via sliders and dropdowns
2. **Existing modulation system** - Routes audio/LFO sources to parameters
3. **Modulation is per-frame** - Sources have one value per frame, not per-instance

### Why Instance Can't Be a Modulation Source

Current ModEngine flow:
```
Per frame:
  ModSourcesUpdate()  → values[] populated once
  ModEngineUpdate()   → each route writes to param pointer once
  Draw                → one shape
```

Instance varies N times within a single frame. The architecture assumes one modulated value per parameter per frame.

---

## Design Options

### Option A: Arrangement Presets

Dropdown with built-in position patterns:

| Arrangement | Parameters | Description |
|-------------|------------|-------------|
| None | - | All instances at same position |
| Line | angle, length | Evenly spaced along a line |
| Circle | radius | Evenly spaced around center |
| Grid | rows, cols, spacing | Row-major grid layout |
| Spiral | inner_r, outer_r, turns | Outward spiral from center |
| Random | seed, bounds | Deterministic scatter |

**Pros:** Simple UI, covers common cases, no math required
**Cons:** Limited to predefined patterns

### Option B: Per-Property Spread Values

Each property gets a "spread" slider defining variation from first to last instance:

```
value[i] = base + (i / (count-1)) * spread
```

Example with 5 instances:
```
X Base: 0.3, X Spread: 0.4
→ X positions: 0.30, 0.40, 0.50, 0.60, 0.70
```

**Pros:** Fine-grained control, works for any property
**Cons:** Independent spreads only produce lines; coupled relationships (circles, spirals) impossible

### Option C: Arrangement + Spread Hybrid

- **Arrangement** controls position (x, y)
- **Spread** controls other properties (rotation, size, color, opacity)

```
Instances:   [8]
Arrangement: [Circle]  Radius: [0.25]

Rotation:    [0°]      Spread: [360°]   ← face outward
Size:        [0.05]    Spread: [0.00]   ← uniform
Alpha:       [1.0]     Spread: [-0.5]   ← fade out around circle
```

**Pros:** Best of both - patterns for position, flexibility for other properties
**Cons:** Two different mental models

### Option D: Polar Spread

Instead of Cartesian (x, y) spread, use polar (angle, radius) spread from shape center:

```
Angle Spread: 360°  → instances distributed around center
Radius Spread: 0.2  → instances at varying distances
```

Combined with base position, creates circles/spirals naturally:
- Angle spread only → circle
- Both spreads → spiral
- Radius spread only → radial line

**Pros:** Single model handles circles, spirals, radial patterns
**Cons:** Less intuitive for grids/lines

### Option E: Expression Fields (Advanced Mode)

Optional text fields for power users:

```
X: 0.5 + 0.3 * cos(i * 6.28 / n)
Y: 0.5 + 0.3 * sin(i * 6.28 / n)
```

Where `i` = instance index, `n` = count, plus audio variables.

**Pros:** Full MilkDrop power
**Cons:** Requires programming knowledge, parsing/evaluation complexity

---

## Implementation Considerations

### Data Structure

```c
struct InstanceConfig {
    int count;                    // 1-1024
    int arrangement;              // ARRANGE_NONE, ARRANGE_CIRCLE, etc.
    float arrangementParams[4];   // Pattern-specific (radius, angle, spacing, etc.)

    // Per-property spreads (applied after arrangement)
    float rotationSpread;
    float sizeSpread;
    float alphaSpread;
    // ... other properties
};
```

### Rendering Loop

```c
void DrawShapeInstances(Drawable* shape, InstanceConfig* inst) {
    for (int i = 0; i < inst->count; i++) {
        float t = (inst->count > 1) ? (float)i / (inst->count - 1) : 0;

        // Arrangement computes base position
        Vector2 pos = ComputeArrangementPosition(shape, inst, i);

        // Spreads modify other properties
        float rotation = shape->rotation + t * inst->rotationSpread;
        float size = shape->radius + t * inst->sizeSpread;
        float alpha = shape->alpha + t * inst->alphaSpread;

        DrawShapeAt(pos.x, pos.y, rotation, size, alpha, ...);
    }
}
```

### Interaction with Modulation

Modulation applies to **base values** before instancing:
```
bass increases → shape.x increases → all instances shift right together
```

The spread distributes instances relative to the modulated base. Group moves as a unit while maintaining internal arrangement.

---

## Open Questions

1. Should spread values be modulatable? (e.g., bass widens the spread)
2. How to handle textured shapes with instancing? (each samples same UV or offset?)
3. Performance at high instance counts (100+)?
4. Preset save/load for instance configurations?

---

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html)
- [milkdrop-feedback-architecture.md](./milkdrop-feedback-architecture.md) - Shape instancing section
