# Orrery

A procedural hierarchical ring tree where circles orbit on their parent's circumference, building compound spirograph-like motion. Deeper rings spin faster and respond to higher frequencies, so the structure becomes a living frequency decomposition - bass pulses the core, treble lights up the orbiting leaves. Connecting lines between leaf rings form a shifting geometric web that densifies with the music.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: After Scatter generators, before Texture generators
- **Section Index**: 10 (Geometric)

## Attribution

- **Based on**: "Circles spinning" by evewas
- **Source**: https://www.shadertoy.com/view/scXXz4
- **License**: CC BY-NC-SA 3.0

## References

- [Circles spinning](https://www.shadertoy.com/view/scXXz4) - Hierarchical ring SDF with recursive coordinate chaining and connecting lines

## Reference Code

```glsl
float cutoff(in float sdf, in float thickness){
    float fade = .001;
    
    float p = smoothstep(clamp(sdf - thickness, 0.0, 1.0), 0. - fade, fade);
    return p;
}

vec4 ringSDF(in vec2 uv, in float r, in float spd, in float rotOffset, in float rotRadius){
    vec2 offset = rotRadius*vec2(cos(iTime*spd + rotOffset), sin(iTime*spd + rotOffset));
    vec2 UV = uv - offset;
    float p = abs(length(UV) -r);
    p = cutoff(p, 0.0003);
    
    return vec4(UV, p, r);
}


float lineSDF(in vec2 uv, in vec2 a, in vec2 b){
    vec2 pa = uv-a;
    vec2 ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    float p = length(pa-h*ba);
    p = cutoff(p, 0.005);
    
    return p;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    //Normalized UVs
    vec2 uv = fragCoord/iResolution.xy;
    uv = uv*2.-1.;
    uv.x *= iResolution.x/iResolution.y;
    
    //Normalized mouse coords
    vec2 mouse = iMouse.xy/iResolution.xy;
    mouse = mouse*2.-1.;
    mouse.x *= iResolution.x/iResolution.y;
    
    //Circle SDFs
    vec4 ri1 = ringSDF            (  uv,      0.6,   0.,   0.,  0.     );
        vec4 ri2 = ringSDF        (  ri1.xy,  0.4,   .9,  0.,  ri1.w  );
            vec4 ri3 = ringSDF    (  ri2.xy,  0.05,  4.,   0.,  ri2.w  );
            vec4 ri8 = ringSDF    (  ri2.xy,  0.03,  3.5,  2.,  ri2.w  );
        vec4 ri4 = ringSDF        (  ri1.xy,  0.2,   1.5,   2.,  ri1.w  );
            vec4 ri5 = ringSDF    (  ri4.xy,  0.07,  2.,   0.,  ri4.w  );
            vec4 ri9 = ringSDF    (  ri4.xy,  0.04,  2.5,  0.,  ri4.w  );
        vec4 ri6 = ringSDF        (  ri1.xy,  0.2,   1.,   4.,  ri1.w  );
            vec4 ri7 = ringSDF    (  ri6.xy,  0.08,  3.,   0.,  ri6.w  );
            vec4 ri10 = ringSDF   (  ri6.xy,  0.1,   3.,   2.,  ri6.w  );
    
    float riCom = min(min(min(min(min(min(min(min(min(ri1.z, ri2.z), ri3.z), ri4.z), ri5.z), ri6.z), ri7.z), ri8.z), ri9.z), ri10.z); 
    
    //Line SDFs
    float l1 = lineSDF            (  uv,  uv-ri3.xy,  uv-ri5.xy  );
    float l2 = lineSDF            (  uv,  uv-ri8.xy,  uv-ri3.xy  );
    float l3 = lineSDF            (  uv,  uv-ri7.xy,  uv-ri3.xy  );
    float l4 = lineSDF            (  uv,  uv-ri5.xy,  uv-ri7.xy  );
    float l5 = lineSDF            (  uv,  uv-ri9.xy,  uv-ri8.xy  );
    float l6 = lineSDF            (  uv,  uv-ri9.xy,  uv-ri10.xy );
    float l7 = lineSDF            (  uv,  uv-ri10.xy, uv-ri8.xy  );
    
    float lCom = min(min(min(min(min(min(l1, l2), l3), l4), l5), l6), l7);



    float p = min(lCom, riCom);
    
    vec3 col = vec3(p);
    fragColor = vec4(col,1.0);
}
```

## Algorithm

### Core adaptation from reference

**Keep verbatim:**
- `cutoff()` SDF threshold function (smoothstep antialiasing)
- `ringSDF()` recursive coordinate chaining pattern: returns `vec4(localUV, sdf, radius)` so children orbit in parent's local space
- `lineSDF()` line segment distance function
- The key rule: `rotRadius = parent.radius` (children orbit ON parent's circumference)

**Replace:**
- Hard-coded 10-ring tree -> procedural loop over `depth` levels and `branches` per level
- Fixed radii/speeds -> computed from scaling rules with per-ring variation
- Monochrome output -> gradient LUT sampling by depth-normalized index `t`
- Static brightness -> FFT-driven per-ring brightness using same index `t`
- Hand-picked line pairs -> selectable line mode (all leaves / siblings / nearest)
- `iTime` -> CPU-accumulated `time` uniform (speed accumulation on CPU)

### Procedural tree construction

The shader builds the tree in a nested loop. Total ring count = 1 + branches + branches^2 + ... up to `depth` levels.

For each ring, compute:
- **Depth-level base radius**: `rootRadius * pow(radiusDecay, level)` where `radiusDecay` is the per-level shrink factor
- **Per-ring radius**: base radius + deterministic variation from `radiusVariation` parameter, using a hash of the ring's flat index
- **Depth-level base speed**: `baseSpeed * pow(speedScale, level)`
- **Per-ring speed**: base speed + deterministic variation from `speedVariation`, same hash
- **Rotation offset**: `2.0 * PI * siblingIndex / branches` to evenly space siblings around parent

Each ring calls the same recursive coordinate chain as the reference: subtract parent's orbital offset to get local UV, compute ring SDF in local space, pass local UV down to children.

### Ring storage

Since the shader must evaluate every pixel against every ring, rings are stored in arrays:
- `vec2 ringPos[MAX_RINGS]` - world-space center of each ring (for line drawing)
- `float ringSDF[MAX_RINGS]` - distance field value at current pixel
- `float ringT[MAX_RINGS]` - depth-normalized index for color/FFT lookup
- `int leafIndices[MAX_LEAVES]` - indices of deepest-level rings (for line connections)

`MAX_RINGS` capped at a reasonable compile-time constant (e.g., 40) to bound the loop. Actual ring count is `ringCount` uniform.

### Line modes

- **All Leaves**: iterate all leaf pairs, compute lineSDF between each pair
- **Siblings**: iterate leaves, connect only pairs sharing a parent (same branch at level N-1)

### Color and FFT

Ring index `t` (0 at root, 1 at deepest leaves) drives both:
- Color: `texture(gradientLUT, vec2(t, 0.5)).rgb`
- FFT: `baseFreq * pow(maxFreq / baseFreq, t)` -> sample `fftTexture` -> brightness

Lines inherit the average `t` of their two endpoint rings.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| branches | int | 2-6 | 3 | Children per ring at each depth level |
| depth | int | 2-4 | 3 | Number of hierarchy levels (root = level 0) |
| rootRadius | float | 0.1-0.8 | 0.5 | Radius of the outermost root ring |
| radiusDecay | float | 0.1-0.8 | 0.4 | Per-level radius shrink factor |
| radiusVariation | float | 0.0-1.0 | 0.0 | Random radius offset per ring within its level (0 = uniform) |
| baseSpeed | float | 0.1-3.0 | 0.8 | Orbital speed at level 1 (radians/sec, CPU-accumulated) |
| speedScale | float | 1.0-4.0 | 2.0 | Speed multiplier compounding per depth level |
| speedVariation | float | 0.0-1.0 | 0.0 | Random speed offset per ring within its level |
| strokeWidth | float | 0.001-0.02 | 0.005 | Base ring stroke thickness |
| strokeTaper | float | 0.0-1.0 | 0.5 | Depth-scaled thinning (0 = uniform, 1 = leaves are thinnest) |
| lineMode | int | 0-1 | 0 | 0 = All Leaves, 1 = Siblings |
| lineBrightness | float | 0.0-1.0 | 0.5 | Line opacity (0 = no lines) |
| baseFreq | float | 27.5-440.0 | 55.0 | Low end of FFT range (Hz) |
| maxFreq | float | 1000-16000 | 14000 | High end of FFT range (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT gain multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum ring brightness when audio is silent |

## Modulation Candidates

- **rootRadius**: breathes the entire structure in and out
- **radiusDecay**: shifts the balance between large outer rings and tiny inner ones
- **radiusVariation**: smoothly transitions between uniform and chaotic sibling sizes
- **baseSpeed**: global tempo control for all orbital motion
- **speedScale**: exaggerates or flattens the speed hierarchy
- **speedVariation**: drifts between synchronized and desynchronized motion
- **strokeWidth**: pulses line weight with the music
- **strokeTaper**: shifts visual emphasis between root and leaves
- **lineBrightness**: fades the connecting web in and out
- **branches**: (integer) jumps between sparse and dense tree topologies

### Interaction Patterns

- **Cascading threshold (baseBright x gain)**: at low baseBright, rings only appear when their frequency band has energy. Quiet passages show only the root (bass), busy passages light up the full tree. The web of lines follows since it inherits leaf brightness.
- **Competing forces (radiusDecay x strokeTaper)**: high decay makes leaves tiny but high taper makes them thin. Push both and leaves nearly vanish. Pull them apart (low decay, high taper) and you get large but gossamer leaves.
- **Resonance (speedScale x branches)**: at integer ratios of speedScale to branches, rings periodically align into symmetric rosette formations then scatter. Higher branch counts make these alignments rarer and more visually striking when they occur.

## Notes

- Total ring count grows as a geometric series: branches=3, depth=3 gives 1+3+9+27=40 rings. Cap at MAX_RINGS=40 and clamp depth accordingly to avoid exceeding the array.
- The `cutoff()` smoothstep gives clean antialiased edges at any stroke width. The `fade` constant (0.001) may need scaling relative to `strokeWidth` to avoid aliasing on very thin rings.
- CPU accumulates `time` from `baseSpeed * deltaTime`; the shader multiplies by per-ring speed factors. No speed accumulation in the shader.
- Per-ring deterministic variation uses a simple integer hash of the flat ring index: `fract(sin(float(index) * 127.1) * 43758.5453)`. Stable across frames, no seed needed.
