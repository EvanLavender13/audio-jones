# Matrix Rain

Falling columns of procedural glyphs with bright leading characters and fading green trails — the iconic "digital rain" from The Matrix. Overlays on existing content, with optional texture color sampling so characters take on the colors of whatever is beneath.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Grid division → procedural character rendering → per-column falling brightness mask → overlay compositing
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [Shader Studies: Matrix Effect](https://shahriyarshahrabi.medium.com/shader-studies-matrix-effect-3d2ead3a84c5) - Breakdown of text+rain architecture, grid cell math, character randomization
- [Shadertoy: Digital Rain (ldccW4)](https://www.shadertoy.com/view/ldccW4) - Original Will Kirby implementation using font texture
- [Rezmason/matrix (GitHub)](https://github.com/Rezmason/matrix) - MSDF glyph rendering, stationary grid with illumination wave, configurable palettes
- [3D Matrix Rain (Andrew Hung)](https://andrewhungblog.wordpress.com/2018/08/29/procedural-graphics-series-3d-matrix-rain/) - Parallax perspective variant
- User-provided Shadertoy snippets (4 implementations covering font-based, hash-based, rune-based, and cell-based approaches)

## Algorithm

### Grid Division

Divide screen into fixed-size cells. Each cell holds one character:

```glsl
vec2 cellSize = vec2(charWidth, charHeight) / resolution;
vec2 cellUV = floor(uv / cellSize);       // integer cell ID (column, row)
vec2 localUV = fract(uv / cellSize);      // 0-1 coordinate within cell
```

### Procedural Character Rendering (Rune Approach)

Generate pseudo-random glyphs using line segments. Each character draws 4 strokes snapped to a 2x3 grid:

```glsl
float rune_line(vec2 p, vec2 a, vec2 b) {
    p -= a; b -= a;
    float h = clamp(dot(p, b) / dot(b, b), 0.0, 1.0);
    return length(p - b * h);
}

float rune(vec2 uv, vec2 seed, float highlight) {
    float d = 1e5;
    for (int i = 0; i < 4; i++) {
        vec4 pos = hash4(seed);
        seed += 1.0;
        // Force each stroke to touch a different box edge
        if (i == 0) pos.y = 0.0;
        if (i == 1) pos.x = 0.999;
        if (i == 2) pos.x = 0.0;
        if (i == 3) pos.y = 0.999;
        // Snap endpoints to 2x3 grid
        vec4 snaps = vec4(2, 3, 2, 3);
        pos = (floor(pos * snaps) + 0.5) / snaps;
        if (pos.xy != pos.zw)
            d = min(d, rune_line(uv, pos.xy, pos.zw + 0.001));
    }
    return smoothstep(0.1, 0.0, d) + highlight * smoothstep(0.4, 0.0, d);
}
```

### Alternative: Hash-Based Binary Characters

Simpler approach using random thresholding on a sub-grid within each cell:

```glsl
float rchar(vec2 outer, vec2 inner, float time) {
    vec2 seed = floor(inner * 4.0) + outer.y;
    // Some columns refresh characters over time
    if (hash(vec2(outer.y, 23.0)) > 0.98) {
        seed += floor((time + hash(vec2(outer.y, 49.0))) * 3.0);
    }
    return float(hash(seed) > 0.5);
}
```

### Rain Trail Mask

Per-column falling brightness creates the "lead bright → trail dark" gradient:

```glsl
vec3 rain(vec2 fragCoord, vec2 resolution, float time) {
    float col = floor(fragCoord.x / charWidth);
    float offset = hash(vec2(col, 0.0)) * 100.0;   // stagger start positions
    float speed = hash(vec2(col, 1.0)) * 0.5 + 0.5; // vary speed per column

    float y = fract(fragCoord.y / resolution.y + time * speed + offset);
    // Bright at leading edge (y near 0), dark at tail (y near 1)
    return vec3(0.1, 1.0, 0.35) / (y * 20.0);
}
```

Multiple "fallers" (rain drops) per column at different speeds create overlapping trails:

```glsl
float brightness = 0.0;
for (int i = 0; i < numFallers; i++) {
    float fSpeed = hash(vec2(col, float(i))) * 0.7 + 0.3;
    float fOffset = hash(vec2(col, float(i) + 100.0)) * trailHeight;
    float f = 3.0 - row * 0.05 - mod((time + float(i) * 3534.34) * fSpeed, trailHeight);
    if (f > 0.0 && f < 1.0) brightness += f;
}
```

### Character Animation (Refresh)

Characters change periodically. Lead character (tip of trail) refreshes fast; tail characters change slowly:

```glsl
float timeFactor;
if (charIndex == 0) {
    timeFactor = floor(time * 5.0);  // lead character flickers rapidly
} else {
    float baseRate = hash(vec2(col, 2.0));
    float charRate = pow(hash(vec2(charIndex, col)), 4.0);
    timeFactor = floor(time * (baseRate + charRate * 4.0));
}
// Use timeFactor as part of character seed
```

### Tail Fade

Characters near the end of a strip fade out:

```glsl
float alpha = charMask * clamp((stripLength - 0.5 - charIndex) / 2.0, 0.0, 1.0);
```

### Color Modes

**Classic green** — fixed gradient from bright white-green (lead) to dark green (tail):

```glsl
vec3 color;
if (charIndex == 0) {
    color = vec3(0.67, 1.0, 0.82);  // bright lead
} else {
    float t = float(charIndex) / stripLength;
    color = mix(vec3(0.0, 1.0, 0.0), vec3(0.0, 0.2, 0.0), t);  // green gradient
}
```

**Texture-sampled color** — sample underlying image, apply as tint to character mask:

```glsl
vec3 baseColor = texture(texture0, fragTexCoord).rgb;
vec3 result = baseColor * charMask * trailBrightness;
```

### Overlay Compositing

Blend rain characters over the underlying image:

```glsl
vec3 original = texture(texture0, fragTexCoord).rgb;
vec3 rainColor = charMask * trailBrightness * selectedColor;
float rainAlpha = charMask * trailBrightness * overlayIntensity;
vec3 result = mix(original, rainColor, rainAlpha);
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| cellSize | float | 4.0–32.0 | Character grid cell size in pixels |
| rainSpeed | float | 0.1–5.0 | Base falling speed of rain columns |
| trailLength | float | 5.0–40.0 | Number of characters in each rain strip |
| fallerCount | int | 1–20 | Number of independent rain drops per column |
| charStyle | int | 0–1 | 0 = rune strokes, 1 = binary hash blocks |
| colorMode | int | 0–1 | 0 = classic green, 1 = sample texture |
| overlayIntensity | float | 0.0–1.0 | How opaque the rain overlay is (0 = invisible, 1 = fully covers) |
| refreshRate | float | 0.1–5.0 | How often characters change (higher = more flicker) |
| leadBrightness | float | 0.5–3.0 | Brightness boost on leading character |

## Modulation Candidates

- **rainSpeed**: Faster rain during loud passages creates urgency; slow rain during quiet creates ambiance
- **overlayIntensity**: Modulating opacity makes rain appear/disappear with audio energy
- **trailLength**: Longer trails during sustained notes, shorter during staccato
- **leadBrightness**: Pulse lead character brightness with beat detection
- **refreshRate**: Faster character flicker during high-frequency content

## Notes

- The rune approach (4 line segments per character) produces Matrix-authentic glyphs without a font texture. More expensive than hash blocks but looks correct.
- Hash-based binary characters are cheaper but read as "random dots" rather than recognizable symbols.
- Multiple fallers per column prevent the "single raindrop" look and create the dense, overlapping trails from the films.
- The "texture color" mode multiplies the character mask by the underlying image color, creating a Matrix-tinted view of existing content.
- Cell size directly trades character detail vs performance — larger cells render faster but look blockier.
- The overlay compositing (mix with alpha) preserves underlying content between characters, distinguishing this from ASCII Art which fully replaces the image.
