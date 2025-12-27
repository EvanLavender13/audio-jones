---
name: documentation-standards
description: This skill should be used when writing or updating module documentation, running /sync-architecture, or when generating architecture docs. Provides the module doc template, writing style rules, and staleness detection.
---

# Documentation Standards

Apply these standards when creating or updating module documentation in `docs/modules/`.

## Writing Style

**Voice & Tense:** Active voice, present tense. "The parser validates" not "is validated" or "will validate."

**Brevity:** 15-25 words per sentence max. One idea per paragraph. Cut words that don't teach something specific.

**Specificity:** Replace vague verbs with precise ones:
- "handles/manages/processes" → validates, parses, routes, stores, creates, deletes, filters
- "fast/slow/many" → concrete values with units (e.g., "<100ms", "up to 1000")

**Structure:**
- Lead with the action or finding
- Conditions before instructions: "To enable X, set Y" not "Set Y to enable X"
- Reference files by name only (`audio.cpp`), never line numbers—they go stale immediately

**Comments in code:** Explain WHY, not WHAT. Code shows what.

**Anti-patterns:** Never write "responsible for managing", "handles various operations", "main functionality", or "processes data as needed."

## Module Doc Template

Every module doc follows this structure:

```markdown
# [Module] Module
> Part of [AudioJones](../architecture.md)

## Purpose
[1-2 sentences: what problem this module solves]

## Files
[Bulleted list with one-line descriptions]

## Data Flow
[Mermaid diagram: entry points → transforms → exit points]

## Internal Architecture
[Prose: how components interact, key abstractions, design decisions]

## Usage Patterns
[Prose: how other modules integrate, initialization requirements]
```

### Section Guidelines

**Purpose:** State the problem solved, not implementation details. One concrete outcome.

**Files:** Each bullet: `filename.cpp` - single verb phrase describing responsibility.

**Data Flow:** Mermaid flowchart showing data transformation. Follow `architecture-diagrams` skill rules. Entry points on left, exit points on right.

**Diagram syntax constraint:** Node names cannot contain `[]` brackets—Mermaid reserves these for shape definitions. Use parentheses or spell out: `Buffer(1024)` or `Buffer 1024 frames`, not `Buffer[1024]`.

**Internal Architecture:** Explain design decisions, not API surface. Why this structure? What patterns? Trade-offs made.

**Usage Patterns:** How callers integrate. Init/Uninit order. Thread safety. Prerequisites.

## Staleness Rules

When syncing documentation against source code:

| Condition | Action |
|-----------|--------|
| Item documented but not found in code | **REMOVE** - delete from docs |
| Signature changed, semantic meaning unclear | **FLAG** - mark with `[VERIFY]` prefix |
| Accurate prose, wording differs from code comments | **PRESERVE** - don't normalize |
| New item in code, not in docs | **ADD** - document following template |
| Section empty or placeholder | **GENERATE** - fill from source analysis |

### Detection Heuristics

- Missing: Grep for documented function names, flag if zero matches
- Changed: Compare parameter counts, return types; flag semantic shifts
- Stale wording: Only flag if factually incorrect, not stylistically different

## Verification Checklist

Before finalizing module documentation:

- [ ] Purpose explains WHAT problem, not HOW solved
- [ ] Each file has exactly one responsibility listed
- [ ] Data Flow diagram has labeled arrows (data types)
- [ ] Internal Architecture explains at least one design decision
- [ ] Usage Patterns includes Init/Uninit requirements if applicable
- [ ] No vague verbs: handles, manages, processes, various, etc.
- [ ] No line numbers in code references (use file names only)

## Example: Minimal Module Doc

```markdown
# Audio Module
> Part of [AudioJones](../architecture.md)

## Purpose
Captures system audio via WASAPI loopback and stores samples in a ring buffer for downstream processing.

## Files
- `audio.h` - Public interface: init, uninit, buffer access
- `audio.cpp` - WASAPI device enumeration, capture callback, ring buffer write

## Data Flow
flowchart LR
    WASAPI[WASAPI Loopback] -->|int16 stereo| Callback
    Callback -->|int16 frames| RB[(Ring Buffer)]
    RB -->|4096 frames| Consumer[Analysis Module]

%% Legend:
%% -> data flow with payload type
%% [(name)] persistent buffer

## Internal Architecture
Ring buffer uses power-of-2 sizing (32768 samples) for fast modulo via bitmask. Single producer (capture callback) writes; multiple consumers read with independent cursors. No locks—atomic head/tail pointers ensure wait-free access.

## Usage Patterns
Call `AudioInit()` before any buffer reads. Pass device index from enumeration or -1 for default. Call `AudioUninit()` on shutdown—blocks until capture thread exits. Buffer access via `AudioGetSamples(count, dest)` copies samples without advancing read cursor; call `AudioConsume(count)` to advance.
```
