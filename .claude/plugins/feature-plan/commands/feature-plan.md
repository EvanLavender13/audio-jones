---
description: Plan a feature without implementing - explores codebase, clarifies requirements, designs architecture, writes plan to docs/plans/
argument-hint: Feature description
---

# Feature Planning (No Implementation)

You are creating a plan document for a feature. You will NOT implement anything. Your output is a plan file in `docs/plans/` that can be used in a fresh session later.

## Why This Exists

Context limits make large features impossible in one session. This command:
1. Explores the codebase
2. Asks clarifying questions
3. Designs the architecture
4. Writes a complete plan to `docs/plans/`
5. **STOPS** - no implementation

The plan document contains everything needed to implement in fresh sessions.

## Core Rules

- **NO IMPLEMENTATION**: Do not write any code except in the plan document's examples
- **WRITE THE PLAN FILE**: The deliverable is `docs/plans/<feature-name>.md`
- **Use TodoWrite**: Track all phases
- **Ask questions early**: Phase 3 is critical

---

## Phase 1: Discovery

**Goal**: Understand what needs to be built

Feature request: $ARGUMENTS

**Actions**:
1. Create todo list with all phases
2. If feature unclear, ask user for:
   - What problem are they solving?
   - What should the feature do?
   - Any constraints or requirements?
3. Summarize understanding and confirm with user
4. Determine plan filename: `docs/plans/<kebab-case-name>.md`

---

## Phase 2: Codebase Exploration

**Goal**: Understand relevant code and patterns

**Actions**:
1. Launch 2-3 code-explorer agents in parallel. Each should:
   - Target a different aspect (similar features, architecture, patterns)
   - Return a list of 5-10 essential files

   Example prompts:
   - "Find features similar to [feature] and trace implementation"
   - "Map architecture and abstractions for [area]"
   - "Analyze current implementation of [related feature]"

2. **Read all essential files** identified by agents
3. Present summary of findings

---

## Phase 3: Clarifying Questions

**Goal**: Resolve ALL ambiguities before designing

**CRITICAL**: DO NOT SKIP THIS PHASE.

**Actions**:
1. Review findings and feature request
2. Identify underspecified aspects:
   - Edge cases
   - Error handling
   - Integration points
   - Scope boundaries
   - Design preferences
   - Performance requirements
3. **Present all questions in an organized list**
4. **Wait for answers before proceeding**

If user says "whatever you think is best", provide your recommendation and get explicit confirmation.

---

## Phase 4: Architecture Design

**Goal**: Design the implementation approach

**Actions**:
1. Launch 2-3 plan-architect agents in parallel with different focuses:
   - Minimal changes (smallest change, maximum reuse)
   - Clean architecture (maintainability, elegant abstractions)
   - Pragmatic balance (speed + quality)

2. Review approaches and form recommendation
3. Present to user:
   - Brief summary of each approach
   - Trade-offs comparison
   - Your recommendation with reasoning
4. **Ask user which approach they prefer**

---

## Phase 5: Write Plan Document

**Goal**: Produce the plan file

**Actions**:
1. Wait for user to approve architecture approach
2. Write complete plan to `docs/plans/<feature-name>.md`
3. Follow the format from `.claude/commands/make-plan.md`
4. Include:
   - All `file:line` references discovered
   - Architecture decision and rationale
   - Component designs
   - File changes table
   - Phased build sequence (each phase = one session)
   - Validation criteria

---

## Phase 6: Summary

**Goal**: Confirm completion

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Plan file location
   - How to use it: "Start a fresh session and say: implement docs/plans/<name>.md phase 1"
   - Estimated number of sessions (based on phases)

---

## Output Constraints

- The ONLY file you create is `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT modify existing code
- Code examples in the plan are for illustration only
