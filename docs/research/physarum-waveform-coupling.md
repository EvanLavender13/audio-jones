# Physarum Waveform Coupling: Temporal Mismatch Problem

Research on why standard Physarum sensing fails for audio-reactive waveforms, and viable alternatives.

## The Problem

Waveforms move at 60fps. Physarum agents sense-turn-move over 2-5 frames. The waveform is gone before agents arrive.

```
Frame N:   Waveform at position A
           Agent senses A, begins turning
Frame N+1: Waveform at position B
           Agent still turning toward A
Frame N+2: Agent arrives at A
           Waveform long gone, only decay residue remains
```

**Current behavior**: Agents follow the blurred feedback trails, not the live waveform. Setting `accumSenseBlend = 1.0` doesn't help—the timing mismatch is fundamental.

## Why Standard Approaches Fail

Traditional Physarum assumes a **static or slow-moving** chemoattractant landscape. Flow Lenia research uses **stationary** food sources. Neither model handles stimuli that move faster than agent response time.

**These approaches fail for moving waveforms**:

| Approach | Why It Fails |
|----------|--------------|
| Improve sensor fidelity | Agents still take multiple frames to respond |
| Gradient-based steering | Gradient of a moving target is meaningless next frame |
| Multi-sensor integration | More sensors, same timing problem |
| Consumption mechanics | Can't consume something that's already gone |

## Viable Approaches

Only approaches that bypass the sense-turn-move loop can work.

### Approach 1: Direct Injection

The waveform deposits chemical directly into `trailMap`. Agents don't chase the waveform—they navigate a chemical field that reflects where the waveform has been.

```glsl
// Compute pass before agent update
vec3 waveform = texture(accumMap, uv).rgb;
vec4 trail = imageLoad(trailMap, coord);
vec3 injected = trail.rgb + waveform * injectionRate;
imageStore(trailMap, coord, vec4(injected, 0.0));
```

**What happens**: Waveform continuously "paints" chemical into the trail landscape. Trail decay erases old positions. Agents cluster where waveform has passed, creating motion trails with natural delay.

**Tradeoff**: Agents respond to history, not live waveform. Delay is intentional and tunable via `decayHalfLife`.

### Approach 2: Spawn at Waveform

Agents continuously respawn at waveform positions. No navigation needed—they start where the action is.

```glsl
if (random(agentId, time) < respawnRate) {
    vec2 samplePos = randomPosition(agentId, time);
    float intensity = texture(accumMap, samplePos / resolution).r;
    if (intensity > threshold) {
        agent.x = samplePos.x;
        agent.y = samplePos.y;
        agent.heading = random(agentId, time + 1) * TAU;
    }
}
```

**What happens**: Agents "rain down" onto the waveform, then diffuse outward following trail gradients. Creates radial patterns emanating from waveform geometry.

**Tradeoff**: Loses the "slime mold navigating toward food" aesthetic. Agents don't converge on the waveform—they explode outward from it.

### Approach 3: Force Field

Waveform applies forces directly to agent heading. Instant response, no sensing delay.

```glsl
vec2 gradient;
gradient.x = texture(accumMap, uv + vec2(1,0)/res).r - texture(accumMap, uv - vec2(1,0)/res).r;
gradient.y = texture(accumMap, uv + vec2(0,1)/res).r - texture(accumMap, uv - vec2(0,1)/res).r;

vec2 force = normalize(gradient) * forceStrength * intensity;
agent.heading = atan(sin(agent.heading) + force.y, cos(agent.heading) + force.x);
```

**What happens**: Agents react in the same frame the waveform appears. Heading snaps toward/away from bright regions.

**Tradeoff**: Loses emergent Physarum behavior. Agents become particles in a vector field, not organisms following chemical trails.

## Comparison

| Approach | Preserves Physarum Behavior | Response Time | Visual Character |
|----------|----------------------------|---------------|------------------|
| Injection | Yes - agents still follow trails | Delayed (intentional) | Flowing trails behind waveform motion |
| Spawn | Partial - outward diffusion only | Immediate | Radial bursts from waveform |
| Force field | No - becomes particle system | Immediate | Direct waveform tracking |

## Current Implementation State

Existing code (`src/render/physarum.cpp`, `shaders/physarum_agents.glsl`) supports `accumSenseBlend` for waveform sensing, but this only blends what agents sense—it doesn't address timing.

The injection approach requires:
- New compute shader (~30 lines)
- New uniform for injection rate
- Dispatch before agent update

The spawn approach requires:
- Shader modification (~20 lines)
- New uniforms for spawn rate/threshold

The force approach requires:
- Shader modification (~15 lines)
- New uniform for force strength

## Sources

- Existing implementation: `src/render/physarum.cpp:287-331`, `shaders/physarum_agents.glsl`
- Jones (2010): "Characteristics of pattern formation in approximations of physarum transport networks"
