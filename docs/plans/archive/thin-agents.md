# Thin Doc-Referencing Agents

Replace hardcoded agent definitions with thin methodology shells that read sync-docs-maintained documentation at runtime. Eliminates staleness (stale file paths like `shader_setup_<category>.cpp`) and maintenance burden (duplicated project knowledge).

**Research**: `docs/research/thin-agents.md`

## Design

### Principle

Split agent knowledge into two categories:
- **Methodology** (stable): HOW to explore or review — lives in agent file, rarely changes
- **Project knowledge** (volatile): WHAT to check or trace — lives in docs, maintained by sync-docs

### Agent: code-explorer

**Frontmatter**:
- `name: code-explorer`
- `description: Traces feature implementations through codebase layers. Use when exploring features or understanding how existing code works.`
- `tools: [Glob, Grep, LS, Read]`
- `model: inherit`
- `color: yellow`

**Body** (~25 lines):
- Role: "You are a codebase analyst. You trace feature implementations through a project's architecture."
- First instruction: Read `docs/architecture.md` and `docs/structure.md` before tracing any code
- Keep the output format section verbatim from current agent (entry points, data flow, key files, patterns, integration points)
- Remove: data flow diagram, all tracing patterns, all hardcoded file paths

### Agent: code-reviewer

**Frontmatter**:
- `name: code-reviewer`
- `description: Reviews code against plan and conventions with confidence-based filtering. Use when feature-review or implement needs code quality analysis.`
- `tools: [Glob, Grep, LS, Read]`
- `model: inherit`
- `color: red`

**Body** (~30 lines):
- Role: "You are a code reviewer. You review implementations against their design plan and project conventions."
- Inputs: plan path (via Read), diff stat + changed file list (provided in prompt by calling skill), focus area
- First instruction: Read `docs/conventions.md` as the review checklist — this replaces all hardcoded naming patterns, config patterns, shader patterns, and integration checklist
- Keep: confidence scoring system (0-100 scale, only report >= 80), output format (severity grouping, file:line refs, concrete fix suggestions), focus area system (Simplicity/DRY, Bugs/Correctness, Conventions)
- Remove: all AudioJones-specific checks, hardcoded naming patterns, stale file paths, integration checklist
- Change: agent no longer runs `git diff` — it receives stat output in its prompt and uses Read to inspect changed files

### Skill: feature-review

Changes to `.claude/skills/feature-review/SKILL.md`:

**Phase 1 (Setup)**: After running `git diff main...HEAD --stat`, also capture the stat output into a variable for later use in agent prompts.

**Phase 3 (Launch Reviewers)**: Update agent dispatch prompts to include:
1. The `git diff --stat` output (file list with change counts)
2. Instruction: "Use Read to inspect the changed files. Do NOT run git commands."

No other phases change.

### Skill: implement

Changes to `.claude/skills/implement/SKILL.md`:

**Phase 4 (Code Review)**: Update reviewer dispatch prompt to include:
1. `git diff main...HEAD --stat` output captured by orchestrator
2. Instruction: "Use Read to inspect the changed files. Do NOT run git commands."

No other phases change.

### MEMORY.md

Remove the workaround note: "Do NOT use `code-reviewer` or `code-explorer` agent types — they are broken and use excessive Bash/git commands requiring constant approval. Use `general-purpose` agents for reviews instead, with explicit instructions to use only Read/Grep/Glob tools."

---

## Tasks

### Wave 1: Agent Rewrites

#### Task 1.1: Rewrite code-explorer agent

**Files**: `.claude/agents/code-explorer.md`

**Do**: Rewrite the file. Use the frontmatter and body spec from the Design section above. The complete file should be ~25-30 lines. Keep the output format section (return: entry points, data flow, key files, patterns, integration points) from the current file. Remove everything else (data flow diagram, tracing patterns for effects/simulations/drawables/modulation, key integration points section). Add instruction to read `docs/architecture.md` and `docs/structure.md` before beginning exploration.

**Verify**: File is under 35 lines. No hardcoded file paths. Tools list is exactly `[Glob, Grep, LS, Read]`.

#### Task 1.2: Rewrite code-reviewer agent

**Files**: `.claude/agents/code-reviewer.md`

**Do**: Rewrite the file. Use the frontmatter and body spec from the Design section above. The complete file should be ~30-35 lines. Keep the confidence scoring section (0-100 scale, only report >= 80) and output format section (severity grouping, file:line refs, concrete fix suggestions) from the current file. Keep the focus area system (Simplicity/DRY, Bugs/Correctness, Conventions). Remove: all "AudioJones-Specific Checks" (naming patterns, config patterns, shader patterns, integration checklist). Add instruction to read `docs/conventions.md` as the review checklist. Change inputs: agent now receives diff stat and changed file list in its prompt (no `git diff` command). Add instruction to use Read tool to inspect changed files.

**Verify**: File is under 40 lines. No hardcoded file paths. No Bash in tools. Tools list is exactly `[Glob, Grep, LS, Read]`.

---

### Wave 2: Skill + Memory Updates

#### Task 2.1: Update feature-review skill

**Files**: `.claude/skills/feature-review/SKILL.md`
**Depends on**: Wave 1 complete

**Do**: Two edits:

1. **Phase 3, multi-agent dispatch** (around line 61): Update the agent prompt description. Currently says "Each agent receives: plan path + focus area". Change to: "Each agent receives: plan path, focus area, and `git diff --stat` output. Include in prompt: the stat output and instruction 'Use Read to inspect the changed files listed above. Do NOT run git or bash commands.'"

2. **Phase 3, single-agent dispatch** (around line 71): Same change. Currently says "Agent receives: plan path". Change to include stat output and Read instruction.

**Verify**: Both dispatch sections mention passing stat output and the Read instruction.

#### Task 2.2: Update implement skill

**Files**: `.claude/skills/implement/SKILL.md`
**Depends on**: Wave 1 complete

**Do**: One edit:

**Phase 4 (Code Review)** (around line 152): Currently says "Agent receives: plan path, focus on both correctness and consistency". Change to: "Agent receives: plan path, `git diff main...HEAD --stat` output, focus on both correctness and consistency (code should read like one author, not N agents). Include in prompt: the stat output and instruction 'Use Read to inspect the changed files listed above. Do NOT run git or bash commands.'"

**Verify**: Phase 4 mentions passing stat output and the Read instruction.

#### Task 2.3: Update MEMORY.md

**Files**: `memory/MEMORY.md` (full path: `/home/evan/.claude/projects/-mnt-c-Users-EvanUhhh-source-repos-AudioJones/memory/MEMORY.md`)
**Depends on**: Wave 1 complete

**Do**: Remove the line that says "Do NOT use `code-reviewer` or `code-explorer` agent types — they are broken and use excessive Bash/git commands requiring constant approval. Use `general-purpose` agents for reviews instead, with explicit instructions to use only Read/Grep/Glob tools."

**Verify**: MEMORY.md no longer contains any mention of broken agents or the workaround.

---

## Final Verification

- [ ] Both agent files are thin: no hardcoded file paths, no project-specific checks
- [ ] Both agents have tools `[Glob, Grep, LS, Read]` only — no Bash, TodoWrite, WebSearch, etc.
- [ ] feature-review skill passes diff stat to reviewer agents
- [ ] implement skill passes diff stat to reviewer agent
- [ ] MEMORY.md broken-agent workaround is removed
- [ ] No other files modified
