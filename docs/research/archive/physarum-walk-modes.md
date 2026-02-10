# Physarum Walk Modes

A walk mode selector replaces the single Levy flight parameter with a family of step-size strategies. Each mode interprets the existing `stepSize` as its base unit but applies different distributions, correlations, or state-dependent scaling to produce distinct emergent network topologies.

## Classification

- **Category**: SIMULATIONS (Physarum enhancement)
- **Core Operation**: Per-agent step computation in the existing physarum compute shader
- **Pipeline Position**: Compute dispatch (before trail processing)

## References

- [Bridging correlated random walks and Levy walks](https://royalsocietypublishing.org/doi/10.1098/rsif.2010.0292) - Persistence as source of Levy-like movement
- [Intermittent random walks for optimal search](https://www.researchgate.net/publication/1856830_Intermittent_random_walks_for_an_optimal_search_strategy_One-dimensional_case) - Run/tumble switching math
- [Transient Anomalous Diffusion in Run-and-Tumble](https://www.frontiersin.org/articles/10.3389/fphy.2019.00120/full) - Two-state motility model with persistence
- [Persistent Random Walks (MIT OCW)](https://ocw.mit.edu/courses/18-366-random-walks-and-diffusion-fall-2006/8c699d35fafdde8b4741d0aea231e706_lecture10.pdf) - Telegrapher's equation, correlated walk math
- [Scaling laws in DLA of persistent random walkers](https://www.sciencedirect.com/science/article/pii/S0378437111005243) - Crossover between ballistic and diffusion-limited aggregation
- [Fractality a la carte: general aggregation model](https://www.nature.com/articles/srep19505) - Tunable fractal dimension via walk parameters
- [Characteristics of pattern formation in Physarum transport networks](https://dl.acm.org/doi/abs/10.1162/artl.2010.16.2.16202) - Jeff Jones' parameter-space mapping of emergent patterns

## Current Implementation

The shader (`shaders/physarum_agents.glsl:264-272`) computes step length as:

```glsl
float agentStep = stepSize;
if (levyAlpha > 0.001) {
    float u = max(float(hash(hashState)) / 4294967295.0, 0.001);
    agentStep = stepSize * pow(u, -1.0 / levyAlpha);
    agentStep = min(agentStep, stepSize * 50.0);
}
pos += moveDir * agentStep;
```

The `PhysarumAgent` struct has 4 padding floats (`_pad[4]`) available for per-agent state.

## Walk Modes

### 0: Normal (existing)
Fixed step = `stepSize`. No modification.

### 1: Levy (existing)
Power-law distributed: `stepSize * pow(u, -1/alpha)`. Heavy-tailed jumps produce superdiffusive exploration with occasional long flights.

### 2: Persistent
Heading blends toward previous heading each frame:
```glsl
float prevHeading = agent._pad[0];
agent.heading = mix(agent.heading, prevHeading, persistence);
agent._pad[0] = agent.heading;  // Store for next frame
float agentStep = stepSize;
```
- Persistence ∈ [0, 1]: 0 = uncorrelated (same as Normal), 1 = straight lines
- Produces long coherent filaments and smoother trails

### 3: Run & Tumble
Two-state mode stored in `_pad[0]` (timer countdown):
```glsl
float timer = agent._pad[0];
timer -= 1.0;
if (timer <= 0.0) {
    // Switch mode: if was running, tumble; if tumbling, run
    bool wasRunning = agent._pad[1] > 0.5;
    agent._pad[1] = wasRunning ? 0.0 : 1.0;
    timer = wasRunning ? tumbleDuration : runDuration;
    if (wasRunning) {
        agent.heading += randomAngle;  // Random reorientation
    }
}
agent._pad[0] = timer;
bool running = agent._pad[1] > 0.5;
float agentStep = running ? stepSize * runMultiplier : stepSize * 0.2;
```
- `runDuration`: frames of straight-line motion (10–100)
- `tumbleDuration`: frames of small diffusive steps (5–20)
- `runMultiplier`: speed boost during run phase (1–5)
- Produces dense clusters connected by ballistic bridges

### 4: Antipersistent
Heading biases away from previous direction (subdiffusive, trapped motion):
```glsl
float prevHeading = agent._pad[0];
float diff = agent.heading - prevHeading;
agent.heading += diff * antiPersistence;  // Amplify direction changes
agent._pad[0] = agent.heading;
float agentStep = stepSize;
```
- `antiPersistence` ∈ [0, 1]: strength of reversal tendency
- Produces tight trapped knots and dense nuclei

### 5: Ballistic (DLA-like)
Agents move in straight lines at `stepSize` until local trail density exceeds a threshold, then reduce step to near-zero (stick):
```glsl
float localDensity = sampleTrail(pos);
bool stuck = localDensity > stickThreshold;
float agentStep = stuck ? stepSize * 0.05 : stepSize;
if (!stuck) {
    // Maintain current heading (no turning)
}
```
- `stickThreshold` ∈ [0, 1]: trail density that causes sticking
- Produces fractal branching trees (lightning/mineral veins)

### 6: Adaptive
Step size scales with local trail density:
```glsl
float localDensity = sampleTrail(pos);
float scale = mix(1.0, densityResponse, localDensity);
float agentStep = stepSize * scale;
```
- `densityResponse` > 1: agents accelerate in dense regions (explosive expansion)
- `densityResponse` < 1: agents slow in dense regions (self-regulating density)
- `densityResponse` ∈ [0.1, 5.0]

## Parameters

| Parameter | Type | Range | Mode | Effect |
|-----------|------|-------|------|--------|
| walkMode | int/enum | 0–6 | All | Selects active walk strategy |
| stepSize | float | 0.1–10.0 | All | Base step magnitude (existing) |
| levyAlpha | float | 0.0–3.0 | Levy | Power-law exponent (existing) |
| persistence | float | 0.0–1.0 | Persistent | Directional memory strength |
| antiPersistence | float | 0.0–1.0 | Antipersistent | Reversal tendency |
| runDuration | float | 10–100 | Run & Tumble | Frames per run phase |
| tumbleDuration | float | 5–30 | Run & Tumble | Frames per tumble phase |
| runMultiplier | float | 1.0–5.0 | Run & Tumble | Speed boost during run |
| stickThreshold | float | 0.0–1.0 | Ballistic | Density to trigger sticking |
| densityResponse | float | 0.1–5.0 | Adaptive | Step scale factor from density |

## Per-Agent State Requirements

The existing `_pad[4]` array provides storage without struct expansion:

| Mode | _pad[0] | _pad[1] | _pad[2] | _pad[3] |
|------|---------|---------|---------|---------|
| Persistent | prev_heading | — | — | — |
| Run & Tumble | timer | mode (0/1) | — | — |
| Antipersistent | prev_heading | — | — | — |
| Others | unused | unused | unused | unused |

## Modulation Candidates

- **persistence / antiPersistence**: Increasing produces more coherent/trapped motion; audio-reactive changes create pulsing between diffuse and filamentous states
- **runMultiplier**: Higher values stretch the ballistic bridges longer; modulation creates breathing network topology
- **stickThreshold**: Lower threshold freezes agents sooner, growing denser trees; modulation alternates between growth and dispersal
- **densityResponse**: Modulating above/below 1.0 switches between explosive and self-limiting behavior
- **walkMode itself**: Integer switching between modes mid-simulation produces hybrid emergent patterns

## Notes

- The walk mode enum replaces `levyAlpha` as the primary step-size control. Levy becomes mode 1 with its own sub-parameter.
- Modes 2–4 (Persistent, Run & Tumble, Antipersistent) interact strongly with `turningAngle` — persistence fights against large turns, while antipersistence amplifies them.
- Ballistic mode (5) effectively disables chemotaxis for unstuck agents — they ignore sensors and move straight. This produces fundamentally different topology (tree growth vs network formation).
- Adaptive mode (6) pairs well with `gravityStrength` — agents slow near center (high density) creating a dense core with fast peripheral exploration.
- All modes respect the existing `boundsMode` (toroidal/reflect/etc.) since boundary handling occurs after step application.
