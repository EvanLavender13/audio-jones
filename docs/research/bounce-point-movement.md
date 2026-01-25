# Bounce Point Movement

A CPU-side point movement utility that replaces Lissajous oscillation for effect centers. A point travels freely within a padded rectangle, curving smoothly away from edges via repulsion forces. Produces DVD-bounce-like space-filling coverage with entirely smooth (differentiable) trajectories — no sharp corners.

## Classification

- **Category**: Infrastructure / Utility (replaces Lissajous motion in multiple effects)
- **Core Operation**: CPU integrates position + velocity each frame; edge repulsion force curves trajectory near boundaries
- **Pipeline Position**: Pre-render CPU computation, outputs `vec2` uniform to shaders

## References

- [The Bouncing DVD Logo Explained](http://prgreen.github.io/blog/2013/09/30/the-bouncing-dvd-logo-explained/) - Mathematical analysis of DVD bounce periodicity, GCD/LCM relationships, corner-hit conditions
- [Realistic Bounce and Overshoot (Motionscript)](https://www.motionscript.com/articles/bounce-and-overshoot.html) - Exponential decay formulas, velocity matching for smooth bounce/overshoot
- [Dynamical Billiards (MathWorld)](https://mathworld.wolfram.com/Billiards.html) - Ergodicity proof: irrational-slope trajectories in rectangles densely fill the space
- [Soft Billiards with Corners (Springer)](https://link.springer.com/article/10.1023/A:1023884227180) - Smooth Hamiltonian approximations to hard-cornered billiards

## Algorithm

### State

```
struct BouncePoint {
    float posX, posY;   // Current position (0.0-1.0 UV space)
    float velX, velY;   // Velocity vector (units/sec)
};
```

### Per-Frame Update (CPU)

```cpp
void BouncePointUpdate(BouncePoint* bp, float dt, float speed, float padX, float padY) {
    // Normalize velocity to desired speed
    float len = sqrtf(bp->velX * bp->velX + bp->velY * bp->velY);
    if (len > 0.0001f) {
        bp->velX = (bp->velX / len) * speed;
        bp->velY = (bp->velY / len) * speed;
    }

    // Edge repulsion forces
    float fx = 0.0f, fy = 0.0f;
    float repulsionStrength = speed * 4.0f;  // Scale with speed for consistent curvature

    // Left edge
    float dLeft = bp->posX - padX;
    if (dLeft < padX && dLeft > 0.0f) {
        fx += repulsionStrength * (1.0f / dLeft - 1.0f / padX);
    }
    // Right edge
    float dRight = (1.0f - padX) - bp->posX;
    if (dRight < padX && dRight > 0.0f) {
        fx -= repulsionStrength * (1.0f / dRight - 1.0f / padX);
    }
    // Top edge
    float dTop = bp->posY - padY;
    if (dTop < padY && dTop > 0.0f) {
        fy += repulsionStrength * (1.0f / dTop - 1.0f / padY);
    }
    // Bottom edge
    float dBot = (1.0f - padY) - bp->posY;
    if (dBot < padY && dBot > 0.0f) {
        fy -= repulsionStrength * (1.0f / dBot - 1.0f / padY);
    }

    // Apply forces to velocity
    bp->velX += fx * dt;
    bp->velY += fy * dt;

    // Re-normalize to maintain constant speed
    len = sqrtf(bp->velX * bp->velX + bp->velY * bp->velY);
    if (len > 0.0001f) {
        bp->velX = (bp->velX / len) * speed;
        bp->velY = (bp->velY / len) * speed;
    }

    // Integrate position
    bp->posX += bp->velX * dt;
    bp->posY += bp->velY * dt;

    // Hard clamp as safety (should rarely trigger)
    bp->posX = fmaxf(padX * 0.5f, fminf(1.0f - padX * 0.5f, bp->posX));
    bp->posY = fmaxf(padY * 0.5f, fminf(1.0f - padY * 0.5f, bp->posY));
}
```

### Initialization

```cpp
void BouncePointInit(BouncePoint* bp, float startX, float startY, float angleDeg) {
    bp->posX = startX;
    bp->posY = startY;
    float rad = angleDeg * (3.14159265f / 180.0f);
    bp->velX = cosf(rad);
    bp->velY = sinf(rad);
}
```

### Key Properties

- **Constant speed**: Velocity magnitude stays fixed; only direction changes
- **Smooth curvature**: Repulsion force is inversely proportional to distance from edge, producing natural curves
- **Ergodic coverage**: Non-rational initial angles produce trajectories that densely fill the padded rectangle over time
- **Deterministic**: Fixed initial position + angle → identical trajectory every run

### Repulsion Force Model

The force follows a clamped inverse-distance model:

```
F(d) = k * (1/d - 1/pad)   when d < pad
F(d) = 0                    when d >= pad
```

Where:
- `d` = distance from point to edge
- `pad` = padding threshold (repulsion zone width)
- `k` = `speed * 4.0` (scales curvature radius with speed)

This produces:
- Zero force in the interior (free straight-line travel)
- Smoothly increasing deflection as the point approaches an edge
- The `1/pad` term ensures force is zero at the boundary of the repulsion zone (no discontinuity)

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `speed` | float | 0.01 - 1.0 | Velocity magnitude in UV-space units/sec. Higher = faster traversal |
| `paddingX` | float | 0.05 - 0.4 | Horizontal repulsion zone width. Larger = tighter turns, smaller usable area |
| `paddingY` | float | 0.05 - 0.4 | Vertical repulsion zone width. Larger = tighter turns, smaller usable area |

### Internal (not user-facing)

| Parameter | Type | Default | Purpose |
|-----------|------|---------|---------|
| `initialAngle` | float | Randomized or preset-stored | Launch direction (degrees). Irrational-slope angles give best coverage |

## Modulation Candidates

- **speed**: Modulating produces acceleration/deceleration — point lingers in some areas, rushes through others
- **paddingX / paddingY**: Modulating dynamically changes the usable area — shrinking padding lets the point reach edges, expanding squeezes it toward center

## Integration Points

Replaces Lissajous motion in these systems (each currently stores amplitude + freqX + freqY):

| System | Current State | What Changes |
|--------|---------------|--------------|
| Wave Ripple | `originAmplitude`, `originFreqX/Y` → Lissajous | Replace with shared `BouncePoint`, output to `currentWaveRippleOrigin[]` |
| Mobius | `pointAmplitude`, `pointFreq1/2` → Lissajous (2 points) | Two `BouncePoint` instances, output to `currentMobiusPoint1/2[]` |
| Poincare Disk | `translationSpeed`, `translationAmplitude` → circular | Replace with `BouncePoint`, output to `currentPoincareTranslation[]` |
| Cymatics | `sourceAmplitude`, `sourceFreqX/Y` → Lissajous per source | N `BouncePoint` instances (one per source), output to source positions |
| Attractor Flow | Static `x`, `y` center | Add optional `BouncePoint` for center drift |
| Physarum homes | Hash-fixed positions (FIXED_HOME, MULTI_HOME) | Add optional slow `BouncePoint` drift to attractor positions (GPU uniform update) |

### Architecture Consideration

A reusable `BouncePoint` struct + `BouncePointUpdate` function lives in a shared utility (e.g., `src/util/bounce_point.h`). Each effect that uses it stores one or more `BouncePoint` instances in its config/state and calls update per frame. The motion mode (Lissajous vs Bounce) could be a toggle per effect, preserving backward compatibility with existing presets.

## Notes

- Initial angle determines trajectory character. Angles producing irrational slopes relative to the padded rectangle's aspect ratio give the most visually interesting, non-repeating paths. A random angle at startup (or stored per-preset) works well.
- The `repulsionStrength = speed * 4.0` constant controls curvature radius. Higher multiplier = tighter turns near edges. This could optionally become a 4th "curvature" parameter, but 4.0 works well as a fixed default.
- Unlike Lissajous, the point's future position depends on its history — you can't compute position at arbitrary time t without running the simulation forward. This means presets must store the initial state (pos + angle) rather than phase.
- For Cymatics with multiple sources, each source gets its own `BouncePoint` with a different initial angle, producing independent trajectories that avoid synchronization.
