---
name: commit
description: Use when creating a git commit for staged or unstaged changes. Triggers on "commit this", "save my changes", or when implementation work completes and needs version control.
---

# Commit Changes

Create a git commit following project conventions.

## Commit Message Format

**Subject line:** Imperative mood, 50 chars max, capitalized, no period

- "Add spectrum analyzer" NOT "Added spectrum analyzer"
- Test: "If applied, this commit will [your subject]"

**Body:** Optional. Most commits need only a subject line. When included:

- State WHAT capability exists after this commit (1-2 sentences max)
- Skip if subject line is self-explanatory

**Anti-patterns** (never include):

- Implementation journey ("was implemented and reverted", "tried X then Y")
- Redundant expansion of subject ("This adds..." when subject says "Add...")
- Phase/step references from planning docs
- Rationale for rejected approaches

**Scope:** One logical change per commit. Atomic commits simplify review and revert.

## Instructions

1. Run `git status` and `git diff --staged` (or `git diff` if nothing staged) to understand changes
2. Run `git log --oneline -5` to see recent commit style
3. Draft a commit message following the format above
4. Stage relevant files and commit:

**Simple commit (preferred):**
```bash
git commit -m "Add vertical offset parameter for linear waveforms"
```

**With body (only when subject needs clarification):**
```bash
git commit -m "$(cat <<'EOF'
Add vertical offset parameter for linear waveforms

Enables vertical stacking of multiple waveforms in linear mode.
EOF
)"
```

## Safety

- NEVER amend commits you didn't create (check `git log -1 --format='%an %ae'`)
- NEVER force push to main/master
- NEVER skip hooks unless explicitly requested
