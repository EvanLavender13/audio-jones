# Codebase Structure

> Last sync: [YYYY-MM-DD] | Commit: [SHA]

## Codebase Size

| Language | Files | Code | Comments |
|----------|-------|------|----------|
| [lang] | [n] | [n] | [n] |
| **Total** | [n] | [n] | [n] |

## Directory Layout

```
[project-root]/
├── [dir]/          # [Purpose]
├── [dir]/          # [Purpose]
└── [file]          # [Purpose]
```

## Directory Purposes

**[Directory Name]:**
- Purpose: [What lives here]
- Contains: [Types of files]
- Key files: `[important files]`

For large, pattern-based directories (`src/effects/`, `shaders/`): show file count + category names only. Do NOT list individual file names or key files — the naming conventions section makes paths derivable. Example: "76 effect modules across 14 categories: artistic, cellular, color, ..."

For small directories (`src/analysis/`, `src/audio/`, `src/config/`, etc.): include key files.

## Key File Locations

**Entry Points:**
- `[path]`: [Purpose]

**Configuration:**
- `[path]`: [Purpose]

**Core Logic:**
- `[path]`: [Purpose]

Do NOT include an "Effect Modules" subsection — effects are covered by category in Directory Purposes.

## Naming Conventions

**Files:**
- [Pattern]: [Example]

**Directories:**
- [Pattern]: [Example]

## Where to Add New Code

**[Code Type]:**
- Primary code: `[path]`
- Config: `[path]`
- Registration: `[path]`

**[Code Type]:**
- Primary code: `[path]`
- Config: `[path]`
- Registration: `[path]`

## Special Directories

**[Directory]:**
- Purpose: [What it contains]
- Generated: [Yes/No]
- Committed: [Yes/No]

---

*Run `/sync-docs` to regenerate.*
