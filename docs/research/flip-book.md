# Flip Book

Choppy stop-motion animation where the full image updates at a configurable reduced frame rate (e.g., 8-12 fps) while the app runs at full speed. Held frames get a slight random UV jitter each update tick, selling the hand-animated instability of pages being thumbed through. The hold rate is modulatable, so audio energy can snap fresh frames on beats while quiet passages freeze into near-slideshows.

## Classification

- **Category**: TRANSFORMS > Retro
- **Pipeline Position**: Output stage, among reorderable transforms
- **Badge**: `"RET"`, Section index 6

## References

- [3D Game Shaders for Beginners — Posterization](https://lettier.github.io/3d-game-shaders-for-beginners/posterization.html) - Time quantization pattern (`floor(time * rate) / rate`)
- [Ping-Pong Technique (Medium)](https://olha-stefanishyna.medium.com/stateful-rendering-with-ping-pong-technique-6c6ef3f5091a) - Render texture frame-hold pattern
- [PS1 Jitter Shader (Codrops)](https://tympanus.net/codrops/2024/09/03/how-to-create-a-ps1-inspired-jitter-shader-with-react-three-fiber/) - Grid-snapped vertex jitter as wobble inspiration

## Algorithm

No external reference code — the technique combines three well-known GPU patterns:

### 1. CPU-side frame timer (Setup function)

Accumulate `frameTimer += deltaTime`. When `frameTimer >= 1.0 / fps`, reset the timer and increment an integer `frameIndex`. Pass `frameIndex` to the shader as the jitter seed. The `fps` parameter is modulatable (range 2–60).

### 2. Conditional render texture update (Custom render callback)

The effect owns one `RenderTexture2D heldFrame`. Each frame:
- If `frameIndex` changed since last render: blit the current scene texture into `heldFrame` (simple fullscreen quad draw, no shader)
- Always: render `heldFrame` through the jitter shader to `pe->currentRenderDest`

This follows the single-intermediate-texture pattern (like oil paint) but with a custom render callback (like slit scan corridor) for the conditional update logic.

### 3. Hash-based UV jitter (Fragment shader)

```glsl
uniform sampler2D texture0;   // held frame
uniform vec2 resolution;
uniform float jitterSeed;     // float(frameIndex)
uniform float jitterAmount;   // 0.0–1.0, pixels of offset

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
    vec2 jitter = vec2(
        fract(sin(jitterSeed * 12.9898) * 43758.5453),
        fract(sin(jitterSeed * 78.233 + 0.5) * 43758.5453)
    ) * 2.0 - 1.0;

    vec2 uv = fragTexCoord + jitter * jitterAmount / resolution;
    finalColor = texture(texture0, uv);
}
```

The jitter vector changes only when `frameIndex` increments (i.e., when a new frame is captured), so held frames wobble to a new position each update tick but stay stable between ticks.

### Adaptation notes

- Allocate `heldFrame` via `RenderUtilsInitTextureHDR` (float32, universal convention)
- Manual `EffectDescriptorRegister()` because Init takes `(Effect*, int, int)` and the effect has a custom render callback
- `EFFECT_FLAG_NEEDS_RESIZE` — Resize unloads and re-inits the held texture
- Blit uses `RenderUtilsDrawFullscreenQuad` with no shader active (raylib default passthrough)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| fps | float | 2.0–60.0 | 12.0 | Target frame rate — lower = choppier |
| jitter | float | 0.0–8.0 | 2.0 | Wobble intensity in pixels per held frame |

## Modulation Candidates

- **fps**: Controls choppiness. Low values (2–4) create dramatic slideshow; high values (24+) approach smooth motion. Primary audio target.
- **jitter**: Controls wobble magnitude. Subtle at low values, chaotic at high values.

### Interaction Patterns

**Cascading threshold (fps × jitter):** At very low fps (2–4), even small jitter values feel dramatic because each wobble position persists for 250–500 ms. At high fps (24+), jitter must be stronger to register since positions change rapidly. Modulating fps with bass energy while jitter rides a slower LFO creates a dynamic where loud sections snap crisp frames while quiet sections drift and wobble.

## Notes

- When `fps >= 60` (or whatever the display rate), the effect becomes a no-op pass-through — every frame updates the held texture immediately
- The held texture is never blended or accumulated — it's a pure sample-and-hold, so there's no ghosting or persistence
- Cost is minimal: one extra fullscreen blit on update ticks, one fullscreen shader pass every frame, one render texture in memory
- `frameTimer` must handle variable `deltaTime` gracefully — if a single frame exceeds `1/fps`, multiple ticks may be skipped; just clamp to one update per render frame
