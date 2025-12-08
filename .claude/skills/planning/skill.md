---
name: planning
description: Update ROADMAP.md with consistent structure and tone. Invoke when adding features, reprioritizing work, or promoting items between sections.
---

# Project Planning

Maintain `ROADMAP.md` as the single source of truth for planned work.

## Sections

| Section | Purpose | Format |
|---------|---------|--------|
| **Current Focus** | Active work (limit: 1 feature) | Checkboxes with phases |
| **Next Up** | Prioritized, ready to start | Table with complexity/impact |
| **Backlog** | Unordered ideas | Bullets |
| **Far Future** | Major architectural work | Bullets with rationale |
| **Completed** | Finished items | Bullets |

## Item Formats

### Current Focus

Multi-phase feature with trackable progress:

```markdown
**Feature Name** - One-sentence description.

- [x] Phase 1: Completed step
- [ ] Phase 2: Next step
- [ ] Phase 3: Final step

Details: `docs/research/feature-plan.md`
```

### Next Up

Table for quick priority scanning:

```markdown
| Item | Complexity | Impact |
|------|------------|--------|
| Short description | Low/Medium/High | Low/Medium/High |
```

### Backlog / Far Future

Simple bullets. Far Future items include brief rationale:

```markdown
- **Item name** - Why it requires significant work
```

## Promotion Workflow

```
Backlog → Next Up → Current Focus → Completed
```

**Backlog → Next Up**: Add complexity/impact assessment.

**Next Up → Current Focus**: Break into phases, create research doc if needed.

**Current Focus → Completed**: Move checked items, update Completed section.

## Rules

1. **One Current Focus** - Complete or pause before starting new work
2. **Reference, don't duplicate** - Link to research docs for implementation details
3. **Assess before promoting** - Every Next Up item needs complexity/impact rating
4. **Track phases** - Current Focus items use checkboxes for progress

## Writing Style

Apply technical-writing standards:

- Active voice: "Extracts FFT from BeatDetector" not "FFT is extracted"
- Present tense: "Adds spectrum bars" not "Will add spectrum bars"
- Specific verbs: "Parses/renders/validates" not "handles/processes/manages"

## Verification

Before committing ROADMAP.md changes:

- [ ] Only one item in Current Focus
- [ ] Next Up items have complexity/impact ratings
- [ ] Research docs linked, not duplicated
- [ ] Active voice throughout
