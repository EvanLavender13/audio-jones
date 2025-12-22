# Flow Lenia: Texture-Sensing Cellular Automata

Research on Flow Lenia as an alternative to Physarum for waveform-reactive visualization.

## What Flow Lenia Is

Flow Lenia extends Lenia cellular automata with mass conservation. Instead of discrete agents, the entire grid is a continuous field of "creature mass" that grows, decays, and flows based on local rules.

**Core update**:
```
A^(t+dt) = A^t + dt * G(K * A^t)
```

- `K * A` = convolve neighborhood with Gaussian ring kernel
- `G(u) = 2 * exp(-((u - μ) / σ)²) - 1` = growth function, returns [-1, 1]
- Mass grows where neighborhood density matches μ, decays elsewhere

**Mass conservation**: Flow Lenia redistributes mass via flow fields rather than adding/subtracting. Total mass stays constant unless external sources exist.

## How Creatures Sense Food

In Flow Lenia research, "food" is a separate texture channel. Creatures sense food via cross-channel convolution:

```glsl
float foodNearby = convolve(foodChannel, sensingKernel);
float growth = G(neighborSum) + foodWeight * foodNearby;
```

Creatures grow faster near food. Over time, creature mass flows toward food regions.

## The Same Timing Problem

Flow Lenia research uses **stationary food sources** (blue squares that persist indefinitely). AudioJones waveforms exist for one frame then move.

The standard model assumes:
1. Food appears at position A
2. Creature senses food, begins growing toward A
3. Creature mass flows toward A over many frames
4. Creature arrives and consumes food

With a 60fps waveform, step 3-4 never complete. The food is gone before creatures arrive.

## Adaptation for Moving Waveforms

### Option A: Injection (Not Consumption)

The waveform injects mass directly into the creature field. No sensing or navigation required.

```glsl
creatureMass += waveformIntensity * injectionRate;
```

**What happens**: Waveform acts as a mass source. Wherever the waveform touches, creature mass appears. Lenia dynamics then redistribute this mass according to growth/decay rules.

**Character**: Mass accumulates along waveform path. Creatures "grow" from the waveform rather than navigating to it.

### Option B: History Buffer

Accumulate waveform into a persistent buffer. Creatures sense the buffer, not live waveform.

```glsl
foodBuffer = mix(foodBuffer, waveformIntensity, 0.1) * 0.98;
// Creatures sense foodBuffer
```

**What happens**: Buffer creates temporal smearing. Waveform "echoes" persist long enough for creature dynamics to respond.

**Character**: Blurred, smoothed response. Creatures follow averaged waveform positions.

### Option C: Instant Growth Boost

Waveform triggers immediate growth without navigation.

```glsl
float growth = G(neighborSum) + waveformIntensity * boostStrength;
```

**What happens**: Wherever waveform overlaps creature mass, that mass grows faster. No flow toward waveform—just local amplification.

**Character**: Existing creatures pulse brighter when waveform touches them.

## Flow Lenia vs Physarum

| Aspect | Flow Lenia | Physarum |
|--------|------------|----------|
| Representation | Continuous mass field | Discrete agents |
| Update | Per-pixel convolution | Per-agent sense-turn-move |
| GPU cost | O(pixels × R²) | O(agents) + O(pixels) trails |
| Emergent forms | Blob-like creatures | Network tendrils |
| Existing code | None | `src/render/physarum.cpp` |

**Key difference**: Physarum creates network structures from agent trails. Flow Lenia creates blob-like creatures from field dynamics. Different visual vocabulary.

## Implementation Complexity

Flow Lenia requires building from scratch:
- Per-pixel compute shader with neighborhood convolution
- Kernel parameter storage (μ, σ per cell for multi-species)
- Flow field computation for mass conservation
- Separate from existing Physarum infrastructure

The injection adaptation to existing Physarum is simpler and tests the core concept (waveform → chemical landscape → agent response).

## Mathematical Details

### Kernel Function
```
K(r) = exp(-((r - μ_K) / σ_K)²)
```
Gaussian ring at radius μ_K. Cells sample neighbors at a specific distance, not uniformly.

### Growth Function
```
G(u) = 2 * exp(-((u - μ) / σ)²) - 1
```
Peak growth at u = μ. Negative growth (decay) when neighborhood density differs from optimal.

### Localized Parameters
Each cell stores (μ, σ, kernel weights). Parameters flow with mass, enabling multi-species dynamics.

## Sources

- [Flow Lenia Project](https://sites.google.com/view/flowlenia/)
- [Flow Lenia Paper (MIT Press)](https://direct.mit.edu/artl/article/31/2/228/130572/Flow-Lenia-Emergent-Evolutionary-Dynamics-in-Mass)
- [Sensorimotor Lenia](https://developmentalsystems.org/sensorimotor-lenia/)
- [Lenia Project](https://chakazul.github.io/lenia.html)
- [arXiv:2212.07906](https://arxiv.org/abs/2212.07906)
