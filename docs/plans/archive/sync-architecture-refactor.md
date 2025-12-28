# Sync-Architecture Refactor

Restructure the `/sync-architecture` command to produce understanding-focused documentation with hierarchical module agents, replacing the current API-catalog approach.

## Current State

The existing system uses three sequential agents with manual orchestration:

- `.claude/commands/sync-architecture.md` - 5-phase orchestrator
- `.claude/agents/code-scanner.md` - Extracts API surface (functions, structs, constants)
- `.claude/agents/doc-comparator.md` - Compares code to docs, produces diff report
- `.claude/agents/doc-writer.md` - Applies patches from diff report
- `docs/architecture.md:106-141` - Configuration Reference table (36 rows, high maintenance)
- `docs/architecture.md:77-84` - Data Flow Summary (drifts out of sync)
- `docs/modules/*.md` - Function/Types/Constants tables (API catalog, not understanding)

**Problems:**
1. Orchestrator merges all code-scanner outputs, bottlenecks as codebase grows
2. Module docs catalog APIs instead of explaining architecture
3. doc-writer rarely used (most changes are single-field additions)
4. Configuration Reference duplicates information from module docs
5. Data Flow Summary duplicates System Diagram

## Phase 1: Create Documentation Standards Skill

**Goal**: Establish shared formatting rules all module agents will follow.

**Create:**
- `.claude/skills/documentation-standards/SKILL.md` - Writing style, module doc template, staleness rules

**Template defines:**
```markdown
# [Module] Module
> Part of [AudioJones](../architecture.md)

## Purpose
[1-2 sentences: what problem this module solves]

## Files
[Bulleted list with one-line descriptions]

## Data Flow
[Mermaid diagram: entry points → transforms → exit points]

## Internal Architecture
[Prose: how components interact, key abstractions, design decisions]

## Usage Patterns
[Prose: how other modules integrate, initialization requirements]
```

**Staleness rules (for future syncs, not initial generation):**
- REMOVE: Item documented but not found in code
- FLAG: Signature changed, semantic meaning unclear
- PRESERVE: Accurate prose, even if wording differs from code comments

**Done when**: Skill file exists and can be invoked.

---

## Phase 2: Create Module Sync Agent

**Goal**: Single parameterized agent that generates documentation for any module.

**Create:**
- `.claude/agents/module-sync.md` - Scans source, generates fresh doc, returns delta

**Agent responsibilities:**
1. Receive module path (e.g., `src/audio/`)
2. Scan source files to understand purpose, structure, data flow, internal design
3. Generate complete module doc following documentation-standards template
4. Create mermaid data flow diagram showing entry → transforms → exit
5. Return delta report for architecture.md updates (purpose for Module Index)

**Input format:**
```
Sync module documentation for src/<module>/
```

**Output format:**
```json
{
  "module": "audio",
  "purpose": "Captures system audio via WASAPI loopback into ring buffer",
  "warnings": []
}
```

**Tools:** Glob, Grep, LS, Read, Write, TodoWrite

**Done when**: Agent generates a complete module doc from scratch (test with `src/audio/`).

---

## Phase 3: Reset Existing Documentation

**Goal**: Clean slate for fresh generation.

**Delete:**
- `docs/modules/audio.md`
- `docs/modules/analysis.md`
- `docs/modules/automation.md`
- `docs/modules/render.md`
- `docs/modules/config.md`
- `docs/modules/ui.md`
- `docs/modules/main.md`

**Modify `docs/architecture.md`:**
- Delete Configuration Reference section (lines ~106-141)
- Delete Data Flow Summary section (lines ~77-84)
- Keep: Overview, System Diagram, Module Index, Thread Model, Directory Structure

**Done when**: Module docs deleted, architecture.md trimmed to ~100 lines.

---

## Phase 4: Refactor Orchestrator Command

**Goal**: Update sync-architecture to use hierarchical module agents.

**Modify:**
- `.claude/commands/sync-architecture.md` - New phase structure

**New command phases:**

### Phase 1: Preparation
- Invoke `documentation-standards` skill
- Invoke `architecture-diagrams` skill
- Discover modules from `src/*/` directories

### Phase 2: Module Sync (Parallel)
- Launch `module-sync` agent for each module in parallel:
  - `src/audio/`
  - `src/analysis/`
  - `src/automation/`
  - `src/render/`
  - `src/config/`
  - `src/ui/`
  - `src/main.cpp` (special case: single file)
- Each agent creates `docs/modules/<name>.md` from scratch
- Collect delta reports (purpose strings) from all agents

### Phase 3: Architecture.md Updates
- Update Module Index table with purposes from delta reports
- Verify System Diagram includes all modules

### Phase 4: Validation
- Verify module links in Module Index resolve to actual files
- Check all mermaid diagrams render correctly
- Present summary to user

**Done when**: Full command runs and generates all documentation.

---

## Phase 5: Cleanup

**Goal**: Remove obsolete agents.

**Delete:**
- `.claude/agents/code-scanner.md`
- `.claude/agents/doc-comparator.md`
- `.claude/agents/doc-writer.md`

**Done when**: Only `module-sync.md` remains in agents directory (plus any unrelated agents).

---

## Data Flow

### Before
```
Orchestrator
  ├─> code-scanner (group 1) ──┐
  ├─> code-scanner (group 2) ──┼─> Merge in orchestrator
  └─> code-scanner (group 3) ──┘
          ↓
      doc-comparator (all merged results)
          ↓
      doc-writer (diff report)
```

### After
```
Orchestrator
  ├─> module-sync (audio) ────> creates doc + returns purpose
  ├─> module-sync (analysis) ─> creates doc + returns purpose
  ├─> module-sync (automation) > creates doc + returns purpose
  ├─> module-sync (render) ───> creates doc + returns purpose
  ├─> module-sync (config) ───> creates doc + returns purpose
  ├─> module-sync (ui) ───────> creates doc + returns purpose
  └─> module-sync (main) ─────> creates doc + returns purpose
          ↓
  Orchestrator updates Module Index with collected purposes
```

**Key difference**: Each module agent owns its entire doc generation. Orchestrator only coordinates and updates Module Index.
