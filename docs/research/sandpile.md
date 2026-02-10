# Sandpile

Driven abelian sandpile where feedback texture brightness injects grains through a configurable threshold. Cells accumulate grains, topple when exceeding a threshold, and cascade avalanches outward in fractal patterns. Continuous timestep and faithful update rule keep the system evolving — accumulation drives LUT color, activation modulates brightness to reveal avalanche wavefronts.

## Classification

- **Category**: SIMULATIONS
- **Pipeline Position**: Simulation stage (compute/ping-pong before output)
- **Compute Model**: Fragment shader ping-pong (same pattern as curl advection)
- **Chosen Approach**: Balanced — full Chronos generalized smooth sandpile with feedback injection, configurable grid resolution, two-channel color output

## References

- Chronos, "Generalized Smooth Abelian Sandpile" (Shadertoy) — all algorithm details, parameter configurations, and GLSL provided by user from 6 shader variants
- Existing codebase: `src/simulation/curl_advection.cpp` — ping-pong texture pattern, feedback injection via accumTexture threshold, ColorLUT sampling

## Algorithm

### State

Each pixel stores `vec4(accumulation, accumulation, accumulation, activation)`:
- **R/G/B**: grain accumulation (0-1 normalized density)
- **A**: topple activation (0-1, how much the cell just toppled)

Two ping-pong textures hold this state, flipped each frame.

### Neighborhood Sampling

For each pixel, sample a `(2r+1)^2` grid centered on it. Each neighbor contributes to two weighted sums based on distance from center:

- **Inner disk** (distance < inner_radius): weighted average of neighbor **accumulation** (R channel)
- **Outer annulus** (inner_radius < distance < outer_radius): weighted average of neighbor **activation** (A channel)

Weights use smoothstep for soft cell boundaries:
```
InnerWeight = 1 - smoothstep(inner_radius - eps, inner_radius + eps, dist)
OuterWeight = (1 - InnerWeight) * (1 - smoothstep(outer_radius - eps, outer_radius + eps, dist))
```
where `eps = max(cell_smoothing, 0.0001)`.

Distance metric is configurable: Euclidean `length(offset)`, Manhattan `abs(x)+abs(y)`, or Chebyshev `max(abs(x),abs(y))`.

### Transition Function

Smooth topple decision:
```
Activation = threshold * smoothstep(threshold - threshold_smoothing, threshold + threshold_smoothing, Accumulation)
```
Low smoothing = sharp discrete topple. High smoothing = gradual soft transitions.

### Update Rule (Faithful + Continuous)

```
AnnulusActivation = OuterAccumulation / OuterWeightTotal
DiskAccumulation  = InnerAccumulation / InnerWeightTotal
Accumulation      = AnnulusActivation + DiskAccumulation
Activation        = transition(Accumulation)

// Faithful: accumulate neighbor contributions, subtract own activation
NewAccumulation = PreviousAccumulation + AnnulusActivation - Activation

// Continuous timestep: blend toward new state
Accumulation = mix(PreviousAccumulation, NewAccumulation, timestep)
```

The faithful rule prevents diffusive averaging — preserves intricate fractal structure. Continuous timestep smooths the evolution for fluid animation.

### Grain Injection (Feedback Texture)

Each frame, sample the feedback/accumulation texture at each grid cell:
```
brightness = dot(feedbackSample.rgb, vec3(0.299, 0.587, 0.114))
contribution = max(brightness - injectionThreshold, 0.0)
Accumulation += injectionRate * contribution
```

Same pattern as curl advection's `accumTexture` injection. The threshold gates which parts of the scene inject grains — high threshold = only bright transients, low threshold = more of the scene feeds in. Users shape injection geometry via drawables and other effects upstream.

### Color Output

Two-channel mapping through ColorConfig LUT:
- **LUT index**: accumulation (grain height) — picks the color along the gradient
- **Brightness multiplier**: activation (topple state) — toppling cells flare bright, settled cells dim

This ensures gradient/rainbow modes show height-based color while SOLID mode still reveals avalanche dynamics as brightness pulses.

### Grid Resolution

Simulation runs at a configurable fraction of screen resolution (e.g., 1/2, 1/4, 1/8). Lower resolution = fewer pixels = more topple iterations affordable per frame = avalanches propagate further. Output upsampled (nearest or bilinear) to screen for rendering.

### Wrapping

Edges wrap toroidally: `wrap(p) = (p + resolution) % resolution`. Avalanches that reach one edge continue from the opposite side.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| threshold | float | 0.1-1.0 | 0.5 | Topple trigger level — low = frequent small topples, high = rare large avalanches |
| thresholdSmoothing | float | 0.0-0.3 | 0.04 | Sharpness of topple transition — 0 = discrete snap, high = gradual |
| cellSmoothing | float | 0.0-5.0 | 1.5 | Soft vs hard cell boundaries in weight function |
| outerRadius | float | 1.0-20.0 | 6.0 | Neighborhood size — larger = wider avalanche spread per step |
| innerRadius | float | 0.0-outerRadius | outerRadius/3 | Inner disk size — ratio to outer controls annulus shape |
| timestep | float | 0.01-0.5 | 0.05 | Continuous update rate — low = slow smooth evolution, high = fast snappy |
| injectionThreshold | float | 0.0-1.0 | 0.3 | Brightness cutoff for feedback injection — high = only bright spots inject |
| injectionRate | float | 0.0-1.0 | 0.1 | Grain injection strength from feedback texture |
| distanceMetric | enum | euclidean/manhattan/chebyshev | euclidean | Neighborhood shape — euclidean=circular, manhattan=diamond, chebyshev=square |
| gridScale | int | 1-8 | 2 | Resolution divisor — 1=full res, 4=quarter res, 8=eighth res |
| dissipation | float | 0.0-0.05 | 0.001 | Per-frame grain drain — prevents runaway accumulation |

## Modulation Candidates

- **threshold**: shifts system between frequent small topples and rare massive avalanches
- **timestep**: controls evolution speed — slow dreamy drift vs fast reactive snapping
- **injectionRate**: gates how aggressively the scene feeds the sandpile
- **injectionThreshold**: tightens or loosens the brightness gate on grain injection
- **outerRadius**: changes avalanche spread pattern — small = tight fractal, large = broad waves
- **cellSmoothing**: morphs between crisp geometric cells and soft organic blending
- **dissipation**: controls how quickly the field drains — high = transient flashes, low = persistent buildup

### Interaction Patterns

- **Cascading Threshold**: `injectionThreshold` gates `injectionRate` — grains only enter the system where feedback brightness exceeds the threshold. During quiet passages, the gate closes and the sandpile drains toward calm. Loud moments breach the gate and flood grains in, triggering avalanche cascades.
- **Competing Forces**: `injectionRate` vs `dissipation` — injection builds accumulation while dissipation drains it. The sandpile sits at the balance point between these two forces. When injection dominates, avalanches cascade everywhere. When dissipation dominates, the field settles to near-empty.
- **Resonance**: `threshold` and `outerRadius` amplify each other at extremes — high threshold with large radius means cells need a lot of accumulation to topple but when they do, the cascade spreads wide and hits many neighbors at once, creating rare but massive avalanche events.

## Notes

- **Performance**: The `(2r+1)^2` neighborhood loop dominates cost. At outerRadius=10, each pixel samples 441 neighbors. Grid resolution scaling is the primary mitigation — running at 1/4 res reduces work by 16x.
- **Faithful update rule**: Essential for interesting fractal patterns. Without it, the system diffuses into smooth blobs.
- **Distance metric**: Manhattan produces the sharpest diamond/geometric fractals (user's preferred "really cool" variants). Euclidean gives smoother circular patterns.
- **No separate compute shader**: Unlike physarum/boids, this runs as a fragment shader ping-pong, same as curl advection. No agent buffers or spatial hashing needed.
