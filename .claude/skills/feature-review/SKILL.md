---
name: feature-review
description: Use when reviewing an implemented feature against its plan document. Triggers after implementation completes, on "review this", or when checking if code matches specification.
---

# Feature Review

Review an implemented feature against its design plan. Checks simplicity, correctness, and conventions using single or multi-agent approach.

## Core Principles

- **Plan as source of truth**: The plan document defines what should have been built
- **Git diff as scope**: Review the actual changes made
- **Confidence filtering**: Only surface issues with >= 80% confidence
- **User decides fixes**: Present findings and let user choose what to address

---

## Phase 1: Setup

**Goal**: Identify the plan and confirm there are changes to review

**Actions**:
1. Create todo list with all phases
2. If no plan path in $ARGUMENTS, ask: "Which plan document? (e.g., docs/plans/feature-name.md)"
3. Confirm the plan file exists
4. Run `git diff main...HEAD --stat` to confirm changes exist on this branch
5. If diff empty, ask: review uncommitted (`git diff --stat`) or staged (`git diff --cached --stat`)?

**STOP**: Do not proceed until plan path is confirmed and changes exist.

---

## Phase 2: Agent Strategy Selection

**Goal**: Let user choose review approach based on change size

**Actions**:
1. Assess complexity from git diff (exclude docs/, .md files):
   - **Small**: <100 lines, 1-3 files
   - **Medium**: 100-500 lines, 3-10 files
   - **Large**: >500 lines or >10 files

2. **Ask user using AskUserQuestion**:
   - Question: "How should I review this implementation?"
   - Options:
     - **Single agent** - One comprehensive reviewer (lower token usage)
     - **Multi-agent** - Three parallel reviewers (higher token usage)
   - Include complexity assessment and recommendation

**STOP**: Do not proceed until user chooses approach.

---

## Phase 3: Launch Reviewers

**Goal**: Get reviews covering all focus areas

### If user chose "Multi-agent":

1. Launch 3 code-reviewer agents **in parallel** (single message, multiple Task calls)
2. Assign each a different focus:
   - **Agent 1**: Simplicity/DRY/Elegance
   - **Agent 2**: Bugs/Functional Correctness
   - **Agent 3**: Project Conventions
3. Each agent receives: plan path + focus area

### If user chose "Single agent":

1. Launch 1 code-reviewer agent covering ALL focus areas
2. Agent receives: plan path

---

## Phase 4: Consolidate Findings

**Goal**: Merge results into actionable list

**Actions**:
1. Collect all issues from reviewer(s)
2. Deduplicate if multi-agent (same issue found by multiple reviewers)
3. Sort by severity: Critical first, then Important
4. Group by file for navigation

---

## Phase 5: Present Findings

**Goal**: Give user clear picture and options

**Actions**:
1. Present summary: issue counts (Critical / Important), files affected
2. List each issue with:
   - Severity and confidence
   - File:line reference
   - Description and suggested fix
3. **Ask user**: "How would you like to proceed?"
   - Fix all issues now
   - Fix critical issues only
   - Review issues individually
   - Proceed without fixes

**STOP**: Do not proceed until user chooses action.

---

## Phase 6: Address Issues

**Goal**: Fix issues based on user choice

**Actions**:
1. If user wants fixes, work through selected issues
2. For each fix: read file section → apply fix → mark todo complete
3. After all fixes, run `git diff` to show changes made

**Skip if**: User chose "Proceed without fixes"

---

## Phase 7: Summary

**Goal**: Wrap up the review

**Actions**:
1. Mark all todos complete
2. Summarize: issues found vs fixed, files modified, deferred issues

---

## Output Constraints

- Do NOT fix issues without user consent
- Do NOT skip the confidence threshold (>= 80%)

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I'll fix these obvious issues" | User decides. Present and ask. |
| "This 50% confidence issue is important" | Below threshold. Don't report it. |
| "I'll skip the multi-agent option" | User chooses approach. Ask them. |
| "The plan is wrong, not the code" | Plan is source of truth. Report deviation. |
