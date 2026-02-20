# Bit Crush Walk Mode Variations

Research into alternative lattice walk rules that preserve the hard-pixel aesthetic while producing different cell pattern vocabularies. All variants keep `ceil()` snapping so blocks stay crisp.

## Current Walk Rule

```glsl
p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
```

Fixed diagonal bias from `vec2(-1.0, 1.0)`. Produces recurring staircase/zigzag motifs due to `p.x * p.y` hash symmetry and constant direction vector.

## Variant Ideas

### 1. Rotating Direction Vector

Replace the fixed `vec2(-1.0, 1.0)` with a per-iteration rotation:

```glsl
vec2 dir = vec2(-cos(float(i)), sin(float(i)));
p = ceil(p + r(ceilCell, time) + r(floorCell, time) * dir);
```

**Expected result**: Cells spread in varied directions per step instead of always the same diagonal. Should break the repeating staircase motif into more diverse shapes.

### 2. Offset Neighborhood Query

Query non-adjacent cells instead of `ceil`/`floor` of the same position:

```glsl
vec2 cellA = floor(p / cellSize) + vec2(-1.0, 0.0);
vec2 cellB = floor(p / cellSize) + vec2(0.0, 1.0);
p = ceil(p + r(cellA, time) + r(cellB, time) * vec2(-1.0, 1.0));
```

**Expected result**: Each step consults a wider neighborhood. Cells that were identical under the original rule now diverge because they see different neighbors.

### 3. Alternating Snap Function

Alternate between `ceil()`, `floor()`, and `round()` per iteration:

```glsl
int mode = i % 3;
if (mode == 0) p = ceil(p + ...);
else if (mode == 1) p = floor(p + ...);
else p = round(p + ...);
```

**Expected result**: Different snapping creates different attractor positions. `floor` pulls down-left, `ceil` pulls up-right, `round` is neutral. Alternating should produce more varied block alignments.

### 4. Cross-Coupled Axes

Make x movement depend on y position and vice versa:

```glsl
float rx = r(ceilCell, time);
float ry = r(ceilCell.yx, time);  // swap x/y for different hash
p = ceil(vec2(p.x + rx + ry * 0.5, p.y + ry - rx * 0.5));
```

**Expected result**: X and Y no longer evolve semi-independently. More tangled, organic-looking blocks instead of rectilinear staircases. Could produce L-shapes and T-shapes that the current rule doesn't.

### 5. Asymmetric Hash (tried, subtle)

Replace `p.x * p.y` with `dot(p, vec2(0.7, 1.3))` in the hash function. Breaks the symmetry where `(3,5)` and `(5,3)` produce the same hash. Effect was subtle — fewer repeated motifs but same overall character.

## Implementation Approach

Could expose as a `walkMode` int uniform (0-4) with a switch/if-chain in the shader loop body. Each mode selects a different step rule. UI would be a combo dropdown in the Lattice section.

Alternatively, some variants could be combined — e.g., rotating direction + cross-coupled axes might compound nicely. Worth testing combinations.

## Performance Note

All variants have the same computational cost as the current rule (one loop, same iteration count). The alternating snap variant adds two branches per iteration but the GPU should handle that fine since all pixels take the same branch per iteration.
