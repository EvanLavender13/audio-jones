# Thin Doc-Referencing Agents

Replace the two hardcoded agent definitions with thin methodology shells that read sync-docs-maintained documentation at runtime, eliminating the staleness and maintenance burden.

## Classification

- **Category**: General — developer tooling / agent architecture
- **Scope**: `.claude/agents/` definitions + skills that dispatch them

## References

- [obra/superpowers code-reviewer](https://github.com/anthropics/claude-code-plugins) - Generic reviewer pattern with zero project knowledge
- Existing `sync-docs` skill — already maintains `docs/conventions.md`, `docs/architecture.md`, `docs/structure.md`

## Problem

Both agents embed hardcoded AudioJones knowledge that duplicates and drifts from the docs:

| Agent line | Hardcoded knowledge | Already in docs |
|---|---|---|
| explorer:34 | `shader_setup_<category>.cpp` | **STALE** — colocation removed these files |
| explorer:36 | `imgui_effects_<category>.cpp` | **STALE** — UI colocation removed these |
| reviewer:41 | `shader_setup_<category>.cpp` | **STALE** |
| reviewer:48 | `imgui_effects_<category>.cpp` | **STALE** |
| explorer:12-25 | Data flow diagram | `docs/architecture.md` |
| explorer:29-55 | Tracing patterns with file paths | `docs/structure.md` |
| reviewer:27-32 | Naming patterns | `docs/conventions.md` "Naming Patterns" |
| reviewer:34-37 | Config struct patterns | `docs/conventions.md` "Module Design" |
| reviewer:44-50 | Integration checklist | `docs/conventions.md` "Effect Descriptor Pattern" |

Both also have excessive tool access (reviewer has Bash, explorer has BashOutput/KillShell/TodoWrite).

## Design

### Principle

Split agent knowledge into two categories:
- **Methodology** (stable): HOW to explore or review — lives in agent file, rarely changes
- **Project knowledge** (volatile): WHAT to check or trace — lives in docs, maintained by sync-docs

### Agent: code-explorer

**Keep**: Output format (entry points, data flow, key files, patterns, integration points)
**Remove**: All hardcoded file paths, data flow diagram, tracing patterns
**Add**: Instructions to read `docs/architecture.md` and `docs/structure.md` before tracing
**Tools**: `[Glob, Grep, LS, Read]` only

### Agent: code-reviewer

**Keep**: Confidence scoring (80+ threshold), output format (severity grouping, file:line refs), focus area system
**Remove**: All AudioJones-specific checks, hardcoded naming patterns, stale file paths, integration checklist
**Add**: Instructions to read `docs/conventions.md` as the review checklist
**Replace**: `git diff` via Bash → receive diff context in the dispatch prompt from the calling skill
**Tools**: `[Glob, Grep, LS, Read]` only

### Skill updates

Three skills dispatch these agents and need updating:

| Skill | Current | Change |
|---|---|---|
| `feature-plan/SKILL.md` | `subagent_type=code-explorer` | Keep — agent type name unchanged |
| `feature-review/SKILL.md` | `code-reviewer` agents | Keep — agent type name unchanged |
| `implement/SKILL.md` | `code-reviewer` agent | Keep — agent type name unchanged |

Since agent type names stay the same, skills only need minor prompt adjustments:
- **feature-review**: Include `git diff` output in the agent prompt instead of telling agents to run `git diff` themselves
- **implement**: Same — pass diff context in prompt
- **feature-plan**: No change needed (explorers already just read code)

### MEMORY.md update

Remove the "Do NOT use code-reviewer or code-explorer" workaround note — agents will be functional again.

## Files Changed

| File | Action |
|---|---|
| `.claude/agents/code-explorer.md` | Rewrite: ~25 lines, methodology only |
| `.claude/agents/code-reviewer.md` | Rewrite: ~30 lines, methodology only |
| `.claude/skills/feature-review/SKILL.md` | Update dispatch prompts to pass diff context |
| `.claude/skills/implement/SKILL.md` | Update dispatch prompt to pass diff context |
| `memory/MEMORY.md` | Remove broken-agent workaround note |

## Notes

- Agent type names (`code-explorer`, `code-reviewer`) are unchanged — no need to update every skill reference
- If `sync-docs` adds a new doc file relevant to review (e.g., a shader conventions doc), just add one line to the agent's "read these files" list
- The confidence scoring system in code-reviewer is stable methodology, not project knowledge — it stays
