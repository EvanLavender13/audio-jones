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
1. Parse initial request
2. If unclear, ask ONE clarifying question (not a list)
3. Summarize understanding in one sentence, confirm with user
4. Determine plan filename: `docs/plans/<kebab-case-name>.md`

---

## Phase 2: Research Check

**Goal**: Ensure we have the technical details

**Actions**:
1. Check `docs/research/` for existing research on this feature
2. If research exists: read it, use as spec foundation
3. If no research AND feature needs algorithm/shader work: tell user to run `/research <name>` first
4. For general features (no algorithm): proceed without research

---

## Phase 3: Codebase Exploration

**Goal**: Find patterns to follow

**Actions**:
1. Read `docs/structure.md` for file locations
2. Find similar existing features:
   - For effects: read a similar effect's files
   - For UI: read similar panel
   - For general: read related code
3. Identify the file checklist:
   - For effects: use add-effect skill as template
   - For other features: enumerate files to create/modify

---

## Phase 4: Clarifying Questions

**Goal**: Resolve ambiguities

**Actions**:
1. List any unclear aspects (edge cases, design choices)
2. Present questions to user
3. Wait for answers before proceeding

If user says "whatever you think": recommend an approach, get explicit confirmation.

---

## Phase 5: Write Specification

**Goal**: Complete spec that agents can implement from

**Actions**:
1. Write complete type definitions (structs, enums)
2. Write algorithm/shader code if applicable
3. Define all parameters with ranges, defaults, UI labels
4. Define constants (enum names, display names, categories)

**Test**: Could an agent implement any file using ONLY this spec? If not, add more detail.

---

## Phase 6: Assign Waves

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

## Phase 7: Write Plan Document

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

## Phase 8: Summary

**Actions**:
1. Tell user:
   - Plan file location
   - Wave structure summary (e.g., "Wave 1: 1 task, Wave 2: 7 parallel tasks")
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
| "I'll figure out the struct later" | Agents need complete types. Define them now. |
| "The algorithm is straightforward" | Write the actual code. Agents shouldn't guess. |
| "These tasks depend on each other" | Check file overlap. No overlap = parallel. |
| "I'll add parameters as I go" | All parameters defined in spec. No discovery during implementation. |
