---
name: feature-plan
description: Use when planning a new feature before implementation. Triggers on "plan", "design", "architect", or when describing a feature to build. Produces wave-structured plans for parallel execution.
---

# Feature Planning

Produce a complete specification with wave-assigned tasks for parallel execution. Plans are prompts—they contain everything an executor agent needs to implement its assigned files without asking questions.

## Core Principles

- **Plans are prompts**: Each task contains full context (types, algorithms, naming) so agents work independently
- **Wave-based parallelism**: Tasks with no file overlap run in parallel
- **Complete specs upfront**: Config structs, algorithms, parameter ranges defined before any implementation
- **No dependency ordering theater**: If verification requires everything, don't pretend phases are incremental
- **Ask clarifying questions**: Identify ambiguities and edge cases before designing
- **Understand before acting**: Read existing code patterns first

---

## Plan Document Structure

```markdown
# [Feature Name]

[One paragraph: what we're building and why]

## Specification

### Types

[Complete struct/class definitions agents will implement]

```cpp
struct FeatureConfig {
    bool enabled = false;
    float param1 = 0.5f;  // Range: 0.0-1.0, modulatable
    // ... all fields with defaults and ranges
};
```

### Algorithm (if applicable)

[Complete algorithm/shader logic with actual code, not prose]

```glsl
// UV transform, pixel operations, etc.
vec2 transformed = ...;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| param1 | float | 0.0-1.0 | 0.5 | Yes | "Parameter" |

### Constants

- Enum name: `TRANSFORM_FEATURE_NAME`
- Display name: `"Feature Name"`
- Category: `TRANSFORM_CATEGORY_X`

---

## Tasks

### Wave 1: [Foundation]

Tasks that must complete before others can start (usually creates files that get #included).

#### Task 1.1: [Name]

**Files**: `path/to/file.h`
**Creates**: [What this task produces that others need]

**Build**:
- Step-by-step implementation instructions
- Reference spec sections by name

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: [Parallel Implementation]

All tasks in this wave run simultaneously. No file overlap between tasks.

#### Task 2.1: [Name]

**Files**: `path/to/file.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Implementation instructions referencing spec

**Verify**: Compiles.

#### Task 2.2: [Name]

**Files**: `path/to/other.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Implementation instructions

**Verify**: Compiles.

[... more parallel tasks ...]

---

## Final Verification

After all waves complete:
- [ ] Build succeeds with no warnings
- [ ] [Feature-specific checks]
```

---

## Phase 1: Discovery

**Goal**: Understand what to plan

**Actions**:
1. Create todo list with all phases
2. Ask user (even if you think you understand):
   - What problem does this solve?
   - What should it do?
   - Any constraints?
3. Summarize understanding and confirm with user
4. Determine plan filename: `docs/plans/<kebab-case-name>.md`

**STOP**: Do not proceed until user confirms understanding is correct.

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

3. Store choice for Phase 3

---

## Phase 2: Research Check

**Goal**: Ensure we have the technical details

**Actions**:
1. Check `docs/research/` for existing research on this feature
2. If research exists: read it thoroughly—these contain vetted algorithms
3. If no research AND feature needs algorithm/shader work: tell user to run `/research <name>` first
4. For general features (no algorithm): proceed without research
5. **Effect Detection**: If research doc exists OR feature involves shaders/transforms/visual effects, mark as **effect plan**. Effect plans MUST follow add-effect skill structure.

**Never invent algorithms** when research documentation exists.

---

## Phase 3: Codebase Exploration

**Goal**: Understand relevant existing code and patterns

### If user chose "Multi-agent":

1. Launch 2-3 code-explorer agents in parallel (Task tool, subagent_type=code-explorer). Each agent should:
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

1. Read `docs/structure.md` for file locations
2. Find similar existing features:
   - For effects: read a similar effect's files
   - For simulations: read a similar simulation's files (header, cpp, config)
   - For UI: read similar panel
   - For general: read related code
3. Identify the file checklist:
   - For effects: use add-effect skill as template
   - For simulations: enumerate all integration points (config, init, update, resize, reset, UI, preset, params)
   - For other features: enumerate files to create/modify
4. Read all identified files thoroughly
5. Present summary of findings and patterns to user

---

## Phase 4: Clarifying Questions

**CRITICAL**: Do NOT skip this phase.

**Goal**: Resolve all ambiguities before designing

**Actions**:
1. Review codebase findings and original request
2. Identify underspecified aspects in these categories:
   - **Complexity level**: Simple/moderate/full version of the feature?
   - **Performance tradeoffs**: Quality vs speed priorities?
   - **Edge cases**: What happens at boundaries?
   - **Integration points**: How does it connect to existing systems?
   - **Scope boundaries**: What's explicitly out of scope?
   - **Design preferences**: Multiple valid approaches—which does user prefer?
3. **Present all questions to user in organized list**
4. **Wait for answers before proceeding**

If user says "whatever you think is best": provide specific recommendation and get explicit confirmation.

**STOP**: Do not proceed until user answers all questions.

---

## Phase 5: Architecture Design

**Goal**: Design implementation approach with user input

**Actions**:
1. Design 2-3 approaches based on codebase understanding:
   - **Minimal**: Smallest change, maximum reuse of existing patterns
   - **Full-featured**: Complete implementation with all bells and whistles
   - **Balanced**: Pragmatic middle ground
2. Present each approach with:
   - What it includes/excludes
   - Trade-offs
   - Your recommendation with reasoning
3. **Ask user which approach they prefer**

**STOP**: Do not proceed until user chooses an approach.

---

## Phase 6: Write Specification

**Goal**: Complete spec that agents can implement from

**Actions**:
1. Write complete type definitions (structs, enums)
2. Write algorithm/shader code if applicable
3. Define all parameters with ranges, defaults, UI labels
4. Define constants (enum names, display names, categories)

**Test**: Could an agent implement any file using ONLY this spec? If not, add more detail.

---

## Phase 7: Assign Waves

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
- Wave 2: Everything else (no file overlap between them):
  - `effect_config.h` modifications
  - `*.fs` shader creation
  - `post_effect.h/cpp` modifications
  - `shader_setup.h/cpp` modifications
  - `imgui_effects*.cpp` modifications
  - `preset.cpp` modifications
  - `param_registry.cpp` modifications

---

## Phase 8: Write Plan Document

**Goal**: Create `docs/plans/<feature-name>.md`

**Actions**:
1. Write header with overview
2. Write complete Specification section
3. Write Tasks grouped by wave
4. Each task includes:
   - **Files**: Exact paths
   - **Creates** or **Depends on**: Dependencies
   - **Build**: Step-by-step instructions referencing spec
   - **Verify**: How to confirm task is complete
5. Write Final Verification checklist

---

## Phase 9: Research Fidelity Check

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

## Phase 10: Summary

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
- Do NOT write partial specs—complete or ask for more info
- Include actual code in specs (structs, shaders), not prose descriptions

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I understand the request" | Ask anyway. Phase 1 questions are mandatory. |
| "No clarifying questions needed" | WRONG. Phase 4 is CRITICAL. Find questions. |
| "I'll figure out the struct later" | Agents need complete types. Define them now. |
| "The algorithm is straightforward" | Write the actual code. Agents shouldn't guess. |
| "These tasks depend on each other" | Check file overlap. No overlap = parallel. |
| "I'll add parameters as I go" | All parameters defined in spec. No discovery during implementation. |
| "User will tell me if something's wrong" | No. Ask upfront. That's the whole point. |
