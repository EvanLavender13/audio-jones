---
name: lint
description: Use when running static analysis on C++ source files. Triggers on "run clang-tidy", "check for warnings", "static analysis", "complexity check", or when verifying code quality.
---

# Code Quality Analysis

Run clang-tidy, cppcheck, and lizard. Report issues, fix what's straightforward, flag what needs planning.

## Core Principles

- **Three passes**: clang-tidy for bugs/style, cppcheck for deeper analysis, lizard for complexity
- **Triage before fixing**: Not all warnings need action
- **UI functions are verbose**: Long `imgui_effects_*.cpp` functions are expected, not problems
- **User decides**: Present findings, let user choose what to address

---

## Phase 0: Source Selection

**Goal**: Determine whether to use existing results or run fresh analysis

**Actions**:
1. **Ask user** (via AskUserQuestion): "Use existing `tidy.log` or run `./scripts/lint.sh` fresh?"
   - **Use log**: Read `tidy.log` and proceed to Phase 3 (Present Findings)
   - **Run fresh**: Execute `./scripts/lint.sh` (optionally with thread count arg), then proceed to Phase 3

**STOP**: Wait for user response before continuing.

---

## Phase 1: Static Analysis (clang-tidy)

**Goal**: Find bugs, performance issues, and style violations

**Run by**: `scripts/lint.sh` Phase 1

**Severity grouping** (for triage in Phase 3):

   | Category | Severity | Action |
   |----------|----------|--------|
   | `bugprone-*` | High | Review, fix if real bug |
   | `clang-analyzer-*` | High | May need planning |
   | `concurrency-*` | High | Needs careful review |
   | `performance-*` | Medium | Fix if straightforward |
   | `cert-*` | Medium | Review for applicability |
   | `readability-*` | Low | Fix or suppress |

---

## Phase 2: Complexity Analysis (lizard)

**Goal**: Identify complexity hotspots

**Run by**: `scripts/lint.sh` Phase 3

**Thresholds**:

   | Metric | Warning | Error |
   |--------|---------|-------|
   | CCN (cyclomatic complexity) | >15 | >20 |
   | NLOC (function length) | >75 | >150 |
   | Parameters | >5 | >7 |

**Filter expected verbosity**: Flag but don't alarm on:
   - `imgui_effects_*.cpp` functions (UI code is inherently verbose)
   - `preset.cpp` serialization functions
   - Functions with "Draw" or "Panel" in name

---

## Phase 3: Present Findings

**Goal**: Give user clear picture and options

**Actions**:
1. Summarize all passes:
   - clang-tidy: N warnings (X high, Y medium, Z low)
   - cppcheck: N findings by category
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

3. Verify fixes by running `./scripts/lint.sh` again

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

- Lint script: `scripts/lint.sh` (runs all three tools, saves to `tidy.log`)
- clang-tidy config: `.clang-tidy` (project root)
- clang-tidy checks: https://clang.llvm.org/extra/clang-tidy/checks/list.html
- cppcheck: https://cppcheck.sourceforge.io/manual.html
- lizard: `pip install lizard`
