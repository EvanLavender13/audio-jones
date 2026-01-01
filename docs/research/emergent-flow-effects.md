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

## 2. Strange Attractor Flow

Particles trace trajectories through chaotic dynamical systems. Lorenz, Rössler, and other strange attractors produce perpetual, non-repeating motion by mathematical necessity.

### Core Behavior

Each particle:
1. Evaluates attractor differential equations at current position (e.g., Lorenz: dx/dt = σ(y-x), dy/dt = x(ρ-z)-y, dz/dt = xy-βz)
2. Integrates position using Euler or RK4
3. Deposits trail at new position
4. Optionally respawns when leaving bounds

Chaotic dynamics guarantee particles never settle into periodic orbits. Trajectories diverge exponentially, creating continuous mixing.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| sigma | 5-15 | Lorenz: Prandtl number (rate of rotation vs convection) |
| rho | 20-30 | Lorenz: Rayleigh number (temperature difference driving flow) |
| beta | 1-4 | Lorenz: Aspect ratio of convection cells |
| timeScale | 0.001-0.05 | Integration step size (controls speed) |
| depositAmount | 0.01-0.2 | Trail intensity per deposit |

Different attractor types (Lorenz, Rössler, Aizawa, Thomas) swap the differential equations while keeping the same infrastructure.

### Color

Hue from gradient position at spawn. Alternative: hue from velocity magnitude or z-coordinate (depth in 3D projection).

### Audio Reactivity

- Bass: modulate rho (changes attractor shape dramatically)
- Mids: modulate timeScale (speed of flow)
- Beat: respawn percentage of particles at random positions

### Why Emergent

Strange attractors exhibit sensitive dependence on initial conditions. Nearby particles diverge exponentially while remaining bounded within the attractor's basin. Creates organic streaming without repetition.

### References

- [BrutPitt/glChAoS.P](https://github.com/BrutPitt/glChAoS.P) - 256M particles, 40+ attractor types, OpenGL 4.5
- [IM56/Lorenz-Particle-System](https://github.com/IM56/Lorenz-Particle-System) - DirectX 11/HLSL, 500K particles
- [fuse* Strange Attractors GPU](https://fusefactory.github.io/openfuse/strange%20attractors/particle%20system/Strange-Attractors-GPU/) - Fragment shader implementation with curl noise

---

## 3. SPH Fluid Particles

Smoothed Particle Hydrodynamics simulates fluid as interacting particles. Pressure and viscosity forces create continuous turbulent flow.

### Core Behavior

Each particle:
1. Finds neighbors within smoothing radius (spatial hash grid)
2. Computes density from neighbor contributions (SPH kernel)
3. Computes pressure force (repels from high-density regions)
4. Computes viscosity force (smooths velocity differences)
5. Adds gravity, integrates velocity and position
6. Deposits trail at new position

Incompressibility and viscosity create fluid-like motion. Energy input (gravity, boundaries) prevents settling.

### Parameters

| Parameter | Range | Effect |
|-----------|-------|--------|
| smoothingRadius | 10-50 | Range of particle-particle influence |
| pressureStiffness | 100-1000 | Resistance to compression (higher = more incompressible) |
| viscosity | 0.01-0.5 | Velocity smoothing (higher = thicker fluid) |
| gravity | 0-500 | Downward acceleration |
| depositAmount | 0.01-0.2 | Trail intensity per deposit |

### Color

Hue from gradient position. Alternative: hue from velocity magnitude (fast = bright) or pressure (high density = saturated).

### Audio Reactivity

- Bass: modulate gravity direction/magnitude (slosh the fluid)
- Mids: modulate viscosity (water vs honey)
- Beat: inject impulse at random positions (stir the fluid)

### Why Emergent

Pressure gradients push particles apart; viscosity couples their velocities; boundaries redirect flow. The balance creates turbulent eddies and streaming that never fully stabilize.

### References

- [multiprecision/sph_opengl](https://github.com/multiprecision/sph_opengl) - OpenGL compute shader, MIT license
- [AminAliari/fluid-simulation](https://github.com/AminAliari/fluid-simulation) - Vulkan/DX12, spatial hashing
- [Wicked Engine SPH Tutorial](https://wickedengine.net/2018/05/scalabe-gpu-fluid-simulation/) - Detailed HLSL walkthrough
- [Gornhoth/Unity-SPH](https://github.com/Gornhoth/Unity-Smoothed-Particle-Hydrodynamics) - Three implementations compared

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
