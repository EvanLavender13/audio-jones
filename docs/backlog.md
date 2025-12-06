# Backlog

Ideas and potential features. Not committed, just captured.

## Smoothing Modes

Linear mode shows edge artifacts from smoothing:
- Left edge: truncated window (no data before sample 0)
- Right edge: mirror point plateau (sample 1023 appears twice)

Could add configurable modes:
- Wrap (circular buffer) - ends average toward each other, "plucked string" effect
- Truncate (current) - edges fade out
- None - raw data at boundaries

Low priority since circular mode handles this well via palindrome.

## Circular Palindrome Symmetry

The palindrome creates visible mirror symmetry in circular mode - an axis where both sides reflect each other. This is inherent to mirroring 1024 samples to get 2048.

Options:
- Use 2048 actual samples (no mirror) - but then start/end won't match seamlessly
- Crossfade the ends somehow
- Accept it as a visual characteristic

May not be worth "fixing" - the symmetry has its own aesthetic.

## EffectsConfig Struct

Effect parameters (halfLife, baseBlurScale, beatBlurScale) scatter across Visualizer, Preset, and UI function signatures. Adding one parameter requires changes to 5+ files.

Consolidate into single struct:
```c
typedef struct EffectsConfig {
    float halfLife;
    float baseBlurScale;
    float beatBlurScale;
} EffectsConfig;
```

Reduces per-parameter changes from 8 locations to 3 (struct, JSON macro, UI slider). Worth doing before adding chromatic aberration or vignette effects.
