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

## Code Style Violations

### preset.cpp: Replace std:: file I/O with raylib

`preset.cpp` uses `std::fstream` and `std::filesystem` for file operations. Could use raylib's file API instead:

Current:
- `std::ofstream` / `std::ifstream` for reading/writing JSON
- `std::filesystem` for `exists()`, `create_directories()`, `directory_iterator()`

raylib alternatives:
- `LoadFileText()` / `SaveFileText()` - text file I/O
- `DirectoryExists()` / `MakeDirectory()` - directory checks
- `LoadDirectoryFiles()` / `UnloadDirectoryFiles()` - directory listing

nlohmann/json still needed (parses from `char*` just fine). Would eliminate `<fstream>` and `<filesystem>` includes.

Low priority - current code works, isolated to one file per style guidelines.

## Beat Detection Improvements

Current implementation (`src/beat.cpp`) uses fixed sensitivity multiplier requiring manual tuning per track. See `docs/research/beat-detection-techniques.md` for full analysis.

### Adaptive Variance Threshold

Replace fixed sensitivity with variance-based threshold:
```
variance = Σ(history[i] - average)² / history_size
threshold = -15 × variance + 1.55
```

Low variance (consistent volume) raises threshold; high variance (dynamic music) lowers it. Reduces need for manual sensitivity adjustment.

### Multi-Band Low-Pass Filters

Add parallel IIR filters at different cutoffs (80Hz, 200Hz, 500Hz) for crude frequency separation without FFT overhead. Enables independent beat detection for:
- Sub-bass (kick drum fundamental)
- Bass (kick body)
- Low-mid (snare, toms)

Each band drives separate visual effects.

### Intensity Smoothing

Current decay is linear (`intensity -= DECAY * dt`). Exponential decay creates smoother visual transitions:
```
intensity *= pow(DECAY_RATE, dt)
```

Where `DECAY_RATE` is 0.0-1.0 per second (e.g., 0.1 = 90% decay/sec).
