---
name: brainstorm
description: Use when exploring a vague idea, finding inspiration, or researching a technique before planning. Triggers on "I want to add...", "what if...", "research", "look up", or when uncertain about what to build.
---

# Brainstorm & Research

Turn a vague idea into a researched concept through collaborative dialogue. Explore together, then fetch real references and write a research document.

## Core Principles

- **Explore, don't eliminate**: "Both" is a valid answer. Build up constraints, don't force binary choices.
- **One question at a time**: Multiple choice preferred. Open-ended when needed.
- **Propose approaches with recommendations**: Present 2-3 options, lead with your pick and why.
- **No hallucinated implementations**: Real references or stop.
- **One approach in the output**: Pick the best fit. No "alternatively..." sections.
- **Look for parameter synergies**: Don't just list what wiggles. Identify where parameters *interact* to create emergent dynamics — gating, tension, resonance between modulated values.

---

## Phase 1: Orient

**Goal**: Understand starting point and context

**Actions**:
1. Read the effects inventory: @docs/effects.md
2. Check `docs/research/` for existing research on this topic
3. If $ARGUMENTS is blank: ask what visual quality or mood they want to explore
4. If $ARGUMENTS has an idea: restate it in one sentence

---

## Phase 2: Explore

**Goal**: Build up understanding through dialogue

**Actions**:
1. Ask questions one at a time. Prefer multiple choice (2-4 options) when possible.
2. Frame questions as "and/or" not "either/or" when both could apply:
   - "Geometric, organic, or a blend?"
   - "Subtle enhancement or dramatic transformation?"
   - "Sharp/crisp or soft/diffused?"
3. Accept compound answers. "Both" or "somewhere between" are progress, not failure to narrow.
4. After 3-5 exchanges, summarize what you've learned about:
   - Visual goal (what it looks like)
   - Motion behavior (how it animates over time, if at all)
   - Constraints (what it must or must not do)
5. As the design takes shape, actively look for **parameter interaction patterns** — places where two or more parameters create emergent dynamics when modulated together:
   - **Cascading thresholds**: Parameter A must reach a level before Parameter B produces visible output. Different audio bands gate each other — sections of a song unlock visual layers that quieter sections hide entirely. (Example: constellation wave amplitude gates triangle fill visibility.)
   - **Competing forces**: Two parameters push in opposite directions. The visual result shows their tension — neither dominates, and the balance shifts with the music.
   - **Resonance/alignment**: Two parameters amplify each other when they coincide. Peaks only appear when both values spike together, creating rare bright moments.
   These patterns make modulation produce song-reactive dynamics (verse looks different from chorus) rather than static or cyclical visuals. Note promising interactions for the research document.

**Note**: Skip audio reactivity questions—all parameters expose to modulation engine. Users configure their own routes. The interaction patterns above are about *structural relationships between parameters*, not about which audio source maps where.

**STOP**: Do not proceed until concept direction is clear enough to research.

---

## Phase 3: Classify

**Goal**: Determine domain type for research

**Actions**:
1. Classify into one domain:

| Type | Characteristics | Example |
|------|----------------|---------|
| Effect (Transform) | Reads input texture, outputs modified pixels | Kuwahara, kaleidoscope |
| Effect (Feedback) | Reads previous frame, accumulates over time | Flow field, blur |
| Effect (Output) | Post-process after transforms | Chromatic aberration, bloom |
| Simulation | Per-agent or field-based compute, writes to texture | Physarum, boids, curl flow |
| Drawable | Generates geometry drawn to accumulation buffer | Waveforms, particle trails |
| General | Software/architecture, not a visual pipeline component | UI redesign, serialization |

2. Present classification with one sentence explaining why.

**STOP**: Do not proceed until user confirms classification.

---

## Phase 4: Research

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

5. **Pick one approach.** If multiple sources describe different methods, select the one that best fits this codebase's complexity level and pipeline. State why. Do not document alternatives.

---

## Phase 5: Compatibility Gate

**Goal**: Verify the technique works in this pipeline

### Effect (Transform/Feedback/Output)
- Reads input texture? Compatible.
- Ignores input, generates procedurally? Incompatible as transform. Propose adaptation or reclassify.

### Simulation
- Per-agent with trails? Follows Physarum/Boids pattern (compute shader + trail texture).
- Field-based (no agents)? Follows Curl Advection pattern (texture ping-pong).
- Requires 3D geometry or mesh? Incompatible.

### Drawable
- Produces 2D geometry (lines, points, shapes)? Draws to accumulation buffer.
- Requires depth buffer or 3D? Incompatible.

### General
- No compatibility gate. Proceed.

**STOP**: Do not proceed until user approves classification and compatibility.

---

## Phase 6: Approach Selection

**Goal**: Present implementation approaches for user choice

**Actions**:
1. Based on research, propose 2-3 approaches:
   - **Minimal**: Smallest viable version, maximum reuse
   - **Full**: Complete implementation of the technique
   - **Balanced**: Pragmatic middle ground (if applicable)

2. For each approach, describe:
   - What it includes/excludes
   - Trade-offs (complexity, performance, visual fidelity)

3. Lead with your recommendation and explain why.

**STOP**: Do not proceed until user chooses an approach.

---

## Phase 7: Write Research Document

**Goal**: Create `docs/research/<name>.md`

**Actions**:
1. Write document following the structure below (by type)
2. Include ONLY the chosen approach—no alternatives section

### Effects
```markdown
# [Name]

[One paragraph: visual result]

## Classification

- **Category**: [From inventory, e.g. TRANSFORMS > Style]
- **Pipeline Position**: [From inventory pipeline order]
- **Chosen Approach**: [Minimal/Balanced/Full] - [one sentence why]

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

## Modulation Candidates

- **parameter**: what changes visually when modulated

### Interaction Patterns

Identify parameter pairs that create emergent dynamics when modulated together. Not every effect has these — only document genuine structural relationships.

- **Cascading Threshold**: [param A] gates [param B] — B only produces visible output when A exceeds a level
- **Competing Forces**: [param A] and [param B] oppose each other — visual result shows their shifting balance
- **Resonance**: [param A] and [param B] amplify each other when both peak — creates rare bright moments

If no meaningful interactions exist, omit this section rather than forcing it.

**DO NOT prescribe audio sources or mapping recommendations. Users configure their own modulation routes.**

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
- **Chosen Approach**: [Minimal/Balanced/Full] - [one sentence why]

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

## Modulation Candidates

- **parameter**: what changes in the emergent behavior

### Interaction Patterns

Identify parameter pairs that create emergent dynamics when modulated together. Not every effect has these — only document genuine structural relationships.

- **Cascading Threshold**: [param A] gates [param B] — B only produces visible output when A exceeds a level
- **Competing Forces**: [param A] and [param B] oppose each other — visual result shows their shifting balance
- **Resonance**: [param A] and [param B] amplify each other when both peak — creates rare bright moments

If no meaningful interactions exist, omit this section rather than forcing it.

## Notes

[Performance scaling, particle counts, stability]
```

### Drawables / General
Use the Effect template, adjusted for the domain. Skip pipeline position for General.

---

## Phase 8: Summary

**Actions**:
1. Tell user:
   - Research document location
   - Classification and chosen approach
   - "To plan implementation: `/feature-plan docs/research/<name>.md`"

---

## Output Constraints

- ONLY create `docs/research/<name>.md`
- Do NOT create plan documents
- Do NOT proceed past gates without user approval
- Do NOT present alternative approaches in the research doc
- Do NOT hallucinate algorithms—real references or stop

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "They need to pick one or the other" | "Both" is valid. Explore the combination. |
| "I'll skip exploration, the idea is clear" | Phase 2 builds shared understanding. Do it. |
| "I can invent the algorithm" | Real references only. Ask for sources if fetches fail. |
| "I'll include multiple approaches in the doc" | One approach. The chosen one. |
| "This technique is obviously compatible" | Run the compatibility gate anyway. |
| "I know what they want" | Ask. One question at a time. |
