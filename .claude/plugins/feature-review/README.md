# feature-review

Review implemented features against their plan documents using parallel code reviewers.

## Purpose

Validates that implementation matches the design plan by launching specialized reviewers for different aspects: simplicity, correctness, and conventions. Uses confidence-based filtering to surface only high-priority issues.

## Usage

```
/feature-review <path-to-plan>
```

Example:
```
/feature-review docs/plans/websocket-server.md
```

The command orchestrates parallel code-reviewer agents, each with a different focus:

1. **Simplicity/DRY/Elegance** - Code readability, unnecessary complexity, duplication
2. **Bugs/Functional Correctness** - Logic errors, edge cases, plan compliance
3. **Project Conventions** - CLAUDE.md rules, naming, patterns, style

## Workflow

1. Setup - Get plan document and git diff of changes
2. Launch Reviewers - Run 3 parallel code-reviewer agents with different focuses
3. Consolidate Findings - Merge, deduplicate, and sort by severity
4. Present Findings - Show issues with confidence scores and suggested fixes
5. Address Issues - Fix selected issues if user requests
6. Summary - Report issues found vs fixed

## Core Principles

- **Plan as source of truth** - The plan document defines what should have been built
- **Git diff as scope** - Review only the actual changes made
- **Confidence filtering** - Only surface issues with >= 80% confidence
- **User decides fixes** - Present findings and let user choose what to address

## Components

| Type | Name | Purpose |
|------|------|---------|
| Command | feature-review | Orchestrates the multi-agent review workflow |
| Agent | code-reviewer | Reviews code from a specific perspective with confidence scoring |

## Output

Produces a consolidated review report with:
- Issues grouped by severity (Critical / Important)
- File:line references for each issue
- Suggested fixes
- Option to apply fixes automatically
