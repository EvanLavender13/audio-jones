# LLM Codebase Review Methodology

Process guide for AI agents conducting systematic code reviews. This document describes HOW to review, not WHAT to find.

## Output Location

Place all review artifacts in `docs/reviews/`:
- Working notes: `docs/reviews/YYYY-MM-DD-REVIEWER-notes.md`
- Final report: `docs/reviews/YYYY-MM-DD-REVIEWER-review.md`

Replace `REVIEWER` with the model or agent name (e.g., `claude-opus`, `gpt-4o`).

Create the directory if it does not exist.

## Philosophy

A review discovers facts about the code, then evaluates those facts against project conventions. The agent builds understanding through investigation, not by applying preset conclusions.

**Core principle**: `CLAUDE.md` is the authoritative source for project conventions. Read it first and treat its rules as overriding general best practices.

## Phase 1: Context Discovery

Before evaluating anything, understand what you're reviewing.

### 1.1 Extract Project Conventions

Read `CLAUDE.md` and answer:
- What patterns does this project explicitly allow?
- What patterns does this project explicitly forbid?
- What naming conventions apply?
- What architectural decisions constrain implementation?

Build a mental model of "correct code" for THIS project. Do not assume general best practices apply - the project may intentionally deviate.

### 1.2 Map the Architecture

Read `docs/architecture.md` (if it exists) and answer:
- What are the major subsystems?
- How does data flow between them?
- What owns what state?
- What are the system boundaries?

If no architecture doc exists, construct this understanding by exploring the codebase.

### 1.3 Identify Entry Points

Find:
- Where does execution begin?
- What are the main loops or event handlers?
- Where do external inputs enter the system?
- Where do outputs leave the system?

## Phase 2: Systematic Exploration

Explore methodically to ensure coverage. Do not review files in arbitrary order.

### 2.1 Dependency-Order Traversal

Start from modules with zero internal dependencies (leaves), work toward modules that depend on everything (roots). This ensures you understand foundations before reviewing code that uses them.

To find dependency order:
1. List all source files
2. For each file, identify what other project files it includes
3. Sort topologically: files with no project includes first

### 2.2 Per-Module Investigation

For each module, answer these discovery questions:

**Purpose**
- What transformation or service does this module provide?
- What is its single responsibility?

**Interface**
- What functions/types does it expose?
- What preconditions do callers need to satisfy?
- What postconditions does it guarantee?

**Resources**
- What does this module allocate (memory, handles, GPU resources)?
- Where is each allocation released?
- What happens if allocation fails?

**Dependencies**
- What external libraries does it use?
- What project modules does it depend on?
- Could any dependency be removed?

**Concurrency**
- Is this code called from multiple threads?
- What data could be accessed concurrently?
- What synchronization exists?

### 2.3 Extraction Opportunities

Identify functions and modules doing too much. These are candidates for splitting.

**Function scope**
- Does the function name require "and" to describe accurately?
- Are there local variables used in separate, non-overlapping sections?
- Does nested logic have a clear name if extracted?
- Does the function mix abstraction levels (orchestration with low-level details)?

**Module scope**
- Does the file handle multiple unrelated responsibilities?
- Do comment banners separate distinct "sections"?
- Do some functions never interact with others in the same file?
- Can you state the module's purpose in one sentence without "and"?

**Repeated patterns**
- Does similar logic appear in multiple locations?
- Would a helper function clarify intent?

### 2.4 Record Observations

As you explore, record factual observations separately from judgments:

```
OBSERVATION: Function X allocates memory in Y but never frees it
OBSERVATION: Module A includes Module B but only uses one function
OBSERVATION: Error path in Z returns without logging
EXTRACTION: render.cpp:120-180 - particle update logic operates independently from rendering
EXTRACTION: ProcessAudio() mixes FFT setup, beat detection, smoothing - three distinct concerns
```

Save evaluation for Phase 3.

## Phase 3: Evaluation

Compare observations against project conventions and general correctness.

### 3.1 Convention Compliance

For each observation, ask:
- Does this violate any rule stated in `CLAUDE.md`?
- Does this violate the project's established patterns?
- If it deviates, is the deviation justified and documented?

**Important**: Only flag convention violations if the project explicitly prohibits them. General "best practices" that conflict with project conventions are not violations.

### 3.2 Correctness Analysis

Independent of conventions, evaluate:

**Logic**
- Does the code do what it claims?
- Are edge cases handled?
- Are assumptions documented?

**Resource Safety**
- Can resources leak?
- Can use-after-free occur?
- Can buffers overflow?

**Error Handling**
- What happens when operations fail?
- Are errors propagated or swallowed?
- Can the system recover?

**Thread Safety** (if applicable)
- What invariants could be violated by concurrent access?
- Are critical sections protected?

### 3.3 Classify Findings

For each issue discovered:

| Severity | Criteria |
|----------|----------|
| Critical | Causes crash, data loss, or security vulnerability |
| Major | Incorrect behavior under normal operation |
| Minor | Style issue or edge-case problem |
| Note | Potential improvement, not a defect |

## Phase 4: Report

Structure findings for human review.

### Report Format

```markdown
# Code Review Report

**Scope**: [What was reviewed - e.g., "Full codebase" or "src/audio.cpp"]
**Conventions Reference**: CLAUDE.md

## Summary

[2-3 sentences: overall assessment and most important findings]

## Findings

### [Severity] Brief title

**Location**: `file:function` or `file:line`
**Category**: Logic | Resource | Convention | Security | Performance

[1-2 sentences: What is the issue? Why does it matter?]

**Evidence**:
[Code snippet or specific observation]

**Recommendation**:
[Specific actionable fix]

---

[Repeat for each finding, grouped by severity]

## Observations

[Factual notes that aren't defects but may be useful:
- Patterns noticed
- Potential future concerns
- Questions for the maintainer]
```

### Report Quality Checklist

Before submitting:
- [ ] Every finding references a specific location
- [ ] Every finding explains WHY it's a problem
- [ ] Every finding has a concrete recommendation
- [ ] Convention violations cite the specific rule violated
- [ ] No findings contradict explicit project conventions

## Appendix: Investigation Techniques

### Finding All Source Files
```
Glob: **/*.cpp **/*.h **/*.c
```

### Finding Dependencies Between Modules
```
Grep: #include "  (project includes use quotes, not angle brackets)
```

### Finding Resource Allocation
```
Grep: malloc|calloc|new|Create|Init|Load|Open
```

### Finding Resource Release
```
Grep: free|delete|Destroy|Uninit|Unload|Close
```

### Finding Error Handling
```
Grep: return.*(-1|NULL|false|nullptr)|if.*==.*NULL|if.*<.*0
```

### Finding TODO/FIXME Comments
```
Grep: TODO|FIXME|HACK|XXX
```

## Anti-Patterns

**Avoid**:
- Assuming what findings will exist before investigating
- Applying general best practices that conflict with project conventions
- Reviewing more than ~400 lines without recording observations
- Flagging patterns that `CLAUDE.md` explicitly allows

**Prefer**:
- Discovering facts first, evaluating second
- Respecting project-specific choices even if they seem unusual
- Chunking large reviews into manageable pieces
- Citing specific locations and evidence for every finding

---

## Sources

- [Google Engineering Practices: Code Review Standard](https://google.github.io/eng-practices/review/reviewer/standard.html)
- [Palantir: Best Practices for LLM Prompt Engineering](https://www.palantir.com/docs/foundry/aip/best-practices-prompt-engineering)
- [SmartBear: Best Practices for Code Review](https://smartbear.com/learn/code-review/best-practices-for-peer-code-review/)
- [Prompt Engineering Guide: LLM Agents](https://www.promptingguide.ai/research/llm-agents)
