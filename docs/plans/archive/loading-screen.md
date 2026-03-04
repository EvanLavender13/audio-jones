# Loading Screen

Replace the blank white window during startup with a black screen and centered progress bar. The bar advances through 4 coarse init phases via a progress callback passed into `PostEffectInit`. Total init is ~1-2 seconds — no text labels, just a moving bar.

**Research**: `docs/research/loading-screen.md`

## Design

### Types

```c
// Progress callback type — called between init phases
// progress: 0.0 to 1.0
typedef void (*PostEffectProgressFn)(float progress, void *userData);
```

No new structs. The loading screen renderer is a free function, not a stateful object.

### Algorithm

`PostEffectInit` accepts an optional `PostEffectProgressFn` callback + `void *userData`. Between each phase it calls `callback(progress, userData)`. `main.cpp` passes a callback that calls `DrawLoadingFrame(progress)`.

```
PostEffectInit(screenW, screenH, callback, userData):
    alloc + core shaders + uniforms + render textures
    callback(0.25, userData)

    7 simulations + blend compositor + noise texture
    callback(0.50, userData)

    EFFECT_DESCRIPTORS[] loop + generator scratch + FFT/waveform + half-res
    callback(0.75, userData)

    return pe
    // caller does audio + analysis + modulation, then calls callback(1.0) itself
```

`DrawLoadingFrame(float progress)`:
```
BeginDrawing()
ClearBackground(BLACK)

barWidth = screenWidth * 0.5
barHeight = 6
barX = (screenWidth - barWidth) / 2
barY = screenHeight / 2

DrawRectangle(barX, barY, barWidth, barHeight, {40, 40, 40, 255})         // track
DrawRectangle(barX, barY, barWidth * progress, barHeight, {0, 230, 242, 255})  // fill

EndDrawing()
```

Uses only `DrawRectangle` — available immediately after `InitWindow`.

### Parameters

None. No config struct. The bar dimensions are derived from window size.

### Constants

- Bar track color: `{40, 40, 40, 255}`
- Bar fill color: `{0, 230, 242, 255}` (matches `ThemeColor::NEON_CYAN`, hardcoded to avoid ImGui dependency)
- Bar width: 50% of window width
- Bar height: 6px
- Bar position: vertically and horizontally centered

---

## Tasks

### Wave 1: Loading screen renderer + PostEffectInit signature change

#### Task 1.1: Create loading screen renderer

**Files**: `src/ui/loading_screen.h` (create), `src/ui/loading_screen.cpp` (create)
**Creates**: `DrawLoadingFrame(float progress)` function

**Do**:
- `loading_screen.h`: Include guard, `#include "raylib.h"`, declare `void DrawLoadingFrame(float progress);`
- `loading_screen.cpp`: Implement per the Algorithm section. `BeginDrawing`, `ClearBackground(BLACK)`, compute bar geometry from `GetScreenWidth()`/`GetScreenHeight()`, draw track rectangle, draw fill rectangle, `EndDrawing`.
- No text, no title, no percentage. Just the bar.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Add progress callback to PostEffectInit

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Creates**: `PostEffectProgressFn` typedef, updated `PostEffectInit` signature

**Do**:
- `post_effect.h`: Add `typedef void (*PostEffectProgressFn)(float progress, void *userData);` before the `PostEffect` struct. Change `PostEffectInit` signature to `PostEffect *PostEffectInit(int screenWidth, int screenHeight, PostEffectProgressFn onProgress, void *userData);`
- `post_effect.cpp`: Update `PostEffectInit` to accept the two new parameters. Add 3 callback invocations between the 3 internal phases:
  1. After `SetResolutionUniforms` + render texture allocation → `onProgress(0.25f, userData)`
  2. After all 7 simulation inits + `BlendCompositorInit()` + `NoiseTextureInit()` → `onProgress(0.50f, userData)`
  3. After `EFFECT_DESCRIPTORS[]` loop + generator scratch + FFT/waveform textures + half-res textures → `onProgress(0.75f, userData)`
- Guard each callback call: `if (onProgress != NULL) onProgress(...);`
- Do not reorder any init calls. `NoiseTextureInit()` stays in its current position (after sims, before effects). The existing error handling (goto cleanup) remains unchanged.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Wire up in main.cpp

#### Task 2.1: Integrate loading screen into main

**Files**: `src/main.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "ui/loading_screen.h"` to includes.
- Before `AppContextInit`, render one initial black frame: `BeginDrawing(); ClearBackground(BLACK); EndDrawing();` — this eliminates the white flash immediately.
- Define a `static void OnLoadingProgress(float progress, void *userData)` that calls `DrawLoadingFrame(progress)`. The `userData` parameter is unused (cast to `(void)userData`).
- In `AppContextInit`: pass `OnLoadingProgress` and `NULL` to the updated `PostEffectInit` call.
- After `AppContextInit` returns successfully (line 198-202 area, before the main loop), call `DrawLoadingFrame(1.0f)` to show the completed bar. This covers the audio/analysis/modulation init that happens after `PostEffectInit` returns inside `AppContextInit`.

**Verify**: Build succeeds. Launch the app — should see black background with cyan progress bar instead of white flash.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] App launches with black background (no white flash)
- [ ] Progress bar visibly advances during init
- [ ] Bar disappears when main loop begins
- [ ] Existing init error handling still works (shader load failure → cleanup → exit)
