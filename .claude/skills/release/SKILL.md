---
name: release
description: Use when creating a tagged release with changelog. Triggers on "release", "tag a release", "cut a release", or when ready to publish a new version.
---

# Create Release

Draft a changelog entry, commit it, and tag locally. User pushes when ready.

## Core Principles

- **Grouped counts, not commit logs**: "12 new transform effects" not 12 bullet points
- **User-facing changes only**: Skip internal refactors, code cleanup, CI changes
- **Fun ALL-CAPS tag names**: Match the project's preset naming vibe (SMOOTHBOB, GALACTO, ZOOP)
- **Tag locally, never push**: User decides when to push the tag

---

## Phase 1: Gather Context

**Goal**: Understand what changed since the last release

**Actions**:
1. Find the most recent tag:
   ```bash
   git describe --tags --abbrev=0 2>/dev/null
   ```
2. If no tags exist: use the full commit history (`git log --oneline`)
3. If tags exist: get commits since last tag (`git log --oneline <last-tag>..HEAD`)
4. Count commits and report: "N commits since last release (TAG)" or "N total commits (first release)"

**STOP**: If there are zero new commits since the last tag, tell user there's nothing to release.

---

## Phase 2: Draft Changelog

**Goal**: Write a concise, user-facing changelog entry

**Actions**:
1. Scan the commit subjects for themes. Group into categories:
   - New effects (count by type: transforms, generators, simulations)
   - New features (modulation, presets, UI capabilities)
   - Notable fixes (only if user-visible)
   - Infrastructure (only if user-facing, e.g., "Windows builds no longer need VC++ Redistributable")
2. Draft 3-6 bullet points using grouped counts:
   - **Good**: "12 new transform effects across 9 categories"
   - **Bad**: "Add kaleidoscope effect" repeated 12 times
   - **Good**: "Modulation engine with LFO routing to any parameter"
   - **Bad**: "Add param_registry.cpp, add modulation_engine.cpp, add lfo.cpp"
3. Present the draft to the user for editing

**STOP**: Wait for user approval or edits before proceeding.

---

## Phase 3: Suggest Tag Name

**Goal**: Propose a fun ALL-CAPS tag name

**Actions**:
1. Check existing tags and preset names to avoid collisions
2. Suggest 3 tag name options — short, punchy, ALL-CAPS, 4-8 characters
   - Draw from the vibe of the changes (e.g., heavy on simulations → organic/alive names)
   - Match the energy of existing names: SMOOTHBOB, GALACTO, ZOOP, BINGBANG
3. Present options, user picks or provides their own

**STOP**: Wait for user to confirm the tag name.

---

## Phase 4: Write CHANGELOG.md

**Goal**: Create or update the changelog file

**Actions**:
1. If `CHANGELOG.md` doesn't exist: create it with header `# Changelog` followed by the new entry
2. If it exists: prepend the new section after the `# Changelog` header, above existing entries
3. New section format:
   ```markdown
   ## TAG-NAME

   - Bullet point one
   - Bullet point two
   ```
4. Stage `CHANGELOG.md`

---

## Phase 5: Commit and Tag

**Goal**: Record the changelog and create the tag

**Actions**:
1. Commit `CHANGELOG.md` with message: `Add TAG-NAME changelog entry`
2. Create annotated tag:
   ```bash
   git tag -a TAG-NAME -m "TAG-NAME"
   ```
3. Report to user:
   - "Tagged `TAG-NAME` locally"
   - "Push when ready: `git push origin main && git push origin TAG-NAME`"

---

## Output Constraints

- Do NOT push tags or commits to remote
- Do NOT include every commit as a separate bullet
- Do NOT write more than 6 changelog bullets per release
- Do NOT invent tag names that collide with existing tags or preset filenames
- Do NOT skip user approval of the changelog draft or tag name

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I'll list every commit" | Group and count. 3-6 bullets max. |
| "I'll push the tag for them" | Tag locally. User pushes. |
| "This refactor is worth mentioning" | User-facing changes only. |
| "No commits since last tag" | Nothing to release. Tell user and stop. |
| "I'll pick the tag name myself" | Suggest options. User decides. |
