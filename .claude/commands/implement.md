---
description: Implement a plan phase-by-phase with progress tracking and automatic handoff
argument-hint: Path to plan (e.g., docs/plans/feature-name.md) [phase-number]
---

# Phased Implementation with Progress Tracking

You are implementing a feature plan phase-by-phase. Track progress in a companion file, implement one phase per invocation, and proactively manage context for smooth session handoffs.

## Core Principles

- **One phase per invocation**: Focus on a single phase to keep context manageable
- **Progress persistence**: Update `.progress.md` file after each phase
- **Context awareness**: Monitor usage and generate handoff before 50% threshold
- **Incremental commits**: Commit after each successful phase

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
6. Determine target phase:
   - If phase number provided in args, use that
   - Otherwise, find first incomplete phase from progress file
   - If all phases complete, inform user and exit
7. Create todo list with: Setup, Implement Phase N, Update Progress, Commit

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

**Goal**: Complete the current phase

**Actions**:
1. Read all files mentioned in the phase's "Build" section
2. Implement changes as described:
   - Follow CLAUDE.md code style strictly
   - Make minimal changes - only what the phase specifies
   - Do NOT implement future phases
3. For each file modified, verify the change compiles/works
4. Run build command: `cmake.exe --build build`
5. If build fails, fix issues before proceeding
6. Verify "Done when" criteria from the plan

---

## Phase 4: Update Progress

**Goal**: Record completion and prepare for next phase

**Actions**:
1. Update the progress file with phase completion:
   ```markdown
   ## Phase N: [Name]
   - Status: completed
   - Completed: [today's date]
   - Files modified: [list]
   - Notes: [brief summary of what was done]
   ```
2. If there are more phases:
   - Update `current_phase` in frontmatter to next phase number
   - Mark next phase as `pending` if not already tracked
3. Save progress file

---

## Phase 5: Commit & Summary

**Goal**: Commit changes and inform user of status

**Actions**:
1. Stage and commit changes with message: `Implement <plan-name> phase N: <phase-title>`
2. Summarize for user:
   - What was implemented
   - Current progress: "Phase N of M complete"
   - Next phase preview (if any)
   - Context status: "~X% context used"
3. If approaching 50% context or more phases remain, remind user:
   - "Run `/implement <plan-path>` to continue with phase N+1"

---

## Progress File Format

Create/update `<plan-name>.progress.md`:

```markdown
---
plan: docs/plans/<name>.md
current_phase: 1
total_phases: 5
started: YYYY-MM-DD
last_updated: YYYY-MM-DD
---

# Implementation Progress: <Feature Name>

## Phase 1: <Name from plan>
- Status: in_progress | completed | skipped
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
- **All phases complete**: Congratulate user, suggest running `/feature-review`

---

## Output Constraints

- Implement ONLY the current phase
- Do NOT skip ahead or combine phases
- Do NOT implement code not specified in the phase
- ALWAYS update progress file, even on partial completion
