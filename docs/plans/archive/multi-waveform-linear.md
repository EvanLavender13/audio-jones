# Multiple Waveforms in Linear Mode

Enable all configured waveforms in linear mode (currently only first renders) and add vertical offset for stacking.

## Status: Complete (Phases 1-2)

Phase 3 (palindrome scrolling) was implemented and reverted - the palindrome mirror point at sample 1024 creates a visible slope reversal artifact when scrolling. Linear mode retains color scrolling animation instead.

---

## Phase 1: Enable Multi-Waveform Linear Rendering ✓

**Goal**: All configured waveforms render in linear mode, overlapped at center.

**Changes**:
- `src/render/waveform_pipeline.cpp:65-68` - Loop through all waveforms matching circular mode structure

---

## Phase 2: Add Vertical Offset Parameter ✓

**Goal**: Each waveform can be positioned above or below center line.

**Changes**:
- `src/config/waveform_config.h:12` - Added `verticalOffset` field (default 0.0, range -0.5 to 0.5)
- `src/render/waveform.cpp:202,219-220,230` - Applied `verticalOffset * ctx->screenH` to Y-coordinates
- `src/ui/imgui_waveforms.cpp:89` - Added "Y-Offset" slider in Geometry section

---

## Phase 3: Palindrome Scrolling ✗ (Reverted)

**Goal**: Rotation parameter scrolls waveform data through palindrome buffer.

**Outcome**: Implemented but reverted. The palindrome works for circular mode where the visual loop is natural, but for linear scrolling the mirror point creates a visible "fold" artifact. The values are continuous at the join, but the derivative (slope) flips sign abruptly.

**Alternatives considered**:
- Direct wrap (no palindrome): Has value discontinuity (jump) - worse than slope discontinuity
- Crossfade at join: Blurs waveform features near the seam, still visible

**Decision**: Keep color scrolling for linear mode. Rotation animates colors across the waveform rather than scrolling the data.

---

## Notes

**Parameter semantics**: `radius` controls circular positioning, `verticalOffset` controls linear positioning. Both always visible in UI, each used only by its respective mode.

**Preset compatibility**: New `verticalOffset` field defaults to 0.0, existing presets load without visual change.
