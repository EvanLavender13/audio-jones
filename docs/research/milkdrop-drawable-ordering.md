# MilkDrop Drawable Ordering and Feedback

## Problem Statement

In AudioJones, textured shapes cover whatever content lies beneath them in the drawable list. When a waveform is positioned "in front" of a shape (drawn before it), the shape draws on top and replaces the waveform's pixels in the feedback buffer. The waveform cannot accumulate trails because its pixels are overwritten each frame.

## MilkDrop's Solution

MilkDrop enforces a **fixed draw order** rather than allowing arbitrary ordering:

```
1. Warp shader (samples previous frame with UV displacement + decay)
2. Motion vectors
3. Custom shapes (up to 4/16, textured or solid)
4. Custom waveforms (up to 4/16)
5. Basic waveform (modes 0-7)
6. Darken center
7. Borders
   ↓
Result becomes prevTexture for next frame
```

Key insight: **shapes draw BEFORE waveforms**. Always. This is not configurable.

### Why This Works

1. **Warp processes the previous frame** - which contains previously accumulated shapes AND waveforms
2. **Shapes draw first**, sampling the warped result (old content with trails)
3. **Waveforms draw second**, on top of shapes
4. **Everything feeds back** - shapes and waveforms both contribute to the next frame

Shapes don't need the current frame's waveform to be already accumulated. They sample the *previous* frame's accumulated content. The current waveform draws afterward and will appear in the *next* frame's shape sampling.

### What Shapes Sample

From the preset authoring guide: "you can map the previous frame's image onto the shape"

The "previous frame's image" is the output of the warp shader—the warped, decayed content including all shapes and waveforms from prior frames. Shapes sample downstream in time (previous frame), not downstream in the current frame's render order.

## AudioJones Current Behavior

```
1. Simulations (physarum, curl flow, etc.)
2. Feedback (warp, blur, decay) on accumTexture
3. Drawables → all drawn to accumTexture in user-defined list order
4. Output chain (transforms, gamma)
```

Shapes sample from `outputTexture`, which contains the previous frame's post-transform result. This matches MilkDrop.

The problem: drawable list order is arbitrary. If a shape is later in the list (visually "in front"), it draws over earlier drawables and replaces their pixels in the feedback buffer.

## Solution Approaches

### Option A: Enforce MilkDrop's Draw Order (Recommended)

Render drawables in fixed groups rather than a flat list:

```
1. Feedback (warp, blur, decay)
2. Shapes (textured or solid) - all shapes, in list order among themselves
3. Waveforms and spectrum - all waveforms, in list order among themselves
```

**Pros:**
- Matches MilkDrop exactly
- No special buffers or extra GPU passes
- Predictable behavior
- Shapes always sample accumulated content; waveforms always feed back

**Cons:**
- Loses user control over shape-vs-waveform layering
- Shapes can never appear "in front of" waveforms

### Option B: Multi-Layer Feedback

Maintain separate feedback paths for shapes and waveforms:

1. **Waveform layer**: draws and accumulates independently
2. **Shape layer**: samples waveform layer, draws separately
3. **Composite**: combine layers for final output

**Pros:**
- Preserves arbitrary ordering
- Each layer accumulates independently

**Cons:**
- Extra render targets and GPU bandwidth
- More complex pipeline
- Deviates from MilkDrop's proven model

### Option C: Pre/Post Shape Phases

Split drawable rendering into two phases:

```
Phase 1 (before shapes): All drawables marked "in front of shapes"
→ Feed back normally

Phase 2: Shapes sample the accumulated Phase 1 result

Phase 3 (after shapes): All drawables marked "behind shapes"
→ Draw on top, feed back
```

**Pros:**
- User controls which drawables appear in/behind shapes
- Single feedback buffer

**Cons:**
- UI complexity (drawable layering relative to shapes)
- Still constrains ordering compared to a flat list

## Recommendation

**Option A matches MilkDrop's design and solves the problem without complexity.**

The limitation (shapes cannot appear in front of waveforms) is acceptable because:
1. MilkDrop presets work this way and achieve excellent results
2. Shapes sampling accumulated waveform trails IS the primary use case
3. If a shape genuinely needs to cover waveforms, make it solid (non-textured)

Implementation: sort drawables by type before rendering (shapes first, then waveforms/spectrum), preserving relative order within each type.

## References

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html)
- [Butterchurn Renderer](https://github.com/jberg/butterchurn) - WebGL MilkDrop implementation
- `docs/research/milkdrop-feedback-architecture.md` - Existing architecture research
