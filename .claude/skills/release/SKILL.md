---
name: release
description: Use when creating a tagged release with changelog. Triggers on "release", "tag a release", "cut a release", or when ready to publish a new version.
---

# Create Release

Draft a changelog entry, commit it, and tag locally. User pushes when ready.

## Core Principles

- **One bullet per distinct feature**: Each meaningful change gets its own line
- **User-facing changes only**: Skip internal refactors, code cleanup, CI changes
- **Goofy ALL-CAPS tag names**: Derive from the release content, match existing preset/tag naming energy
- **Tag locally, never push**: User decides when to push the tag

---

## Phase 1: Gather Context

**Goal**: Understand what changed since the last release

**Actions**:
1. Find the most recent tag:
   ```bash
   git describe --tags --abbrev=0 2>/dev/null
   ```
2. If no tags exist: use the full commit history (`git log`)
3. If tags exist: get commits since last tag (`git log <last-tag>..HEAD`)
4. Count commits and report: "N commits since last release (TAG)" or "N total commits (first release)"

**STOP**: If there are zero new commits since the last tag, tell user there's nothing to release.

---

## Phase 2: Draft Changelog

**Goal**: Write a concise, user-facing changelog entry

**Actions**:
1. Read full commit messages and sort each change into one of three categories:
   - **New Effects**: new generators, transforms, simulations, presets
   - **Enhancements**: reworks, new params/modes on existing effects, UI improvements, infrastructure that's user-facing
   - **Fixes**: bug fixes
2. Draft one bullet per distinct feature under its category. Describe what the user gets, not what files changed. Omit a category if it has no entries.
3. Present the draft to the user for editing

**STOP**: Wait for user approval or edits before proceeding.

---

## Phase 3: Suggest Tag Name

**Goal**: Propose a fun ALL-CAPS tag name

**Actions**:
1. Check existing tags and preset names to avoid collisions
2. Read the existing tags and preset filenames to understand the project's naming style — match that energy
3. Suggest 3-4 tag name options — ALL-CAPS, 4-10 characters
   - Each name must be a novel nonsense word derived from the release's actual content (effects, techniques, visual themes)
   - Mash up, abbreviate, or phonetically corrupt real words from the changelog — do NOT invent names from thin air
   - No "cool" or "epic" names. Goofy, dumb, weird. But grounded in what changed.
   - NEVER reuse examples from this document or previous tags. Every suggestion must be original.
4. Present options with a one-line rationale tying each name back to the release content, user picks or provides their own

**STOP**: Wait for user to confirm the tag name.

---

## Phase 4: Write CHANGELOG.md

**Goal**: Create or update the changelog file

**Actions**:
1. If `CHANGELOG.md` doesn't exist: create it with header `# Changelog` followed by the new entry
2. If it exists: prepend the new section after the `# Changelog` header, above existing entries
3. New section format (date is today's date, YYYY-MM-DD):
   ```markdown
   ## YYYY-MM-DD — TAG-NAME

   ### New Effects
   - ...

   ### Enhancements
   - ...

   ### Fixes
   - ...
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
- Do NOT list internal commits (file renames, refactors) as bullets
- Do NOT invent tag names that collide with existing tags or preset filenames
- Do NOT skip user approval of the changelog draft or tag name

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "I'll list every commit" | One bullet per distinct feature, not per commit. |
| "I'll push the tag for them" | Tag locally. User pushes. |
| "This refactor is worth mentioning" | User-facing changes only. |
| "No commits since last tag" | Nothing to release. Tell user and stop. |
| "I'll pick the tag name myself" | Suggest options. User decides. |
