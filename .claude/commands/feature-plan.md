---
description: Plan a feature without implementing - explores codebase, clarifies requirements, designs architecture, writes plan to docs/plans/
argument-hint: Feature description
---

# Feature Planning (No Implementation)

You are helping a developer plan a new feature. Follow a systematic approach: understand the codebase deeply, identify and ask about all underspecified details, design architecture, then write a plan document. Do NOT implement anything.

## Core Principles

- **Ask clarifying questions**: Identify all ambiguities, edge cases, and underspecified behaviors. Ask specific, concrete questions rather than making assumptions. Wait for user answers before proceeding.
- **Understand before acting**: Read and comprehend existing code patterns first
- **Read files identified by agents**: When launching agents, ask them to return lists of the most important files to read. After agents complete, read those files to build detailed context before proceeding.
- **Use TodoWrite**: Track all progress throughout

---

## Phase 1: Discovery

**Goal**: Understand what needs to be planned

Initial request: $ARGUMENTS

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

**Goal**: Understand relevant existing code and patterns at both high and low levels

**Actions**:
1. Launch 2-3 code-explorer agents in parallel. Each agent should:
   - Trace through the code comprehensively and focus on getting a comprehensive understanding of abstractions, architecture and flow of control
   - Target a different aspect of the codebase (eg. similar features, high level understanding, architectural understanding, etc)
   - Include a list of 5-10 key files to read

   **Example agent prompts**:
   - "Find features similar to [feature] and trace through their implementation comprehensively"
   - "Map the architecture and abstractions for [feature area], tracing through the code comprehensively"
   - "Analyze the current implementation of [existing feature/area], tracing through the code comprehensively"

2. Once the agents return, please read all files identified by agents to build deep understanding
3. Present comprehensive summary of findings and patterns discovered

---

## Phase 3: Clarifying Questions

**Goal**: Fill in gaps and resolve all ambiguities before designing

**CRITICAL**: This is one of the most important phases. DO NOT SKIP.

**Actions**:
1. Review the codebase findings and original feature request
2. Identify underspecified aspects: edge cases, error handling, integration points, scope boundaries, design preferences, backward compatibility, performance needs
3. **Present all questions to the user in a clear, organized list**
4. **Wait for answers before proceeding to architecture design**

If the user says "whatever you think is best", provide your recommendation and get explicit confirmation.

---

## Phase 4: Architecture Design

**Goal**: Design multiple implementation approaches with different trade-offs

**Actions**:
1. Launch 2-3 plan-architect agents in parallel with different focuses: minimal changes (smallest change, maximum reuse), clean architecture (maintainability, elegant abstractions), or pragmatic balance (speed + quality)
2. Review all approaches and form your opinion on which fits best for this specific task (consider: complexity, scope, team context)
3. Present to user: brief summary of each approach, trade-offs comparison, **your recommendation with reasoning**, concrete implementation differences
4. **Ask user which approach they prefer**

---

## Phase 5: Write Plan

**Goal**: Create the plan document

**Actions**:
1. Take the user's chosen approach from Phase 4
2. Write the plan to `docs/plans/<feature-name>.md` using this structure:

```markdown
# [Feature Name]

What we're building and why. One paragraph max.

## Current State

Quick orientation - what files exist, where to hook in:
- `src/path/file.cpp:123` - brief description
- `src/path/other.h:45` - another relevant location

## Phase 1: [Name]

**Goal**: One sentence describing the outcome.

**Build**:
- Create/modify files with brief descriptions
- No code, just what components to make

**Done when**: Simple verification statement.

---

## Phase 2: [Name]
...
```

3. Keep phases small enough to complete in one session each
4. Do NOT include implementation code - describe components, don't write them

---

## Phase 6: Summary

**Goal**: Document what was planned

**Actions**:
1. Mark all todos complete
2. Tell user:
   - Plan file location
   - How to use: `/implement docs/plans/<name>.md`
   - Number of phases

---

## Output Constraints

- ONLY create `docs/plans/<feature-name>.md`
- Do NOT create source files
- Do NOT write implementation code in the plan
