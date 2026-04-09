# Preset Crossfade

Frozen-snapshot dissolve between presets. When a playlist advance fires, the current final frame is captured into a crossfade texture. The pipeline hard-switches to the target preset immediately (single render). A trivial blend shader composites the decaying snapshot (fading out) with the live target (fading in) as the very last step before presenting to screen. One pipeline, one extra texture, one small shader.

## Classification

- **Category**: General / Architecture
- **Pipeline Position**: Post-gamma compositing (very end of `RenderPipelineApplyOutput`)

## References

- Internal: `src/render/render_pipeline.cpp` - `RenderPipelineApplyOutput()` final blit at line 329
- Internal: `src/automation/easing.h` - `EasingEvaluate()` with 7 curves
- Internal: `src/ui/imgui_playlist.cpp` - `ImGuiPlaylistAdvance()` trigger point
- Internal: `src/render/post_effect.h` - `PostEffect` struct owns render textures

## Algorithm

### Trigger sequence (on playlist advance)

1. **Capture snapshot**: copy the current final frame (post-gamma `pingPong` texture) into a dedicated `crossfadeTexture` render texture owned by `PostEffect`
2. **Load target preset normally**: call the existing `PresetToAppConfigs()` path - pipeline hard-switches to target
3. **Start crossfade**: set `crossfadeActive = true`, `crossfadeElapsed = 0`

### Per-frame compositing (while active)

1. The full pipeline runs normally for the target preset - feedback, drawables, output chain, gamma - producing the live frame in the final pingPong texture
2. Advance `crossfadeElapsed += deltaTime`
3. Compute `t = clamp(crossfadeElapsed / crossfadeDuration, 0, 1)`
4. Apply easing: `easedT = EasingEvaluate(t, crossfadeCurve)`
5. Instead of blitting the final texture directly to screen, run a crossfade blend shader:
   - Input A: `crossfadeTexture` (frozen source snapshot, fading out)
   - Input B: final pingPong texture (live target, fading in)
   - Output: `mix(A, B, easedT)` drawn to screen
6. When `t >= 1.0`: set `crossfadeActive = false`, crossfade texture sits idle until next trigger

### Crossfade during crossfade

If the user triggers another advance mid-crossfade:
1. Composite the current blended frame into `crossfadeTexture` (snapshot of mid-transition state)
2. Load the new target preset
3. Reset `crossfadeElapsed = 0`

This chains naturally - the snapshot now contains a ghost of the previous transition.

### Integration point

In `RenderPipelineApplyOutput()`, the final two lines are:

```
RenderPass(pe, src, &pe->pingPong[writeIdx], pe->gammaShader, SetupGamma);
RenderUtilsDrawFullscreenQuad(pe->pingPong[writeIdx].texture, pe->screenWidth, pe->screenHeight);
```

Replace the final `DrawFullscreenQuad` with:

```
if (pe->crossfadeActive) {
    // Blend shader: mix(snapshot, live, progress)
    BeginShaderMode(pe->crossfadeShader);
    // bind crossfadeTexture as texture1, set progress uniform
    SetupCrossfade(pe);
    RenderUtilsDrawFullscreenQuad(pe->pingPong[writeIdx].texture, ...);
    EndShaderMode();
} else {
    RenderUtilsDrawFullscreenQuad(pe->pingPong[writeIdx].texture, ...);
}
```

### Crossfade shader (trivial)

```glsl
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;  // live target (current frame)
uniform sampler2D texture1;  // frozen snapshot (source)
uniform float progress;      // 0 = all snapshot, 1 = all live
out vec4 finalColor;
void main() {
    vec4 live = texture(texture0, fragTexCoord);
    vec4 snap = texture(texture1, fragTexCoord);
    finalColor = mix(snap, live, progress);
}
```

### Snapshot capture

Capture happens once at crossfade start. Use a blit from the current final pingPong to `crossfadeTexture`:

```
BeginTextureMode(pe->crossfadeTexture);
DrawTextureRec(finalTex, {0, 0, w, -h}, {0, 0}, WHITE);
EndTextureMode();
```

The negative height handles raylib's flipped render texture convention (already used by `BlitTexture` in the pipeline).

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| duration | float | 0.5-10.0 | 2.0 | Crossfade time in seconds |
| curve | int (ModCurve) | 0-6 | 3 (EASE_IN_OUT) | Easing shape for blend progress |

These are global settings on the playlist, not per-effect config fields. They do not need modulation registration.

## Modulation Candidates

None - infrastructure feature. Duration and curve are UI settings.

## Notes

- **Frozen source**: the snapshot is a freeze-frame, not a living animation. At typical durations (1-3s) the decay is fast enough that staleness is invisible. Longer durations (5-10s) will show the frozen nature more, but the easing curve masks it.
- **GPU cost**: one extra fullscreen texture in VRAM (same size as existing pingPong textures), one extra fullscreen quad draw with a trivial shader during the transition. Effectively free.
- **Memory**: `crossfadeTexture` is allocated once at init alongside the other render textures in `PostEffect`. It persists but costs nothing when not in use.
- **Resize handling**: `crossfadeTexture` must be reallocated in the resize path alongside the pingPong textures.
- **Feedback continuity**: the target preset's feedback chain starts from whatever is in the accumulation texture at switch time. This means the target inherits a ghost of the source's visual state in its feedback buffer, which actually helps the transition feel organic rather than starting from black.
