# Hex Rush Ring Buffer & Parameter Improvements

Make gapChance and patternSeed only affect new rings scrolling in from offscreen instead of instantly changing all visible rings. Also make wallSpacing, wallThickness, and wallGlow modulatable, and replace the hardcoded frequency band count with a configurable freqBins parameter.

## Design

### Ring History Buffer

A 256x1 float texture acts as a circular buffer indexed by `ringIndex % 256`. Each texel stores two values packed into RG channels: the gapChance (R) and patternSeed (G) that were active when that ring first appeared at the screen edge.

**CPU side** (in `HexRushEffectSetup`):
- Compute the max visible ring index: `floor((0.9 * 10.0 + wallAccum) / wallSpacing)`
- For each new ring from `lastFilledRing + 1` to `maxRing`, write the current `cfg->gapChance` and `cfg->patternSeed` into `ringBuffer[ring % 256]`
- Update `lastFilledRing = maxRing`
- Upload via `UpdateTexture`

**Shader side**: Replace `uniform float gapChance` and direct `patternSeed` usage in the hash with a `texelFetch` from the buffer texture using `int(ringIndex) & 255`.

### Types

**New fields in `HexRushEffect`** (runtime state):
- `float ringBuffer[512]` — CPU-side RG pairs (256 entries x 2 floats)
- `Texture2D ringBufferTex` — 256x1 RG32F GPU texture
- `int lastFilledRing` — highest ring index written to buffer
- `int ringBufferLoc` — shader uniform location

**Remove from `HexRushEffect`**: `int gapChanceLoc` (no longer a uniform)

**New field in `HexRushConfig`**:
- `int freqBins = 48` — discrete frequency bins for ring FFT lookup (12-120)

**New in `HEX_RUSH_CONFIG_FIELDS`**: add `freqBins` after `baseBright`

**New in `HexRushEffect`**: `int freqBinsLoc`

### Shader Changes (hex_rush.fs)

Remove `uniform float gapChance`. Add:
```glsl
uniform sampler2D ringBuffer;
uniform int freqBins;
```

Replace gap hash block (lines 71-77):
```glsl
// Per-ring gapChance and patternSeed from ring history buffer
vec2 ringData = texelFetch(ringBuffer, ivec2(int(ringIndex) & 255, 0), 0).rg;
float ringGap = ringData.r;
float ringSeed = ringData.g;

float wallHash = fract(sin(ringIndex * 127.1 + segIndex * 311.7 + ringSeed) * 43758.5453);
float hasWall = step(ringGap, wallHash);

float gapSeg = floor(fract(sin(ringIndex * 269.3 + ringSeed) * 43758.5453) * float(sides));
if (segIndex == gapSeg) hasWall = 0.0;
```

Replace hardcoded `bandCount = 20.0` (line 92) with `float(freqBins)`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| freqBins | int | 12-120 | 48 | No | Freq Bins |
| wallSpacing | float | 0.2-2.0 | 0.5 | Yes | Wall Spacing |
| wallThickness | float | 0.02-0.6 | 0.15 | Yes | Wall Thickness |
| wallGlow | float | 0.0-2.0 | 0.5 | Yes | Wall Glow |

gapChance and patternSeed remain modulatable — changes just apply to new rings only.

---

## Tasks

### Wave 1: Config & Struct Changes

#### Task 1.1: Update HexRushConfig and HexRushEffect structs

**Files**: `src/effects/hex_rush.h`
**Creates**: Updated struct layouts for Wave 2

**Do**:
- Add `int freqBins = 48; // Discrete frequency bins for ring FFT lookup (12-120)` after `baseBright` in `HexRushConfig`
- Add `freqBins` to `HEX_RUSH_CONFIG_FIELDS` after `baseBright`
- In `HexRushEffect`: add `float ringBuffer[512];`, `Texture2D ringBufferTex;`, `int lastFilledRing;`, `int ringBufferLoc;`, `int freqBinsLoc;`
- In `HexRushEffect`: remove `int gapChanceLoc;`

**Verify**: Header parses (no syntax errors in isolation).

---

### Wave 2: Implementation (parallel, no file overlap)

#### Task 2.1: C++ Init/Setup/Uninit and param registration

**Files**: `src/effects/hex_rush.cpp`
**Depends on**: Wave 1

**Do**:

*Init changes*:
- After LUT init, create the ring buffer texture:
  - Fill `e->ringBuffer` with pairs: `[cfg->gapChance, cfg->patternSeed]` repeated 256 times
  - Create `Image img = {e->ringBuffer, 256, 1, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32}` — actually use `PIXELFORMAT_UNCOMPRESSED_R32G32` (RG32F) if available, otherwise use `PIXELFORMAT_UNCOMPRESSED_R32G32B32A32` and pack into RGBA with BA = 0. Check which raylib pixel formats support 2-channel float. If only RGBA32F is available, use that with 1024 floats (256 entries x 4 channels) and waste BA channels.
  - `e->ringBufferTex = LoadTextureFromImage(img)` — do NOT call `UnloadImage` (data points to struct array)
  - `SetTextureFilter(e->ringBufferTex, TEXTURE_FILTER_POINT)`
  - `e->lastFilledRing = -1`
- Cache uniform locations: replace `gapChanceLoc` line with `e->ringBufferLoc = GetShaderLocation(e->shader, "ringBuffer")`, add `e->freqBinsLoc = GetShaderLocation(e->shader, "freqBins")`
- On init failure, `UnloadTexture(e->ringBufferTex)` in the cleanup cascade

*Setup changes*:
- After `e->wallAccum += ...`, compute new rings and fill buffer:
  ```
  int maxRing = (int)floorf((0.9f * 10.0f + e->wallAccum) / cfg->wallSpacing);
  for (int ring = e->lastFilledRing + 1; ring <= maxRing; ring++) {
      int idx = (ring & 255) * 4;  // if RGBA32F
      e->ringBuffer[idx] = cfg->gapChance;
      e->ringBuffer[idx + 1] = cfg->patternSeed;
  }
  if (maxRing > e->lastFilledRing) {
      e->lastFilledRing = maxRing;
      UpdateTexture(e->ringBufferTex, e->ringBuffer);
  }
  ```
- Remove `SetShaderValue(...gapChanceLoc...gapChance...)` line
- Add `SetShaderValueTexture(e->shader, e->ringBufferLoc, e->ringBufferTex)`
- Add `SetShaderValue(e->shader, e->freqBinsLoc, &cfg->freqBins, SHADER_UNIFORM_INT)`
- `patternSeed` uniform stays bound as-is (shader still needs it for non-buffered uses? No — all patternSeed usage moves to the buffer). Remove the `patternSeed` SetShaderValue line and `patternSeedLoc` from Init. Actually keep `patternSeedLoc` and the uniform — the shader still references `patternSeed` nowhere else, so remove both the loc and the SetShaderValue. Remove `patternSeedLoc` from the struct in Task 1.1 as well.

*Correction to Task 1.1*: Also remove `int patternSeedLoc;` from `HexRushEffect`.

*Uninit changes*:
- Add `UnloadTexture(e->ringBufferTex)` before the existing `UnloadShader`

*Param registration*:
- Add `ModEngineRegisterParam("hexRush.wallSpacing", &cfg->wallSpacing, 0.2f, 2.0f)`
- Add `ModEngineRegisterParam("hexRush.wallThickness", &cfg->wallThickness, 0.02f, 0.6f)`
- Add `ModEngineRegisterParam("hexRush.wallGlow", &cfg->wallGlow, 0.0f, 2.0f)`
- freqBins is int, not modulatable — no registration needed

**Verify**: `cmake.exe --build build` compiles (shader errors are runtime, not build-time).

#### Task 2.2: Shader updates

**Files**: `shaders/hex_rush.fs`
**Depends on**: Wave 1 (conceptually, but no #include dependency)

**Do**:
- Remove `uniform float gapChance;` (line 24)
- Remove `uniform float patternSeed;` (line 28)
- Add `uniform sampler2D ringBuffer;` and `uniform int freqBins;` in the uniform block
- Replace gap hash block (lines 71-77) with the shader code from the Algorithm section above — read `ringData` from `texelFetch`, use `ringGap` for step threshold and `ringSeed` for both hash functions
- Replace `float bandCount = 20.0;` (line 92) with `float bandCount = float(freqBins);`

**Verify**: Shader compiles at runtime (launch app with Hex Rush enabled).

#### Task 2.3: UI — make sliders modulatable and add freqBins

**Files**: `src/ui/imgui_effects_gen_geometric.cpp`
**Depends on**: Wave 1

**Do**:
- Replace `ImGui::SliderFloat("Wall Thickness##hexrush", ...)` with `ModulatableSlider("Wall Thickness##hexrush", &cfg->wallThickness, "hexRush.wallThickness", "%.2f", modSources)`
- Replace `ImGui::SliderFloat("Wall Spacing##hexrush", ...)` with `ModulatableSlider("Wall Spacing##hexrush", &cfg->wallSpacing, "hexRush.wallSpacing", "%.2f", modSources)`
- Replace `ImGui::SliderFloat("Wall Glow##hexrush", ...)` with `ModulatableSlider("Wall Glow##hexrush", &cfg->wallGlow, "hexRush.wallGlow", "%.2f", modSources)`
- Add `ImGui::SliderInt("Freq Bins##hexrush", &cfg->freqBins, 12, 120);` in the Audio section after Base Bright (matches glyph_field/scan_bars pattern)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Hex Rush launches without shader compile errors
- [ ] Changing Gap Chance slider only affects new rings from screen edge, not existing visible rings
- [ ] Changing Pattern Seed only affects new rings from screen edge
- [ ] Wall Thickness, Wall Spacing, Wall Glow show modulation indicators and accept LFO routing
- [ ] Freq Bins slider visible in Audio section, adjusts frequency band granularity
- [ ] Existing presets with Hex Rush still load correctly (new fields get defaults)
