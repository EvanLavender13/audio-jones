---
name: lint
description: Use when running static analysis on C++ source files. Triggers on "run clang-tidy", "check for warnings", "static analysis", or when verifying code quality before a commit or release.
---

# Static Analysis with clang-tidy

Run clang-tidy on source files and triage results. Fix straightforward issues inline; plan complex refactors.

## Check Categories

| Category | Purpose | Severity |
|----------|---------|----------|
| `bugprone-*` | Detect common bug patterns (null deref, infinite loops) | High |
| `clang-analyzer-*` | Deep static analysis (data flow, memory) | High |
| `performance-*` | Inefficient patterns (unnecessary copies, moves) | Medium |
| `readability-*` | Code clarity (naming, structure) | Low |
| `cert-*` | CERT C secure coding violations | Medium |
| `concurrency-*` | Thread safety issues | High |

## Instructions

### 1. Run Analysis

```bash
# Filter to project sources only (excludes third-party headers)
clang-tidy.exe -p build ./src/**/*.cpp 2>&1 | grep -E "AudioJones\\\\src\\\\.*warning:"
```

### 2. Parse Output

clang-tidy outputs in format: `file:line:col: severity: message [check-name]`

Group findings by:
- **Errors** (WarningsAsErrors in .clang-tidy): Fix immediately
- **Warnings**: Triage by check category

### 3. Triage Results

**No warnings**: Report clean analysis. Stop.

**Straightforward fixes** (clear transformation, localized):
- `bugprone-suspicious-*`: Review logic, fix if genuinely wrong
- `performance-*`: Apply suggested optimization
- `readability-implicit-bool-conversion`: Add explicit comparison
- `readability-isolate-declaration`: Split multi-var declarations
- Fix inline without planning

**Complex issues** (architectural, unclear fix, multiple locations):
- `clang-analyzer-*` memory issues: May need ownership redesign
- `concurrency-*` race conditions: Requires synchronization strategy
- Use `EnterPlanMode` to design fix approach

**False positives** (intentional code pattern):
- Add `// NOLINT(check-name)` with brief justification
- Or `// NOLINTNEXTLINE(check-name)` on line before

### 4. Apply Fixes

When fixing:
- Address one check category at a time
- Preserve existing behavior
- Test after each batch of fixes if tests exist

### 5. Verify

Re-run clang-tidy to confirm fixes:

```bash
clang-tidy.exe -p build ./src/**/*.cpp 2>&1 | grep -E "AudioJones\\\\src\\\\.*warning:" | wc -l
```

Zero = clean.

## Suppression Guidelines

Suppress only when:
- Check doesn't apply to C (some checks are C++-specific)
- Intentional pattern (e.g., raylib callback signatures)
- Third-party API constraints

Document suppressions:
```c
// NOLINT(readability-named-parameter) - raylib callback signature
void AudioCallback(void *buffer, unsigned int frames) { ... }
```

## Reference

Configuration: `.clang-tidy` (project root)
Check documentation: https://clang.llvm.org/extra/clang-tidy/checks/list.html
