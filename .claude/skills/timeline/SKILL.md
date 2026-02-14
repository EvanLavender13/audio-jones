---
name: timeline
description: Use when generating a project milestone timeline from git history. Triggers on "timeline", "milestones", "project history", or when the user wants to see the story of how the project evolved.
---

# Project Timeline

Generate a milestone timeline from git history using chunked parallel analysis and a merge pass.

## Core Principles

- **Milestones, not features**: A milestone is when the project *became something it wasn't before* — not when it got more of what it already had
- **First-of-kind only**: The first of a new capability class is a milestone. The Nth is not.
- **Commit hashes are sacred**: Every entry must reference a real, verified commit hash. Never fabricate.
- **Incremental by default**: First run processes full history. Subsequent runs process only new commits.
- **Divide and conquer**: Chunk the log across parallel agents to avoid context overload and hash fabrication.
- **Let the history speak**: Don't prescribe how many milestones to find. Early history is naturally denser — foundations get laid fast, then the project matures. The distribution is logarithmic, not uniform.

---

## What Is a Milestone?

A milestone changes the project's *identity* or *capability class*. Ask: "After this commit, can the project do something fundamentally new?"

**IS a milestone**:
- First working prototype (the project exists)
- New architectural layer (persistence, automation, compute pipeline)
- Structural reorganization that changes how the project scales
- New category of capability (not a new instance of an existing category)
- Paradigm shift in workflow (fixed pipeline becomes user-composable)

**IS NOT a milestone**:
- Adding the Nth instance of something that already exists
- Bug fixes, polish, parameter tweaks
- Individual feature additions within an established pattern
- Documentation updates
- Refactors that don't change capability

---

## Output Format

File: `docs/timeline.md`

```markdown
# [Project Name] Timeline

<!-- last: <full-commit-hash> -->

**<YYYY-MM-DD>** — **[Milestone Title]** — `<short-hash>`
> One or two sentences: what changed about the project's identity or capabilities.

**<YYYY-MM-DD>** — **[Milestone Title]** — `<short-hash>`
> One or two sentences.

...
```

Rules:
- Entries in chronological order (oldest first)
- Each entry leads with date for scannability, hash trails at the end
- Date from `git log --format='%as' -1 <hash>` (author date, YYYY-MM-DD)
- Full hash in the `<!-- last: -->` comment for incremental tracking
- One-line title, 1-2 sentence description
- Description says what the project *became*, not what code changed

---

## Phase 1: Determine Range

**Goal**: Figure out what commits to process.

**Actions**:
1. Check if `docs/timeline.md` exists
2. **If it exists**: extract the `<!-- last: <hash> -->` comment. Run `git log --oneline <hash>..HEAD | wc -l` to count new commits.
   - If 0 new commits: tell user "Timeline is up to date" and stop.
   - If new commits exist: set range to `<hash>..HEAD`
3. **If it doesn't exist**: set range to full history. Run `git log --oneline | wc -l` for total count.
4. Report to user: "Processing N commits (full history / since last run)"

---

## Phase 2: Chunk and Dispatch

**Goal**: Split the commit range into manageable chunks and fan out agents.

**Actions**:
1. Run `git log --oneline --reverse [range]` to get the full list
2. Split into chunks of ~200 commits each. Record exact hash boundaries for each chunk.
3. Dispatch one agent per chunk in parallel (single message, multiple Task calls):

```
Task(
  prompt="[chunk agent prompt below]",
  subagent_type="general-purpose",
  description="Scan commits [start-hash]..[end-hash]"
)
```

### Chunk Agent Prompt

```markdown
# Find Milestone Candidates

You are scanning a chunk of git history for milestone candidates. A milestone is when the project BECAME SOMETHING IT WASN'T BEFORE — not when it got more of what it already had.

## Your Commit Range

[PASTE THE EXACT `git log --oneline --reverse` OUTPUT FOR THIS CHUNK]

## Rules

- A milestone changes the project's IDENTITY or CAPABILITY CLASS
- The first of a new kind of thing is a candidate. The 2nd, 3rd, Nth is NOT.
- Architectural reorganizations that change how the project scales are candidates.
- Bug fixes, polish, parameter tweaks, documentation are NEVER milestones.
- Adding one more feature within an established pattern is NOT a milestone.
- Most chunks will have 0-2 candidates. Returning an empty list is fine and expected.

## What to Return

For each candidate:

- **Hash**: The exact short hash from the log (copy it character-for-character)
- **Title**: 3-6 word milestone name
- **Description**: One sentence — what the project became after this commit
- **Confidence**: High (clearly transformative) or Medium (judgment call)

If your chunk has no milestones, return an empty list.

Return ONLY the structured list. No commentary.
```

---

## Phase 3: Merge and Deduplicate

**Goal**: Combine candidates from all chunks into a final timeline.

**Actions**:
1. Collect all candidate lists from chunk agents
2. Apply deduplication rules:
   - If multiple candidates describe the same capability emerging, keep only the first
   - If a candidate is "more of what already exists" in context of earlier candidates, drop it
   - Prefer High confidence over Medium when they overlap
3. Verify each surviving candidate's hash: `git log --format='%h %as' -1 <hash>` — confirm it exists and capture the date
4. Order chronologically

---

## Phase 4: Write Timeline

**Goal**: Write or update `docs/timeline.md`

**Actions**:

### Full History (new file)
1. Get the HEAD hash: `git rev-parse HEAD`
2. Get the project name from the repository root directory name
3. Write `docs/timeline.md` with all entries and `<!-- last: <full-HEAD-hash> -->`

### Incremental (append to existing)
1. Read existing `docs/timeline.md`
2. Remove the old `<!-- last: -->` line
3. Append new entries after the last existing entry
4. Add new `<!-- last: <full-HEAD-hash> -->` at the end
5. If no new milestones found in the range, just update the `<!-- last: -->` hash

---

## Phase 5: Summary

**Actions**:
1. Tell user:
   - How many commits processed
   - How many milestones identified
   - File location
2. If full history: display the complete timeline
3. If incremental with new milestones: display only the new entries

---

## Output Constraints

- ONLY create/modify `docs/timeline.md`
- Do NOT create branch, PR, or commit (user decides when to commit)
- Do NOT include entries without verified commit hashes and dates

---

## Red Flags - STOP

| Thought | Reality |
|---------|---------|
| "This feature addition is important" | Is it the FIRST of its kind? If not, skip it. |
| "I remember this hash" | Copy it character-for-character from `git log` output. Never type from memory. |
| "I'll process all commits in one pass" | Chunk and dispatch. Single-pass leads to hallucinated hashes. |
| "Every new module is a milestone" | Only the FIRST one in its class. The rest are "more of what it already is." |
| "I should add context about what came before" | Each chunk agent works independently. The merge pass handles context. |
| "This chunk needs more candidates" | Most chunks have zero milestones. That's correct. Don't inflate. |
