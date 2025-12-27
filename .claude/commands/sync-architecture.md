---
description: Regenerate architecture documentation from current code state using multi-agent analysis
---

# Sync Architecture Documentation

You are synchronizing architecture documentation with the current codebase. Each module agent owns its complete documentation generation.

## Why Hierarchical Agents

Each module-sync agent:
- Analyzes one module directory in isolation
- Generates complete documentation from scratch
- Returns a delta report (purpose string for Module Index)

The orchestrator coordinates agents and updates architecture.md with collected purposes.

## Core Rules

- **Generate fresh** - Each agent creates complete module doc, not patches
- **Parallel execution** - Launch all module-sync agents simultaneously
- **Use TodoWrite** to track all phases
- **Follow skills** - Invoke documentation-standards and architecture-diagrams skills

---

## Phase 1: Preparation

**Goal**: Load standards and discover modules

**Actions**:
1. Create todo list with all 4 phases
2. Invoke the `documentation-standards` skill
3. Invoke the `architecture-diagrams` skill
4. Discover modules by listing `src/*/` directories
5. Verify `docs/modules/` directory exists

**Expected modules**:
- `src/audio/`
- `src/analysis/`
- `src/automation/`
- `src/render/`
- `src/config/`
- `src/ui/`
- `src/main.cpp` (single-file module)

---

## Phase 2: Module Sync (Parallel)

**Goal**: Generate all module documentation

**Actions**:
1. Launch module-sync agents in parallel for ALL modules:

```
Task(module-sync): "Sync module documentation for src/audio/"
Task(module-sync): "Sync module documentation for src/analysis/"
Task(module-sync): "Sync module documentation for src/automation/"
Task(module-sync): "Sync module documentation for src/render/"
Task(module-sync): "Sync module documentation for src/config/"
Task(module-sync): "Sync module documentation for src/ui/"
Task(module-sync): "Sync module documentation for src/main.cpp"
```

2. Each agent:
   - Reads all source files in the module
   - Generates complete `docs/modules/<name>.md` following template
   - Creates Mermaid data flow diagram
   - Returns JSON delta report: `{"module": "...", "purpose": "...", "warnings": [...]}`

3. Collect all delta reports
4. Note any warnings for user review

---

## Phase 3: Architecture.md Updates

**Goal**: Update Module Index with fresh purposes

**Actions**:
1. Read `docs/architecture.md`

2. Update Module Index table with purposes from delta reports:
   ```markdown
   | Module | Purpose | Documentation |
   |--------|---------|---------------|
   | audio | [purpose from delta] | [audio.md](modules/audio.md) |
   | analysis | [purpose from delta] | [analysis.md](modules/analysis.md) |
   ...
   ```

3. Verify System Diagram includes all discovered modules
   - If a module is missing from the diagram, FLAG for user review (don't auto-update complex diagrams)

4. Update sync date at top of file

---

## Phase 4: Validation

**Goal**: Confirm documentation integrity

**Actions**:
1. Verify all module links in Module Index resolve:
   - Each `[name.md](modules/name.md)` points to existing file
   - Each module in `docs/modules/` has entry in Module Index

2. Check mermaid diagrams:
   - All diagrams have labeled arrows
   - No syntax errors (node names don't contain `[]`)
   - Legends present

3. Summary to user:
   - Modules synced: list with any warnings
   - Module Index: updated/unchanged
   - System Diagram: verified/needs-review
   - Any manual follow-up needed

---

## Module Doc Template Reference

Each module doc follows this structure (from documentation-standards skill):

```markdown
# [Module] Module
> Part of [AudioJones](../architecture.md)

## Purpose
[1-2 sentences: what problem this module solves]

## Files
[Bulleted list with one-line descriptions]

## Data Flow
[Mermaid diagram: entry points -> transforms -> exit points]

## Internal Architecture
[Prose: how components interact, key abstractions, design decisions]

## Usage Patterns
[Prose: how other modules integrate, initialization requirements]
```

---

## Delta Report Format

Each module-sync agent returns:

```json
{
  "module": "<name>",
  "purpose": "<1-sentence purpose for Module Index>",
  "warnings": ["<any issues found>"]
}
```

---

## Output Constraints

- Update only `docs/architecture.md` and `docs/modules/*.md`
- Do NOT modify source code
- Do NOT modify `CLAUDE.md`
- Present warnings to user, don't silently ignore them
