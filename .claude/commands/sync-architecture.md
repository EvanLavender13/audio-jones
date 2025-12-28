---
description: Regenerate architecture documentation from current code state using multi-agent analysis
---

# Sync Architecture Documentation

You are synchronizing architecture documentation with the current codebase. Only modules with code changes since last sync get updated.

## Git-Based Change Detection

The `docs/architecture.md` header contains:
```
> Last sync: YYYY-MM-DD | Commit: <short-sha>
```

Before syncing:
1. Read `lastSyncCommit` from architecture.md header
2. Run `git diff <lastSyncCommit>..HEAD --name-only` to list changed files
3. Map changed files to modules (e.g., `src/render/waveform.cpp` → `render`)
4. Only sync modules that have source file changes

If no `lastSyncCommit` exists, sync all modules (first run).

## Core Rules

- **Incremental updates** - Only sync modules with code changes since last commit
- **Edit, don't overwrite** - Use Edit tool to preserve existing prose where accurate
- **Parallel execution** - Launch module-sync agents simultaneously for changed modules only
- **Use TodoWrite** to track all phases
- **Follow skills** - Invoke documentation-standards and architecture-diagrams skills

---

## Phase 1: Preparation

**Goal**: Load standards, discover modules, detect changes

**Actions**:
1. Create todo list with all 4 phases
2. Invoke the `documentation-standards` skill
3. Invoke the `architecture-diagrams` skill
4. Read `docs/architecture.md` and extract `lastSyncCommit` SHA from header
5. Run `git diff <lastSyncCommit>..HEAD --name-only | grep '^src/'` to get changed source files
6. Map changed files to modules:
   - `src/audio/*` → audio
   - `src/analysis/*` → analysis
   - `src/automation/*` → automation
   - `src/render/*` → render
   - `src/config/*` → config
   - `src/ui/*` → ui
   - `src/main.cpp` → main
7. Report which modules need syncing vs which are unchanged

**If no changes detected**: Skip to Phase 4 (validation only)

---

## Phase 2: Module Sync (Parallel)

**Goal**: Update documentation for changed modules only

**Actions**:
1. Launch module-sync agents in parallel for CHANGED modules only:

```
# Example: if only render, config, ui, main changed:
Task(module-sync): "Sync module documentation for src/render/"
Task(module-sync): "Sync module documentation for src/config/"
Task(module-sync): "Sync module documentation for src/ui/"
Task(module-sync): "Sync module documentation for src/main.cpp"
```

2. Each agent:
   - Reads all source files in the module
   - Reads existing `docs/modules/<name>.md`
   - Applies staleness rules from documentation-standards skill
   - Uses Edit tool to update sections, preserving accurate existing prose
   - Returns JSON delta report: `{"module": "...", "purpose": "...", "warnings": [...]}`

3. Collect all delta reports
4. Note any warnings for user review
5. Report unchanged modules as "skipped (no code changes)"

---

## Phase 3: Architecture.md Updates

**Goal**: Update Module Index and record sync commit

**Actions**:
1. Read `docs/architecture.md`

2. Update Module Index table purposes only for modules that were synced:
   - Leave unchanged module rows untouched
   - Update purpose text only for modules that received new delta reports

3. Verify System Diagram includes all discovered modules
   - If a module is missing from the diagram, FLAG for user review (don't auto-update complex diagrams)

4. Get current commit SHA: `git rev-parse --short HEAD`

5. Update header with date AND commit SHA:
   ```markdown
   > Last sync: 2025-12-28 | Commit: b9f6f6f
   ```

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
