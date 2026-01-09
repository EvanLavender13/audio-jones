---
description: Research a new visual effect - finds references, classifies pipeline compatibility, gates on confirmed sources before planning
argument-hint: Effect description (e.g., "fractal zoom that tiles infinitely")
---

## Existing Effects Inventory

@docs/effects.md

---

# Research Effect

Research a new visual effect before planning or implementation. Finds real references, classifies pipeline compatibility, and requires user approval before proceeding.

## Core Principles

- **No hallucinated implementations**: If references fail to load, stop and say so
- **Classify before planning**: Determine UV transform vs procedural generation upfront
- **Gate on compatibility**: User must approve classification before any planning
- **Honest failure reporting**: Explicitly state when WebFetch fails or sources are unavailable

---

## Phase 1: Discovery

**Goal**: Understand what effect the user wants

**Context**: The effects inventory above shows existing effects and categories. Use this to identify gaps and avoid duplicating existing functionality.

**Actions**:
1. Create todo list with all phases
2. Parse initial request from $ARGUMENTS
3. If description is vague, ask user:
   - What visual result are they imagining?
   - Any reference images, videos, or Shadertoy links?
   - Which category from the injected effects inventory fits?
4. Summarize understanding and confirm with user

---

## Phase 2: Research

**Goal**: Find real, fetchable references for the technique

**Actions**:
1. Search for the technique using WebSearch, prioritizing sources that don't block bots:
   - Inigo Quilez articles (iquilezles.org)
   - GPU Gems chapters
   - Wikipedia math/graphics articles
   - Catlike Coding tutorials
   - Academic papers (GDC, SIGGRAPH)
   - GitHub shader repositories

2. Attempt WebFetch on promising URLs

3. **Critical**: For EACH fetch attempt, report status explicitly:
   - ✓ **Fetched**: URL loaded, summarize key content
   - ✗ **Blocked**: Bot detection, paywall, or timeout
   - — **Not attempted**: Skipped for reason

4. If ALL fetches fail:
   - **STOP** - do not proceed to classification
   - Tell user: "I cannot find accessible references for this technique. Options: (a) provide a reference URL you can access, (b) paste shader code or algorithm description, (c) abandon this effect"
   - Wait for user input

5. Check `docs/research/` for existing research on this technique

---

## Phase 3: Classification

**Goal**: Categorize the effect and assess pipeline compatibility

**Actions**:
1. Based on successfully fetched references, determine the effect's category:

   | Category | Core Operation | Compatible? |
   |----------|----------------|-------------|
   | **UV Transform** | `texture(input, warpedUV)` | ✓ Yes |
   | **Color Transform** | `f(texture(input, uv))` | ✓ Yes |
   | **Procedural Overlay** | Generate pattern, blend with input | ✓ With adaptation |
   | **Procedural Replace** | Generate pattern, ignore input | ✗ No |
   | **Particle Simulation** | Compute agents, trail buffer | ✓ Yes (complex) |

2. Present classification to user:

   ```
   EFFECT: [Name]
   CATEGORY: [Category from table]
   CORE OPERATION: [One sentence: what the shader computes]

   REFERENCES:
     - [URL 1]: ✓ Fetched / ✗ Blocked
     - [URL 2]: ✓ Fetched / ✗ Blocked

   COMPATIBILITY: [Explanation of why it fits or doesn't fit]
   ```

3. If **Procedural Replace** (incompatible):
   - Explain what makes the "cool version" incompatible
   - Propose alternatives: simplified UV-transform version, overlay adaptation, or different effect entirely
   - Be honest if simplified version loses the appeal (like Poincaré → fisheye)

---

## Phase 4: Gate

**Goal**: Get explicit user approval before any planning or implementation

**Actions**:
1. **Ask user using AskUserQuestion**:
   - If compatible: "Proceed to create research document for this effect?"
   - If needs adaptation: "The full version is incompatible. Accept the [simplified/overlay] version, or abandon?"
   - If incompatible: "This effect cannot work in our pipeline. Research a different effect, or abandon?"

2. **STOP and wait for user response**

3. If user rejects or abandons, mark todos complete and exit

---

## Phase 5: Write Research Document

**Goal**: Create research doc for use by `/feature-plan`

**Actions**:
1. Write to `docs/research/<effect-name>.md` using this structure:

```markdown
# [Effect Name]

[One paragraph: what this effect does visually]

## Classification

- **Category**: [UV Transform | Color Transform | Procedural Overlay | Particle Simulation]
- **Core Operation**: [What the shader computes]
- **Pipeline Position**: [Where in render order: pre-transform, reorderable, post-transform]

## References

- [Title](URL) - [Brief description of what this source provides]

## Algorithm

[The actual math/formulas from the references. Include:]
- UV transformation equations
- Key functions (GLSL snippets from references)
- Parameter definitions with ranges

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| ... | ... | ... | ... |

## Audio Mapping Ideas

[Optional: how parameters could respond to FFT/beat]

## Notes

[Any caveats, edge cases, or implementation considerations]
```

2. Include actual formulas and code snippets from fetched references
3. Do NOT invent algorithms - only document what references provide

---

## Phase 6: Summary

**Goal**: Direct user to next step

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Research document location
   - "To plan implementation: `/feature-plan <effect-name>`"
   - Key considerations from research

---

## Error Handling

- **All fetches blocked**: Stop at Phase 2, ask user for alternative sources
- **Effect is procedural generation**: Stop at Phase 4, present alternatives
- **Existing research found**: Read it, ask if user wants fresh research or to use existing
- **User provides reference directly**: Skip WebSearch, fetch their URL

---

## Output Constraints

- ONLY create `docs/research/<effect-name>.md`
- Do NOT write shader code or C++ implementation
- Do NOT create plan documents (that's `/feature-plan`)
- Do NOT proceed past Phase 4 without explicit user approval
