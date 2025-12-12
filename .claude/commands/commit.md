# Commit Changes

Create a git commit following project conventions.

## Commit Message Format

**Subject line:** Imperative mood, 50 chars max, capitalized, no period

- "Add spectrum analyzer" NOT "Added spectrum analyzer"
- Test: "If applied, this commit will [your subject]"

**Body:** Explain WHY, not HOW. Code shows what changed; only the message explains the reasoning.

**Scope:** One logical change per commit. Atomic commits simplify review and revert.

## Instructions

1. Run `git status` and `git diff --staged` (or `git diff` if nothing staged) to understand changes
2. Run `git log --oneline -5` to see recent commit style
3. Draft a commit message following the format above
4. Stage relevant files and commit using HEREDOC format:

```bash
git commit -m "$(cat <<'EOF'
Subject line here

Body explaining why this change was made.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"
```

## Safety

- NEVER amend commits you didn't create (check `git log -1 --format='%an %ae'`)
- NEVER force push to main/master
- NEVER skip hooks unless explicitly requested
