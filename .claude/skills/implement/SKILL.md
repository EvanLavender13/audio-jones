---
name: implement
description: Use when executing a feature plan phase-by-phase. Triggers on "implement this plan", "start building", or when a plan document exists in docs/plans/ and the user wants to begin coding.
---

# Phased Implementation

Execute a feature plan phase-by-phase. Track progress in a companion file. Stop at checkpoints so user can evaluate context usage and decide whether to continue or start a new session.

## Core Principles

- **Sequential execution**: One phase at a time, in order
- **Checkpoint pauses**: Stop at `<!-- CHECKPOINT -->` markers and report status
- **User controls session boundaries**: User decides when context is too high to continue
- **Incremental commits**: Commit after each successful phase

---

## Phase 1: Setup

**Goal**: Load plan, determine current phase, prepare for implementation

**Actions**:
1. Parse arguments: first argument is plan path (required), second is phase number (optional override)
2. Read the plan file
3. Parse plan structure—identify all `## Phase N:` headers and `<!-- CHECKPOINT -->` markers
4. Determine progress file path: `<plan-path>.progress.md`
   - Example: `docs/plans/feature.md` → `docs/plans/feature.progress.md`
5. Read progress file if exists, otherwise create initial structure
6. **Create feature branch** (first run only):
   - If no progress file existed, create branch: `git checkout -b <kebab-case-from-filename>`
   - If branch exists, checkout: `git checkout <branch-name>`
7. Determine target phase:
   - If phase number in args, use that
   - Otherwise find first incomplete phase
   - If all phases complete, inform user and exit
8. Create todo list: Setup, Implement Phase N, Build, Update Progress, Commit

---

## Phase 2: Implement

**Goal**: Complete the current phase

**Actions**:
1. Read all files mentioned in the phase's `**Files**:` section
2. Implement changes as described:
   - Follow CLAUDE.md code style strictly
   - Make minimal changes—only what the phase specifies
   - Do NOT implement future phases
3. Run build: `cmake.exe --build build`
4. If build fails, fix issues before proceeding
5. If phase has `**Verify**:` field, run those commands and confirm expected result
6. Verify "Done when" criteria from the plan

---

## Phase 3: Update Progress

**Goal**: Record completion, check for checkpoint

**Actions**:
1. Update progress file for completed phase:
   ```markdown
   ## Phase N: [Name]
   - Status: completed
   - Completed: [today's date]
   - Files modified: [list]
   - Notes: Brief implementation notes
   ```
2. Check if a `<!-- CHECKPOINT -->` follows this phase in the plan
3. If checkpoint reached:
   - Update `current_phase` in frontmatter to next phase
   - Set `checkpoint_reached: true`
4. If no checkpoint, set `checkpoint_reached: false`
5. Save progress file

---

## Phase 4: Commit

**Goal**: Commit changes

**Actions**:
1. Stage files from the phase's `**Files**:` list
2. Commit with message: `Implement <plan-name> phase N: <phase-title>`
3. Include co-author line

---

## Phase 5: Report Status

**Goal**: Inform user and determine next action

**Actions**:

### If checkpoint reached:
```
Phase N complete. Checkpoint reached.

Progress: N of M phases complete
Next: Phase N+1 - [title]

Context is at [X]%. Options:
- "continue" to proceed with Phase N+1
- "new session" to stop here (run `/implement <plan>` in fresh session)
```

**Wait for user response before continuing.**

### If no checkpoint (more phases in current segment):
```
Phase N complete.

Progress: N of M phases complete
Next: Phase N+1 - [title]
```

**Automatically proceed to Phase N+1.**

### If all phases complete:
```
All phases complete!

Run `/feature-review docs/plans/<name>.md` to verify implementation.
```

---

## Progress File Format

Create/update `<plan-name>.progress.md`:

```markdown
---
plan: docs/plans/<name>.md
branch: <feature-branch-name>
current_phase: 1
total_phases: 5
checkpoint_reached: false
started: YYYY-MM-DD
last_updated: YYYY-MM-DD
---

# Implementation Progress: <Feature Name>

## Phase 1: <Name from plan>
- Status: completed
- Completed: YYYY-MM-DD
- Files modified:
  - src/path/file.cpp
- Notes: Brief implementation notes

## Phase 2: <Name from plan>
- Status: in_progress

## Phase 3: <Name from plan>
- Status: pending
```

---

## Error Handling

- **Plan not found**: Ask user for correct path
- **Build fails**: Attempt to fix; if unable, inform user with error details
- **Phase unclear**: Ask user for clarification before implementing
- **All phases complete**: Congratulate user, suggest `/feature-review`

---

## Output Constraints

- Implement ONLY the current phase
- Do NOT skip ahead to future phases
- Do NOT implement code not specified in the phase
- ALWAYS update progress file
- ALWAYS stop and wait for user at checkpoints

---

## Red Flags - STOP

If you catch yourself thinking:

| Thought | Reality |
|---------|---------|
| "I'll just do the next phase too" | Stop at checkpoints. User controls pacing. |
| "These phases could run in parallel" | No. Sequential only. Shared files break parallel edits. |
| "I'll batch these small phases" | Each phase gets its own commit. No batching. |
| "The checkpoint is arbitrary, I'll continue" | Checkpoints exist for context management. Stop. |

---

## Post-Implementation Notes

**Purpose**: Document changes made after all planned phases complete.

**Trigger**: User requests change to completed feature AND confirms it works.

**Action**: Append to plan's `## Post-Implementation Notes` section:

```markdown
## Post-Implementation Notes

### Added: `paramName` parameter (YYYY-MM-DD)

**Reason**: Why this was needed.

**Changes**:
- `file.h`: Added field
- `file.cpp`: Updated logic
```
