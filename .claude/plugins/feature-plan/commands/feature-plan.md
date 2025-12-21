---
description: Plan a feature without implementing - explores codebase, clarifies requirements, writes plan to docs/plans/
argument-hint: Feature description
---

# Feature Planning (No Implementation)

Create a plan document for a feature. Do NOT implement anything. Output is a plan file in `docs/plans/` for use in fresh sessions.

## Core Rules

- **NO IMPLEMENTATION**: Do not write source code
- **PLANS ARE BRIEF**: Describe WHAT to build, not HOW to write every line
- **PHASES FIT IN ONE SESSION**: Each phase ~50% context or less

---

## Phase 1: Discovery

Feature request: $ARGUMENTS

1. If feature unclear, ask what problem they're solving
2. Confirm understanding
3. Determine filename: `docs/plans/<kebab-case-name>.md`

---

## Phase 2: Codebase Exploration

1. Launch 2-3 code-explorer agents in parallel targeting:
   - Similar features
   - Relevant architecture/patterns
   - Integration points

2. Read essential files they identify
3. Summarize findings briefly

---

## Phase 3: Clarifying Questions

**DO NOT SKIP.**

1. Identify underspecified aspects (scope, edge cases, preferences)
2. Present questions in an organized list
3. Wait for answers

If user says "whatever you think", give your recommendation and get confirmation.

---

## Phase 4: Write Plan

Write the plan to `docs/plans/<feature-name>.md` using this format:

```markdown
# [Feature Name]

What we're building and why. One paragraph.

## Current State

- `src/path/file.cpp:123` - what's here
- `src/path/other.h:45` - another location

## Phase 1: [Name]

**Goal**: One sentence outcome.

**Build**:
- Create/modify files with brief descriptions
- No code, just what components to make

**Done when**: Simple verification statement.

---

## Phase 2: [Name]
...
```

---

## Phase 5: Summary

Tell user:
- Plan file location
- How to use: "Start fresh session, say: implement docs/plans/<name>.md phase 1"
- Number of phases

---

## Output Constraints

- ONLY create `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT write implementation code in the plan
- Keep phases small enough for one session each
