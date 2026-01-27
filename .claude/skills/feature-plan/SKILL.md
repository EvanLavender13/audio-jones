---
name: feature-plan
description: Use when planning a new feature before implementation. Triggers on "plan", "design", "architect", or when the user describes a feature they want to build but hasn't started coding.
---

# Feature Planning

Systematic approach: understand codebase, clarify requirements, design architecture, write plan document. Do NOT implement anything.

## Core Principles

- **Ask clarifying questions**: Identify ambiguities and edge cases before designing
- **Understand before acting**: Read existing code patterns first
- **Linear phases**: Plans are sequential. No parallel execution—this codebase has a linear pipeline where phases touch shared files in order.
- **Checkpoints for context management**: Mark natural pause points where user can evaluate context usage

---

## Phase 1: Discovery

**Goal**: Understand what to plan

Initial request: $ARGUMENTS

**Actions**:
1. Create todo list with all phases
2. If feature unclear, ask user:
   - What problem does this solve?
   - What should it do?
   - Any constraints?
3. Summarize understanding and confirm
4. Determine plan filename: `docs/plans/<kebab-case-name>.md`

---

## Phase 2: Codebase Exploration

**Goal**: Understand relevant existing code and patterns

### Research Docs (always first)

1. Check `docs/research/` for related documents
2. If research exists, **read it thoroughly**—these contain vetted algorithms and implementation specifics
3. **Never invent algorithms** when research documentation exists
4. Record which research docs are relevant for Phase 5 fidelity check
5. **Effect Detection**: If research doc exists OR feature involves shaders/transforms/visual effects, mark as **effect plan**. Effect plans MUST follow add-effect skill structure.

### Exploration

Use Glob, Grep, and Read tools to explore:
- Search for similar features or patterns
- Read architecture docs (`docs/architecture.md`)
- Trace through relevant code paths
- Identify 5-10 key files and read them thoroughly

Present summary of findings and patterns discovered.

---

## Phase 3: Clarifying Questions

**Goal**: Resolve all ambiguities before designing

**CRITICAL**: Do NOT skip this phase.

**Actions**:
1. Review codebase findings and original request
2. Identify underspecified aspects: edge cases, error handling, integration points, scope boundaries, design preferences
3. **Present all questions to user in organized list**
4. **Wait for answers before proceeding**

If user says "whatever you think is best", provide recommendation and get explicit confirmation.

---

## Phase 4: Architecture Design

**Goal**: Design implementation approach

**Actions**:
1. Design 2-3 approaches based on codebase understanding:
   - **Minimal changes**: Smallest change, maximum reuse
   - **Clean architecture**: Better maintainability
   - **Pragmatic balance**: Speed + quality tradeoff
2. Form opinion on which fits best
3. Present: brief summary of each, trade-offs, **your recommendation with reasoning**
4. **Ask user which approach they prefer**

---

## Phase 5: Write Plan

**Goal**: Create plan document with checkpointed phases

**Actions**:
1. Take user's chosen approach from Phase 4
2. Write plan to `docs/plans/<feature-name>.md` using structure below
3. **Phases are sequential**—each builds on the previous
4. **Mark checkpoints** at natural "compiles but incomplete" boundaries
5. Keep phases small enough to complete in one sitting

### Plan Structure

```markdown
# [Feature Name]

What we're building and why. One paragraph max.

## Current State

Quick orientation—what files exist, where to hook in:
- `src/path/file.cpp:123` - brief description
- `src/path/other.h:45` - another relevant location

## Technical Implementation (shaders/algorithms only)

Include when feature involves shaders or complex algorithms.

- **Source**: Link to ShaderToy, research doc, or paper
- **Core algorithm**: Actual math/formulas, not prose
- **Parameters**: What each uniform controls, with value ranges

---

## Phase 1: [Name]

**Goal**: One sentence describing the outcome.
**Files**: `path/to/file.h`, `path/to/file.cpp`

**Build**:
- Create `path/to/new.cpp` - brief description
- Modify `path/to/existing.h` - what changes

**Verify**: Exact command(s) to run and what to observe.
Example: `cmake.exe --build build` → compiles without errors.

**Done when**: Simple acceptance statement.

---

## Phase 2: [Name]
**Files**: `path/to/other.cpp`
...

---

<!-- CHECKPOINT: Compiles, shader loads but no UI -->

## Phase 3: [Name]
...

---

## Phase N: [Name]
...

---

<!-- CHECKPOINT: Feature complete, ready for testing -->
```

### Checkpoint Guidelines

Insert `<!-- CHECKPOINT: description -->` comments at natural boundaries:

- After config + shader created (compiles, does nothing visible)
- After uniforms wired up (shader invocable but not controllable)
- After UI added (feature visible and controllable)
- After serialization + params (feature complete)

Checkpoints indicate where user can safely pause and evaluate context usage. The implementer stops at each checkpoint and reports progress.

### Content Guidelines

- **For general code**: Describe components, don't write full implementations
- **For shaders/algorithms**: Include actual formulas, UV transforms, math. Implementer must be able to write shader from your spec without guessing.
- **For effect plans**: Invoke `add-effect` skill, structure phases to match its checklist
- Always include `**Files**:` for each phase

---

## Phase 6: Research Fidelity Check

**Goal**: Verify plan faithfully represents research

**Skip if**: No research docs identified in Phase 2.

**Actions**:
1. Compare plan's Technical Implementation section against research docs
2. Check for:
   - **Drift**: Formulas that differ (sign flips, missing terms)
   - **Invention**: Approaches that appear nowhere in research
   - **Omission**: Steps dropped from multi-step algorithms
   - **Parameter mismatch**: Value ranges or semantics that differ
3. If issues found:
   - Present discrepancies to user
   - Ask: "Fix these, or intentional deviation?"
   - If fixing: update plan, re-check once
   - If intentional: note with `<!-- Intentional deviation: [reason] -->`

---

## Phase 7: Summary

**Goal**: Document what was planned

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Plan file location
   - How to use: `/implement docs/plans/<name>.md`
   - Number of phases and checkpoint locations
   - Whether fidelity check passed

---

## Output Constraints

- ONLY create `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT write C++ implementation code in plan
- **DO include**: Shader algorithms, UV math, formulas from research docs
