---
name: lint
description: Use when running static analysis on C++ source files. Triggers on "run clang-tidy", "check for warnings", "static analysis", "complexity check", or when verifying code quality.
---

# Code Quality Analysis

Run clang-tidy static analysis and lizard complexity metrics. Report issues, fix what's straightforward, flag what needs planning.

## Core Principles

- **Two passes**: clang-tidy for bugs/style, lizard for complexity
- **Triage before fixing**: Not all warnings need action
- **UI functions are verbose**: Long `imgui_effects_*.cpp` functions are expected, not problems
- **User decides**: Present findings, let user choose what to address

---

## Phase 1: Static Analysis (clang-tidy)

**Goal**: Find bugs, performance issues, and style violations

**Actions**:
1. Run clang-tidy on project sources:
   ```bash
   clang-tidy.exe -p build ./src/**/*.cpp 2>&1 | grep -E "AudioJones\\\\src\\\\.*warning:"
   ```

2. Group findings by severity:

   | Category | Severity | Action |
   |----------|----------|--------|
   | `bugprone-*` | High | Review, fix if real bug |
   | `clang-analyzer-*` | High | May need planning |
   | `concurrency-*` | High | Needs careful review |
   | `performance-*` | Medium | Fix if straightforward |
   | `cert-*` | Medium | Review for applicability |
   | `readability-*` | Low | Fix or suppress |

3. If no warnings: report clean, proceed to Phase 2

---

## Phase 2: Complexity Analysis (lizard)

**Goal**: Identify complexity hotspots

**Actions**:
1. Run lizard on source files:
   ```bash
   lizard ./src -C 15 -L 75 -a 5
   ```

2. Parse results using thresholds:

   | Metric | Warning | Error |
   |--------|---------|-------|
   | CCN (cyclomatic complexity) | >15 | >20 |
   | NLOC (function length) | >75 | >150 |
   | Parameters | >5 | >7 |

3. **Filter expected verbosity**: Flag but don't alarm on:
   - `imgui_effects_*.cpp` functions (UI code is inherently verbose)
   - `preset.cpp` serialization functions
   - Functions with "Draw" or "Panel" in name

4. Present findings:
   ```
   ## Complexity Hotspots

   | File | Function | CCN | NLOC | Note |
   |------|----------|-----|------|------|
   | render_pipeline.cpp | RenderFrame | 23 | 180 | Review |
   | imgui_effects_warp.cpp | DrawWarpPanel | 8 | 220 | Expected (UI) |
   ```

---

## Phase 3: Present Findings

**Goal**: Give user clear picture and options

**Actions**:
1. Summarize both passes:
   - clang-tidy: N warnings (X high, Y medium, Z low)
   - lizard: N hotspots (M actionable, P expected UI verbosity)

2. List actionable items with file:line references

3. **Ask user**: "How would you like to proceed?"
   - Fix all straightforward issues
   - Fix high-severity only
   - Review individually
   - Skip fixes (informational only)

**STOP**: Do not fix without user consent.

---

## Phase 4: Apply Fixes

**Goal**: Address selected issues

**Skip if**: User chose informational only

**Actions**:
1. For clang-tidy fixes:
   - Address one category at a time
   - Preserve existing behavior
   - Add `// NOLINT(check-name) - reason` for intentional patterns

2. For complexity issues (rare - most are UI verbosity):
   - Extract helper functions if logic is genuinely tangled
   - Don't refactor just to hit line count targets

3. Verify fixes:
   ```bash
   clang-tidy.exe -p build ./src/**/*.cpp 2>&1 | grep -E "AudioJones\\\\src\\\\.*warning:" | wc -l
   ```

---

## Phase 5: Summary

**Goal**: Report what was done

**Actions**:
1. List fixes applied with file:line references
2. List suppressions added with justifications
3. List deferred items (if any)
4. Report final warning/hotspot counts

---

## Suppression Guidelines

Suppress clang-tidy warnings only when:
- Check doesn't apply to C-style code
- Intentional pattern (raylib callbacks, ImGui macros)
- Third-party API constraints

Always document:
```c
// NOLINT(readability-named-parameter) - raylib callback signature
void AudioCallback(void *buffer, unsigned int frames) { ... }
```

---

## Output Constraints

- Do NOT fix issues without user consent
- Do NOT treat UI function length as a problem
- Do NOT plan major refactors inline—use EnterPlanMode
- Do NOT suppress without justification

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I'll fix all these warnings" | User decides. Present and ask. |
| "This 200-line UI function needs refactoring" | It's ImGui. That's normal. |
| "I'll suppress this to make it pass" | Suppressions need justification. |
| "The complexity is too high" | CCN matters more than NLOC for UI code. |

---

## Reference

- clang-tidy config: `.clang-tidy` (project root)
- clang-tidy checks: https://clang.llvm.org/extra/clang-tidy/checks/list.html
- lizard: `pip install lizard`
