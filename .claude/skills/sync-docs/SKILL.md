---
name: sync-docs
description: Use when synchronizing codebase documentation after code changes. Triggers on "sync docs", "update documentation", "regenerate docs", or after significant implementation work.
---

# Sync Documentation

Analyze codebase and generate structured documentation to `docs/`.

## Output Documents

| Document | Purpose | Audience |
|----------|---------|----------|
| `docs/stack.md` | Technology stack, dependencies, build system | Humans |
| `docs/architecture.md` | System structure, data flow, modules | Humans |
| `docs/concerns.md` | Tech debt, known issues, fragile areas | Humans |
| `docs/structure.md` | Where to add new code | Claude |
| `docs/conventions.md` | Coding patterns and style rules | Claude |

## Why This Matters

**These documents are consumed by:**

- **Humans** read stack.md, architecture.md, concerns.md for orientation
- **Claude instances** reference structure.md and conventions.md when writing code
- **Feature planning** loads architecture.md and structure.md for context

**What this means for output:**

1. **File paths are critical** - `src/render/shader_setup.cpp` not "the shader setup code"
2. **Patterns matter more than lists** - Show HOW with code examples, not just WHAT exists
3. **Be prescriptive** - "Use PascalCase for functions" not "PascalCase is sometimes used"
4. **CONCERNS.md drives priorities** - Issues identified may become future work

## Philosophy

**Document quality over brevity:**
Include enough detail to be useful as reference.

**Always include file paths:**
Every finding needs a file path in backticks. No exceptions.

**Write current state only:**
Describe what IS, never what WAS. No temporal language.

**Be prescriptive, not descriptive:**
"Use X pattern" is more useful than "X pattern is used."

---

## Git-Based Change Detection

Each document header contains:
```
> Last sync: YYYY-MM-DD | Commit: <short-sha>
```

**Before syncing:**
1. Read `docs/stack.md` and extract `lastSyncCommit` SHA from header
2. Run `git diff <lastSyncCommit>..HEAD --name-only` to list changed files
3. Map changed files to document types using table below
4. Only regenerate documents with relevant changes

**If no `lastSyncCommit` exists:** First run - generate all 5 documents.

### Change-to-Document Mapping

| Changed Files | Documents to Update |
|---------------|---------------------|
| `CMakeLists.txt`, `vcpkg.json`, `*.cmake` | stack.md |
| `src/*/` new directories | architecture.md, structure.md |
| `src/**/*.h` (interface changes) | architecture.md, structure.md, conventions.md |
| `src/**/*.cpp` content | concerns.md (rescan TODOs) |
| `shaders/*` | structure.md |
| `CLAUDE.md` | conventions.md |

**After syncing:** Update header in each regenerated doc:
```bash
git rev-parse --short HEAD  # Get current SHA
```

---

## Process

### Phase 1: Detect Changes

1. Check if `docs/stack.md` exists (first-run vs incremental)
2. If incremental:
   - Extract `lastSyncCommit` from header: `grep "Commit:" docs/stack.md`
   - Run `git diff <lastSyncCommit>..HEAD --name-only`
   - Map changed files to documents using table above
3. Report which documents will be generated vs skipped

**First run:** Generate all 5 documents.

### Phase 2: Generate Documents

Spawn agents with `run_in_background: true` for each document type needed.

**Agent prompt format:**
```
Generate docs/<name>.md for this codebase.

Template: Read .claude/skills/sync-docs/templates/<name>.md
Output: Write to docs/<name>.md

[Include relevant exploration commands from below]

Fill template with findings. Use "Not detected" for missing items.
Always include file paths in backticks.

Return only confirmation: document path and line count.
```

### Phase 3: Finalize

1. Wait for agents with `TaskOutput`
2. Verify documents exist
3. Report summary

---

## Exploration Commands by Document Type

### stack (tech focus)

```bash
# Build system
cat CMakeLists.txt 2>/dev/null | head -100
cat vcpkg.json 2>/dev/null

# Config files
ls -la *.cmake .clang-* 2>/dev/null

# Compiler/standard detection
grep -i "CMAKE_CXX_STANDARD\|set(CMAKE" CMakeLists.txt 2>/dev/null
```

### architecture (arch focus)

```bash
# Directory structure
find . -type d -not -path '*/.git/*' -not -path '*/build/*' | head -50

# Entry points
ls src/main.* src/*/main.* 2>/dev/null

# Module discovery
ls -d src/*/ 2>/dev/null

# Header analysis for interfaces
find src/ -name "*.h" | head -20
```

### structure (arch focus)

```bash
# Full file listing
find src/ -type f \( -name "*.cpp" -o -name "*.h" \) | head -100

# Shader files
find shaders/ -type f 2>/dev/null | head -30

# Config patterns
find src/ -name "*_config.h" 2>/dev/null

# Codebase size (SLOC by language)
cloc src/ shaders/ --force-lang="GLSL,fs" 2>/dev/null
```

### conventions (quality focus)

```bash
# Sample source files for pattern analysis
ls src/*.cpp src/*/*.cpp 2>/dev/null | head -10

# Read CLAUDE.md for authoritative rules
cat CLAUDE.md 2>/dev/null

# Formatting config
ls .clang-format .editorconfig 2>/dev/null
```

### concerns (concerns focus)

```bash
# TODO/FIXME comments
grep -rn "TODO\|FIXME\|HACK\|XXX" src/ --include="*.cpp" --include="*.h" 2>/dev/null | head -50

# Large files (line count hotspots)
find src/ -name "*.cpp" -exec wc -l {} \; 2>/dev/null | sort -rn | head -10

# Complexity hotspots (CCN > 15, sorted by complexity, top 10)
lizard src/ -l cpp -w -s cyclomatic_complexity 2>/dev/null | head -10

# Incomplete implementations
grep -rn "// TODO\|NotImplemented\|assert(false)" src/ --include="*.cpp" 2>/dev/null | head -30
```

---

## Critical Rules

**WRITE DOCUMENTS DIRECTLY.** Agents write to `docs/`, not return content.

**ALWAYS INCLUDE FILE PATHS.** Every finding needs a path in backticks.

**USE THE TEMPLATES.** Fill the template structure from `.claude/skills/sync-docs/templates/`.

**BE THOROUGH.** Explore deeply. Read actual files. Don't guess.

**RETURN ONLY CONFIRMATION.** Agent responses should be ~10 lines max.

---

## Success Criteria

- [ ] All requested documents written to `docs/`
- [ ] Documents follow template structure
- [ ] File paths included throughout
- [ ] Agents completed without errors
