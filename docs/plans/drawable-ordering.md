# Drawable Ordering

Enforce MilkDrop's fixed draw order so shapes render before waveforms/spectrum, allowing textured shapes to sample accumulated waveform trails from the previous frame.

## Current State

- `src/render/drawable.cpp:142-191` — `DrawableRenderFull()` iterates drawables in user-defined list order
- `src/config/drawable_config.h:7` — `DrawableType` enum defines `DRAWABLE_WAVEFORM`, `DRAWABLE_SPECTRUM`, `DRAWABLE_SHAPE`
- `docs/research/milkdrop-drawable-ordering.md` — Research doc explaining the feedback problem and MilkDrop's solution

## Design

Two-pass rendering in `DrawableRenderFull()`:

1. **Pass 1**: Render all `DRAWABLE_SHAPE` (textured or solid), preserving relative list order
2. **Pass 2**: Render all `DRAWABLE_WAVEFORM` and `DRAWABLE_SPECTRUM`, preserving relative list order

This matches MilkDrop: shapes draw first (sampling previous frame's accumulated content), waveforms draw second (visible on top, feeding back to next frame).

No UI toggle — always enforced silently.

---

## Phase 1: Implement two-pass rendering

**Goal**: Shapes render before waveforms/spectrum in `DrawableRenderFull()`.

**Depends on**: —

**Files**: `src/render/drawable.cpp`

**Build**:
- Extract the per-drawable rendering logic (lines 152-189) into a static helper function
- Replace the single loop with two passes:
  - First pass: skip non-shapes, render shapes only
  - Second pass: skip shapes, render waveforms/spectrum only
- Both passes maintain the same waveformIndex/spectrumIndex tracking by doing a full iteration each time, only drawing when the type matches

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe`
- Create a textured shape and a waveform
- Drag the shape to appear "after" the waveform in the drawable list
- Observe waveform trails persist through the shape's texture (previously they would be overwritten)

**Done when**: Waveform trails accumulate correctly regardless of drawable list order.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1 | — |
