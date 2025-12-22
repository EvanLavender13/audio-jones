# Waveform Trailmap Injection

Experimental feature: draw the waveform directly onto the physarum trailMap so agents sense and react to waveform geometry through the trail system rather than (or in addition to) the accumTexture.

## Current State

- `src/render/physarum.cpp:353-393` - `PhysarumProcessTrails()` runs blur/decay on trailMap
- `src/render/physarum.h:39` - `trailMap` is a `RenderTexture2D` (RGBA32F)
- `src/render/waveform.cpp:232-285` - `DrawWaveformCircular()` draws using raylib line primitives
- `src/render/post_effect.cpp:184-202` - `ApplyPhysarumPass()` coordinates physarum updates
- `src/main.cpp:109-123` - `RenderWaveforms()` draws to accumTexture

The waveform is currently drawn to `accumTexture` only. Physarum agents can sense it via `accumSenseBlend`, but the waveform never enters the trailMap directly.

## Phase 1: Enable Waveform Drawing to TrailMap

**Goal**: Draw waveform geometry onto `trailMap` using existing drawing functions.

**Build**:

1. **Add config toggle** (`src/render/physarum.h`):
   - Add `bool waveformToTrailmap = false` to `PhysarumConfig`

2. **Add trailMap draw access** (`src/render/physarum.h` and `src/render/physarum.cpp`):
   - Add `PhysarumBeginTrailMapDraw(Physarum* p)` - calls `BeginTextureMode(p->trailMap)`
   - Add `PhysarumEndTrailMapDraw(Physarum* p)` - calls `EndTextureMode()`
   - Guard both with null/enabled checks

3. **Add UI toggle** (`src/ui/ui.cpp`):
   - Add checkbox in physarum panel: "Waveform to Trail"

4. **Wire up drawing** (`src/main.cpp` in `RenderWaveforms()`):
   - After `PostEffectBeginAccum()` returns and before drawing waveforms to accumTexture
   - If physarum enabled AND `waveformToTrailmap` enabled:
     - Call `PhysarumBeginTrailMapDraw()`
     - Call `WaveformPipelineDraw()` with same parameters
     - Call `PhysarumEndTrailMapDraw()`
   - Then continue with normal accumTexture drawing

5. **Preset serialization** (`src/config/preset.cpp`):
   - Add `waveformToTrailmap` to physarum config read/write

**Done when**: Enabling the toggle causes waveform geometry to appear on the trailMap (visible via debug overlay), and physarum agents react to it through the trail sensing system.
