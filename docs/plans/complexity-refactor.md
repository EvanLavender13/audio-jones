# Complexity Refactor Plan

Lizard static analysis identifies 14 functions exceeding warning thresholds (CCN > 10, NLOC > 50, params > 5). `UIDrawWaveformPanel` at `src/ui/ui_main.cpp:52` exceeds error threshold with CCN 18 and 9 parameters.

## Thresholds

| Metric | Warning | Error |
|--------|---------|-------|
| Cyclomatic complexity (CCN) | >10 | >15 |
| Lines of code (NLOC) | >50 | >100 |
| Parameters | >5 | >7 |

## Phase 1: UI Helper Extraction

Extract slider boilerplate into reusable helpers. Reduces ~60 lines across 5 files.

### DrawLabeledSlider

Eliminates 4-line pattern repeated 25+ times:

```cpp
UILayoutRow(l, rowH);
DrawText("Label", l->x + l->padding, l->y + 4, 10, GRAY);
(void)UILayoutSlot(l, labelRatio);
GuiSliderBar(UILayoutSlot(l, 1.0f), NULL, NULL, &value, min, max);
```

New signature in `src/ui/ui_widgets.cpp`:

```cpp
void DrawLabeledSlider(UILayout* l, const char* label, float* value, float min, float max);
```

### DrawIntSlider

Eliminates int-float-int conversion at `src/ui/ui_panel_effects.cpp:24-26`:

```cpp
float blurFloat = (float)effects->baseBlurScale;
GuiSliderBar(..., &blurFloat, 0.0f, 4.0f);
effects->baseBlurScale = lroundf(blurFloat);
```

New signature:

```cpp
void DrawIntSlider(UILayout* l, const char* label, int* value, int min, int max);
```

### Files

| File | NLOC Before | NLOC After |
|------|-------------|------------|
| `src/ui/ui_panel_spectrum.cpp` | 70 | ~40 |
| `src/ui/ui_panel_effects.cpp` | 42 | ~25 |
| `src/ui/ui_color.cpp` | 51 | ~35 |

## Phase 2: Parameter Grouping

Groups 7 config pointers into single struct. Reduces `UIDrawWaveformPanel` from 9 params to 3.

```cpp
struct AppConfigs {
    WaveformConfig* waveforms;
    int* waveformCount;
    int* selectedWaveform;
    EffectConfig* effects;
    AudioConfig* audio;
    SpectrumConfig* spectrum;
    BeatDetector* beat;
};
```

Affects: `src/ui/ui_main.cpp`, `src/ui/ui_panel_preset.cpp`, `src/main.cpp`

## Phase 3: Accordion Extraction

Extracts toggle-conditional-draw pattern into callback helper. Eliminates 4 conditionals from `UIDrawWaveformPanel`.

```cpp
typedef void (*AccordionDrawFn)(UILayout* l, void* context);
int DrawAccordionSection(UILayout* l, const char* title, bool* expanded,
                         AccordionDrawFn draw, void* context);
```

Target: CCN 18 to ~10

## Phase 4: Analysis Layer

### ComputeSpectralFlux

Extracts `src/analysis/beat.cpp:47-61` (spectral flux + bass energy loop). Reduces `BeatDetectorProcess` CCN from 13 to ~9.

### NormalizeWaveform

Extracts `src/render/waveform.cpp:122-140` (peak-find + scale). Reduces `ProcessWaveformBase` CCN from 11 to ~7.

## Phase 5: Main Loop (Optional)

`main()` and `AppContextInit()` sit at warning threshold (CCN 9). Extract `RenderFrame()` and consolidate `globalTick` increments if needed.

## Verification

```bash
lizard ./src -C 15 -L 100 -a 7 --warnings_only
```

Exit code 0 indicates no functions exceed error thresholds.
