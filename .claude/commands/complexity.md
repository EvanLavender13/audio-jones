---
description: Analyze code complexity with lizard and triage results for refactoring
allowed-tools:
  - Read
  - Edit
  - Bash
  - Glob
  - Grep
  - TodoWrite
  - EnterPlanMode
---

# Code Complexity Analysis

Run lizard static analysis and triage results. Fix minor issues inline; plan major refactors.

## Thresholds

| Metric | Warning | Error | Rationale |
|--------|---------|-------|-----------|
| CCN (cyclomatic complexity) | >10 | >15 | NIST235: 10 baseline, 15 for experienced teams |
| NLOC (lines of code) | >50 | >100 | McConnell: 100+ routines no more error-prone |
| Parameters | >5 | >7 | Clean Code: 3 ideal, 5+ wrap in struct |

## Instructions

### 1. Run Analysis

Execute lizard on source files, excluding vendor code:

```bash
lizard ./src -C 10 -L 50 -a 5
```

Parse output for:
- Functions exceeding warning thresholds (review needed)
- Functions exceeding error thresholds (refactor required)

### 2. Triage Results

**No warnings**: Report clean bill of health. Stop.

**Minor issues** (1-2 functions, clear fix):
- Extract helper functions to reduce CCN
- Split long functions at logical boundaries
- Group parameters into structs where sensible
- Fix inline without planning

**Major issues** (3+ functions, architectural smell, unclear approach):
- Use `EnterPlanMode` to design refactor strategy
- Consider module boundaries and data flow
- Plan incremental changes that maintain functionality

### 3. Apply Fixes

When refactoring:
- Preserve existing behavior (no functional changes)
- Name extracted functions by what they DO, not when they run
- Keep related code together (cohesion over arbitrary splitting)
- Test after each change if tests exist

### 4. Verify

Re-run lizard after fixes to confirm thresholds pass:

```bash
lizard ./src -C 15 -L 100 -a 7 --warnings_only
```

Exit code 0 = clean.

## Reference

Thresholds derived from:
- McCabe (1976): Original CCN 10 recommendation
- NIST235: "Limits as high as 15 used successfully"
- McConnell, Code Complete: Function length research
- Martin, Clean Code: Parameter count guidelines
