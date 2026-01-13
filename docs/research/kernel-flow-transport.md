# Kernel Flow Transport

A density-conserving material transport simulation that creates organic veiny patterns through flow convergence. Material moves along a self-organizing vector field, accumulating into ridges.

## Classification

- **Category**: SIMULATIONS (new simulation type)
- **Core Operation**: Gaussian-weighted material transport along computed flow field
- **Pipeline Position**: Same as Physarum - generates to trail map, composites to output

## References

- [Lomateron Shadertoy](https://www.shadertoy.com/user/lomateron) - Original technique creator
- [Aiekick "Smooth Veins"](https://www.shadertoy.com/view/DdKczc) - Extended version with theming
- User-provided shader source (minimal 5-buffer implementation)

## Algorithm

Four compute passes per frame, all using 13x13 Gaussian-weighted kernels (radius 6).

### Pass 1: Gradient Detection (Movement)

Computes radial and tangential flow gradients from neighbor differences:

```glsl
for (i,j in -6..6) {
    vec2 dir = normalize(vec2(i,j));
    float weight = exp(-dot(ij*0.5, ij*0.5));  // Gaussian
    vec4 diff = sample(u + ij) - center;

    radial += weight * dot(diff.xy, dir);           // Divergence-like
    tangential += abs(weight * dot(diff.xy, perp(dir)));  // Curl-like
}
```

**Output**: `vec2(radial, tangential)` - encodes flow structure

### Pass 2: Propagation

Spreads flow field based on gradients:

```glsl
flow.xy += radial * dir * 0.1;           // Radial gradient creates expansion
flow.xy += (tangential_diff) * dir * 0.5; // Tangential difference creates rotation
```

Self-feedback creates persistent flow patterns.

### Pass 3: Normalization

Computes conservation weights to prevent material loss:

```glsl
vec2 predicted = currentFlow.xy;
float density = 0.0;
for (i,j in -6..6) {
    vec2 offset = predicted + vec2(i,j);
    density += exp(-dot(offset, offset));  // Sum Gaussians at landing positions
}
weight = 1.0 / density;  // Sparse areas get higher weight
```

### Pass 4: Transport (Trail)

Gaussian-weighted material transport - the core of the technique:

```glsl
for (i,j in -6..6) {
    vec4 neighborTrail = sample_trail(u + ij);
    vec4 neighborFlow = sample_flow(u + ij);

    // How close is neighbor's predicted position to THIS pixel?
    vec2 landing = neighborFlow.xy - vec2(i,j);
    float contribution = exp(-dot(landing, landing)) * neighborFlow.z;

    result.xy += contribution * neighborFlow.xy;
    result.z += contribution;  // Total mass
}
result.xy /= result.z;  // Normalize
```

Material flows along vectors but mass is conserved via soft assignment.

### Rendering

Simple sinusoidal color mapping:

```glsl
color = sin(flow.x * 4.0 + vec4(1,3,5,4)) * 0.25
      + sin(flow.y * 4.0 + vec4(1,3,2,4)) * 0.25
      + 0.5;
```

## Key Properties

| Property | Value |
|----------|-------|
| Kernel radius | 6 pixels (13x13 samples) |
| Samples per pixel | 169 per pass, 676 total |
| Material conservation | Yes (soft Gaussian assignment) |
| Stable patterns | Yes (converges to ridges) |
| Injection method | Mouse/programmatic Gaussian blob |

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| kernelRadius | int | 2-10 | Pattern scale, cost grows quadratically |
| radialWeight | float | 0.0-0.5 | Expansion vs. compression |
| tangentialWeight | float | 0.0-1.0 | Rotational flow strength |
| gaussianSigma | float | 0.3-0.8 | Kernel sharpness |
| injectionRate | float | 0.0-1.0 | New material per frame |

## Comparison to Physarum

| Aspect | Physarum | Kernel Flow |
|--------|----------|-------------|
| Agents | Yes (millions) | No |
| Sensing | 3-point directional | Full kernel neighborhood |
| Trail | Deposit + diffuse | Transport + conserve |
| Emergence | Swarm behavior | Flow convergence |
| Cost | O(agents) | O(pixels * kernel^2) |
| Look | Branching networks | Smooth veiny ridges |

## Compatibility Assessment

**Compatible** with AudioJones pipeline:

1. **Compute shader**: Uses 4 passes like trail diffusion
2. **Trail map**: Output is RGBA density, same as Physarum
3. **Compositing**: Standard blend modes apply
4. **Audio reactivity**: Injection rate, kernel params can modulate

**Concerns**:

1. **Performance**: 676 texture samples per pixel per frame is expensive. May need reduced kernel (radius 3-4) or lower resolution.
2. **Multi-buffer**: Needs 4 textures (movement, propagation, normalization, trail) vs. Physarum's 2 (agents SSBO, trail).
3. **No agents**: Cannot directly inject waveforms as agent positions. Would need injection-point approach instead.

## Audio Mapping Ideas

- **Injection positions**: Place Gaussian blobs at FFT peaks or beat events
- **Kernel radius**: Modulate with bass energy (larger = smoother flow)
- **Flow strength**: Map to mid frequencies
- **Decay/injection balance**: Map to overall energy

## Implementation Notes

1. Could share trail map infrastructure with Physarum (ping-pong textures)
2. Movement and propagation buffers are intermediate - could use half-resolution
3. Normalization pass prevents runaway accumulation but adds cost
4. Consider radius 4 (81 samples) for real-time performance

## Sources

- Lomateron Shadertoy profile
- Aiekick "Smooth Veins" shader (NC-SA 3.0)
- User-provided minimal implementation
