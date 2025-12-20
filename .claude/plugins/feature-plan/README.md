# feature-plan

Plan features without implementing - explores codebase, clarifies requirements, designs architecture, writes plan document.

## Purpose

Creates complete implementation plans in `docs/plans/` that can be executed in fresh sessions. Separates planning from implementation to handle context limits on large features.

## Usage

```
/feature-plan <feature description>
```

The command orchestrates two specialized agents:

1. **code-explorer** - Analyzes codebase to understand architecture, patterns, and existing features
2. **plan-architect** - Designs feature architecture and produces a complete plan document

## Workflow

1. Discovery - Understand feature requirements
2. Codebase Exploration - Launch parallel code-explorer agents for different aspects
3. Clarifying Questions - Resolve all ambiguities before designing
4. Architecture Design - Launch plan-architect agents with different design focuses
5. Write Plan - Create `docs/plans/<feature-name>.md`
6. Summary - Confirm completion and explain how to implement

## Core Principles

- **No implementation** - Only output is the plan document
- **Ask questions early** - Phase 3 resolves ambiguities before design
- **Multiple perspectives** - Agents explore minimal, clean, and pragmatic approaches
- **Complete plans** - Include file:line references, phased steps, validation criteria

## Components

| Type | Name | Purpose |
|------|------|---------|
| Command | feature-plan | Orchestrates the multi-agent planning workflow |
| Agent | code-explorer | Traces execution paths and maps architecture |
| Agent | plan-architect | Designs implementation approach and writes plan |

## Output

Creates only:
- `docs/plans/<feature-name>.md`

Does NOT modify source code or existing files.
