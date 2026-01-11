# GPU Profiler

CPU-based profiler misattributes GPU execution time. Compute dispatches return immediately; actual work runs asynchronously. Current zones show ~0.01ms while real GPU time is 15-17ms.

## Problem

```
CPU:  [Submit 0.01ms].............[Wait at SwapBuffers ~17ms]
GPU:                 [====Actual Work 17ms====]
```

Profiler closes zones after CPU submit, not GPU completion. Wait time appears as untracked overhead at frame end.

## Solution

Replace `GetTime()` CPU timing with OpenGL GPU timestamp queries (`GL_TIME_ELAPSED` or `glQueryCounter`). These measure actual GPU execution time per zone.

## Scope

- Add GPU query pool (need 2+ queries per zone for double-buffering)
- Query results lag one frame (GPU hasn't finished current frame yet)
- ~50-100 lines in profiler.h/.cpp
