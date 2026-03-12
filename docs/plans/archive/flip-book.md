# Flip Book

Choppy stop-motion effect that holds frames at a reduced rate (e.g., 8–12 fps) while the app runs at full speed. Each held frame gets a hash-based UV jitter for hand-animated wobble. The hold rate is modulatable, so audio energy can snap fresh frames on beats while quiet passages freeze into near-slideshows.

**Research**: `docs/research/flip-book.md`

## Design

### Types

**`FlipBookConfig`** (in `src/effects/flip_book.h`):

```
bool enabled = false;
float fps = 12.0f;    // Target frame rate (2.0-60.0)
float jitter = 2.0f;  // Wobble intensity in pixels per held frame (0.0-8.0)
```

**`FlipBookEffect`** (in `src/effects/flip_book.h`):

```
Shader shader;
RenderTexture2D heldFrame;
float frameTimer;       // Accumulated time since last frame capture
int frameIndex;         // Integer seed, incremented on capture
int lastRenderedIndex;  // Tracks whether heldFrame needs update

// Uniform locations
int resolutionLoc;
int jitterSeedLoc;
int jitterAmountLoc;
```

### Algorithm

#### 1. CPU-side frame timer (in Setup)

Accumulate `frameTimer += deltaTime`. When `frameTimer >= 1.0f / fps`, reset `frameTimer` to zero and increment `frameIndex`. Pass `float(frameIndex)` to the shader as the jitter seed. Clamp to one update per render frame (if deltaTime > 1/fps, still only increment once).

#### 2. Conditional render texture update + jitter pass (in Render)

The effect owns one `RenderTexture2D heldFrame`. Each frame:

- If `frameIndex != lastRenderedIndex`: blit `pe->currentSceneTexture` into `heldFrame` via `RenderUtilsDrawFullscreenQuad` with no shader active (raylib default passthrough). Update `lastRenderedIndex`.
- Always: render `heldFrame` through the jitter shader to `*pe->currentRenderDest`.

#### 3. Hash-based UV jitter (Fragment shader)

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // held frame
uniform vec2 resolution;
uniform float jitterSeed;     // float(frameIndex)
uniform float jitterAmount;   // 0.0-8.0, pixels of offset

void main() {
    vec2 jitter = vec2(
        fract(sin(jitterSeed * 12.9898) * 43758.5453),
        fract(sin(jitterSeed * 78.233 + 0.5) * 43758.5453)
    ) * 2.0 - 1.0;

    vec2 uv = fragTexCoord + jitter * jitterAmount / resolution;
    finalColor = texture(texture0, uv);
}
```

The jitter vector changes only when `frameIndex` increments, so held frames wobble to a new position each capture tick but stay stable between ticks.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| fps | float | 2.0–60.0 | 12.0 | Yes | `"FPS##flipBook"` |
| jitter | float | 0.0–8.0 | 2.0 | Yes | `"Jitter##flipBook"` |

### Constants

- Enum name: `TRANSFORM_FLIP_BOOK`
- Display name: `"Flip Book"`
- Category badge: `"RET"`
- Section index: `6`
- Flags: `EFFECT_FLAG_NEEDS_RESIZE`
- Config field macro: `FLIP_BOOK_CONFIG_FIELDS`
- Registration: Manual `EffectDescriptorRegister()` — custom render callback, sized init

---

## Tasks

### Wave 1: Header

#### Task 1.1: Effect header

**Files**: `src/effects/flip_book.h` (create)
**Creates**: `FlipBookConfig`, `FlipBookEffect` structs, `FLIP_BOOK_CONFIG_FIELDS` macro, function declarations

**Do**: Create header following the Types section above. Declare these functions:
- `FlipBookEffectInit(FlipBookEffect*, int width, int height)` → `bool`
- `FlipBookEffectSetup(FlipBookEffect*, const FlipBookConfig*, float deltaTime)`
- `FlipBookEffectRender(FlipBookEffect*, PostEffect*)`
- `FlipBookEffectResize(FlipBookEffect*, int width, int height)`
- `FlipBookEffectUninit(FlipBookEffect*)`
- `FlipBookConfigDefault(void)` → `FlipBookConfig`
- `FlipBookRegisterParams(FlipBookConfig*)`

Forward-declare `PostEffect` with `typedef struct PostEffect PostEffect;` (same as `slit_scan_corridor.h`).

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/flip_book.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement all lifecycle functions, UI, and registration. Follow `slit_scan_corridor.cpp` as the structural pattern.

**Init** (`FlipBookEffectInit`): Load `shaders/flip_book.fs`, cache uniform locations (`resolution`, `jitterSeed`, `jitterAmount`), allocate `heldFrame` via `RenderUtilsInitTextureHDR`. Initialize `frameTimer = 0`, `frameIndex = 0`, `lastRenderedIndex = -1` (force first-frame capture).

**Setup** (`FlipBookEffectSetup`): Implement the CPU-side frame timer from the Algorithm section. Bind `jitterSeed` (`float(frameIndex)`) and `jitterAmount` (`cfg->jitter`) uniforms.

**Render** (`FlipBookEffectRender`): Implement the conditional blit + jitter pass from the Algorithm section. Use `pe->currentSceneTexture` as input, `pe->currentRenderDest` as output. Bind `resolution` uniform here (needs `pe->screenWidth/Height`).

**Resize** (`FlipBookEffectResize`): Unload and re-init `heldFrame`. Reset `lastRenderedIndex = -1` to force re-capture.

**Uninit** (`FlipBookEffectUninit`): Unload shader and `heldFrame`.

**RegisterParams**: Register `"flipBook.fps"` (2.0–60.0) and `"flipBook.jitter"` (0.0–8.0).

**UI section** (`DrawFlipBookParams`): Two `ModulatableSlider` calls for FPS (`"%.1f"`) and Jitter (`"%.1f"`).

**Bridge functions**: `SetupFlipBook(PostEffect*)` calls `FlipBookEffectSetup`, `RenderFlipBook(PostEffect*)` calls `FlipBookEffectRender`. Both non-static (convention: bridge functions are not static). The Init_/Uninit_/Resize_/Register_/GetShader_ adapter wrappers for manual registration ARE static.

**Registration**: Manual `EffectDescriptorRegister()` call at bottom. Follow the slit scan corridor pattern exactly. Key field mapping:
- `scratchSetup` = `SetupFlipBook` (binds uniforms before render)
- `render` = `RenderFlipBook` (custom render callback)
- `setup` = `nullptr` (no blend compositor)
- `getShader` = returns `&pe->flipBook.shader`

Includes: `"flip_book.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `<stddef.h>`

**Verify**: Compiles after Tasks 2.2–2.5 are also complete.

---

#### Task 2.2: Shader

**Files**: `shaders/flip_book.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the fragment shader from the Algorithm section verbatim.

**Verify**: File exists, valid GLSL syntax.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/flip_book.h"` with the other effect includes
2. Add `TRANSFORM_FLIP_BOOK,` before `TRANSFORM_EFFECT_COUNT` in the enum
3. Add `TRANSFORM_FLIP_BOOK` to the `TransformOrderConfig::order` array (end, before closing brace)
4. Add `FlipBookConfig flipBook;` to `EffectConfig` struct

**Verify**: Compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/flip_book.h"` with the other effect includes
2. Add `FlipBookEffect flipBook;` member in the `PostEffect` struct (near the other effect members)

**Verify**: Compiles.

---

#### Task 2.5: Build system + serialization

**Files**: `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `src/effects/flip_book.cpp` to `EFFECTS_SOURCES` in `CMakeLists.txt`
2. In `effect_serialization.cpp`:
   - Add `#include "effects/flip_book.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FlipBookConfig, FLIP_BOOK_CONFIG_FIELDS)`
   - Add `X(flipBook) \` to the `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: Full build succeeds.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Retro section (badge "RET")
- [ ] Effect can be reordered via drag-drop
- [ ] FPS slider controls choppiness (low = slideshow, high = smooth)
- [ ] Jitter slider controls wobble magnitude
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to `flipBook.fps` and `flipBook.jitter`
