# Gradient Palindrome Fix

Fix gradient color mode to form a palindrome (start→end→start) matching rainbow mode behavior, eliminating harsh color transitions at waveform/spectrum wrap points.

## Current State

- `src/render/waveform.cpp:17-32` - `GetSegmentColor()` handles color modes for waveform
- `src/render/spectrum_bars.cpp:116-131` - `GetBandColor()` handles color modes for spectrum bars
- Both functions apply palindrome transformation for rainbow mode but pass `t` directly for gradient mode

## Phase 1: Apply Palindrome to Gradient Mode

**Goal**: Make gradient colors mirror like rainbow mode does.

**Modify**:
- `src/render/waveform.cpp` - In `GetSegmentColor()`, transform `t` before passing to `GradientEvaluate()`:
  ```c
  // Before: GradientEvaluate(..., t)
  // After:  GradientEvaluate(..., 1.0f - fabsf(2.0f * t - 1.0f))
  ```
- `src/render/spectrum_bars.cpp` - Same transformation in `GetBandColor()`

**Done when**: Gradient colors form smooth palindrome in both circular and linear modes for waveform and spectrum bars.
