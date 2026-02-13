---
name: feature-plan
description: Use when planning a new feature before implementation. Triggers on "plan", "design", "architect", or when describing a feature to build.
---

# Feature Planning

Specify architecture with wave-assigned tasks for parallel execution.

## Core Principles

- **Research before questions**: Read existing research docs BEFORE asking clarifying questions
- **Questions before design**: Resolve ALL ambiguities before architecture design
- **Plans specify architecture**: Types, integration points, naming, parameter ranges
- **Wave-based parallelism**: Tasks with no file overlap run in parallel
- **Decisions upfront**: Struct layouts, parameter ranges, approach locked before code

---

## Plan Document Structure

```markdown
# [Feature Name]

[One paragraph: what we're building and why]

**Research**: `docs/research/<name>.md` (if applicable)

## Design

### Types

[Struct/enum layouts: field names, types, defaults]

### Algorithm (if applicable)

[Prose or pseudocode. Inline all shader formulas and SDF functions. Key GLSL as math or code.]

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|

### Constants

- Enum name: `TRANSFORM_FEATURE_NAME`
- Display name: `"Feature Name"`
- Category: `TRANSFORM_CATEGORY_X`

---

## Tasks

### Wave 1: [Foundation]

#### Task 1.1: [Name]

**Files**: `path/to/file.h`
**Creates**: [What this task produces that others need]

**Do**: What to build, which existing code to follow as pattern

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: [Parallel Implementation]

#### Task 2.1: [Name]

**Files**: `path/to/file.cpp`
**Depends on**: Wave 1 complete

**Do**: What to build, which existing code to follow as pattern

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] [Feature-specific checks]
```

---

## Phase 1: Discovery

**Goal**: Understand what to plan

**Actions**:
1. Create todo list with all phases
2. Parse initial request from $ARGUMENTS
3. **If user provided a document path** (e.g., `/feature-plan docs/research/foo.md`):
   - Read that document immediately
   - Extract feature name and requirements
   - Determine plan filename: `docs/plans/<kebab-case-name>.md`
   - Skip to Phase 2 (document IS the spec)
4. **If no document provided**:
   - If request is unclear, ask what they want to build
   - Determine plan filename from description

---

## Phase 2: Research Check

**CRITICAL**: This phase determines whether you build from vetted research or invent approaches.

**Goal**: Load existing research BEFORE any other exploration

**Actions**:
1. If feature involves effects, simulations, generators, or drawables: invoke add-effect skill.
2. Check `docs/research/` for documents related to this feature
3. **If research exists**:
   - **READ IT NOW**. Do not proceed until you have read the full document.
   - These documents contain vetted algorithms, parameter ranges, and implementation specifics.
   - Record which research docs apply—Phase 9 verifies the plan against these.
4. **If no research AND feature needs algorithm/shader work**:
   - STOP. Tell user to run `/brainstorm <name>` first.
   - Do not invent algorithms.
5. **If no research AND feature is general** (no algorithm):
   - Proceed without research.

**STOP**: Do not proceed until research docs are read (if they exist).

---

## Phase 3: Agent Strategy Selection

**Goal**: Let user choose exploration approach based on complexity

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

**STOP**: Do not proceed until user chooses exploration approach.

---

## Phase 4: Codebase Exploration

**Goal**: Understand relevant existing code and patterns

### If user chose "Multi-agent":

1. Launch 2-3 code-explorer agents in parallel (Task tool, subagent_type=code-explorer). Each agent should:
   - Trace through code comprehensively
   - Target different aspects (similar features, architecture, abstractions)
   - Return list of 5-10 key files to read

2. After agents return, **read all files they identified**
3. Present summary of findings and patterns

### If user chose "No agents":

1. Read `docs/structure.md` for file locations
2. Find similar existing features:
   - For effects: read a similar effect's files
   - For simulations: read similar simulation's files (header, cpp, config)
   - For UI: read similar panel
   - For general: read related code
3. Identify file checklist:
   - For effects: use add-effect skill as template
   - For simulations: enumerate all integration points
   - For other features: enumerate files to create/modify
4. **Read all identified files thoroughly**
5. Present summary of findings and patterns

---

## Phase 5: Clarifying Questions

**CRITICAL**: Do NOT skip this phase.

**Goal**: Resolve ALL ambiguities before designing

**Actions**:
1. Review codebase findings, research docs, and original request
2. Identify underspecified aspects:
   - **Complexity level**: Simple/moderate/full version?
   - **Performance tradeoffs**: Quality vs speed priorities?
   - **Edge cases**: What happens at boundaries?
   - **Integration points**: How does it connect to existing systems?
   - **Scope boundaries**: What's explicitly out of scope?
   - **Design preferences**: Multiple valid approaches—which does user prefer?
3. **Present all questions to user in organized list**
4. **Wait for answers**

If user says "whatever you think is best": provide specific recommendation and get explicit confirmation.

**STOP**: Do not proceed until user answers all questions.

---

## Phase 6: Architecture Design

**Goal**: Design implementation approach with user input

**Actions**:
1. Design 2-3 approaches based on codebase understanding:
   - **Minimal**: Smallest change, maximum reuse of existing patterns
   - **Full-featured**: Complete implementation
   - **Balanced**: Pragmatic middle ground
2. Present each approach with:
   - What it includes/excludes
   - Trade-offs
   - Your recommendation with reasoning
3. **Ask user which approach they prefer**

**STOP**: Do not proceed until user chooses an approach.

---

## Phase 7: Write Design Section

**Goal**: Capture every design decision agents need

**Actions**:
1. Define struct/enum layouts (field names, types, defaults)
2. Describe algorithm in prose or pseudocode — name each function, state what it does. For shader tasks: inline all formulas, SDF functions, and core GLSL the agent needs. The Algorithm section is the single source of truth — research docs feed into the plan, agents never consult them. Shader code is fair game here; C++ function bodies are not.
3. Define all parameters with ranges, defaults, UI labels
4. Define constants (enum names, display names, categories)

**Test**: Does this section cover every decision an agent would otherwise guess at? If yes, stop. If an agent would face an unresolved design choice, add it.

---

## Phase 8: Assign Waves

**Goal**: Group tasks for parallel execution

**Wave Assignment Rules**:

1. **Wave 1**: Tasks that CREATE files others #include
   - Config headers
   - New source files that define types

2. **Wave 2+**: Tasks that MODIFY existing files or depend on Wave 1
   - Check file overlap: same file = same wave or sequential waves
   - No overlap = can be parallel (same wave)

**For Effects** (using add-effect checklist):
- Wave 1: `*_config.h` (creates the struct)
- Wave 2: Everything else (no file overlap between them)

**Validation** (do this BEFORE writing the plan):

1. List every file path across all tasks
2. For each wave, check: does any file appear in multiple tasks?
3. If yes: merge those tasks into one, or move one to a later wave
4. Repeat until no wave has file conflicts

---

## Phase 9: Write Plan Document

**Goal**: Create `docs/plans/<feature-name>.md`

**Actions**:
1. Write header with overview
2. Write Design section (from Phase 7)
3. Write Tasks grouped by wave
4. Each task includes:
   - **Files**: Exact paths (create or modify)
   - **Creates** or **Depends on**: Dependencies
   - **Do**: What to build, which pattern to follow (e.g. "same structure as spectrum_bars.cpp"), anything non-obvious or easy to miss. Reference the Design section for types and parameters. Shader tasks say "Implement the Algorithm section" — never "implement from research doc."
   - **Verify**: How to confirm task is complete
5. Write Final Verification checklist

---

## Phase 10: Research Fidelity Check

**Skip if**: No research docs identified in Phase 2.

**Goal**: Verify plan faithfully represents research

**Actions**:
1. Dispatch agent (Task tool, subagent_type=general-purpose) with prompt:
   - "Read `docs/research/[name].md` and `docs/plans/[name].md`. Compare them. Report ANY: drift (formulas differ), invention (techniques not in research), omission (steps dropped), parameter mismatch (ranges/semantics differ)."

2. Agent returns either:
   - "No issues found" → proceed to summary
   - List of specific discrepancies with quotes

3. If issues found:
   - Present discrepancies to user
   - Ask: "Fix these, or intentional deviation?"
   - If fixing: update plan, re-run check once
   - If intentional: note with `<!-- Intentional deviation: [reason] -->`

---

## Phase 11: Summary

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Plan file location
   - Wave structure summary (e.g., "Wave 1: 1 task, Wave 2: 7 parallel tasks")
   - Whether fidelity check passed (if applicable)
   - "To implement: `/implement docs/plans/<name>.md`"

---

## Output Constraints

- ONLY create `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT write partial designs—complete or ask for more info
- Struct layouts and shader code belong in plans. C++ function bodies do not.

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I understand the request already" | Read the research doc first. Then ask questions. |
| "The research doc is just background" | WRONG. It contains the algorithm you MUST use. Read it. |
| "No clarifying questions needed" | WRONG. Phase 5 is CRITICAL. Find questions. |
| "I'll ask questions later if needed" | No. Ask upfront. That's the whole point. |
| "I'll figure out the struct later" | Agents need type layouts. Define field names and types now. |
| "Let me write the full C++ function" | Describe what it does. Agents write the code. |
| "Research exists but I'll improve it" | NO. Use the researched approach exactly. |
| "I'll reference the research doc for the algorithm" | NO. Inline the formulas. The plan is the single source of truth. |
| "I can skip the fidelity check" | NO. Phase 10 catches drift and invention. |
| "These tasks can run in parallel" | Did you check file overlap? List the files first. |
