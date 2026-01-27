---
name: implement
description: Use when executing a feature plan phase-by-phase. Triggers on "implement this plan", "start building", or when a plan document exists in docs/plans/ and the user wants to begin coding.
---

# Phased Implementation with Progress Tracking

You are implementing a feature plan phase-by-phase. Track progress in a companion file, implement one phase per invocation, and proactively manage context for smooth session handoffs. When phases declare dependencies, detect parallelizable waves and dispatch concurrent agents.

## Core Principles

- **One phase per invocation** (sequential mode) or **one wave per invocation** (parallel mode)
- **Progress persistence**: Update `.progress.md` file after each phase/wave
- **Context awareness**: Monitor usage and generate handoff before 50% threshold
- **Incremental commits**: Commit after each successful phase
- **Wave detection**: Phases with `**Depends on**:` and `**Files**:` metadata enable parallel execution

---

## Phase 1: Setup

**Goal**: Load plan, determine current phase, prepare for implementation

**Actions**:
1. Parse arguments from $ARGUMENTS:
   - First argument: plan path (required)
   - Second argument: phase number (optional override)
2. Read the plan file
3. Parse plan structure to identify all phases (look for `## Phase N:` headers)
4. Determine progress file path: same as plan but with `.progress.md` suffix
   - Example: `docs/plans/feature.md` → `docs/plans/feature.progress.md`
5. Read progress file if it exists, otherwise create initial structure
6. **Create feature branch** (first run only):
   - If no progress file existed, create and checkout a new branch
   - Branch name: kebab-case from plan filename (e.g., `docs/plans/my-feature.md` → `my-feature`)
   - Run: `git checkout -b <branch-name>`
   - If branch already exists, checkout: `git checkout <branch-name>`
7. **Detect execution mode** (wave detection):
   - Look for `## Execution Schedule` table in the plan
   - If table exists → **parallel mode** (read wave assignments directly from the table)
   - If no table but ALL incomplete phases have `**Depends on**:` and `**Files**:` → **parallel mode** (compute waves via Wave Computation below)
   - Otherwise → **sequential mode**
8. Determine target:
   - **Sequential**: If phase number provided in args, use that. Otherwise find first incomplete phase.
   - **Parallel**: Find the first wave containing incomplete phases.
   - If all phases complete, inform user and exit
9. Create todo list with: Setup, Implement [Phase N | Wave N], Build, Update Progress, Commit

---

## Wave Computation (fallback — only when plan lacks `## Execution Schedule`)

**Goal**: Group independent phases into concurrent execution waves

**Algorithm**:
1. Parse each phase's `**Depends on**:` field into a set of predecessor phase numbers
   - `—` or empty means no dependencies
   - `Phase 1` or `Phase 1, Phase 2` means those phases must complete first
2. Parse each phase's `**Files**:` field into a set of file paths
3. Compute waves via topological sort:
   - Wave 1: All phases with no dependencies (or dependencies already completed)
   - Wave 2: All phases whose dependencies are all in Wave 1 or earlier
   - Wave N: All phases whose dependencies are all in waves < N
4. **File conflict check**: Within each wave, verify no two phases share files in their `**Files**:` lists. If conflicts exist, split the conflicting phase into the next wave.
5. Record the wave assignments in the progress file frontmatter:
   ```
   waves:
     1: [1, 2]
     2: [3, 5, 6]
     3: [4]
   ```

**Example** (typical effect plan):
```
Phase 1: Config         — Depends on: —           Files: config/effect.h, config/effect_config.h
Phase 2: Shader         — Depends on: —           Files: shaders/effect.fs
Phase 3: PostEffect     — Depends on: Phase 1, 2  Files: render/post_effect.h, render/post_effect.cpp
Phase 4: Shader Setup   — Depends on: Phase 3     Files: render/shader_setup.h, render/shader_setup.cpp
Phase 5: UI             — Depends on: Phase 1     Files: ui/imgui_effects.cpp, ui/imgui_effects_style.cpp
Phase 6: Serialization  — Depends on: Phase 1     Files: config/preset.cpp, automation/param_registry.cpp

Wave 1: [Phase 1, Phase 2]     — no deps, no file conflicts
Wave 2: [Phase 3, Phase 5, Phase 6] — all depend only on Wave 1, disjoint files
Wave 3: [Phase 4]               — depends on Phase 3 (Wave 2)
```

---

## Phase 2: Context Check

**Goal**: Ensure sufficient context remains for implementation

**Actions**:
1. Estimate implementation complexity from the phase description
2. If you estimate this phase will push context past 50%:
   - STOP before implementing
   - Generate a handoff summary (see Handoff Format below)
   - Append handoff to progress file under current phase
   - Tell user: "Context is limited. Handoff saved. Start new session and run `/implement <plan-path>` to continue."
   - Exit without implementing
3. If context is sufficient, proceed to implementation

---

## Phase 3: Implement

**Goal**: Complete the current phase (sequential) or current wave (parallel)

### Sequential Mode

**Actions**:
1. Read all files mentioned in the phase's "Build" section
2. Implement changes as described:
   - Follow CLAUDE.md code style strictly
   - Make minimal changes - only what the phase specifies
   - Do NOT implement future phases
3. For each file modified, verify the change compiles/works
4. Run build command: `cmake.exe --build build`
5. If build fails, fix issues before proceeding
6. If the phase has a `**Verify**:` field, run those exact commands and confirm the expected result
7. Verify "Done when" criteria from the plan

### Parallel Mode (wave execution)

**Actions**:
1. Identify all incomplete phases in the current wave
2. Dispatch all phase tasks simultaneously using the Task tool:
   - `subagent_type=general-purpose`
   - `allowed_tools=["Edit", "Write", "Read", "Glob", "Grep"]` — NO Bash (agents cannot build)
   - Prompt template:
     ```
     Read the plan at `<plan-path>`. Implement Phase N only.
     The plan's Current State section has file references. Read files as needed.
     Follow CLAUDE.md code style. Only modify files in the phase's Files list.

     IMPORTANT: Do NOT run cmake or any build commands. The orchestrator builds
     once after all agents complete. Multiple concurrent builds will fail.
     ```
   - Do NOT pre-read files for agents—they have Read tool access
3. Wait for all agents to complete
4. Run build command once: `cmake.exe --build build`
5. If build fails:
   - Identify which phase's files cause the error
   - Fix the issue directly (do not re-dispatch)
   - Rebuild until clean
6. Run `**Verify**:` commands for each phase in the wave
7. Commit each phase separately (see Phase 5)

---

## Phase 4: Update Progress

**Goal**: Record completion and prepare for next phase/wave

**Actions**:
1. Update the progress file for each completed phase:
   ```markdown
   ## Phase N: [Name]
   - Status: completed
   - Wave: W (parallel mode only)
   - Completed: [today's date]
   - Files modified: [list]
   - Notes: Brief implementation notes
   ```
2. If there are more phases/waves:
   - Update `current_phase` (sequential) or `current_wave` (parallel) in frontmatter
   - Mark next phases as `pending` if not already tracked
3. Save progress file

---

## Phase 5: Commit & Summary

**Goal**: Commit changes and inform user of status

**Actions**:

### Sequential mode:
1. Stage and commit with message: `Implement <plan-name> phase N: <phase-title>`

### Parallel mode:
1. For each phase in the completed wave, stage its files and commit separately:
   - `git add <files from phase's Files list>`
   - Commit message: `Implement <plan-name> phase N: <phase-title>`
   - Commit phases in numerical order within the wave

### Both modes:
2. Summarize for user:
   - What was implemented (list phases if wave)
   - Current progress: "Phase N of M complete" or "Wave W complete (phases X, Y, Z)"
   - Next phase/wave preview (if any)
   - Context status: "~X% context used"
3. If approaching 50% context or more phases remain, remind user:
   - "Run `/implement <plan-path>` to continue"

---

## Progress File Format

Create/update `<plan-name>.progress.md`:

```markdown
---
plan: docs/plans/<name>.md
branch: <feature-branch-name>
mode: sequential | parallel
current_phase: 1
current_wave: 1 (parallel mode only)
total_phases: 5
total_waves: 3 (parallel mode only)
waves: (parallel mode only)
  1: [1, 2]
  2: [3, 5, 6]
  3: [4]
started: YYYY-MM-DD
last_updated: YYYY-MM-DD
---

# Implementation Progress: <Feature Name>

## Phase 1: <Name from plan>
- Status: in_progress | completed | skipped
- Wave: 1 (parallel mode only)
- Started: YYYY-MM-DD
- Completed: YYYY-MM-DD (if done)
- Files modified:
  - src/path/file.cpp
- Notes: Brief implementation notes

## Phase 2: <Name from plan>
- Status: pending
```

---

## Handoff Format

When context is limited, append this to the current phase in progress file:

```markdown
### Handoff Notes
- **Stopped at**: Brief description of where you stopped
- **In progress**: Any partially complete work
- **Next steps**: What the next session should do first
- **Key context**: Important details the next session needs
- **Open questions**: Any unresolved issues
```

---

## Error Handling

- **Plan not found**: Ask user to provide correct path
- **Build fails**: Attempt to fix, if unable, save handoff notes and inform user
- **Phase unclear**: Ask user for clarification before implementing
- **All phases complete**: Congratulate user, suggest running `/feature-review`, remind about documenting post-implementation changes (see below)
- **Agent fails** (parallel mode): If a dispatched agent errors, read its output, fix the issue directly, then continue with remaining phases in the wave
- **File conflict detected** (parallel mode): If wave computation finds two phases sharing a file, bump the later phase to the next wave and inform user of the adjusted schedule

---

## Output Constraints

- **Sequential**: Implement ONLY the current phase
- **Parallel**: Implement ONLY phases in the current wave
- Do NOT skip ahead to future waves
- Do NOT implement code not specified in the phase
- ALWAYS update progress file, even on partial completion
- In parallel mode, each agent writes ONLY to its declared files—no cross-phase file edits

---

## Post-Implementation Notes

**Purpose**: Document changes made after all planned phases complete — parameter additions, removed features, bug fixes, performance tuning.

**Trigger**: User requests a change to a completed feature AND confirms it works.

**Action**: After the user verifies the change, append to the plan document's `## Post-Implementation Notes` section (create if missing):

```markdown
## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Added: `paramName` parameter (YYYY-MM-DD)

**Reason**: Brief explanation of why this was needed.

**Changes**:
- `file.h`: Added field
- `file.cpp`: Updated logic
- `ui.cpp`: Added slider

### Removed: Feature X

**Reason**: Produced artifacts / hurt performance / didn't work as expected.

**Changes**: List files modified to remove the feature.
```

**Flow**:
1. User requests tweak to completed feature
2. Implement the change
3. User tests and confirms it works
4. **You** update the plan document with the implementation note
5. Commit the plan update alongside or after the code change
