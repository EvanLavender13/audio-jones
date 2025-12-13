# Fractal Feedback

Route accumulated texture through scale/rotation transform to create recursive zoom effect. Initial implementation uses hardcoded small-scale parameters.

## Goal

Add a feedback pass that samples the previous frame with slight inward zoom and rotation. This creates the classic MilkDrop-style infinite tunnel effect where visuals appear to recede into the screen center while slowly spinning. The effect compounds with existing trail/blur system to produce organic, evolving patterns.

Reference: [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html)

## Current State

The post-effect pipeline performs horizontal blur, vertical blur with decay, then chromatic aberration:

- `src/render/post_effect.cpp:110-132` runs the blur passes between `accumTexture` and `tempTexture`
- `src/render/post_effect.cpp:140-158` applies optional chromatic aberration when drawing to screen
- `shaders/blur_v.fs:38-48` handles exponential decay (trail evaporation)
- Two render textures exist: `accumTexture` and `tempTexture` for ping-pong blur

The feedback effect inserts between blur completion and new waveform rendering.

## Algorithm

Apply 2D transformation to UV coordinates when sampling the accumulated texture:

```glsl
// feedback.fs - sample previous frame with zoom + rotation
vec2 center = vec2(0.5);
vec2 uv = fragTexCoord - center;

// Rotate around center
float s = sin(rotation);
float c = cos(rotation);
uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

// Scale toward center (zoom < 1.0 = zoom in)
uv *= zoom;

uv += center;

// Sample with clamping to avoid edge artifacts
vec4 feedback = texture(texture0, clamp(uv, 0.0, 1.0));
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Zoom | 0.98 | 2% inward motion per frame; visible but not overwhelming |
| Rotation | 0.005 rad (~0.3°) | Slow spin; completes full rotation in ~20 seconds at 60fps |
| Blend | 1.0 (replace) | Full replacement in feedback pass; decay handles fading |

The feedback runs once per frame before waveforms render. Since decay already darkens pixels each frame, no additional blend/fade is needed in the feedback shader itself.

## Integration

Insert feedback pass at start of `PostEffectBeginAccum()`:

```
Current pipeline:
  accumTexture → blur_h → tempTexture → blur_v → accumTexture → [draw waveforms]

New pipeline:
  accumTexture → feedback → tempTexture → blur_h → accumTexture → blur_v → tempTexture
                                                                              ↓
                                                          copy back to accumTexture
                                                                              ↓
                                                                    [draw waveforms]
```

Simplification: reuse existing two-texture ping-pong. Feedback writes to `tempTexture`, then blur_h reads from `tempTexture` (instead of `accumTexture`), blur_v writes to `accumTexture`. Pipeline stays at two textures.

The effect runs unconditionally in this initial version. Future work will add UI toggle and parameter controls.

## File Changes

| File | Change |
|------|--------|
| `shaders/feedback.fs` | Create - UV transform shader with hardcoded zoom/rotation |
| `src/render/post_effect.h` | Add `Shader feedbackShader` and uniform locations |
| `src/render/post_effect.cpp` | Load feedback shader, insert pass before blur |

## Implementation Details

### shaders/feedback.fs

```glsl
#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    const float zoom = 0.98;
    const float rotation = 0.005;

    vec2 center = vec2(0.5);
    vec2 uv = fragTexCoord - center;

    float s = sin(rotation);
    float c = cos(rotation);
    uv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
    uv *= zoom;
    uv += center;

    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

### PostEffect struct additions

```c
Shader feedbackShader;
// No uniform locations needed for hardcoded version
```

### PostEffectBeginAccum modification

Insert before horizontal blur (line 110):

```c
// Feedback pass (accumTexture → tempTexture with zoom/rotation)
BeginTextureMode(pe->tempTexture);
BeginShaderMode(pe->feedbackShader);
    DrawTextureRec(pe->accumTexture.texture,
        {0, 0, (float)pe->screenWidth, (float)-pe->screenHeight},
        {0, 0}, WHITE);
EndShaderMode();
EndTextureMode();

// Horizontal blur now reads from tempTexture (output of feedback)
BeginTextureMode(pe->accumTexture);  // Changed destination
BeginShaderMode(pe->blurHShader);
    // ... sample from tempTexture instead of accumTexture
```

Adjust blur pipeline to account for changed input/output textures.

## Validation

- [ ] Waveforms slowly recede toward screen center
- [ ] Rotation visible as gradual clockwise spin
- [ ] No visible seams or artifacts at screen edges
- [ ] Trail decay still functions (old content fades)
- [ ] Performance within 1ms of current frame time at 1080p
- [ ] Builds without warnings

## Future Work

- Add `feedbackEnabled` toggle to `EffectConfig`
- Expose `feedbackZoom` and `feedbackRotation` as uniforms
- UI controls in effects panel
- Audio-reactive modulation (zoom/rotation speed from beat intensity)
