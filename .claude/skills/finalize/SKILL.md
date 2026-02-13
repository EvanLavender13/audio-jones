---
name: finalize
description: Use after feature-review to archive plans and research docs, update effects inventory, and commit. Triggers on "finalize", "wrap up", "close out", or after review completes.
---

# Finalize Feature

Archive completed plans and research docs, update effects inventory if applicable, and commit.

## Phase 1: Identify Artifacts

**Goal**: Find all documents to archive

**Actions**:
1. If $ARGUMENTS has a plan path: use it
2. If not: check `docs/plans/` for active plans, ask user which one
3. Read the plan header for a research doc reference (`**Research**: docs/research/<name>.md`)
4. Report what will be archived:
   - Plan document
   - Research document (if referenced)

---

## Phase 2: Effects Inventory

**Skip if**: Feature is not a visual effect (no new entry needed in docs/effects.md)

**Actions**:
1. Read `docs/effects.md` to find the correct category table
2. Invoke `/write-effect-description` to draft and add the entry

---

## Phase 3: Archive Documents

**Goal**: Move completed documents to archive directories

**Actions**:
1. Move plan files:
   - `docs/plans/<feature>.md` → `docs/plans/archive/<feature>.md`
2. If research doc exists:
   - `docs/research/<feature>.md` → `docs/research/archive/<feature>.md`
3. Stage moved files with `git add`

---

## Phase 4: Commit

**Goal**: Record the closure

**Actions**:
1. Stage all changes (effects.md update if applicable, archived files)
2. Invoke `/commit`

---

## Output Constraints

- Do NOT archive plans before review is complete
- Do NOT skip the effects inventory check for new effects
- Do NOT delete documents — move to archive directories

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I'll delete the old plan" | Archive, never delete. |
| "No research doc to archive" | Check the plan header first. |
| "This effect doesn't need an effects.md entry" | If it has a shader and UI, it needs one. |
