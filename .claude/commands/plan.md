---
description: Write implementation plans in docs/plans/ with consistent structure
allowed-tools:
  - Read
  - Write
  - Glob
  - Grep
  - TodoWrite
---

# Implementation Plans

Plans in `docs/plans/` document how to implement features or changes. They serve three purposes:

1. **Future you** - Capture reasoning before you forget it
2. **Scope control** - Define done before starting
3. **AI context** - Enable Claude to continue work across sessions

## Plan Types

| Type | When | Sections |
|------|------|----------|
| **Feature** | New functionality | Goal, Current State, Algorithm, Integration, Validation |
| **Refactor** | Structural changes | Problem, Targets, Phases, Verification |
| **Research** | Investigation results | Question, Findings, Recommendations |

## Structure

### Feature Plan

```markdown
# [Feature Name]

One-sentence description linking to source research if applicable.

## Goal

What this achieves and why it matters. Reference MilkDrop, Shadertoy, or other
prior art if relevant.

## Current State

Where the codebase is today. Include `file:line` citations:

- `src/analysis/beat.cpp:47-61` extracts bass energy for kick detection
- The remaining 1017 bins go unused

## Algorithm

Technical approach with code examples. Explain the "why" for magic numbers:

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Attack time | 10ms | Captures transients within one FFT frame |
| Release time | 150ms | Prevents jitter on sustained notes |

## Integration

Where changes hook into existing code:

1. Add struct to `AppContext` (`src/main.cpp:21-40`)
2. Call from main loop after `BeatDetectorProcess()`
3. Expose to UI in panels

## File Changes

| File | Change |
|------|--------|
| `src/analysis/bands.h` | Create - struct and function declarations |
| `src/analysis/bands.cpp` | Create - implementation |
| `src/main.cpp` | Add to AppContext, call from loop |

## Validation

- [ ] Acceptance criteria as checkboxes
- [ ] Observable behavior, not implementation details
- [ ] Include performance requirements if relevant

## References (optional)

Link external resources that informed the design.
```

### Refactor Plan

```markdown
# [Refactor Name]

Problem statement with metrics (CCN, NLOC, etc.).

## Targets

| Location | Issue | Target |
|----------|-------|--------|
| `src/ui/ui_main.cpp:52` | CCN 18 | CCN < 10 |

## Phases

### Phase N: [Name]

What changes and why. Keep phases small enough to complete in one session.

## Verification

Command to verify success:

```bash
lizard ./src -C 15 --warnings_only
```
```

### Research Plan

```markdown
# [Question]

## Findings

What you learned, organized by topic.

## Recommendations

Actionable next steps, linked to potential feature plans.
```

## Rules

1. **Cite locations** - Use `file:line` format so Claude can navigate
2. **Explain magic numbers** - Future you will forget why 10ms
3. **Reference, don't duplicate** - Link to research docs for background
4. **Scope validation** - Every plan needs acceptance criteria
5. **Archive completed plans** - Move to `docs/plans/archive/`

## Writing Style

Apply technical-writing standards:

- Active voice: "Extracts RMS energy" not "RMS energy is extracted"
- Present tense for current state, future tense for planned work
- Specific verbs: "computes/renders/parses" not "handles/processes"

## When to Skip

Not everything needs a plan. Skip for:

- Single-file changes with obvious implementation
- Bug fixes with clear root cause
- Direct user requests with specific instructions

Plans add value when you might forget why, when scope could creep, or when work spans multiple sessions.
