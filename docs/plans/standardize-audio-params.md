# Standardize Audio Parameters

Unify FFT/Audio parameter names, types, ranges, defaults, slider types, UI ordering, and section headers across all 10 audio-reactive generator effects. Every effect gets the same 5-param Audio block with consistent `ModulatableSlider`/`ModulatableSliderInt` controls.

## Design

### Standard Audio Parameter Set

Every audio-reactive effect uses these 5 fields with identical names, types, ranges, and UI presentation:

| Field | Config Type | Range | Default | Mod Registration | UI Widget | UI Label | Format |
|-------|-------------|-------|---------|-----------------|-----------|----------|--------|
| `numOctaves` | `float` | 1.0–8.0 | 5.0f | `"<prefix>.numOctaves", 1.0f, 8.0f` | `ModulatableSliderInt` | `"Octaves##tag"` | — |
| `baseFreq` | `float` | 27.5–440.0 | 55.0f | `"<prefix>.baseFreq", 27.5f, 440.0f` | `ModulatableSlider` | `"Base Freq (Hz)##tag"` | `"%.1f"` |
| `gain` | `float` | 0.1–10.0 | 2.0f | `"<prefix>.gain", 0.1f, 10.0f` | `ModulatableSlider` | `"Gain##tag"` | `"%.1f"` |
| `curve` | `float` | 0.1–3.0 | 0.7f | `"<prefix>.curve", 0.1f, 3.0f` | `ModulatableSlider` | `"Contrast##tag"` | `"%.2f"` |
| `baseBright` | `float` | 0.0–1.0 | 0.15f | `"<prefix>.baseBright", 0.0f, 1.0f` | `ModulatableSlider` | `"Base Bright##tag"` | `"%.2f"` |

### UI Section

- Header: `ImGui::SeparatorText("Audio")` — replaces `"FFT"` everywhere
- Slider order: Octaves, Base Freq, Gain, Contrast, Base Bright — always grouped together, no other params interleaved
- All 5 sliders appear consecutively under the "Audio" separator

### Config Type Change: `int numOctaves` → `float numOctaves`

Most effects currently store `numOctaves` as `int`. Changing to `float` enables `ModulatableSliderInt` (LFO modulation). This requires:
- **Header**: `float numOctaves = 5.0f;` (was `int numOctaves = N;`)
- **Shader setup**: Cast before binding: `int numOctavesInt = (int)cfg->numOctaves;` then `SetShaderValue(..., &numOctavesInt, SHADER_UNIFORM_INT);`
- **Param registration**: Add `ModEngineRegisterParam("<prefix>.numOctaves", &cfg->numOctaves, 1.0f, 8.0f);`

Reference pattern: `src/effects/glyph_field.cpp:132-134` and `src/effects/glyph_field.cpp:197`

### Per-Effect Delta Summary

| Effect | numOctaves type | baseFreq default | gain range/default | curve range/default | baseBright range/default | Section | Other |
|--------|----------------|------------------|-------------------|--------------------|-----------------------|---------|-------|
| **Nebula** | int→float | 55→55 ✓ | 1–20/5→0.1–10/2 | 0.5–3/1.5→0.1–3/0.7 | 0–2/0.6→0–1/0.15 | FFT→Audio | — |
| **Motherboard** | int→float | 55→55 ✓ | 1–20/5→0.1–10/2 | 0.5–4/1.5→0.1–3/0.7 | OK | FFT→Audio | default numOctaves 8→5 |
| **Filaments** | int→float | 220→55 | 1–20/5→0.1–10/2 | 0.5–4/1.5→0.1–3/0.7 | 0–0.5/0.05→0–1/0.15 | FFT→Audio | Move baseBright into Audio group |
| **Slashes** | int→float | 55→55 ✓ | 0.1–20/5→0.1–10/2 | 0.1–5/1.0→0.1–3/0.7 | 0–0.5/0.05→0–1/0.15 | FFT→Audio | Move baseBright into Audio group; default numOctaves 4→5 |
| **Spectral Arcs** | int→float | 220→55 | 0.1–20/5→0.1–10/2 | 0.5–4/2.0→0.1–3/0.7 | 0–1/0.1→0–1/0.15 | FFT→Audio | Move baseBright into Audio group; default numOctaves 8→5 |
| **Signal Frames** | int→float | 55→55 ✓ | 1–20/5→0.1–10/2 | 0.5–3/1.5→0.1–3/0.7 | 0–0.5/0.05→0–1/0.15 | FFT→Audio | Convert ALL sliders to Modulatable; add param registration for audio fields; default numOctaves 3→5 |
| **Arc Strobe** | int→float | 220→55 | 1–20/5→0.1–10/2 | 0.5–4/2.0→0.1–3/0.7 | 0–0.5/0.05→0–1/0.15 | FFT→Audio | Keep segmentsPerOctave as extra |
| **Pitch Spiral** | ADD field | 220→55 | 1–20/5→0.1–10/2 | 0.5–4/2.0→0.1–3/0.7 | ADD field (0.15) | ADD "Audio" | Keep numTurns separate; add numOctaves + baseBright to config, shader, registration |
| **Scan Bars** | int→float | 55→55 ✓ | OK | OK | OK | Audio ✓ | Only numOctaves type change + ModulatableSliderInt |
| **Motherboard** | — | — | — | — | — | — | (duplicate row removed) |
| **Glyph Field** | float ✓ | 55→55 ✓ | OK | OK | 1.0→0.15 | Audio ✓ | Only baseBright default change |

### Pitch Spiral: New Fields + Shader Changes

**Config additions** (`src/effects/pitch_spiral.h`):
- Add `float numOctaves = 5.0f;` — controls FFT frequency ceiling: `maxFreq = baseFreq * 2^numOctaves`
- Add `float baseBright = 0.15f;` — baseline brightness for silent spiral lines
- Keep `int numTurns` — controls visual spiral extent (unchanged)

**Shader additions** (`shaders/pitch_spiral.fs`):
- Add `uniform int numOctaves;` and `uniform float baseBright;`
- Frequency ceiling: clamp with `freq < baseFreq * pow(2.0, float(numOctaves))` in addition to existing `bin <= 1.0` check
- Baseline brightness: change `magnitude` calculation to include baseBright for visible-but-silent lines: after the gain/curve shaping, add `magnitude = max(magnitude, baseBright * lineAlpha);` or similar so silent notes glow dimly

**Effect source** (`src/effects/pitch_spiral.cpp`):
- Add `numOctavesLoc`, `baseBrightLoc` location caching
- Add shader uniform binding for both
- Add param registration for `numOctaves`, `baseBright`, and `baseFreq` (baseFreq registration exists but range changes)

---

## Tasks

### Wave 1: Config + Source Changes

All 10 effects' `.h` and `.cpp` files — no file overlap between tasks.

#### Task 1.1: Geometric Group — Config + Source

**Files**: `src/effects/signal_frames.h`, `src/effects/signal_frames.cpp`, `src/effects/arc_strobe.h`, `src/effects/arc_strobe.cpp`, `src/effects/pitch_spiral.h`, `src/effects/pitch_spiral.cpp`, `src/effects/spectral_arcs.h`, `src/effects/spectral_arcs.cpp`
**Creates**: Updated config types and param registrations for geometric generators

**Do**:
For each of the 4 effects, apply the standard parameter set from the Design section:

**Signal Frames**:
- `.h`: Change `int numOctaves = 3` → `float numOctaves = 5.0f`. Update inline range comments for all audio fields to match standard ranges. Change defaults: gain 5.0→2.0, curve 1.5→0.7, baseBright 0.05→0.15
- `.cpp` shader setup: Add `int numOctavesInt = (int)cfg->numOctaves;` cast before `SetShaderValue` (currently passes `&cfg->numOctaves` directly as INT, which will break after type change to float)
- `.cpp` RegisterParams: Add 5 new registration calls for baseFreq, numOctaves, gain, curve, baseBright with standard ranges. Currently these audio params have NO modulation registration.

**Arc Strobe**:
- `.h`: Change `int numOctaves` → `float numOctaves = 5.0f`. Change defaults: baseFreq 220→55, gain 5.0→2.0, curve 2.0→0.7, baseBright 0.05→0.15
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change baseFreq range to 27.5–440, gain to 0.1–10, curve to 0.1–3, baseBright to 0–1. Add numOctaves registration.

**Pitch Spiral**:
- `.h`: ADD `float numOctaves = 5.0f;` and `float baseBright = 0.15f;` fields. ADD `int numOctavesLoc;` and `int baseBrightLoc;` to effect struct. Change defaults: baseFreq 220→55, gain 5.0→2.0, curve 2.0→0.7
- `.cpp` Init: Cache `numOctavesLoc` and `baseBrightLoc` locations
- `.cpp` Setup: Bind both new uniforms (numOctaves as int cast, baseBright as float)
- `.cpp` RegisterParams: Add numOctaves (1–8), baseBright (0–1), baseFreq range change to 27.5–440, gain to 0.1–10, curve to 0.1–3

**Spectral Arcs**:
- `.h`: Change `int numOctaves = 8` → `float numOctaves = 5.0f`. Change defaults: baseFreq 220→55, gain 5.0→2.0, curve 2.0→0.7, baseBright 0.1→0.15
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change baseFreq range to 27.5–440, gain to 0.1–10, curve to 0.1–3. Add numOctaves registration.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.2: Filament Group — Config + Source

**Files**: `src/effects/filaments.h`, `src/effects/filaments.cpp`, `src/effects/slashes.h`, `src/effects/slashes.cpp`
**Creates**: Updated config types and param registrations for filament generators

**Do**:

**Filaments**:
- `.h`: Change `int numOctaves = 5` → `float numOctaves = 5.0f`. Change defaults: baseFreq 220→55, gain 5.0→2.0, curve 1.5→0.7, baseBright 0.05→0.15
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change baseFreq range to 27.5–440, gain to 0.1–10, curve to 0.1–3, baseBright to 0–1. Add numOctaves registration.

**Slashes**:
- `.h`: Change `int numOctaves = 4` → `float numOctaves = 5.0f`. Change defaults: gain 5.0→2.0, curve 1.0→0.7, baseBright 0.05→0.15
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change baseFreq range to 27.5–440, gain to 0.1–10, curve to 0.1–3, baseBright to 0–1. Add numOctaves registration.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.3: Texture Group — Config + Source

**Files**: `src/effects/scan_bars.h`, `src/effects/scan_bars.cpp`, `src/effects/glyph_field.h`, `src/effects/glyph_field.cpp`, `src/effects/motherboard.h`, `src/effects/motherboard.cpp`
**Creates**: Updated config types and param registrations for texture generators

**Do**:

**Scan Bars**:
- `.h`: Change `int numOctaves = 5` → `float numOctaves = 5.0f`. Update inline range comments.
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue (check if already present)
- `.cpp` RegisterParams: Change baseFreq range to 27.5–440. Add numOctaves registration.

**Glyph Field**:
- `.h`: Change baseBright default from 1.0 → 0.15. Already `float numOctaves` ✓
- `.cpp`: Change baseFreq registration range from 20–200 to 27.5–440. No other changes needed.

**Motherboard**:
- `.h`: Change `int numOctaves = 8` → `float numOctaves = 5.0f`. Change defaults: gain 5.0→2.0, curve 1.5→0.7
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change gain range to 0.1–10, curve to 0.1–3. Add numOctaves registration.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

#### Task 1.4: Atmosphere Group — Config + Source

**Files**: `src/effects/nebula.h`, `src/effects/nebula.cpp`
**Creates**: Updated config types and param registrations for nebula

**Do**:

**Nebula**:
- `.h`: Change `int numOctaves = 5` → `float numOctaves = 5.0f`. Change defaults: gain 5.0→2.0, curve 1.5→0.7, baseBright 0.6→0.15
- `.cpp` shader setup: Add int cast for numOctaves before SetShaderValue
- `.cpp` RegisterParams: Change gain range to 0.1–10, baseBright range to 0–1. Add numOctaves registration.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

### Wave 2: UI Files + Pitch Spiral Shader

Depends on Wave 1 complete (numOctaves type changed from int to float breaks existing SliderInt calls).

#### Task 2.1: Geometric UI

**Files**: `src/ui/imgui_effects_gen_geometric.cpp`
**Depends on**: Wave 1 complete

**Do**: For all 4 geometric generators in this file:

**Signal Frames** (around line 23):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##signalframes", ...)` → `ModulatableSliderInt("Octaves##signalframes", &cfg->numOctaves, "signalFrames.numOctaves", modSources);`
- Replace all `ImGui::SliderFloat` for baseFreq/gain/curve/baseBright → `ModulatableSlider` with standard param IDs and formats
- Param IDs: `"signalFrames.baseFreq"`, `"signalFrames.gain"`, `"signalFrames.curve"`, `"signalFrames.baseBright"`

**Arc Strobe** (around line 106):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##arcstrobe", ...)` → `ModulatableSliderInt("Octaves##arcstrobe", &cfg->numOctaves, "arcStrobe.numOctaves", modSources);`
- Verify slider order is Octaves → segmentsPerOctave → Base Freq → Gain → Contrast → Base Bright (segmentsPerOctave stays between Octaves and Base Freq as effect-specific)

**Pitch Spiral** (around line 188):
- ADD `ImGui::SeparatorText("Audio")` before the audio parameter block
- Add `ModulatableSliderInt("Octaves##pitchspiral", &ps->numOctaves, "pitchSpiral.numOctaves", modSources);`
- Reorder sliders to: Octaves → Base Freq → Gain → Contrast → Base Bright (currently Base Freq is first, Octaves/numTurns is last)
- Add `ModulatableSlider("Base Bright##pitchspiral", &ps->baseBright, "pitchSpiral.baseBright", "%.2f", modSources);`
- Keep `numTurns` slider in its own section (Geometry or similar), separate from Audio block

**Spectral Arcs** (around line 247):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##spectralarcs", ...)` → `ModulatableSliderInt("Octaves##spectralarcs", &sa->numOctaves, "spectralArcs.numOctaves", modSources);`
- Move baseBright slider (currently at line 273, after other params) into the Audio group immediately after Contrast

**Verify**: `cmake.exe --build build` compiles. Visually confirm all 4 effects show "Audio" header with 5 grouped modulatable sliders.

---

#### Task 2.2: Filament UI

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Wave 1 complete

**Do**:

**Filaments** (around line 111):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##filaments", ...)` → `ModulatableSliderInt("Octaves##filaments", &cfg->numOctaves, "filaments.numOctaves", modSources);`
- Move `ModulatableSlider("Base Bright##filaments", ...)` from line 135 (after Glow section) into the Audio group after Contrast

**Slashes** (around line 185):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##slashes", ...)` → `ModulatableSliderInt("Octaves##slashes", &cfg->numOctaves, "slashes.numOctaves", modSources);`
- Move `ModulatableSlider("Base Bright##slashes", ...)` from line 218 (after Glow section) into the Audio group after Contrast

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Texture UI

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1 complete

**Do**:

**Scan Bars** (around line 306):
- Section header already "Audio" ✓
- Replace `ImGui::SliderInt("Octaves##scanbars", ...)` → `ModulatableSliderInt("Octaves##scanbars", &sb->numOctaves, "scanBars.numOctaves", modSources);`
- Other sliders already ModulatableSlider ✓

**Glyph Field** (around line 363):
- Already fully standard ✓ — no UI changes needed

**Motherboard** (around line 459):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##motherboard", ...)` → `ModulatableSliderInt("Octaves##motherboard", &cfg->numOctaves, "motherboard.numOctaves", modSources);`
- Other sliders already ModulatableSlider ✓

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Atmosphere UI

**Files**: `src/ui/imgui_effects_gen_atmosphere.cpp`
**Depends on**: Wave 1 complete

**Do**:

**Nebula** (around line 28):
- Change `ImGui::SeparatorText("FFT")` → `ImGui::SeparatorText("Audio")`
- Replace `ImGui::SliderInt("Octaves##nebula", ...)` → `ModulatableSliderInt("Octaves##nebula", &n->numOctaves, "nebula.numOctaves", modSources);`
- Other sliders already ModulatableSlider ✓

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Pitch Spiral Shader

**Files**: `shaders/pitch_spiral.fs`
**Depends on**: Wave 1 complete (new uniforms cached in pitch_spiral.cpp)

**Do**:
- Add `uniform int numOctaves;` and `uniform float baseBright;`
- Add frequency ceiling check: alongside existing `bin <= 1.0 && bin >= 0.0` condition, add `freq < baseFreq * pow(2.0, float(numOctaves))` so notes beyond the octave range render silent
- Add baseline brightness: after magnitude gain/curve shaping, blend in baseBright so visible-but-silent spiral lines glow dimly. Apply as `magnitude = max(magnitude, baseBright);` before the final `lineAlpha` multiply, so baseBright illuminates the spiral structure even when no audio plays

**Verify**: `cmake.exe --build build` compiles. Pitch spiral shows dim spiral lines when silent, full brightness when audio plays.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 10 effects show `"Audio"` section header (no remaining `"FFT"`)
- [ ] All Octaves sliders are `ModulatableSliderInt` (right-click shows modulation menu)
- [ ] All Base Freq/Gain/Contrast/Base Bright sliders are `ModulatableSlider`
- [ ] Pitch spiral has separate Octaves (audio) and numTurns (geometry) controls
- [ ] Audio slider order is consistent: Octaves → Base Freq → Gain → Contrast → Base Bright
- [ ] No baseBright sliders orphaned outside the Audio group
