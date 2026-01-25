---
name: research
description: Use when researching a technique, algorithm, or shader effect before planning implementation. Triggers on "research", "look up", "find references for", or when the user mentions a technique name they want to understand.
---

## Existing Inventory

@docs/effects.md

**Read the inventory above. Use its categories when classifying.**

---

# Research

Research a technique before planning or implementation. Finds real references, classifies the domain type, validates compatibility, and writes a research document. Commits to one approach.

## Core Principles

- **No hallucinated implementations**: stop if references fail to load
- **One approach**: pick the best-fit source and document it. No "alternatively..." sections.
- **Honest failure reporting**: state when WebFetch fails or sources are unavailable

---

## Phase 1: Discovery

**Goal**: Understand what the user wants to build

**Actions**:
1. Create todo list with all phases
2. Parse initial request from $ARGUMENTS
3. If description is vague, ask one clarifying question (not a list)
4. Summarize understanding in one sentence and confirm

---

## Phase 2: Type Classification

**Goal**: Determine what kind of thing this is

Classify into one domain:

| Type | Characteristics | Example |
|------|----------------|---------|
| Effect (Transform) | Reads input texture, outputs modified pixels | Kuwahara, kaleidoscope |
| Effect (Feedback) | Reads previous frame, accumulates over time | Flow field, blur |
| Effect (Output) | Post-process after transforms | Chromatic aberration, bloom |
| Simulation | Per-agent or field-based compute, writes to texture | Physarum, boids, curl flow |
| Drawable | Generates geometry drawn to accumulation buffer | Waveforms, particle trails |
| General | Software/architecture, not a visual pipeline component | UI redesign, serialization |

Present classification. Ask user to confirm before proceeding.

---

## Phase 3: Research

**Goal**: Find real, fetchable references

**Actions**:
1. Search with WebSearch, prioritizing bot-friendly sources:
   - Inigo Quilez articles (iquilezles.org)
   - GPU Gems chapters
   - Catlike Coding tutorials
   - Academic papers (GDC, SIGGRAPH)
   - GitHub shader repositories
   - Godot Shaders

2. **Blocked sites** (do NOT attempt to fetch):
   - **Shadertoy**: ask user to paste shader code
   - **Wikipedia**: ask user to paste relevant section
   - Tell them: "[Site] blocks automated access. Paste the relevant content from [URL]"

3. Fetch promising URLs. For each attempt, report:
   - Fetched: URL loaded, key content
   - Blocked: bot detection, paywall, timeout
   - Not attempted: reason

4. If ALL fetches fail: **STOP**. Ask user to provide a source or paste code.

5. Check `docs/research/` for existing research on this technique.

6. **Pick one approach.** If multiple sources describe different methods, select the one that best fits this codebase's complexity level and pipeline. State why. Do not document the alternatives.

---

## Phase 4: Compatibility Gate

**Goal**: Verify the technique works in this pipeline

### Effect (Transform/Feedback/Output)
- Reads input texture? → compatible
- Ignores input, generates procedurally? → incompatible as transform
- Propose adaptation if possible, or reclassify as Drawable/Simulation

### Simulation
- Per-agent with trails? → follows Physarum/Boids pattern (compute shader + trail texture)
- Field-based (no agents)? → follows Curl Advection pattern (texture ping-pong)
- Requires 3D geometry or mesh? → incompatible

### Drawable
- Produces 2D geometry (lines, points, shapes)? → draws to accumulation buffer
- Requires depth buffer or 3D? → incompatible

### General
- No compatibility gate. Proceed.

**Ask user to approve** classification and compatibility before continuing.

---

## Phase 5: Write Research Document

**Goal**: Create `docs/research/<name>.md`

Structure by type:

### Effects
```markdown
# [Name]

[One paragraph: visual result]

## Classification

- **Category**: [From inventory, e.g. TRANSFORMS > Style]
- **Pipeline Position**: [From inventory pipeline order]

## References

- [Title](URL) - [What this source provides]

## Algorithm

[Actual math/formulas from the selected reference. Include:]
- UV transformation equations or pixel sampling logic
- GLSL snippets from the reference
- Parameter definitions with ranges

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| ... | ... | ... | ... | ... |

## Modulation Candidates

- **parameter**: what changes visually when modulated

## Notes

[Caveats, performance, edge cases]
```

### Simulations
```markdown
# [Name]

[One paragraph: what emerges from the simulation]

## Classification

- **Type**: Per-agent / Field-based
- **Compute Model**: [CPU loop / compute shader / texture ping-pong]

## References

- [Title](URL) - [What this source provides]

## Algorithm

[State update rules, neighbor queries, field equations]
- Per-step logic
- Boundary conditions
- Trail/output generation

## Parameters

| Parameter | Type | Range | Default | Effect on Behavior |
|-----------|------|-------|---------|-------------------|
| ... | ... | ... | ... | ... |

## Modulation Candidates

- **parameter**: what changes in the emergent behavior

## Notes

[Performance scaling, particle counts, stability]
```

### Drawables / General
Use the Effect template, adjusted for the domain. Skip pipeline position for General.

---

## Phase 6: Summary

1. Mark all todos complete
2. Tell user:
   - Research document location
   - "To plan implementation: `/feature-plan <name>`"

---

## Error Handling

- **All fetches blocked**: stop at Phase 3, ask for alternative sources
- **Incompatible technique**: stop at Phase 4, present options
- **Existing research found**: ask if user wants fresh research or to use existing
- **User provides reference directly**: skip WebSearch, fetch their URL

---

## Output Constraints

- ONLY create `docs/research/<name>.md`
- Do NOT write shader code or C++ implementation
- Do NOT create plan documents
- Do NOT proceed past Phase 4 without user approval
- Do NOT present alternative approaches in the research doc
