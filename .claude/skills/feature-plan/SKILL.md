---
name: feature-plan
description: Use when planning a new feature before implementation. Triggers on "plan", "design", "architect", or when the user describes a feature they want to build but hasn't started coding.
---

# Feature Planning (No Implementation)

Systematic approach: understand codebase deeply, identify and ask about underspecified details, design architecture, write plan document. Do NOT implement anything.

## Core Principles

- **Ask clarifying questions**: Identify ambiguities and edge cases before designing
- **Understand before acting**: Read existing code patterns first
- **User controls agent usage**: Ask user to choose no-agent or multi-agent based on complexity
- **Read files identified by agents**: After agents complete, read files they identified
- **Sequential implementation phases**: Plans have no parallel execution—shared files break concurrent edits

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

## Phase 1.5: Agent Strategy Selection

**Goal**: Let user choose exploration approach

**Actions**:
1. Assess feature complexity:
   - **Simple**: Isolated change, 1-2 modules, clear patterns
   - **Moderate**: 2-3 modules, some architectural decisions
   - **Complex**: New subsystem, significant architectural work, unclear scope

2. **Ask user using AskUserQuestion**:
   - Question: "How should I explore the codebase for this feature?"
   - Options:
     - **No agents** - Direct exploration (lower token usage, good for simple/moderate)
     - **Multi-agent** - Parallel code-explorer agents (higher token usage, better for complex)
   - Provide complexity assessment and recommendation

3. Store choice for Phase 2 and Phase 4

---

## Phase 2: Codebase Exploration

**Goal**: Understand relevant existing code and patterns

### Research Docs (ALWAYS first)

1. Check `docs/research/` for related documents
2. If research exists, **read it thoroughly**—these contain vetted algorithms and implementation specifics
3. **Never invent algorithms** when research documentation exists
4. Record which research docs are relevant for Phase 6 fidelity check
5. **Effect Detection**: If research doc exists OR feature involves shaders/transforms/visual effects, mark as **effect plan**. Effect plans MUST follow add-effect skill structure.

### If user chose "Multi-agent":

1. Launch 2-3 code-explorer agents in parallel. Each agent should:
   - Trace through code comprehensively, focus on abstractions, architecture, flow
   - Target different aspects (similar features, high-level understanding, architecture)
   - Return list of 5-10 key files to read

   **Example prompts**:
   - "Find features similar to [feature] and trace through their implementation"
   - "Map the architecture and abstractions for [feature area]"
   - "Analyze the current implementation of [existing feature/area]"

2. After agents return, read all files they identified
3. Present summary of findings and patterns

### If user chose "No agents":

1. Use Glob, Grep, and Read directly:
   - Search for similar features or patterns
   - Read architecture docs (`docs/architecture.md`)
   - Trace through relevant code paths
2. Identify 5-10 key files and read them thoroughly
3. Present summary of findings and patterns

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

**Goal**: Design multiple implementation approaches

### If user chose "Multi-agent":

1. Launch 2-3 plan-architect agents in parallel with different focuses:
   - Minimal changes (smallest change, maximum reuse)
   - Clean architecture (maintainability, elegant abstractions)
   - Pragmatic balance (speed + quality)
2. Review all approaches, form opinion on which fits best
3. Present: brief summary of each, trade-offs, **your recommendation with reasoning**
4. **Ask user which approach they prefer**

### If user chose "No agents":

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

Checkpoints indicate where user can safely pause and evaluate context usage.

### Content Guidelines

- **For general code**: Describe components, don't write full implementations
- **For shaders/algorithms**: Include actual formulas, UV transforms, math. Implementer must be able to write shader from spec without guessing.
- **For effect plans**: Invoke `add-effect` skill, structure phases to match its checklist
- Always include `**Files**:` for each phase

---

## Phase 6: Research Fidelity Check

**Goal**: Verify plan faithfully represents research

**Skip if**: No research docs identified in Phase 2.

**Actions**:
1. Dispatch agent (Task tool, `subagent_type=general-purpose`) with prompt:
   - Include full text of relevant research docs from `docs/research/`
   - Include plan's `## Technical Implementation` section and any GLSL/algorithm content
   - Instruction: "Compare these documents. Report ANY: drift (formulas differ), invention (techniques not in research), omission (steps dropped), parameter mismatch (ranges/semantics differ)."

2. Agent returns either:
   - "No issues found" → proceed to summary
   - List of specific discrepancies with quotes

3. If issues found:
   - Present discrepancies to user
   - Ask: "Fix these, or intentional deviation?"
   - If fixing: update plan, re-run check once
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
