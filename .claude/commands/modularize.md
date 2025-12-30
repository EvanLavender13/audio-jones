---
description: Extract modules from monolithic files using multi-agent cohesion analysis
argument-hint: File path or pattern (e.g., src/main.cpp or src/*.cpp)
---

# Module Extraction Planning

You are analyzing source files to identify module extraction candidates. This command produces a plan document—it does NOT implement extraction.

## Core Rules

- **Analysis only** - Output plan document, do not modify source files
- **User approval** before writing plan (Phase 3)
- **Use TodoWrite** to track all phases
- **Parallel agents** for multiple files

---

## Phase 1: Discovery

**Goal**: Identify complexity hotspots and target files

Target: $ARGUMENTS

**Actions**:
1. Create todo list with all 4 phases

2. Run lizard complexity analysis:
   ```bash
   lizard ./src -C 15 -L 75 -a 5
   ```

3. Parse results using these thresholds:

   | Metric | Warning | Error |
   |--------|---------|-------|
   | CCN (cyclomatic complexity) | >15 | >20 |
   | NLOC (function length) | >75 | >150 |
   | Parameters | >5 | >7 |

4. If no target provided:
   - Group warnings/errors by file
   - Rank files by total complexity issues
   - Present top candidates to user:
     ```
     ## Complexity Hotspots

     | File | Issues | Worst Function | CCN |
     |------|--------|----------------|-----|
     | main.cpp | 5 | RenderFrame | 23 |
     | ui.cpp | 3 | DrawPanel | 18 |
     ```
   - Ask which file(s) to analyze

5. If target provided:
   - Validate file exists
   - Show complexity issues in that file

6. Ask user if needed:
   - Any specific functions they want extracted?
   - Any code they want kept together?

---

## Phase 2: Analysis

**Goal**: Identify extraction candidates and dependencies

**Actions**:
1. Launch modularization-analyst agent for each target file:
   - `"Analyze <file> for module extraction candidates"`

2. For multiple files, launch agents in parallel

3. Collect results and merge:
   - Combine overlapping candidates found in multiple files
   - Consolidate dependency information
   - Identify cross-file extraction opportunities

4. Present findings to user:

```
## Extraction Candidates

### 1. [ModuleName]
- Functions: N
- Cohesion: signals found (data, prefix, functional)
- Internal calls: X%
- Dependencies: clean / has blockers

### 2. [ModuleName]
...

## Issues
- [Circular dependencies, shared state, blockers]

## Extraction Order
1. First module (no dependencies)
2. Second module (depends on #1)
```

---

## Phase 3: User Approval

**Goal**: Confirm which candidates to include in plan

**Actions**:
1. Ask user:
   - Which modules to include in the plan?
   - Any changes to proposed boundaries?
   - Any additional context for the plan?

2. Wait for explicit approval before Phase 4

---

## Phase 4: Write Plan

**Goal**: Create plan document for implementation

**Actions**:
1. Determine filename: `docs/plans/modularize-<target>.md`

2. Write plan using this structure:

```markdown
# Modularize: <target file>

Extracting cohesive modules from <target> to reduce file size and improve maintainability.

## Current State

- `<target>` - N lines, M functions
- Key structs: list primary data types

## Extraction Candidates

### 1. [ModuleName] Module

**Functions to extract**:
- `FunctionName` (line N) - role
- ...

**Primary struct**: `StructName`

**Cohesion evidence**: [why these belong together]

**Dependencies**: [what this module calls/is called by]

---

### 2. [ModuleName] Module
...

---

## Extraction Order

1. **ModuleName** - no dependencies on other candidates
2. **ModuleName2** - depends on #1

## Blockers to Resolve

- [Issues that must be addressed before or during extraction]

## Implementation Notes

- Each module follows Init/Uninit pattern per CLAUDE.md
- Update CMakeLists.txt after each extraction
- Run build verification after each module
- Run /sync-architecture when complete
```

3. Tell user:
   - Plan file location
   - How to use: "Run `/feature-plan implement docs/plans/modularize-<target>.md`"

---

## Output Constraints

- ONLY create `docs/plans/modularize-<target>.md`
- Do NOT modify source files
- Do NOT write implementation code
- Do NOT create header/source file templates
