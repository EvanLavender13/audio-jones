---
name: feature-plan
description: Use when planning a new feature before implementation. Triggers on "plan", "design", "architect", or when the user describes a feature they want to build but hasn't started coding.
---

# Feature Planning (No Implementation)

You are helping a developer plan a new feature. Follow a systematic approach: understand the codebase deeply, identify and ask about all underspecified details, design architecture, then write a plan document. Do NOT implement anything.

## Core Principles

- **Ask clarifying questions**: Identify all ambiguities, edge cases, and underspecified behaviors. Ask specific, concrete questions rather than making assumptions. Wait for user answers before proceeding.
- **Understand before acting**: Read and comprehend existing code patterns first
- **User controls agent usage**: Ask user to choose between no-agent or multi-agent exploration based on complexity and token budget
- **Read files identified by agents**: When using agents, ask them to return lists of the most important files to read. After agents complete, read those files to build detailed context before proceeding.
- **Use TodoWrite**: Track all progress throughout

---

## Phase 1: Discovery

**Goal**: Understand what needs to be planned

Initial request: $ARGUMENTS

**Actions**:
1. Create todo list with all phases
2. If feature unclear, ask user for:
   - What problem are they solving?
   - What should the feature do?
   - Any constraints or requirements?
3. Summarize understanding and confirm with user
4. Determine plan filename: `docs/plans/<kebab-case-name>.md`

---

## Phase 1.5: Agent Strategy Selection

**Goal**: Let user choose exploration approach based on complexity and usage budget

**Actions**:
1. Assess feature complexity:
   - **Simple**: Isolated change, touches 1-2 modules, clear existing patterns to follow
   - **Moderate**: Crosses 2-3 modules, some architectural decisions needed
   - **Complex**: New subsystem, significant architectural work, or unclear scope

2. **Ask user using AskUserQuestion**:
   - Question: "How should I explore the codebase for this feature?"
   - Options:
     - **No agents** - Direct exploration (lower token usage, good for simple/moderate features)
     - **Multi-agent** - Parallel code-explorer agents (higher token usage, better for complex features)
   - Provide your complexity assessment and recommendation

3. Store user's choice for Phase 2 and Phase 4

---

## Phase 2: Codebase Exploration

**Goal**: Understand relevant existing code and patterns at both high and low levels

### Research Docs (ALWAYS do this first)

**Actions**:
1. Check `docs/research/` for any documents related to this feature
2. If research exists, **read it thoroughly** - these contain vetted algorithms, ShaderToy references, and implementation specifics
3. **Never invent algorithms** when research documentation exists. Use the researched approach.
4. Record which research docs are relevant — Phase 6 (Fidelity Check) compares these against the plan
5. **Effect Detection**: If research doc exists OR feature involves shaders/transforms/visual effects, mark this as an **effect plan**. Effect plans MUST follow the add-effect skill structure in Phase 5.

### If user chose "Multi-agent":

**Actions**:
1. Launch 2-3 code-explorer agents in parallel. Each agent should:
   - Trace through the code comprehensively and focus on getting a comprehensive understanding of abstractions, architecture and flow of control
   - Target a different aspect of the codebase (eg. similar features, high level understanding, architectural understanding, etc)
   - Include a list of 5-10 key files to read

   **Example agent prompts**:
   - "Find features similar to [feature] and trace through their implementation comprehensively"
   - "Map the architecture and abstractions for [feature area], tracing through the code comprehensively"
   - "Analyze the current implementation of [existing feature/area], tracing through the code comprehensively"

2. Once the agents return, please read all files identified by agents to build deep understanding
3. Present comprehensive summary of findings and patterns discovered

### If user chose "No agents":

**Actions**:
1. Use Glob, Grep, and Read tools directly to explore:
   - Search for similar features or patterns
   - Read architecture docs (`docs/architecture.md`)
   - Trace through relevant code paths manually
2. Identify 5-10 key files and read them thoroughly
3. Present comprehensive summary of findings and patterns discovered

---

## Phase 3: Clarifying Questions

**Goal**: Fill in gaps and resolve all ambiguities before designing

**CRITICAL**: This is one of the most important phases. DO NOT SKIP.

**Actions**:
1. Review the codebase findings and original feature request
2. Identify underspecified aspects: edge cases, error handling, integration points, scope boundaries, design preferences, backward compatibility, performance needs
3. **Present all questions to the user in a clear, organized list**
4. **Wait for answers before proceeding to architecture design**

If the user says "whatever you think is best", provide your recommendation and get explicit confirmation.

---

## Phase 4: Architecture Design

**Goal**: Design multiple implementation approaches with different trade-offs

### If user chose "Multi-agent":

**Actions**:
1. Launch 2-3 plan-architect agents in parallel with different focuses: minimal changes (smallest change, maximum reuse), clean architecture (maintainability, elegant abstractions), or pragmatic balance (speed + quality)
2. Review all approaches and form your opinion on which fits best for this specific task (consider: complexity, scope, team context)
3. Present to user: brief summary of each approach, trade-offs comparison, **your recommendation with reasoning**, concrete implementation differences
4. **Ask user which approach they prefer**

### If user chose "No agents":

**Actions**:
1. Design 2-3 approaches yourself based on codebase understanding:
   - **Minimal changes**: Smallest change, maximum reuse of existing code
   - **Clean architecture**: Better maintainability, elegant abstractions
   - **Pragmatic balance**: Speed + quality tradeoff
2. Form your opinion on which fits best (consider: complexity, scope, team context)
3. Present to user: brief summary of each approach, trade-offs comparison, **your recommendation with reasoning**, concrete implementation differences
4. **Ask user which approach they prefer**

---

## Phase 5: Write Plan

**Goal**: Create the plan document with pre-computed execution schedule

**Actions**:
1. Take the user's chosen approach from Phase 4
2. **Prefer vertical slices**: Group phases by independent feature slices (e.g., "config + registration", "shader", "UI + serialization") rather than horizontal layers.
3. Write the plan to `docs/plans/<feature-name>.md` using this structure:

```markdown
# [Feature Name]

What we're building and why. One paragraph max.

## Current State

Quick orientation - what files exist, where to hook in:
- `src/path/file.cpp:123` - brief description
- `src/path/other.h:45` - another relevant location

## Technical Implementation (shaders/algorithms only)

Include this section when the feature involves shaders or complex algorithms.

- **Source**: Link to ShaderToy, research doc, or paper
- **Core algorithm**: The actual math/formulas, not prose descriptions
- **UV transformations**: Exact equations if applicable
- **Parameters**: What each uniform controls, with value ranges

## Phase 1: [Name]

**Goal**: One sentence describing the outcome.
**Depends on**: — *(none, or comma-separated phase numbers)*
**Files**: `path/to/file.h`, `path/to/file.cpp`

**Build**:
- Create `path/to/new.cpp` - brief description
- Modify `path/to/existing.h` - what changes
- No code, just what components to make

**Verify**: Exact command(s) to run and what to observe.
Example: `cmake.exe --build build && ./build/AudioJones.exe` → panel shows X.

**Done when**: Simple acceptance statement.

---

## Phase 2: [Name]
**Depends on**: Phase 1
**Files**: `path/to/other.cpp`
...

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1, Phase 2 | — |
| 2 | Phase 3, Phase 4, Phase 5 | Wave 1 |
| 3 | Phase 6 | Wave 2 |

Phases in the same wave execute as parallel subagents. Each wave completes before the next begins.
```

4. Keep phases small enough to complete in one session each
5. For general code: describe components, don't write full implementations
6. **For shaders and algorithms**: Include the actual formulas, UV transformations, and math. Reference the specific ShaderToy or research source. The implementer must be able to write the shader from your specification without guessing.
7. **For effect plans**: Invoke the `add-effect` skill using the Skill tool, then structure phases to match its checklist.
8. **Dependencies and files**: Always include `**Depends on**:` and `**Files**:` for each phase. Use `—` for phases with no dependencies.
9. **Compute the Execution Schedule** (always, regardless of whether plan-architect was used):
   a. Parse each phase's `**Depends on**:` into predecessor sets
   b. Parse each phase's `**Files**:` into file sets
   c. Assign waves via topological sort:
      - Wave 1: phases with no dependencies
      - Wave N: phases whose dependencies all resolve in waves < N
   d. **File conflict check**: If two phases in the same wave share any file path, bump the later phase to the next wave
   e. Write the `## Execution Schedule` table at the end of the plan document
   f. The table is the single source of truth for wave assignments

---

## Phase 6: Research Fidelity Check

**Goal**: Verify the plan faithfully represents the research, with no hallucinated or drifted content

**Skip condition**: No research docs were identified in Phase 2. Proceed directly to Phase 7.

**Actions**:
1. Dispatch a fresh agent (Task tool, `subagent_type=general-purpose`, `allowed_tools=["Read", "Glob", "Grep"]`) with this prompt:
   - Include the full text of all relevant research docs from `docs/research/`
   - Include the plan's `## Technical Implementation` section and any GLSL/algorithm content from phase descriptions
   - Instruction: "Compare these two documents. Report ANY of the following issues:"

2. The agent checks for:
   - **Drift**: Formulas, equations, or UV transforms that differ between research and plan (even subtly—sign flips, missing terms, reordered operations)
   - **Invention**: Techniques, algorithm names, or approaches in the plan that appear nowhere in the research sources
   - **Omission**: Multi-step algorithms where steps were dropped, loops reduced, or conditionals removed
   - **Parameter mismatch**: Value ranges, types, or semantics that differ between research and plan
   - **Source confusion**: Content attributed to one reference that actually came from a different one (or from none)

3. The agent returns one of:
   - **"No issues found"** — proceed to summary
   - **List of specific discrepancies** with quotes from both documents

4. If issues found:
   - Present the discrepancies to the user
   - Ask: "Should I fix these in the plan, or are the changes intentional?"
   - If fixing: update the plan file, then re-run this check once more
   - If intentional: note the deliberate deviations in the plan with a `<!-- Intentional deviation from research: [reason] -->` comment

---

## Phase 7: Summary

**Goal**: Document what was planned

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Plan file location
   - How to use: `/implement docs/plans/<name>.md`
   - Paste the `## Execution Schedule` table from the plan
   - Whether fidelity check passed clean or had deviations

---

## Output Constraints

- ONLY create `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT write general C++ implementation code in the plan
- **DO include**: Shader algorithms, UV math, formulas from research docs. These ARE required - the implementer cannot guess shader logic.
