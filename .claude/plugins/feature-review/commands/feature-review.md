---
description: Review implemented feature against its plan document
argument-hint: Path to plan document (e.g., docs/plans/feature-name.md)
---

# Feature Review

Review an implemented feature against its design plan. Launches parallel reviewers to check simplicity, correctness, and conventions.

## Core Principles

- **Plan as source of truth**: The plan document defines what should have been built
- **Git diff as scope**: Review the actual changes made
- **Confidence filtering**: Only surface issues with >= 80% confidence
- **User decides fixes**: Present findings and let user choose what to address

---

## Phase 1: Setup

**Goal**: Get the plan document and changes to review

**Actions**:
1. Create todo list with all phases
2. If no plan path provided in $ARGUMENTS, ask user:
   - "Which plan document should I review against? (e.g., docs/plans/feature-name.md)"
3. Read the plan document
4. Run `git diff` to capture current changes
5. If diff is empty, ask user if they want to review staged changes (`git diff --cached`) or a specific commit range

---

## Phase 2: Launch Reviewers

**Goal**: Get parallel reviews from different perspectives

**Actions**:
1. Launch 3 code-reviewer agents **in parallel** with the plan content and git diff
2. Assign each a different focus:

   **Agent 1 - Simplicity/DRY/Elegance**:
   "Review this implementation for simplicity, DRY violations, and code elegance. Focus: unnecessary complexity, duplication, readability issues."

   **Agent 2 - Bugs/Functional Correctness**:
   "Review this implementation for bugs and functional correctness. Focus: logic errors, edge cases, does implementation match the plan's design?"

   **Agent 3 - Project Conventions**:
   "Review this implementation for project convention adherence. Focus: CLAUDE.md rules, naming, patterns, style consistency."

3. Include in each agent prompt:
   - The full plan document content
   - The git diff output
   - Their specific focus area

---

## Phase 3: Consolidate Findings

**Goal**: Merge results into actionable list

**Actions**:
1. Collect all issues from the 3 reviewers
2. Deduplicate (same issue found by multiple reviewers)
3. Sort by severity: Critical first, then Important
4. Group by file for easier navigation

---

## Phase 4: Present Findings

**Goal**: Give user clear picture and options

**Actions**:
1. Present summary:
   - Total issues found (Critical / Important counts)
   - Files affected
2. List each issue with:
   - Severity and confidence
   - File:line reference
   - Description and suggested fix
3. Ask user: "How would you like to proceed?"
   - Fix all issues now
   - Fix critical issues only
   - Review issues individually
   - Proceed without fixes

---

## Phase 5: Address Issues (if requested)

**Goal**: Fix issues based on user choice

**Actions**:
1. If user wants fixes, work through selected issues
2. For each fix:
   - Read the relevant file section
   - Apply the fix
   - Mark todo complete
3. After all fixes, run `git diff` again to show changes made

---

## Phase 6: Summary

**Goal**: Wrap up the review

**Actions**:
1. Mark all todos complete
2. Summarize:
   - Issues found vs fixed
   - Files modified
   - Remaining issues (if any deferred)
