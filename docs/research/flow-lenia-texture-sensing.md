# Lenia and Flow Lenia: Continuous Cellular Automata

Research on Lenia-based systems as texture-reactive visualizers for AudioJones.

## Standard Lenia

Lenia is a continuous generalization of Conway's Game of Life, created by Bert Wang-Chak Chan. The name derives from Latin *lenis* meaning "smooth." Where Game of Life operates on binary cells in discrete time steps, Lenia uses continuous state values (0.0 to 1.0), continuous space (floating-point coordinates), and continuous time (small dt increments).

### The Update Rule

Each cell's state evolves according to neighborhood convolution and a growth function:

```
A^(t+dt) = clamp(A^t + dt * G(K * A^t), 0, 1)
```

The convolution `K * A` samples the neighborhood weighted by a kernel. The growth function `G` determines whether the cell grows or decays based on that neighborhood sum.

### The Kernel: Gaussian Ring

Unlike Game of Life's uniform 8-neighbor sampling, Lenia uses a **ring-shaped kernel**:

```
K(r) = exp(-((r - μ_K) / σ_K)²)
```

This Gaussian ring peaks at radius `μ_K` from each cell. Cells don't sample immediate neighbors uniformly; they sample at a specific distance determined by the kernel parameters. A kernel with `μ_K = 10` and `σ_K = 3` creates a ring sampling neighbors approximately 10 pixels away, weighted by a Gaussian falloff.

### The Growth Function: Optimal Density

The growth function maps neighborhood density to change rate:

```
G(u) = 2 * exp(-((u - μ) / σ)²) - 1
```

This returns values in [-1, 1]. When neighborhood density `u` equals `μ`, growth is maximal (+1). As density diverges from optimal, growth decreases, eventually becoming decay (-1). This creates self-regulation: cells in sparse regions die; cells in overly dense regions die; only cells with "just right" neighbor density thrive.

### Emergent Creatures

The interaction of ring-sampling and density-dependent growth produces self-organizing structures unlike anything in discrete cellular automata. Over 400 distinct "species" have been cataloged.

Visual characteristics:
- **Smooth and fuzzy**: Continuous states create soft edges rather than hard pixel boundaries
- **Geometric**: Many creatures exhibit radial or bilateral symmetry
- **Metameric**: Bodies composed of repeated segments or lobes
- **Resilient**: Creatures heal when damaged, maintaining structural integrity
- **Locomotive**: Many species move autonomously through the grid

The creatures resemble microscopic organisms—amoebas, radiolarians, diatoms. They pulse, contract, rotate, and glide. Some species replicate. Others consume each other.

## Flow Lenia: Mass Conservation Extension

Standard Lenia has a limitation: total "mass" (sum of all cell states) fluctuates freely. Cells grow and decay, but nothing constrains the total. This makes ecosystems unstable. A creature can spontaneously appear from noise or vanish without trace.

Flow Lenia, developed by Plantec et al. at Inria/Google DeepMind, adds mass conservation. The total activation across all cells remains constant:

```
∑ A^t = ∑ A^(t+dt) for all t
```

### Reintegration Tracking

Mass conservation requires a fundamentally different update mechanism. Instead of adding/subtracting to cell states, Flow Lenia computes **flow fields** that redistribute existing mass.

The flow direction combines two gradients:
1. **Affinity gradient** (∇U): Mass flows toward regions where it "wants to be" based on neighborhood conditions
2. **Concentration gradient** (-∇A): High-density regions diffuse outward

```
F = (1-α)∇U - α∇A
```

The balance parameter α increases when local density exceeds a threshold, causing dense regions to prioritize diffusion over affinity-following.

The actual mass transfer uses **reintegration tracking**, a semi-Lagrangian algorithm that tracks where mass "wants to go" and ensures nothing is lost during discretization. When two flow paths converge on the same cell, their masses add rather than overwrite.

### Localized Parameters

A powerful addition: each cell stores its own kernel and growth parameters. These parameters flow with mass through the reintegration process. When creature mass enters a region, it brings its "species identity" (μ, σ values) with it.

This enables multi-species simulations where different creatures with different rules coexist and interact in the same world.

### Food Sensing

Flow Lenia introduces external "food" channels. Creatures sense food via cross-channel convolution—a separate kernel that detects food proximity. The growth function includes food terms:

```
growth = G(neighborSum) + foodWeight * foodNearby
```

Creatures near food grow faster. Combined with mass conservation, this creates chemotaxis: creatures flow toward food over many timesteps. Food consumption transfers mass from the food channel to the creature channel.

## Visual Vocabulary: Blobs vs Networks

Lenia and Flow Lenia produce a distinct visual vocabulary compared to Physarum.

**Physarum** creates **networks**: discrete agents leave trails, trails reinforce paths, agents follow trails, paths strengthen. The emergent structure is tendril-like, branching, connective—resembling slime mold networks or vascular systems.

**Lenia** creates **blobs**: the continuous field self-organizes into localized patterns. No trails, no network connectivity. Creatures are isolated masses that maintain shape through local field dynamics. They're soft, smooth, organic—resembling amoebas or jellyfish.

| Visual Element | Physarum | Lenia |
|----------------|----------|-------|
| Primary shape | Network/tendrils | Isolated blobs |
| Edges | Sharp trails | Soft/fuzzy gradients |
| Connectivity | High (paths link regions) | Low (creatures are self-contained) |
| Movement | Agents along trails | Mass flows within creatures |
| Texture | Dense webbing | Sparse floating forms |

## Compatibility with AudioJones Waveforms

AudioJones renders waveforms at 60fps. Each frame, the waveform exists briefly at a position, then moves. This fundamentally conflicts with Lenia/Flow Lenia's food-sensing model.

### The Timing Mismatch

Flow Lenia food sensing assumes:
1. Food appears at position A
2. Creature senses food, begins flowing toward A
3. Creature mass migrates toward A over many frames (10-100+)
4. Creature arrives, consumes food

At 60fps, step 3 never completes. The waveform has moved by the next frame. Creatures sense "food" that's already gone, begin migrating toward nothing, and the response never completes.

This isn't a parameter tuning issue. It's architectural. Lenia dynamics operate on timescales incompatible with real-time audio.

### What Would Happen

If AudioJones fed live waveforms to a Flow Lenia system:

**With standard food sensing**: Creatures would "chase" the waveform perpetually, always a frame behind. Their mass would smear in the direction of waveform movement rather than forming coherent structures. No food would ever be consumed. Visual result: blurry streaks following the waveform with no creature structure.

**With direct mass injection**: Bypassing food sensing, the waveform could inject mass directly into the creature field. Lenia dynamics would then redistribute this mass according to growth/decay rules. But injected mass in "wrong" density regions immediately decays. Mass in "right" regions survives but doesn't form creatures—creature formation requires many timesteps of stable conditions.

**With accumulated history**: Temporal smearing of the waveform creates pseudo-stable food regions. Creatures could sense this blurred history. But the history is everywhere the waveform has been, not a localized food source. Creatures would spread diffusely rather than localizing.

### Visual Result Assessment

Flow Lenia in AudioJones would not produce the blob-like creatures seen in research videos. Those creatures emerge from stable conditions over hundreds of timesteps. Real-time audio provides no such stability.

The actual visual result would be:
- Diffuse glowing regions that follow waveform history
- No distinct creature shapes
- Soft, blurry texture with constant motion
- Potentially interesting abstract visuals, but not "Lenia creatures"

The technique provides a different aesthetic vocabulary—amorphous, diffuse, smooth—compared to Physarum's sharp networks. Whether that vocabulary suits AudioJones is an artistic question, not a technical one.

## Computational Cost

Flow Lenia is significantly more expensive than Physarum:

**Per-frame operations**:
- Lenia: O(pixels × R²) for neighborhood convolution at radius R
- Flow Lenia: Above + flow field computation + reintegration tracking
- Physarum: O(agents) + O(pixels) for trail diffusion

At 1920×1080 with kernel radius 20, Lenia requires ~800 million samples per frame for convolution alone. Physarum with 100K agents requires ~100K sense operations plus ~2M pixel diffusion.

Multi-species Flow Lenia with per-cell parameters adds another dimension of complexity.

## Hybrid Approach: Physarum Trails as Food Source

The timing problem stems from waveforms being transient. But Physarum trails are stable—agents sense trails, follow them, deposit more, and trails persist. Once a network forms, individual paths remain for many frames while topology evolves slowly.

If Lenia creatures sensed the Physarum trail map instead of raw waveforms:

| Property | Accumulated Waveform | Physarum Trail Map |
|----------|---------------------|-------------------|
| Shape | Smeared path/gradient | Discrete network structure |
| Stability | Constantly shifting | Self-reinforcing |
| Localization | Diffuse across history | Concentrated along paths |
| Persistence | Decays uniformly | Maintained by agent activity |

Physarum would act as an intermediary, converting transient audio into stable spatial structures that Lenia dynamics could respond to.

**Potential visual result**: Sharp Physarum network tendrils with soft Lenia blobs clustered at high-density nodes (trail intersections). A hybrid aesthetic combining network connectivity with organic blob forms.

**Open questions**:
- Trail width vs kernel radius: Physarum trails are thin (few pixels); Lenia kernels sample at radius 10-20+. Creatures might only form at junctions where density is sufficient.
- Feedback dynamics: Lenia consuming trail mass weakens the network; Physarum depositing feeds Lenia. The interaction could be interesting or unstable.

This approach merits future experimentation as it solves the fundamental timing mismatch by using Physarum as a waveform-to-stable-structure converter.

## Conclusion

Flow Lenia is architecturally incompatible with direct real-time audio reactivity. The food-sensing chemotaxis that produces interesting creature behaviors requires stable food sources persisting for many frames. AudioJones waveforms change every frame.

Forced adaptation with raw waveforms produces diffuse, structureless visuals rather than the distinct blob-like creatures that define Lenia's aesthetic.

However, a hybrid approach using Physarum trails as Lenia's food source could work. Physarum converts transient audio into stable network structures, which Lenia could then respond to on its natural timescale. This would produce a novel hybrid aesthetic rather than pure Lenia creatures.

## Sources

- [Lenia Project](https://chakazul.github.io/lenia.html) - Official project page, videos, papers, code
- [Lenia: Biology of Artificial Life](https://arxiv.org/abs/1812.05433) - Original paper (arXiv:1812.05433)
- [Flow Lenia Paper (MIT Press)](https://direct.mit.edu/artl/article/31/2/228/130572/Flow-Lenia-Emergent-Evolutionary-Dynamics-in-Mass) - Mass conservation extension
- [Flow Lenia Project](https://sites.google.com/view/flowlenia/) - Interactive demos and research
- [arXiv:2212.07906](https://arxiv.org/abs/2212.07906) - Flow Lenia technical details
- [Lenia Wikipedia](https://en.wikipedia.org/wiki/Lenia) - Overview and history
- [HEGL Continuous Cellular Automata](https://hegl.mathi.uni-heidelberg.de/continuous-cellular-automata/) - Tutorial and interactive examples
