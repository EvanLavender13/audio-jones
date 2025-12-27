---
description: Regenerate architecture documentation from current code state using multi-agent analysis
---

# Sync Architecture Documentation

You are synchronizing architecture documentation with the current codebase. This is a multi-agent workflow that produces minimal diffs.

## Why Multi-Agent

Documentation sync requires different analytical perspectives:
- **Code scanning** - Extract complete API surface from source
- **Diff analysis** - Compare docs to code, identify minimal changes
- **Technical writing** - Apply patches following standards

Running these as specialized agents produces accurate, minimal documentation updates.

## Core Rules

- **PATCH, don't rewrite** - Only change what's factually incorrect or missing
- **Zero diff if unchanged** - If code matches docs, no edits
- **Use TodoWrite** to track all phases
- **Parallel agents** where possible

---

## Phase 1: Preparation

**Goal**: Set up analysis and load standards

**Actions**:
1. Create todo list with all 5 phases
2. Invoke the `architecture-diagrams` skill
3. Verify documentation directories exist:
   - `docs/architecture.md`
   - `docs/modules/`

---

## Phase 2: Code Analysis

**Goal**: Extract complete codebase structure

**Actions**:
1. Launch 2-3 code-scanner agents in parallel, each covering different modules:
   - `"Scan src/audio/ and src/analysis/ - extract all public APIs, types, and data flow"`
   - `"Scan src/render/ and src/automation/ - extract all public APIs, types, and data flow"`
   - `"Scan src/config/ and src/ui/ and src/main.cpp - extract all public APIs, types, and data flow"`

2. Collect results from all agents
3. Merge into complete codebase structure
4. Note any discrepancies between agents

---

## Phase 3: Documentation Comparison

**Goal**: Identify minimal changes needed

**Actions**:
1. Read all existing documentation:
   - `docs/architecture.md`
   - All files in `docs/modules/`

2. Launch doc-comparator agent:
   - `"Compare these code scan results to existing docs. Code: [merged results from Phase 2]. Docs: [current doc contents]. Identify ONLY changes needed to fix inaccuracies or add missing items."`

3. Review diff report for:
   - False positives (changes that aren't needed)
   - Missing changes
   - Scope creep (style improvements vs accuracy fixes)

---

## Phase 4: Apply Updates

**Goal**: Patch documentation with minimal changes

**Actions**:
1. For significant updates, launch doc-writer agent:
   - `"Apply these documentation patches: [diff report]. Follow technical writing standards for new content only. Preserve existing wording where accurate."`

2. For minor updates (single line changes), apply directly using Edit tool

3. Verify each change:
   - File was correctly modified
   - No unintended side effects
   - Cross-references still valid

---

## Phase 5: Validation

**Goal**: Confirm documentation accuracy

**Actions**:
1. Verify all module links resolve:
   - Each module in `docs/modules/` corresponds to `src/` directory
   - All links in architecture.md point to existing files

2. Verify diagram accuracy:
   - System diagram shows all modules
   - Data flow arrows match actual dependencies
   - No orphaned nodes or missing connections

3. Check cross-references:
   - Module index links work
   - File references exist
   - No broken internal links

4. Summary to user:
   - Files updated
   - Files unchanged (verified correct)
   - Any manual follow-up needed

---

## Documentation Structure Reference

### Main Doc: `docs/architecture.md`

- Overview (1 paragraph)
- System Diagram (module-level Mermaid flowchart)
- Module Index (table with links)
- Data Flow Summary (numbered steps)
- Configuration Reference (table)
- Thread Model (diagram + description)
- Directory Structure (tree)

Keep under 200 lines.

### Module Docs: `docs/modules/<name>.md`

One file per `src/` subdirectory plus main:
- Purpose (1-2 sentences)
- Files (list)
- Function Reference (table)
- Types (struct/enum/constant tables)
- Data Flow (how data enters/exits)

---

## Diagram Standards

System diagram requirements:
- Every arrow labeled with data type
- Every process has input and output
- Single abstraction level (modules, not functions)
- Subgraphs for logical grouping
- Legend explaining notation

---

## Anti-patterns to Avoid

| Mistake | Why It Fails |
|---------|--------------|
| Rewriting accurate prose | Creates unnecessary diffs |
| Adding comments/docstrings | Not doc sync's job |
| Changing formatting style | Scope creep |
| Ignoring existing structure | Loses author intent |
| Skipping validation | Broken links go unnoticed |

---

## Output Constraints

- Update only `docs/architecture.md` and `docs/modules/*.md`
- Do NOT modify source code
- Do NOT modify `CLAUDE.md` (separate command handles this)
- If docs are already accurate, report "No changes needed" and exit
