# Reaction-Diffusion Systems

Gray-Scott model reference: mathematical foundations, parameter space, pattern classification, numerical implementation, and stability analysis.

## 1. Mathematical Foundations

### The Gray-Scott Model

Two chemical species (U and V) react and diffuse on a 2D domain. The governing partial differential equations:

```
∂U/∂t = Dᵤ∇²U - UV² + F(1-U)
∂V/∂t = Dᵥ∇²V + UV² - (F+k)V
```

Where:
- `U`, `V`: concentrations of chemicals A and B (range 0-1)
- `Dᵤ`, `Dᵥ`: diffusion coefficients
- `F`: feed rate (replenishes U, removes V)
- `k`: kill rate (removes V only)
- `∇²`: Laplacian operator (spatial second derivative)

### Reaction Mechanism

Three concurrent processes update concentrations:

| Process | Effect on U | Effect on V | Interpretation |
|---------|-------------|-------------|----------------|
| Feed | +F(1-U) | -FV | U enters system, V exits |
| Kill | - | -kV | V decays |
| Autocatalysis | -UV² | +UV² | U + 2V → 3V |

The autocatalytic term `UV²` requires two V molecules to catalyze conversion of U to V. This nonlinearity drives pattern formation.

### Fixed Points

Up to three steady states exist:

1. **Trivial fixed point**: (U,V) = (1,0). All U, no V. Stable without perturbation.
2. **Non-trivial fixed points**: exist beyond the bifurcation line at F = 4(F+k)².

The trivial state remains locally stable. Finite-amplitude perturbations trigger transitions to patterned states. Both homogeneous and structured solutions coexist for identical parameters (bistability).

### Turing Instability Conditions

Diffusion-driven instability (Turing instability) enables pattern formation. Core requirements:

1. **Differential diffusion**: Dᵥ > Dᵤ (inhibitor diffuses faster than activator)
2. **Stable without diffusion**: reaction kinetics alone reach homogeneous steady state
3. **Unstable with diffusion**: certain spatial wavelengths amplify exponentially

The diffusion ratio Dᵥ/Dᵤ must exceed a threshold (up to 10:1 in activator-inhibitor systems). Standard Gray-Scott uses Dᵤ=1.0, Dᵥ=0.5 (ratio 0.5). This inverts the classical Turing ratio but the reaction kinetics still produce patterns.

### Bifurcation Structure

The Gray-Scott system exhibits multiple bifurcation types:

- **Saddle-node bifurcations**: create/destroy fixed points
- **Hopf bifurcations**: spawn limit cycles (temporal oscillations)
- **Turing bifurcations**: generate spatial patterns from homogeneous states

A hierarchy of saddle-node points drives self-replicating pattern dynamics. Multiple distinct solutions coexist at identical parameter values.

## 2. Pattern Classification (Pearson System)

John Pearson's 1993 paper "Complex Patterns in a Simple System" established a classification scheme using Greek letters. Robert Munafo extended this to 19 pattern types.

### Classification Table

| Type | F Range | k Range | Description |
|------|---------|---------|-------------|
| α (alpha) | 0.010-0.014 | 0.047-0.053 | Spatiotemporal chaos, wavelets, fledgling spirals |
| β (beta) | 0.014-0.026 | 0.039-0.051 | Chaotic oscillation, periodic voids |
| γ (gamma) | 0.022-0.026 | 0.051-0.055 | Unstable wormlike stripes with oscillation |
| δ (delta) | 0.030-0.042 | 0.055-0.059 | Hexagonal spot arrays (true Turing patterns) |
| ε (epsilon) | 0.018-0.022 | 0.055-0.059 | Unstable spots with rings, mitosis, crowding |
| ζ (zeta) | 0.022-0.026 | 0.059-0.061 | Stabilized epsilon, less volatile spots |
| η (eta) | 0.034 | 0.063 | Mixed spots/worms, reaches steady state |
| θ (theta) | 0.030-0.038 | 0.057-0.061 | Concentric rings → connected stripes |
| ι (iota) | 0.046 | 0.0594 | Negative spots, molecule-like interactions |
| κ (kappa) | 0.050-0.058 | 0.063 | Serpentine stripes, hedgerow mazes |
| λ (lambda) | 0.026-0.034 | 0.061-0.065 | Soliton mitosis → hexagonal steady state |
| μ (mu) | 0.046-0.058 | 0.065 | Worms growing from ends, parallel final |
| ν (nu) | 0.046-0.082 | 0.063-0.067 | Inert solitons drifting apart exponentially |
| ξ (xi) | 0.010-0.014 | 0.041-0.047 | Large sustained spirals (BZ-like) |
| π (pi) | 0.062 | 0.061 | Stripes/loops/spots with oscillating forces |
| ρ (rho) | 0.090-0.102 | 0.055-0.059 | Red soap bubbles with stripe borders |
| σ (sigma) | 0.090-0.110 | 0.052-0.057 | Blue soap bubbles with stripe borders |

### Parameter Space Topology

The (F,k) parameter space contains a crescent-shaped region where complex behaviors occur. Outside this region, the system converges to either:
- All U (trivial state): V dies out completely
- All V: U depleted entirely

Within the crescent, parameter changes of 0.001 produce qualitatively different patterns. This sensitivity requires precise parameter control.

### Wolfram Classification Mapping

Patterns map to Wolfram's computational classes:
- **Class 1** (fixed): homogeneous states
- **Class 2-a** (local order): type δ (hexagonal spots)
- **Class 3** (chaos): types α, β, ζ
- **Class 4** (complex): self-replicating solitons, type ε

### Common Parameter Sets

| Name | F | k | Result |
|------|---|---|--------|
| Mitosis | 0.0367 | 0.0649 | Dividing cells |
| Coral | 0.0545 | 0.062 | Branching structures |
| Fingerprint | 0.055 | 0.062 | Parallel stripes |
| Spots | 0.038 | 0.099 | Isolated dots |
| Maze | 0.029 | 0.057 | Connected pathways |
| Waves | 0.034 | 0.095 | Traveling waves |

## 3. Numerical Discretization

### Spatial Discretization: Laplacian Stencils

The continuous Laplacian ∇² requires discrete approximation on a grid.

**5-point stencil** (axis-aligned only):
```
     [0   1   0]
1/h² [1  -4   1]
     [0   1   0]
```

**9-point stencil** (includes diagonals, ~2x lower directional bias):
```
        [0.05  0.2   0.05]
1/h²    [0.2   -1    0.2 ]
        [0.05  0.2   0.05]
```

The 5-point stencil propagates diffusion ~40% faster along coordinate axes than diagonals (anisotropy). The 9-point stencil (center=-1, adjacent=0.2, diagonal=0.05) equalizes propagation speed across all directions.

### Anisotropy Comparison

| Stencil | Isotropy | Texture Fetches | Use Case |
|---------|----------|-----------------|----------|
| 5-point | Poor | 5 | Smooth fields, prototyping |
| 9-point | Good | 9 | Production quality, round features |
| Oono-Puri | Optimal | 9 | Maximum accuracy |

The 9-point stencil (0.2/0.05 weights) achieves near-isotropic diffusion with only 9 texture fetches. The 5-point stencil produces poorly-defined lamellae structures compared to 9-point.

### Temporal Discretization: Stability

Explicit Euler (Forward Time, Centered Space) requires the smallest code but enforces a stability constraint:

```
Δt ≤ Δx² / (2 * max(Dᵤ, Dᵥ))
```

Doubling spatial resolution requires 4x smaller timestep. For a 512×512 grid with Δx=1 and Dᵤ=1.0, this gives Δt ≤ 0.5.

**Alternative schemes:**

| Method | Stability | Ops/Cell | Notes |
|--------|-----------|----------|-------|
| Explicit Euler | Conditional | ~20 | Requires small Δt |
| Implicit Euler | Unconditional | ~200+ | Matrix solve per step |
| Crank-Nicolson | Unconditional | ~100 | 2nd order accuracy |
| IIF2 (Integrating Factor) | Unconditional | ~100 | Diffusion-dominated systems |

GPU implementations use explicit Euler with 4-8 substeps per frame. Implicit methods require linear algebra unsuited to fragment shaders.

### Update Equations (Discrete)

Per-cell per-timestep:

```
lapU = laplacian(U)
lapV = laplacian(V)
UVV = U * V * V

U_new = U + (Dᵤ * lapU - UVV + F * (1 - U)) * Δt
V_new = V + (Dᵥ * lapV + UVV - (k + F) * V) * Δt
```

Clamp results to [0,1] to prevent numerical instability from negative concentrations.

## 4. Boundary Conditions

### Types

| Type | Description | Effect |
|------|-------------|--------|
| Periodic | Wrap edges (torus topology) | Patterns tile seamlessly |
| Neumann | Zero-flux (∂U/∂n = 0) | Reflecting boundary |
| Dirichlet | Fixed values at boundary | Forces edge concentration |

### Implementation

**Periodic**: modular indexing
```
x_neighbor = (x + 1) % width
```

**Neumann**: mirror edge values
```
U[edge] = U[edge + 1]  // reflects derivative
```

### Practical Considerations

Periodic boundaries create seamless tiling but patterns may show repeating artifacts at 1/grid-size intervals. Neumann boundaries allow unrestricted pattern growth at edges. Dirichlet boundaries inject or absorb chemicals at fixed rates, controlling pattern nucleation sites.

## 5. Initial Conditions

### Seeding Strategies

V perturbations break the trivial (U=1,V=0) steady state and initiate pattern growth.

| Method | Description | Result |
|--------|-------------|--------|
| Square seed | V=1 in small square region | Single pattern nucleus |
| Random noise | Gaussian noise on V | Multiple nucleation sites |
| Gradient seed | V varies spatially | Directional growth |
| Point seeds | V=1 at discrete points | Controlled spot locations |

### Initialization Values

Standard: U=1.0 everywhere, V=0.0 everywhere except seed regions where V=1.0.

Seed size determines nucleation success. Seeds under 5 pixels often fail to nucleate. Seeds over 50 pixels saturate local dynamics before patterns form. Typical: 10-20 pixel square.

### Pattern Selection

Initial geometry determines final pattern type:
- Symmetric seeds → hexagons, concentric rings
- Random noise → mazes, fingerprints
- Linear seeds → stripe-dominated structures

Identical parameters yield different final patterns based on initial perturbation geometry.

## 6. Spots vs Stripes

### Physical Basis

Activator-inhibitor balance determines stripe/spot selection:

- **Stripes**: activator occupies ~50% of area
- **Spots**: activator confined to isolated regions (<20% area)

### Parameter Influence

Moving through parameter space:
- Lower k → stripes dominate
- Higher k → spots emerge
- Intermediate k → mixed spots/stripes (worms)

### Geometry Constraints

Narrow domains (aspect ratio >5:1) produce stripes only. Square domains permit both spots and stripes.

## 7. GPU Implementation

### Architecture: Texture Ping-Pong

Two textures store simulation state:
1. Read texture: current U,V values
2. Write texture: receives updated values

After each iteration, swap roles. OpenGL prohibits reading and writing the same texture in one pass.

### Data Layout

| Channel | Contents | Range |
|---------|----------|-------|
| Red | U concentration | 0-1 |
| Green | V concentration | 0-1 |
| Blue | (unused or auxiliary) | - |
| Alpha | (unused) | 1 |

RGBA16F or RGBA32F formats prevent quantization artifacts. 8-bit formats introduce visible banding at low concentration gradients.

### Fragment Shader Structure

```glsl
uniform sampler2D uState;
uniform vec2 uResolution;
uniform float uDU, uDV;
uniform float uFeed, uKill;
uniform float uDeltaT;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 texel = 1.0 / uResolution;

    // Sample current state
    vec2 state = texture(uState, uv).rg;
    float U = state.r;
    float V = state.g;

    // 9-point Laplacian
    float lapU = -U;
    float lapV = -V;

    // Adjacent neighbors (weight 0.2)
    lapU += 0.2 * texture(uState, uv + vec2(texel.x, 0)).r;
    lapU += 0.2 * texture(uState, uv - vec2(texel.x, 0)).r;
    lapU += 0.2 * texture(uState, uv + vec2(0, texel.y)).r;
    lapU += 0.2 * texture(uState, uv - vec2(0, texel.y)).r;

    // Diagonal neighbors (weight 0.05)
    lapU += 0.05 * texture(uState, uv + vec2(texel.x, texel.y)).r;
    lapU += 0.05 * texture(uState, uv - vec2(texel.x, texel.y)).r;
    lapU += 0.05 * texture(uState, uv + vec2(texel.x, -texel.y)).r;
    lapU += 0.05 * texture(uState, uv - vec2(texel.x, -texel.y)).r;

    // (Same for lapV with .g channel)

    // Reaction
    float UVV = U * V * V;

    // Update
    float newU = U + (uDU * lapU - UVV + uFeed * (1.0 - U)) * uDeltaT;
    float newV = V + (uDV * lapV + UVV - (uKill + uFeed) * V) * uDeltaT;

    fragColor = vec4(clamp(newU, 0.0, 1.0), clamp(newV, 0.0, 1.0), 0.0, 1.0);
}
```

### Multiple Substeps

Run multiple simulation iterations per display frame to accelerate pattern development:

```cpp
for (int i = 0; i < substepsPerFrame; i++) {
    bindReadTexture(current);
    bindWriteFramebuffer(next);
    drawFullscreenQuad();
    swap(current, next);
}
```

Typical: 4-8 substeps. Higher counts accelerate evolution but increase GPU load linearly.

### Compute Shader Alternative

Compute shaders enable shared memory optimization. A workgroup loads its 16×16 tile plus 1-pixel halo into shared memory. This reduces texture fetches from 9/pixel to ~1.3/pixel amortized (~7x reduction in memory bandwidth).

Trade-off: explicit synchronization barriers, tile boundary handling, and dispatch configuration add ~50 lines of boilerplate.

## 8. Rendering

### Visualization Strategies

| Method | Effect |
|--------|--------|
| Direct mapping | U→brightness, V→hue |
| Threshold | Binary black/white at V=0.5 |
| Gradient map | V concentration → color LUT |
| Edge detection | Highlight V gradients |

### Example Color Mapping

```glsl
vec3 render(float U, float V) {
    // V concentration drives color
    vec3 color = mix(vec3(0.0), vec3(0.2, 0.5, 1.0), V);

    // Edge glow from gradient magnitude
    float edge = length(vec2(dFdx(V), dFdy(V)));
    color += vec3(1.0, 0.8, 0.4) * edge * 5.0;

    return color;
}
```

## 9. Diffusion Coefficient (D) Effects

The ratio D = Dᵥ/Dᵤ controls pattern character:

| D Value | Effect |
|---------|--------|
| D < 1 | Fewer pattern types accessible |
| D = 1 | Stationary patterns impossible, only traveling waves |
| D = 2 | All 19 Pearson pattern types accessible (standard) |
| D > 2 | Patterns become static and spot-dominated |

Standard D=2 (Dᵤ=1.0, Dᵥ=0.5) accesses all Pearson pattern classes.

## Sources

### Foundational
- [Karl Sims - Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html)
- [MROB Xmorphia - Pearson's Parameterization](http://www.mrob.com/pub/comp/xmorphia/index.html)
- [MROB - Pearson's Classification Extended](http://www.mrob.com/pub/comp/xmorphia/pearson-classes.html)
- [Biological Modeling - Gray-Scott](https://biologicalmodeling.org/prologue/gray-scott)

### Mathematical
- [VisualPDE - Gray-Scott Model](https://visualpde.com/nonlinear-physics/gray-scott.html)
- [Wikipedia - Turing Pattern](https://en.wikipedia.org/wiki/Turing_pattern)
- [Wikipedia - Reaction-Diffusion System](https://en.wikipedia.org/wiki/Reaction–diffusion_system)
- [Springer - Analysis on bifurcation and stability of Gray-Scott model](https://www.sciencedirect.com/science/article/abs/pii/S0378437119308143)

### Implementation
- [ciphrd - Reaction Diffusion on Shader](https://ciphrd.com/2019/08/24/reaction-diffusion-on-shader/)
- [Codrops - WebGPU Compute Shader](https://tympanus.net/codrops/2024/05/01/reaction-diffusion-compute-shader-in-webgpu/)
- [Jason Webb - Reaction-Diffusion Playground](https://jasonwebb.github.io/reaction-diffusion-playground/)
- [GitHub - rszczers/reaction-diffusion-gpu](https://github.com/rszczers/reaction-diffusion-gpu)

### Numerical Methods
- [Wikipedia - Nine-point stencil](https://en.wikipedia.org/wiki/Nine-point_stencil)
- [Wikipedia - Discrete Laplace operator](https://en.wikipedia.org/wiki/Discrete_Laplace_operator)
- [PMC - Pattern formation mechanisms of self-organizing reaction-diffusion systems](https://pmc.ncbi.nlm.nih.gov/articles/PMC7154499/)
