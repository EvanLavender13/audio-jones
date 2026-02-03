# Codebase Concerns

> Last sync: [YYYY-MM-DD] | Commit: [SHA]

## Tech Debt

**[Area/Component]:**
- Issue: [What's the shortcut/workaround]
- Files: `[file paths]`
- Impact: [What breaks or degrades]
- Fix approach: [How to address it]

## Known Bugs

**[Bug description]:**
- Symptoms: [What happens]
- Files: `[file paths]`
- Trigger: [How to reproduce]
- Workaround: [If any]

## Performance Bottlenecks

**[Slow operation]:**
- Problem: [What's slow]
- Files: `[file paths]`
- Cause: [Why it's slow]
- Improvement path: [How to speed up]

## Fragile Areas

**[Component/Module]:**
- Files: `[file paths]`
- Why fragile: [What makes it break easily]
- Safe modification: [How to change safely]

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `[path]` | [n] | [Why it's large] |

## Complexity Hotspots

Functions with cyclomatic complexity > 15 (sorted by CCN):

| Function | Location | CCN | NLOC | Why |
|----------|----------|-----|------|-----|
| [name] | `[file:line]` | [n] | [n] | [Why it's complex] |

## Dependencies at Risk

**[Package]:**
- Risk: [What's wrong]
- Impact: [What breaks]
- Migration plan: [Alternative]

## Missing Features

**[Feature gap]:**
- Problem: [What's missing]
- Blocks: [What can't be done]

## TODO/FIXME Inventory

| Location | Type | Note |
|----------|------|------|
| `[file:line]` | [TODO/FIXME/HACK] | [text] |

---

*Run `/sync-docs` to regenerate.*
