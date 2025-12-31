# Emergent Flow Effects Research

Agent-based effects with simple parameter sets that produce organic, flowing, colorful patterns through emergent behavior. Each follows the Physarum pattern: agents sense environment, make local decisions, deposit trails, create global structure.

## Requirements

All effects must:
- Use continuous color from gradient/rainbow (not discrete color types)
- Have 3-5 simple knobs (not matrices or complex configuration)
- Produce continuous flowing motion (never settle into static state)
- Create emergent structure from local agent rules
- Run on GPU compute shaders like Physarum

---

## 1. Curl Noise Flow

Agents follow the curl of an animated 3D noise field. Curl operation guarantees divergence-free flow: no sinks or sources where particles accumulate forever.

### Core Behavior

Each agent:
1. Samples curl of noise at current position
2. Steers toward curl direction
3. Moves forward
4. Deposits trail at new position

Trails diffuse and decay like Physarum. Trail intensity feeds back to modulate agent speed (slow in dense areas) or noise amplitude (dampen flow where trails accumulate).

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| noiseFrequency | 0.001-0.1 | Scale of flow structures (lower = larger swirls) |
| noiseEvolution | 0.0-2.0 | Speed of pattern change over time |
| trailInfluence | 0.0-1.0 | How much trail density affects agent speed |
| stepSize | 0.5-5.0 | Agent movement speed per frame |
| depositAmount | 0.01-0.2 | Trail intensity per deposit |

### Color

Agents assigned hue from gradient at initialization (same as Physarum). Deposit color based on agent hue. Alternative: hue from velocity direction (angle of curl vector).

### Audio Reactivity

- Bass: modulate noiseEvolution (faster pattern shifts)
- Mids: modulate noiseFrequency (scale of structures)
- Highs: modulate depositAmount (trail brightness)

### Why Emergent

Without trail feedback: pure flow field, no emergence. With trail feedback: agents create lanes, avoid congested areas, form organic streaming patterns that shift with the noise evolution.

### References

- [kbladin/Curl_Noise](https://github.com/kbladin/Curl_Noise) - OpenGL GPU implementation
- [Curl Noise Guide](https://emildziewanowski.com/curl-noise/) - Technical explanation with visuals
- [Bitangent Noise](https://atyuwen.github.io/posts/bitangent-noise/) - Fast divergence-free noise generation

---

## 2. Velocity Feedback Flow

Agents influenced by a feedback buffer containing their own history. The accumulated trail texture directly modulates agent velocity, creating emergent lane formation.

### Core Behavior

Each agent:
1. Samples trail map at current position
2. Computes velocity modification from trail (gradient or intensity)
3. Blends base velocity with trail-derived velocity
4. Moves and deposits

The feedback creates memory: where agents went affects where agents go. Lanes form, merge, split organically.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| baseSpeed | 0.5-5.0 | Agent speed without trail influence |
| trailGradientFollow | 0.0-1.0 | Steer toward trail gradient (follow existing lanes) |
| trailSpeedBoost | 0.0-2.0 | Speed multiplier in trail-dense areas |
| diffusionScale | 0-4 | Trail blur radius |
| decayHalfLife | 0.1-5.0 | Trail persistence |

### Color

Hue from gradient position. Agents in same gradient region follow similar lanes, creating color-banded streams.

### Audio Reactivity

- Beat: pulse trailSpeedBoost (agents accelerate through lanes on beat)
- Spectrum: modulate decayHalfLife per frequency band

### Why Emergent

Positive feedback: agents follow trails → trails get stronger → more agents follow. Balanced by diffusion and decay. Creates self-organizing flow networks.

### References

- [Steven Benton's Generative Particle System](https://stevenmbenton.com/project/generative-particle-system/) - TouchDesigner implementation with feedback loops
- [GPU Particle Systems in TouchDesigner](https://nvoid.gitbooks.io/introduction-to-touchdesigner/content/GLSL/12-7-GPU-Particle-Systems.html)

---

## 3. Point Vortex Dynamics

Agents are point vortices with position and angular momentum. Each vortex creates a velocity field that affects all nearby vortices. Classic 2D fluid dynamics N-body problem.

### Core Behavior

Each vortex:
1. Computes induced velocity from all nearby vortices (inverse distance, perpendicular to separation)
2. Sums contributions
3. Moves according to total induced velocity
4. Optionally deposits trail

Vortices orbit each other, merge into streams, create turbulent mixing.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| vortexStrength | 0.1-5.0 | Circulation magnitude |
| influenceRadius | 10-200 | Range of vortex-vortex interaction |
| damping | 0.9-1.0 | Energy dissipation per frame |
| coreRadius | 1-20 | Regularization to prevent singularity at r=0 |
| depositAmount | 0.0-0.2 | Trail intensity (0 = no trails) |

### Color

Option A: Hue from vortex sign (positive = warm, negative = cool)
Option B: Hue from gradient position like Physarum
Option C: Hue from velocity magnitude (fast = bright)

### Audio Reactivity

- Bass: spawn new vortices at screen edges
- Mids: modulate influenceRadius
- Beat: invert random vortex signs (creates mixing)

### Why Emergent

Vortex-vortex interaction creates complex dynamics from simple inverse-distance law. Pairs orbit, groups form streets, opposite-sign vortices annihilate. Never settles.

### References

- [Point vortex dynamics](https://en.wikipedia.org/wiki/Point_vortex) - Mathematical foundation
- [Vortex methods for fluid simulation](https://www.cs.ubc.ca/~rbridson/docs/english-siggraph2013-notes.pdf) - SIGGRAPH course notes

---

## 4. Chemotactic Swarm (Physarum Variant)

Variant of Physarum where agents sense AND deposit multiple chemical fields. Simpler than species matrix: single gradient mapped to chemical affinity.

### Core Behavior

Two trail maps: attractant and repellent. Each agent:
1. Senses attractant (turn toward) and repellent (turn away)
2. Deposits attractant at current position
3. Deposits repellent in wake (behind movement direction)
4. Moves forward

Creates chasing behavior: agents follow attractant but leave repellent, causing followers to veer around.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| attractantSense | 0.0-1.0 | Weight of attractant in steering |
| repellentSense | 0.0-1.0 | Weight of repellent in steering |
| attractantDeposit | 0.01-0.2 | Attractant trail strength |
| repellentDeposit | 0.01-0.2 | Repellent trail strength |
| repellentOffset | 1-10 | Distance behind agent for repellent deposit |

### Color

Hue from gradient. Attractant map colored by depositor hue. Repellent map could be rendered as dark/negative.

### Audio Reactivity

- Bass: modulate repellentSense (more avoidance = more turbulent)
- Mids: modulate attractantDeposit (stronger trails = tighter clustering)

### Why Emergent

Two competing gradients create oscillation and streaming. Agents can't follow directly behind others (repellent), so they spread laterally, creating braided flow patterns.

### References

- [Chemotaxis models](https://www.nature.com/articles/s41598-019-40700-7) - Bacterial movement patterns
- [Multi-species Physarum](https://cargocollective.com/sagejenson/physarum) - Sage Jenson's work

---

## Implementation Notes

All effects share infrastructure with Physarum:
- Compute shader for agent update
- Trail map (RenderTexture2D with RGBA32F)
- Trail diffusion/decay shader
- Blend compositor for final output
- ColorConfig for gradient/rainbow hue assignment

New effects could reuse:
- `PhysarumAgent` struct (or variant with additional fields)
- Trail processing shaders
- Blend modes and compositing pipeline
