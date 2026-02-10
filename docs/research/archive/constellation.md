# Constellation

Animated network of glowing points connected by fading lines. Points drift within grid cells via sinusoidal motion with radial wave overlay. Lines connect neighbors and fade by length, creating an organic web that pulses and breathes.

## Classification

- **Category**: GENERATORS (new category beneath SIMULATIONS)
- **Pipeline Position**: After feedback, before transforms. Writes additively to ping-pong texture.
- **Render Model**: Fragment shader, procedural generation on black background

## References

- Shadertoy "Plexus (universe within)" by BigWings - user-provided code
- https://www.youtube.com/watch?v=3CycKKJiwis - BigWings tutorial

## Algorithm

### Hash Functions

```glsl
float N21(vec2 p) {
    p = fract(p * vec2(233.34, 851.73));
    p += dot(p, p + 23.456);
    return fract(p.x * p.y);
}

vec2 N22(vec2 p) {
    float n = N21(p);
    return vec2(n, N21(p + n));
}
```

### Point Position

Each grid cell has one point. Position = cell center + animated offset:

```glsl
vec2 GetPos(vec2 cellID, vec2 cellOffset) {
    vec2 hash = N22(cellID + cellOffset);

    // Sinusoidal wander
    vec2 n = hash * (time * animSpeed);

    // Radial wave overlay
    float radial = sin(length(cellID + cellOffset) * radialFreq - time * radialSpeed) * radialAmp;

    // Final position: offset from cell center
    return cellOffset + sin(n + vec2(radial)) * wanderAmp;
}
```

### Line Distance

Signed distance to line segment for rendering:

```glsl
float DistLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * t);
}
```

### Line Rendering

Lines fade by distance from pixel AND by line length:

```glsl
vec4 Line(vec2 p, vec2 a, vec2 b, float lineLen) {
    float dist = DistLine(p, a, b);

    // Soft edge falloff
    float alpha = smoothstep(lineThickness, lineThickness * 0.2, dist);

    // Fade long lines
    alpha *= smoothstep(maxLineLen, maxLineLen * 0.5, lineLen);
    alpha *= lineOpacity;

    // Color by mode
    vec3 col;
    if (interpolateLineColor) {
        // Sample endpoint colors, interpolate by position along line
        vec3 colA = textureLod(pointLUT, vec2(N21(a), 0.5), 0.0).rgb;
        vec3 colB = textureLod(pointLUT, vec2(N21(b), 0.5), 0.0).rgb;
        float t = clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0, 1.0);
        col = mix(colA, colB, t);
    } else {
        // Color by line length
        float lutPos = lineLen / maxLineLen;
        col = textureLod(lineLUT, vec2(lutPos, 0.5), 0.0).rgb;
    }

    return vec4(col, alpha);
}
```

### Point Rendering

Inverse-squared glow:

```glsl
vec3 Point(vec2 p, vec2 pointPos, vec2 cellID) {
    vec2 delta = pointPos - p;
    float glow = 1.0 / dot(delta * glowScale, delta * glowScale);
    glow = clamp(glow, 0.0, 1.0);

    // Color by cell ID hash
    float lutPos = N21(cellID);
    vec3 col = textureLod(pointLUT, vec2(lutPos, 0.5), 0.0).rgb;

    return col * glow * pointBrightness;
}
```

### Layer Composition

For each pixel, iterate the 3x3 neighborhood of cells:

```glsl
vec3 Layer(vec2 uv) {
    vec3 result = vec3(0.0);

    vec2 cellCoord = uv * gridScale;
    vec2 gv = fract(cellCoord) - 0.5;  // Position within cell
    vec2 id = floor(cellCoord);         // Cell ID

    // Gather 9 neighbor positions
    vec2 points[9];
    int idx = 0;
    for (float y = -1.0; y <= 1.0; y++) {
        for (float x = -1.0; x <= 1.0; x++) {
            points[idx++] = GetPos(id, vec2(x, y));
        }
    }

    // Center point (index 4) connects to all neighbors
    for (int i = 0; i < 9; i++) {
        float lineLen = length(points[4] - points[i]);
        vec4 line = Line(gv, points[4], points[i], lineLen);
        result += line.rgb * line.a;
    }

    // Corner-to-corner edges
    result += Line(gv, points[1], points[3], length(points[1] - points[3])).rgb * ...;
    result += Line(gv, points[1], points[5], length(points[1] - points[5])).rgb * ...;
    result += Line(gv, points[7], points[3], length(points[7] - points[3])).rgb * ...;
    result += Line(gv, points[7], points[5], length(points[7] - points[5])).rgb * ...;

    // Render all 9 points
    for (int i = 0; i < 9; i++) {
        vec2 cellID = id + vec2(float(i % 3) - 1.0, float(i / 3) - 1.0);
        result += Point(gv, points[i], cellID);
    }

    return result;
}
```

## Parameters

### Modulatable

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gridScale | float | 5.0 - 50.0 | 21.0 | Point density (cells across screen) |
| animSpeed | float | 0.0 - 5.0 | 1.0 | Wander animation speed multiplier |
| wanderAmp | float | 0.0 - 0.5 | 0.4 | How far points drift from cell center |
| radialAmp | float | 0.0 - 2.0 | 1.0 | Strength of radial wave on positions |
| radialSpeed | float | 0.0 - 5.0 | 0.5 | Radial wave propagation speed |
| pointBrightness | float | 0.0 - 2.0 | 1.0 | Point glow intensity |
| maxLineLen | float | 0.5 - 2.0 | 1.5 | Lines longer than this fade out |
| lineOpacity | float | 0.0 - 1.0 | 0.5 | Overall line brightness |

### Configurable Only

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| lineThickness | float | 0.01 - 0.1 | 0.05 | Width of connection lines |
| pointLUT | ColorConfig | - | - | Color gradient for points |
| lineLUT | ColorConfig | - | - | Color gradient for lines |
| interpolateLineColor | bool | - | false | true: blend endpoint colors; false: sample LUT by length |

## Modulation Candidates

- **gridScale**: Density pulses - more/fewer points on beats
- **animSpeed**: Tempo sync - points move faster with energy
- **wanderAmp**: Breathing - points spread/contract with bass
- **radialAmp**: Ripple intensity reactive to transients
- **pointBrightness**: Flash on beats
- **lineOpacity**: Network fades in/out with energy

## Notes

- Fragment shader iterates 3x3 cell neighborhood per pixel. Performance scales with screen resolution, not point count.
- Line fade by length breaks rigid grid appearance. Lower `maxLineLen` = sparser network.
- Points use inverse-squared falloff which can cause HDR values. Clamp or rely on bloom to handle.
- Corner-to-corner edges (indices 1-3, 1-5, 7-3, 7-5) add diagonal connectivity beyond simple neighbor lines.
