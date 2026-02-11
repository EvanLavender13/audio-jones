---
name: implement
description: Use when executing a feature plan with wave-based parallel agents. Triggers on "implement this plan", "start building", or when a plan exists in docs/plans/ and user wants to begin coding.
---

# Parallel Implementation

Execute a wave-structured plan by dispatching parallel agents. Each agent implements its assigned files independently using the plan's specification. Orchestrator stays lean—just coordinates and collects results.

## Core Principles

- **Fresh context per agent**: Each executor gets 100% fresh context with full spec
- **Parallel within waves**: All tasks in a wave run simultaneously
- **Orchestrator stays lean**: ~15% context for coordination, agents do the work
- **Single verification**: Build and test once after all waves complete

---

## CRITICAL: Orchestrator File Access Rules

**The orchestrator reads ONLY these files:**
1. The plan document (once, at start)
2. Build error output (after failed builds)

**The orchestrator NEVER reads:**
- Source files (.cpp, .h, .c)
- Shader files (.fs, .glsl)
- Any file in `src/`, `shaders/`, or other code directories
- Files "to understand context" or "see what exists"

**Why**: The plan contains the complete specification. Agents have fresh context to read codebase files. If the orchestrator reads codebase files, it wastes context that should go to coordination.

**If you think you need to read a source file**: You don't. The plan has the spec. Dispatch the agent.

---

## Execution Flow

```
1. Load plan
2. Parse waves and tasks
3. For each wave:
   a. Dispatch agents in parallel (one Task call per task)
   b. Wait for all to complete
   c. Build to verify wave
4. Code review
5. Final verification
6. Commit
```

---

## Phase 1: Load Plan

**Goal**: Parse the plan document (this is the ONLY file you read before dispatching agents)

**Actions**:
1. Read the plan file from $ARGUMENTS
2. Extract:
   - Specification section (types, algorithm, parameters, constants)
   - Tasks grouped by wave
   - Final verification checklist
3. Count waves and tasks per wave
4. Report structure to user: "Plan has N waves, M total tasks"
5. **Immediately proceed to Phase 2** — do NOT read any other files

---

## Phase 2: Create Feature Branch

**Goal**: Isolate work

**Actions**:
1. Create branch from plan filename: `git checkout -b <kebab-case-from-filename>`
2. If branch exists: `git checkout <branch-name>`

---

## Phase 3: Execute Waves

**Goal**: Run tasks with maximum parallelism

For each wave in order:

### 3a. Prepare Agent Prompts

The plan contains the complete specification — file names, field names, line numbers, exact changes. Agents read their own target files with fresh context. The orchestrator pastes the plan's task section into the prompt verbatim and dispatches. Do NOT pre-read files to "enrich" agent prompts.

**If a task's Do section references an external document for algorithm content** (e.g., "from the research doc," "as specified in docs/research/"): **STOP**. Tell user: "Task [name] references [doc] for algorithm details — the plan should inline these. Run `/feature-plan` to fix." Do not proceed. Do not summarize the referenced document.

For each task in the wave, build a prompt containing:

```markdown
# Task: [Task Name]

## Your Assignment

Implement the following files: [file list]

## Specification

[PASTE FULL SPEC SECTION FROM PLAN]

## Task Instructions

[PASTE BUILD SECTION FROM THIS TASK]

## Verification

[PASTE VERIFY SECTION FROM THIS TASK]

## Rules

- Read the files you need to modify BEFORE making changes
- Implement ONLY the files listed above
- Follow the specification exactly
- Use the codebase conventions from CLAUDE.md
- Do NOT modify files outside your assignment
- Do NOT run cmake or any build commands (orchestrator handles builds)
- Do NOT run the application
- Report completion with list of files modified
```

### 3b. Dispatch Parallel Agents

**CRITICAL**: Spawn all tasks in a wave with a SINGLE message containing multiple Task calls:

```
Task(prompt="[task 1 prompt]", subagent_type="general-purpose", description="Implement [file1]")
Task(prompt="[task 2 prompt]", subagent_type="general-purpose", description="Implement [file2]")
Task(prompt="[task 3 prompt]", subagent_type="general-purpose", description="Implement [file3]")
```

All agents run in parallel. Tool blocks until all complete.

**Do NOT**:
- Use background agents
- Poll for completion
- Run agents sequentially within a wave

### 3c. Collect Results

After all agents return:
1. List files each agent modified
2. Note any issues reported

### 3d. Build Verification

After each wave:
```bash
cmake.exe --build build
```

- If build succeeds: proceed to next wave
- If build fails: diagnose and fix before continuing

---

## Phase 4: Code Review

**Goal**: Catch correctness and consistency issues before the user runs the feature

**Actions**:
1. Launch 1 `code-reviewer` agent covering all focus areas
2. Agent receives: plan path, focus on both correctness and consistency (code should read like one author, not N agents)
3. If issues reported: fix them directly, re-build
4. If clean: proceed to Phase 5

---

## Phase 5: Final Verification

**Goal**: Confirm feature works

**Actions**:
1. Run each check from the plan's Final Verification section
2. Report results to user
3. If visual verification needed: tell user what to look for

---

## Phase 6: Commit

**Goal**: Record the work

**Actions**:
1. Stage all modified files (list them explicitly, no `git add .`)
2. Commit with message describing the feature
3. Include co-author line

---

## Wave Execution Details

### File Ownership

Each task OWNS its files exclusively. No two tasks in the same wave modify the same file.

If a plan has file overlap within a wave: **the plan is wrong**. Stop and report to user.

### Agent Context

Each agent receives:
- Full specification from the plan (types, algorithm, parameters)
- Its specific task instructions
- Verification criteria

Agents do NOT:
- Run cmake or build commands (orchestrator builds after wave completes)
- Run the application
- Receive other tasks' instructions
- Know about other agents' progress
- Receive the full plan document (too much context)

### Deviation Handling

Agents may discover work not in the plan. Apply these rules:

**Auto-fix (no permission needed)**:
- **Bugs**: Logic errors, type errors, crashes → fix immediately
- **Missing critical**: Error handling, validation, null checks → add immediately
- **Blocking issues**: Missing dependencies, broken imports → fix to unblock

**Ask user**:
- **Architectural changes**: New database tables, schema changes, new services → STOP and ask

Track all deviations for the commit message.

### Error Handling

**Agent reports error**:
1. Read the error
2. If fixable: fix in orchestrator context
3. If architectural: stop and ask user

**Build fails after wave**:
1. Read error output
2. Identify which file(s) caused it
3. Fix directly or dispatch fix agent
4. Re-build before proceeding

---

## Example: Effect Implementation

Plan structure:
- Wave 1: Create config header (1 task)
- Wave 2: All other files (7 tasks in parallel)

Execution:
```
Wave 1: Dispatch 1 agent → config header created → build succeeds
Wave 2: Dispatch 7 agents simultaneously:
  - Agent A: effect_config.h modifications
  - Agent B: shader creation
  - Agent C: post_effect.h/cpp
  - Agent D: shader_setup.h/cpp
  - Agent E: imgui_effects*.cpp
  - Agent F: preset.cpp
  - Agent G: param_registry.cpp
All 7 complete → build → final verification → commit
```

Total: 2 build cycles, not 8.

---

## Output Constraints

- Do NOT execute tasks sequentially when they can be parallel
- Do NOT skip wave verification builds
- Do NOT modify files outside the plan's scope
- Do NOT commit partial implementations

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "Let me read the source files first" | NO. The plan has the spec. Dispatch agents immediately. |
| "I need to understand the codebase" | NO. Agents understand. You coordinate. Read ONLY the plan. |
| "I'll check what's in this file" | NO. You are burning context. Dispatch the agent. |
| "I'll just do these tasks myself" | Dispatch agents. Fresh context = better quality. |
| "I'll run them one at a time to be safe" | Parallel is safe if no file overlap. Check the plan. |
| "The build failed, I'll fix it later" | Fix now. Don't proceed with broken build. |
| "I'll skip the small tasks" | All tasks matter. Dispatch them all. |
| "This agent's work looks wrong" | Check against spec. If spec is right, agent is probably right. |
| "The task says 'from research doc' — I'll include the algorithm" | STOP. The plan is incomplete. Tell the user to fix it. |
