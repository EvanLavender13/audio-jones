# Physarum Boundary Modes — Emergent Interactions

Extends the existing physarum bounds system with attractor-based modes that produce structured emergent patterns when combined with Lévy flights.

## Observations

### Why Redirect Differs from Reflect/Scatter/Random

With Lévy flights enabled, reflect, scatter, and random modes converge to identical grid/banding patterns. The post-collision heading becomes irrelevant — the next Lévy jump immediately overrides it. All three modes clamp position to the edge identically; only the heading assignment differs, and that heading survives exactly one normal-length step.

Redirect produces distinct geometric structure (string-art, caustic curves) because it alters the **attractor geometry** — agents oscillate between boundary and center, creating radial corridors. The key insight: interesting patterns require the boundary mode to change **where** agents are drawn to, not just what direction they face.

### Slow Evolution

Patterns evolve over time because the hash seed incorporates `time * 1000.0`. Trail decay determines how fast old structure fades. The caustic curves visible in redirect mode are envelope surfaces formed by many Lévy jumps at similar angles; as the time-seeded hash drifts, jump angle distributions rotate slowly.

## Classification

- **Category**: SIMULATIONS (extension of existing Physarum)
- **Core Operation**: New boundary collision behaviors in `physarum_agents.glsl`
- **Pipeline Position**: Compute dispatch, same stage as existing physarum

## Proposed Modes

### 1. Fixed Home Point

Each agent has a permanent home position computed from `hash(id)` (no time component). On boundary collision, redirect toward or respawn at that position.

**Emergent prediction**: Constellation of radial nodes, each a mini-starburst. Trail-following links nearby nodes into clusters. Lévy jumps create bridges between distant home clusters.

**Implementation**: Zero extra storage — `hash(id)` is deterministic per agent.

```glsl
uint homeHash = hash(id * 3u + 12345u);
float homeX = float(homeHash) / 4294967295.0 * resolution.x;
homeHash = hash(homeHash);
float homeY = float(homeHash) / 4294967295.0 * resolution.y;
vec2 home = vec2(homeX, homeY);
```

### 2. Orbit

On boundary hit, redirect perpendicular to the center vector (tangent to a circle). Agents enter circular orbits rather than collapsing radially inward.

**Emergent prediction**: Spirograph patterns. Lévy jumps punch agents between orbital rings. Trail persistence creates layered concentric ribbons.

**Implementation**: Rotate the center-pointing vector by 90°.

```glsl
vec2 toCenter = (resolution * 0.5) - pos;
float tangentAngle = atan(toCenter.y, toCenter.x) + PI * 0.5; // +90° = orbit direction
agent.heading = tangentAngle;
```

### 3. Per-Species Orbit Perturbation

Each hue group gets a different angular offset on the orbit direction. Species separate into phase-shifted or counter-rotating orbits.

**Emergent prediction**: Interleaved color bands. Species with opposite orbit directions create crossing points where trail competition produces territorial boundaries.

**Implementation**: Offset the tangent angle by agent hue.

```glsl
float speciesOffset = agent.hue * TWO_PI; // or a fraction of PI
float tangentAngle = atan(toCenter.y, toCenter.x) + PI * 0.5 + speciesOffset;
```

### 4. Gravity Well (Continuous)

Instead of binary edge detection, apply a constant inward acceleration scaled by distance from center. Agents curve back without hitting boundaries. With Lévy flights, occasional jumps escape the well temporarily.

**Emergent prediction**: Smooth orbital arcs rather than hard geometric lines. The well depth parameter controls whether agents orbit tightly or roam freely. Audio modulation of well strength creates breathing/pulsing colonies.

**Implementation**: Apply force each frame, not just on collision.

```glsl
vec2 toCenter = (resolution * 0.5) - pos;
float dist = length(toCenter);
float force = gravityStrength * (dist / length(resolution)); // Scales with distance
float forceAngle = atan(toCenter.y, toCenter.x);
agent.heading = mix(agent.heading, forceAngle, force); // Blend toward center
```

### 5. Multi-Home (N Shared Attractors)

Distribute K fixed attractor points. Each agent redirects toward its assigned attractor (`hash(id) % K`). Creates Voronoi-like territory boundaries between basins.

**Emergent prediction**: K distinct colonies connected by Lévy bridges. With species repulsion, different colors claim different attractors. Increasing K fragments the colony; decreasing K concentrates it.

**Implementation**: Compute K attractor positions from hash, assign agents.

```glsl
uint attractorIdx = hash(id) % uint(attractorCount);
uint attractorSeed = hash(attractorIdx * 7u + 99u);
float ax = float(attractorSeed) / 4294967295.0 * resolution.x;
attractorSeed = hash(attractorSeed);
float ay = float(attractorSeed) / 4294967295.0 * resolution.y;
vec2 target = vec2(ax, ay);
```

### 6. Antipodal Respawn

On boundary hit, teleport to the point diametrically opposite the agent's current position relative to center. Every streak has a mirror twin.

**Emergent prediction**: Perfect point symmetry. All structure duplicates across center. Combined with species offset, creates yin-yang separation.

**Implementation**:

```glsl
vec2 center = resolution * 0.5;
pos = 2.0 * center - pos; // Reflect through center
```

## Redirect vs Respawn Toggle

A boolean toggle applicable to all attractor-based modes (center redirect, fixed home, orbit, multi-home). Controls behavior on boundary collision:

- **Redirect**: Steer heading toward target. Agent walks there, depositing trail along the path. Produces connected structure between boundary and attractor.
- **Respawn**: Teleport position to target. Agent appears instantly, radiates outward. Produces sharp starburst patterns centered on attractors.

Applies retroactively to existing mode 2 (redirect toward center).

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| boundsMode | enum | 0-10 | Selects boundary behavior |
| respawnMode | bool | — | Redirect (steer) vs Respawn (teleport) |
| gravityStrength | float | 0.0-1.0 | Well depth for gravity mode |
| attractorCount | int | 2-8 | Number of shared attractors for multi-home |
| orbitOffset | float | 0.0-1.0 | Per-species angular offset strength for orbit perturbation |

## Modulation Candidates

- **gravityStrength**: Pulsing colony compression/expansion with audio
- **attractorCount**: Fragmenting/merging colonies dynamically
- **orbitOffset**: Rotating species separation angle, creates spinning color wheel

## Notes

- Fixed home and multi-home modes require no additional buffer storage — all positions derive from deterministic `hash(id)` calls
- Gravity well mode operates continuously (not just on boundary hit), making it fundamentally different from the other modes — it may warrant a separate uniform or sub-mode
- The redirect vs respawn toggle changes visual character dramatically: redirect creates connected web structure, respawn creates isolated radial nodes
- Per-species orbit perturbation interacts strongly with `repulsionStrength` — counter-rotating species that repel each other create dynamic vortex boundaries
