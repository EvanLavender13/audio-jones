# sync-architecture

Regenerate architecture documentation from current code state using multi-agent analysis.

## Purpose

Keeps `docs/architecture.md` and `docs/modules/*.md` synchronized with actual codebase structure. Uses a multi-agent workflow to produce minimal, accurate documentation patches.

## Usage

```
/sync-architecture
```

The command orchestrates three specialized agents:

1. **code-scanner** - Extracts module structure, public APIs, types, and data flow from source files
2. **doc-comparator** - Compares code scan results to existing documentation, identifies minimal changes needed
3. **doc-writer** - Applies documentation patches following technical writing standards

## Workflow

1. Preparation - Load technical-writing and architecture-diagrams skills
2. Code Analysis - Launch parallel code-scanner agents for different module groups
3. Documentation Comparison - Compare scan results to existing docs
4. Apply Updates - Patch documentation with minimal changes
5. Validation - Verify links, diagrams, and cross-references

## Core Principles

- **Patch, don't rewrite** - Only change what's factually incorrect or missing
- **Zero diff if unchanged** - If code matches docs, no edits
- **Preserve author voice** - Keep existing phrasing where accurate
- **Minimal changes** - Every edit must fix an inaccuracy

## Components

| Type | Name | Purpose |
|------|------|---------|
| Command | sync-architecture | Orchestrates the multi-agent workflow |
| Agent | code-scanner | Extracts API surface from source files |
| Agent | doc-comparator | Identifies minimal documentation diffs |
| Agent | doc-writer | Applies patches following writing standards |

## Output

Updates only:
- `docs/architecture.md`
- `docs/modules/*.md`

Does NOT modify source code or CLAUDE.md.
