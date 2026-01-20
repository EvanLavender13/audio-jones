# Physarum Emergent Behavior Enhancements

Eight compute shader modifications that increase pattern variety and emergent complexity in the physarum simulation without changing the pipeline architecture.

## Classification

- **Category**: SIMULATIONS (enhancement to existing physarum effect)
- **Core Operation**: Modify agent sensing, steering, and movement algorithms
- **Pipeline Position**: Unchanged (feedback stage, before waveform drawing)

## References

- [MCPM Paper (MIT Press, 2022)](https://direct.mit.edu/artl/article/28/1/22/109954/Monte-Carlo-Physarum-Machine-Characteristics-of) - Stochastic mutation, variable distances, trace field, repulsion mixing
- [DotSwarm (Observable)](https://observablehq.com/@johnowhitaker/dotswarm-exploring-slime-mould-inspired-shaders) - Random steer perturbation, sensitivity-based decisions
- [Sage Jenson Physarum](https://cargocollective.com/sagejenson/physarum) - Contagion system, collision-free design
- [MisTree Lévy Flight Docs](https://knaidoo29.github.io/mistreedoc/levy_flight.html) - Step length distribution formulas
- [Lévy Flight Foraging (PLOS)](https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009490) - Biological basis for power-law movement

---

## 1. Stochastic Mutation Probability (Pmut)

### Current Behavior

Discrete steering: agents deterministically turn toward the lowest-affinity sensor direction. Vector steering: agents follow weighted force sum.

### Enhancement

Replace deterministic choice with probability-based branching. When sensing forward (d0) vs mutation direction (d1):

```glsl
// Probability of turning toward mutation direction
float Pmut = pow(d1, samplingExponent) / (pow(d0, samplingExponent) + pow(d1, samplingExponent));
bool shouldMutate = (random < Pmut);
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| samplingExponent | float | 1.0-10.0 | Higher = sharper filaments, fewer branches. Lower = diffuse clouds, more coverage. |

### Modulation Candidates

- **samplingExponent**: Lower values create diffuse, cloud-like patterns. Higher values concentrate agents into sharp filaments. Audio-reactive variation produces breathing between sharp and soft states.

---

## 2. Variable Sensing Distance (Pdist)

### Current Behavior

Fixed `sensorDistance` uniform applied to all agents equally.

### Enhancement

Sample sensing distance from a probability distribution each frame. Enables multi-scale feature detection.

```glsl
// Maxwell-Boltzmann distribution (for gas-like behavior)
// PDF: f(v) = sqrt(2/pi) * (v^2/a^3) * exp(-v^2/(2*a^2))
// Simplified: use Gaussian with mean = baseDistance, variance = distanceVariance

float sampleDistance = baseSensorDistance + gaussian(seed) * distanceVariance;
sampleDistance = max(sampleDistance, minSensorDistance);
```

Alternative distributions:
- **Exponential**: Bacterial motion dynamics, many short + few long
- **Power-law**: Heavy-tailed, scale-free (see Lévy section)

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| baseSensorDistance | float | 5-50 | Center of distribution |
| distanceVariance | float | 0-20 | Spread of sensing distances |
| minSensorDistance | float | 1-5 | Floor to prevent zero-distance sampling |

### Modulation Candidates

- **distanceVariance**: Low variance produces uniform behavior. High variance creates agents that sense at different scales simultaneously, detecting both fine detail and large structure.

---

## 3. Lévy Flight Step Lengths

### Current Behavior

Fixed `stepSize` uniform for all agent movement.

### Enhancement

Draw step lengths from power-law (Pareto) distribution. Creates occasional long jumps amid mostly short steps—optimal for sparse target foraging.

```glsl
// Lévy flight step length
// CDF: F(t) = 1 - (t/t0)^(-alpha) for t >= t0
// Inverse CDF sampling: t = t0 * u^(-1/alpha) where u ~ Uniform(0,1)

float u = max(random, 0.001);  // Avoid division by zero
float stepLength = minStep * pow(u, -1.0 / levyAlpha);
stepLength = min(stepLength, maxStep);  // Truncate extreme values
```

Optimal α ≈ 2.0 for sparse, randomly distributed targets (inverse-square law).

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| levyAlpha | float | 1.0-3.0 | Power-law exponent. α=1: extreme jumps. α=2: optimal foraging. α=3: near-Brownian. |
| minStep | float | 0.5-2.0 | Minimum step (t₀), scales distribution |
| maxStep | float | 10-100 | Truncation to prevent screen-spanning jumps |

### Modulation Candidates

- **levyAlpha**: Low alpha creates sparse, exploratory patterns with long filaments. High alpha creates dense, localized clustering. Modulating produces alternating exploration/exploitation phases.
- **minStep**: Scales the entire step distribution up or down.

---

## 4. Random Steer Perturbation

### Current Behavior

Random heading adjustment only when both lateral sensors equal (tie-breaker).

### Enhancement

Apply continuous heading noise every frame, scaled by trail absence. Agents wander more when "lost" (low trail density).

```glsl
// Perturbation scaled by trail absence
float trailStrength = 1.0 - frontAffinity;  // High affinity = weak trail
float perturbation = (random - 0.5) * maxPerturbation * trailStrength;
agent.heading += perturbation;
```

Alternative: constant perturbation regardless of trail:

```glsl
agent.heading += (random - 0.5) * randomSteerFactor;
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| maxPerturbation | float | 0-0.5 rad | Maximum heading noise when trail absent |
| randomSteerFactor | float | 0-0.3 rad | Constant perturbation each frame |
| perturbationDecay | float | 0-1 | How much trail presence reduces noise |

### Modulation Candidates

- **maxPerturbation**: Low values produce tight, deterministic paths. High values create fuzzy, organic-looking filaments with wandering edges.
- **randomSteerFactor**: Constant noise regardless of trail—useful for preventing pattern collapse.

---

## 5. More/Adaptive Sensors

### Current Behavior

Three sensors at fixed angles: front, left (+sensorAngle), right (-sensorAngle).

### Enhancement Options

**A. Five or more sensors** for finer directional resolution:

```glsl
// 5 sensors: -60°, -30°, 0°, +30°, +60°
float angles[5] = float[](-1.047, -0.524, 0.0, 0.524, 1.047);
float affinities[5];
for (int i = 0; i < 5; i++) {
    vec2 dir = vec2(cos(heading + angles[i]), sin(heading + angles[i]));
    affinities[i] = sampleBlendedAffinity(pos + dir * sensorDistance, agentHue);
}
// Find minimum affinity index, turn toward it
```

**B. Variable sensor angle per species**:

```glsl
// Wider sensors = better exploration, narrower = better trail following
float speciesSensorAngle = baseSensorAngle + (agent.hue - 0.5) * sensorAngleRange;
```

**C. Adaptive sensor angle based on trail density**:

```glsl
// Widen sensors when lost, narrow when on strong trail
float trailStrength = 1.0 - min(front, min(left, right));
float adaptiveAngle = baseSensorAngle * (1.0 + (1.0 - trailStrength) * adaptiveScale);
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| sensorCount | int | 3-7 | Number of sensing directions |
| sensorAngleRange | float | 0-1.0 rad | Per-species angle variation |
| adaptiveScale | float | 0-1.0 | How much trail affects sensor spread |

### Modulation Candidates

- **sensorAngleRange**: Creates species with different "personalities"—narrow sensors follow trails tightly, wide sensors explore more.
- **adaptiveScale**: Dynamic sensor widening when lost creates exploratory behavior that collapses into tight following when trails are found.

---

## 6. Attraction/Repulsion Mixing

### Current Behavior

`repulsionStrength` uniform globally controls whether different-hue trails attract or repel. All agents use the same value.

### Enhancement

Per-agent random choice to be attractor or repeller. Creates cellular void patterns alongside filaments.

```glsl
// Per-agent repulsion decision (set at initialization or periodically)
float agentRepulsion = (hash(id) & 1) == 0 ? 0.0 : 1.0;  // 50/50 mix

// Or probabilistic mixing:
float repulsionChance = globalRepulsionMix;  // 0-1
float agentRepulsion = (random < repulsionChance) ? 1.0 : 0.0;
```

Per-agent repulsion stored in Agent struct or derived from hue:

```glsl
// Derive from hue: warm colors attract, cool repel
float agentRepulsion = agent.hue > 0.5 ? 1.0 : 0.0;
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| repulsionMix | float | 0-1.0 | Fraction of agents that repel instead of attract |

### Modulation Candidates

- **repulsionMix**: Low values produce classic physarum networks. High values create cellular/bubble patterns. 0.5 produces hybrid structures with filaments separating voids.

---

## 7. Contagion System

### Current Behavior

Agent hue is fixed at initialization and never changes.

### Enhancement

Agents can become "infected" and spread infection via trails. Creates cascading pattern transformations.

```glsl
// Agent state: 0 = normal, 1 = infected
// If infected, deposit different color or increased amount
// When sensing infected trail, chance to become infected

float infectedTrailAmount = sampleInfectionMap(pos);
if (infectedTrailAmount > infectionThreshold && random < infectionChance) {
    agent.infected = 1.0;
}

// Infected agents deposit to infection channel
if (agent.infected > 0.5) {
    depositColor = infectedColor;
    depositAmount *= infectionBoost;
}
```

Requires additional state in Agent struct or separate infection texture.

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| infectionThreshold | float | 0.01-0.5 | Trail intensity required to infect |
| infectionChance | float | 0.001-0.1 | Probability of infection per contact |
| infectionBoost | float | 1.0-3.0 | Deposit multiplier for infected agents |
| infectedColor | vec3 | any HSV | Visual distinction for infected trails |

### Modulation Candidates

- **infectionChance**: Controls speed of cascade. Low values create slow-spreading waves. High values produce rapid pattern transformation.
- **infectionThreshold**: Higher threshold limits infection to dense trail areas.

---

## 8. Trace Field (Non-diffusing)

### Current Behavior

Single trail map with diffusion and decay. Diffusion softens patterns over time.

### Enhancement

Separate non-diffusing "trace" texture that records agent positions. Produces sharper visualization than the diffusing deposit field.

```glsl
// In compute shader: write to both trail (diffuses) and trace (doesn't)
imageStore(trailMap, coord, vec4(newColor, 0.0));  // Existing
imageStore(traceMap, coord, vec4(traceColor, 0.0));  // New

// Trace only decays, never diffuses
// In trail processing shader:
// trailMap gets blur + decay
// traceMap gets only decay (or slower decay)
```

Visualization can blend trail and trace:

```glsl
vec3 finalColor = mix(trailColor, traceColor, traceBlend);
```

### Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| traceDecayHalfLife | float | 0.5-5.0 | How quickly trace fades (separate from trail) |
| traceDepositAmount | float | 0.01-0.2 | How much agents write to trace |
| traceBlend | float | 0-1 | Mix between diffused trail and sharp trace in output |

### Modulation Candidates

- **traceBlend**: 0 = soft diffused look, 1 = sharp precise agent paths. Intermediate values combine both.
- **traceDecayHalfLife**: Short decay shows recent agent positions only. Long decay builds up cumulative history.

---

## Implementation Priority

Suggested order based on impact vs complexity:

1. **Random Steer Perturbation** (4) — Simplest, immediate visual improvement
2. **Stochastic Mutation** (1) — Core MCPM innovation, moderate complexity
3. **Lévy Flight Steps** (3) — Dramatic behavioral change, straightforward math
4. **Variable Sensing Distance** (2) — Multi-scale detection, moderate complexity
5. **More Sensors** (5) — Optional refinement, easy to add
6. **Attraction/Repulsion Mixing** (6) — New visual modes, uses existing repulsion
7. **Trace Field** (8) — New texture, shader changes, higher complexity
8. **Contagion System** (7) — Most complex, requires Agent struct changes

---

## Notes

- All enhancements modify `shaders/physarum_agents.glsl` compute shader
- Lévy flights require careful truncation to prevent screen-spanning jumps
- Trace field requires new texture and render target in `TrailMap`
- Contagion requires Agent struct expansion (add `infected` field)
- Parameters can be added to `PhysarumConfig` incrementally
- Consider grouping related parameters in UI (e.g., "Stochastic" section)
