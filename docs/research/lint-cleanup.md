# Lint Cleanup & Prevention

Clear the backlog of ~500 lint warnings across clang-tidy, cppcheck, and lizard, then add pre-commit guardrails so new code is checked before it enters the repo.

## Classification

- **Category**: General (tooling/process)

## Findings Summary

355 warnings across 3 tools. After filtering false positives:

| Category | Count | Source | Fix Type |
|----------|-------|--------|----------|
| Dead `ConfigDefault()` functions | 112 | cppcheck | Delete (decl + def) |
| Dead `*BeginTrailMapDraw` / `*EndTrailMapDraw` | 8 | cppcheck | Delete (decl + def) |
| Dead utility functions (15 distinct) | 15 | cppcheck | Delete (decl + def) |
| `nlohmann_json_default_obj` unread variable | 126 | cppcheck | Suppress (macro noise) |
| `const` qualifiers (variables + pointer params) | ~300 | clang-tidy + cppcheck | Mechanical — add `const` |
| Missing braces on single-statement bodies | 45 | clang-tidy | Mechanical — add `{}` |
| `passedByValue` (struct params) | 44 | cppcheck | `const T&` |
| C-style casts | 39 | cppcheck | `static_cast` / `reinterpret_cast` |
| `dangerousTypeCast` (offset pointer math) | 10 | cppcheck | Proper C++ casts |
| Unused parameters | 25 | clang-tidy | `(void)param` or remove |
| Multiple declarations per statement | 15 | clang-tidy | Split |
| `readability-function-size` | 24 | clang-tidy | Most are UI (skip) |
| `cert-err33-c` (unchecked returns) | 13 | clang-tidy | Check or cast to `(void)` |
| `memsetClassFloat` | 5 | cppcheck | Aggregate init / `= {}` |
| `bugprone-*` (real bugs) | 4 | clang-tidy | Fix |
| `uninitvar` | 4 | cppcheck | Fix |
| `readability-implicit-bool-conversion` | 6 | clang-tidy | Explicit comparison |
| `readability-avoid-nested-conditional-operator` | 3 | clang-tidy | Simplify |
| `constParameterCallback` | 49 | cppcheck | Add `const` + cast function pointer |
| Lizard complexity hotspots | ~60 | lizard | Most are UI verbosity (skip) |

### Overlap Note

clang-tidy `misc-const-correctness` and cppcheck `constVariable` / `constParameterPointer` flag the same issues. Fixing once satisfies both tools.

### False Positive: `unreadVariable`

126 of 127 `unreadVariable` warnings are `nlohmann_json_default_obj` — an internal variable created by the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro. Suppress with `--suppress=unreadVariable:*effect_serialization.cpp` in cppcheck invocation.

## Strategy

### Batch Order

Fix in this order to maximize impact and minimize conflicts:

**Wave 1 — Dead code removal** (~135 items)
1. Delete all `ConfigDefault()` — 112 functions across ~80 effect files (declaration in `.h`, definition in `.cpp`)
2. Delete all `*BeginTrailMapDraw` / `*EndTrailMapDraw` — 8 functions across 4 simulation files
3. Delete 15 other dead functions: `ApplyEnvelopeDefault`, `BeatDetectorGetBeat`, `BeatDetectorGetIntensity`, `DrawGlow`, `DrawableHasType`, `DrawableValidate`, `IntensityToggleButton`, `ModEngineGetOffset`, `ModEngineHasRoute`, `ModulatableDrawableSliderPercent`, `PresetDefault`, `ProfilerUninit`, `SliderDrawInterval`, `SliderFloatWithTooltip`, `SpatialHashResize`
4. Fix the 1 real `unreadVariable` (`padY` in `imgui_playlist.cpp`)

**Wave 2 — Mechanical const fixes** (~300 items)
- Add `const` to local variables and pointer parameters flagged by both tools
- File-by-file sweep — these are low-risk, high-volume changes

**Wave 3 — Braces and declarations** (~60 items)
- Add `{}` around single-statement `if`/`else`/`for`/`while` bodies
- Split multi-declarations to one-per-line

**Wave 4 — C++ modernization** (~90 items)
- Convert C-style casts to `static_cast<T*>()` (for `malloc`/`calloc` `void*` returns)
- Convert dangerous offset casts to `reinterpret_cast`
- Change `passedByValue` structs (`Texture2D`, `Color`, etc.) to `const T&`

**Wave 5 — Unused parameters** (25 items)
- Add `(void)param;` where the parameter is part of a callback signature
- Remove the parameter where the function signature is under our control

**Wave 6 — Real bugs and safety** (~30 items)
- `bugprone-integer-division`, `bugprone-incorrect-roundings`, `bugprone-implicit-widening` — fix the math
- `uninitvar` — initialize
- `memsetClassFloat` — replace `memset(x, 0, sizeof(X))` with `*x = X{}`
- `cert-err33-c` — check return values or explicitly cast to `(void)`
- `dangerousTypeCast` — use `reinterpret_cast` with comment explaining the offset arithmetic

**Wave 7 — Style polish** (~10 items)
- `readability-implicit-bool-conversion` — explicit `!= nullptr` or `!= 0`
- `readability-avoid-nested-conditional-operator` — refactor nested ternaries
- `constParameterCallback` — add `const` to callback params, update function pointer casts where needed

**Skip:**
- `readability-function-size` — most are UI functions (convention says this is expected)
- Lizard complexity — most flagged functions are UI Draw/Panel functions

### Prevention: Pre-commit clang-tidy

Extend the existing `.git/hooks/pre-commit` (which currently only runs clang-format) to also run clang-tidy on staged `.cpp` files:

```bash
# After the clang-format section, add:

# --- clang-tidy on staged C++ files ---
if command -v clang-tidy.exe &> /dev/null; then
    CLANG_TIDY="clang-tidy.exe"
elif command -v clang-tidy &> /dev/null; then
    CLANG_TIDY="clang-tidy"
else
    # clang-tidy not available, skip
    exit 0
fi

COMPILE_DB="build/compile_commands.json"
if [ ! -f "$COMPILE_DB" ]; then
    echo "Warning: compile_commands.json not found, skipping clang-tidy"
    exit 0
fi

STAGED_CPP=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.cpp$')
if [ -n "$STAGED_CPP" ]; then
    TIDY_FAILED=0
    for FILE in $STAGED_CPP; do
        if [ -f "$FILE" ]; then
            OUTPUT=$($CLANG_TIDY -p build "$FILE" 2>&1 | grep 'warning:')
            if [ -n "$OUTPUT" ]; then
                echo "$OUTPUT"
                TIDY_FAILED=1
            fi
        fi
    done

    if [ $TIDY_FAILED -ne 0 ]; then
        echo ""
        echo "clang-tidy: warnings found in staged files. Fix or commit with --no-verify to skip."
        exit 1
    fi
fi
```

This only checks files being committed (~2-5s), not the whole codebase.

### Cppcheck Suppression for Macro Noise

Add to `scripts/lint.sh` cppcheck invocation:
```bash
--suppress=unreadVariable:*effect_serialization.cpp
```

### Conventions & Skill Updates Required

After the backlog is cleared:

1. **Remove `ConfigDefault()` from `add-effect` skill template** — the function is dead code; struct field defaults make it redundant
2. **Remove `ConfigDefault()` from conventions.md** — both the function naming entry and the "5-6 public functions" count
3. **Update conventions.md** to codify:
   - `const` for variables that aren't mutated
   - Braces around all control flow bodies
   - `static_cast` / `reinterpret_cast` instead of C-style casts
   - `const T&` for struct parameters passed by value

## Notes

- The const-correctness and braces fixes touch nearly every file. Commit each wave separately for reviewable diffs.
- `constParameterCallback` (49 warnings) is the trickiest category — adding `const` to callback params requires updating the function pointer typedefs/casts at registration sites. Save for last.
- The `passedByValue` fixes for raylib types (`Texture2D`, `RenderTexture2D`) may require checking if raylib's API expects by-value. Most do, but our wrapper functions don't need to.
- `ProfilerUninit` being unused suggests a missing cleanup call — verify whether the profiler leaks resources before deleting.
