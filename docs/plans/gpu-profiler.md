# GPU Profiler

Replace CPU-based profiler timing with OpenGL GPU timestamp queries. Current profiler misattributes GPU execution time: compute dispatches return immediately (~0.01ms CPU) while actual GPU work takes 15-17ms. The wait appears as untracked overhead at frame end.

## Current State

- `src/render/profiler.h:10-32` - ProfileZone struct with `startTime`, `lastMs`, `history[]`
- `src/render/profiler.cpp:47-64` - Begin/EndZone using `GetTime()` for CPU timestamps
- `src/render/render_pipeline.cpp:174-226` - Zone bracketing around pipeline stages
- `src/ui/imgui_analysis.cpp:158-400` - Flame chart, sparklines, budget bar (no changes needed)
- `external/glad.h` provides `glGenQueries`, `glBeginQuery`, `glEndQuery`, `glGetQueryObjectui64v`, `GL_TIME_ELAPSED`

## Technical Approach

**GL_TIME_ELAPSED queries**: Wrap zone execution with `glBeginQuery(GL_TIME_ELAPSED, query)` / `glEndQuery(GL_TIME_ELAPSED)`. GPU reports elapsed nanoseconds when query completes.

**Double buffering**: Results lag one frame (GPU hasn't finished current frame when we'd want to read). Maintain 2 query IDs per zone. Frame N writes to buffer[N % 2], reads from buffer[(N + 1) % 2].

**Workflow per frame**:
1. `ProfilerFrameBegin`: Read previous frame's completed queries → update `lastMs`/`history`
2. `ProfilerBeginZone`: `glBeginQuery(GL_TIME_ELAPSED, queries[zone][writeIdx])`
3. `ProfilerEndZone`: `glEndQuery(GL_TIME_ELAPSED)`
4. `ProfilerFrameEnd`: Flip writeIdx, advance history index

## Phase 1: Add GPU Query Infrastructure

**Goal**: Allocate query objects and add buffer index tracking.

**Modify `src/render/profiler.h`**:
- Add `#include "external/glad.h"`
- Add to Profiler struct:
  - `GLuint queries[ZONE_COUNT][2]` - double-buffered query IDs
  - `int writeIdx` - current write buffer (0 or 1)
- Remove `startTime` from ProfileZone (no longer needed)

**Modify `src/render/profiler.cpp`**:
- `ProfilerInit`: Call `glGenQueries(ZONE_COUNT * 2, &profiler->queries[0][0])`. Initialize `writeIdx = 0`. Run a dummy query on each object to avoid first-frame read errors.
- `ProfilerUninit`: Call `glDeleteQueries(ZONE_COUNT * 2, &profiler->queries[0][0])`

**Done when**: Profiler compiles, queries allocated at init, freed at uninit.

---

## Phase 2: Implement GPU Timing

**Goal**: Replace CPU timing with GPU queries.

**Modify `src/render/profiler.cpp`**:

- `ProfilerBeginZone`:
  ```
  GLuint query = profiler->queries[zone][profiler->writeIdx];
  glBeginQuery(GL_TIME_ELAPSED, query);
  ```

- `ProfilerEndZone`:
  ```
  glEndQuery(GL_TIME_ELAPSED);
  ```

- `ProfilerFrameBegin`: Read previous frame's results:
  ```
  int readIdx = 1 - profiler->writeIdx;
  for each zone:
      GLuint query = profiler->queries[zone][readIdx];
      GLuint64 elapsed = 0;
      glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed);
      float ms = elapsed / 1000000.0f;  // ns to ms
      zone->lastMs = ms;
      zone->history[zone->historyIndex] = ms;
  ```

- `ProfilerFrameEnd`: Flip buffer and advance history:
  ```
  profiler->writeIdx = 1 - profiler->writeIdx;
  for each zone: historyIndex = (historyIndex + 1) % PROFILER_HISTORY_SIZE
  ```

**Done when**: Flame chart shows realistic GPU times (15-17ms total) instead of ~0.04ms.

---

## Phase 3: Verify and Test

**Goal**: Confirm accurate timing under various conditions.

**Verification**:
- Toggle simulations on/off - ZONE_SIMULATION time should change proportionally
- Compare against RenderDoc/NSight GPU profiler readings
- Check 1-frame latency doesn't cause visual artifacts in UI

**Edge cases**:
- First frame: dummy queries from Phase 1 prevent uninitialized reads
- Disabled zones: `glBeginQuery`/`glEndQuery` still valid (measures ~0ns)

**Done when**: GPU times match external profiler within 5%, no visual glitches.

---

## Post-Implementation Notes

**EMA Smoothing** (added during Phase 3):
- Raw GPU query results caused jumpy UI readouts
- Added `smoothedMs` field to ProfileZone with 15% EMA factor (`PROFILER_SMOOTHING`)
- UI displays `smoothedMs` for stable values; `history[]` retains raw samples for sparkline variance

**Zone Nesting Fix** (added during Phase 2):
- `GL_TIME_ELAPSED` queries don't nest - starting a new query implicitly ends the active one
- Original code nested `ZONE_SIMULATION` inside `ZONE_FEEDBACK`, causing simulation time to appear under feedback
- Fix: Extracted `RenderPipelineApplySimulation()` and call it separately from `RenderPipelineExecute()`
- Zones now execute sequentially without overlap: Simulation → Feedback → Drawables → Output
