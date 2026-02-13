# Claude Code Workflow

Slash commands guide feature development from planning through implementation and review.

## Command Flow

```mermaid
flowchart TD
    subgraph Ideation[Ideation]
        BS[/brainstorm/] -->|explores & researches| ResearchDoc[(docs/research/*.md)]
    end

    subgraph Plan[Planning]
        ResearchDoc -->|informs| FP[/feature-plan/]
        FP -->|creates| PlanDoc[(docs/plans/*.md)]
    end

    subgraph Implement[Implementation]
        PlanDoc -->|input| IMP[/implement/]
        IMP -->|modifies| Code[Source Files]
    end

    subgraph Review[Review & Close]
        Code -->|input| FR[/feature-review/]
        FR -->|identifies| Issues[Issues]
        Issues -->|fix cycle| Code
        FR -->|approved| FIN[/finalize/]
        FIN -->|archives| Archive[(docs/*/archive/)]
        FIN -->|commits| Commit[Git Commit]
    end

    subgraph Optional[Optional Commands]
        Code -->|analyze| LINT[/lint/]
        LINT -->|fixes| Code
        Commit -->|triggers| SYNC[/sync-docs/]
        SYNC -->|updates| Docs[(docs/*.md)]
    end

%% Legend:
%% [/command/] slash command
%% [(file)] document or file
%% [name] code artifact
%% → action (label describes relationship)
```

## Core Commands

### /feature-plan

Explores codebase, asks clarifying questions, designs architecture, writes plan. When research docs exist, a fresh agent compares the plan against research sources to catch drift, hallucinated techniques, or omitted steps.

**When to use**: Starting a new feature or significant change.

**Output**: `docs/plans/<feature-name>.md` with design spec and wave-grouped tasks for parallel implementation.

### /implement

Implements a plan wave-by-wave with parallel agents. Builds after each wave to verify, then commits on completion.

**When to use**: After `/feature-plan` produces a plan document.

**Usage**: `/implement docs/plans/feature-name.md`

**Output**: Code changes, git commit on completion.

### /feature-review

Reviews implementation against its plan using parallel code-reviewer agents.

**When to use**: After completing `/implement` phases.

**Usage**: `/feature-review docs/plans/feature-name.md`

**Output**: Issue list with severity, file locations, and suggested fixes.

### /finalize

Archives completed plan and research docs, updates effects inventory if applicable, and commits.

**When to use**: After `/feature-review` approves the implementation.

**Usage**: `/finalize docs/plans/feature-name.md`

**Output**: Archived docs, updated `docs/effects.md` (if new effect), git commit.

### /commit

Creates a git commit following project conventions.

**When to use**: After completing work that should be committed.

**Output**: Git commit with imperative-mood subject line.

## Optional Commands

### /brainstorm

Explores a vague idea through collaborative dialogue, then researches the technique with real references. Combines ideation and research in one flow.

**When to use**: Before `/feature-plan` when starting from a vague idea or when adding something that needs sourced algorithms.

**Usage**: `/brainstorm [optional starting idea]`

**Output**: `docs/research/<name>.md` with algorithm, references, and parameters.

### /lint

Runs clang-tidy static analysis and lizard complexity metrics. Triages findings, filters expected UI verbosity, presents options before fixing.

**When to use**: Before review, when code quality check needed, or to find complexity hotspots.

**Output**: Grouped findings by severity, actionable fixes with user consent.

### /sync-docs

Regenerates project documentation from current code state. Only updates documents affected by changes since last sync.

**When to use**: After significant code changes to keep docs current.

**Output**: Updated `docs/` documents (`stack.md`, `architecture.md`, `structure.md`, `conventions.md`, `concerns.md`).

## Typical Workflow

1. **Ideate** (optional): `/brainstorm` — explore idea, research technique, create research doc
2. **Plan**: `/feature-plan <description>` — explore, clarify, design
3. **Implement**: `/implement docs/plans/name.md` — build phase by phase
4. **Review**: `/feature-review docs/plans/name.md` — check against plan
5. **Close**: `/finalize docs/plans/name.md` — archive docs, update effects.md, commit
6. **Sync**: `/sync-docs` — update documentation
