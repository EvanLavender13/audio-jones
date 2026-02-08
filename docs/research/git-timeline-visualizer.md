# Git Timeline Visualizer

> Status: Concept — separate project, not an AudioJones feature

## Idea

Feed a repo's full git history through an LLM to identify major architectural decisions, feature milestones, and turning points. Render the results as an interactive visual timeline.

## Motivation

AudioJones evolved from a simple waveform-with-diffusion demo into a 350+ file visual engine with compute simulations, modulation routing, and 60+ effects. That arc is invisible in a flat commit log. A tool like this surfaces the story of a project — when the big leaps happened, what decisions shaped the architecture, where complexity grew.

## Pipeline

1. **Extract** — `git log --all --format=json` dumps raw history
2. **Analyze** — LLM processes commit batches, classifies each as routine/milestone/decision
3. **Structure** — Output JSON: `{date, title, category, summary, commit_range}`
4. **Render** — Interactive HTML timeline (D3.js, vis-timeline, or similar)

## Existing Pieces

| Layer | Tools | Gap |
|-------|-------|-----|
| Git summarization | git-ai-summarize, Changeish, llm-git | Text-only, no milestone detection |
| Git visualization | Gource, GitKraken, gat graph, Githru | No LLM analysis, shows all commits equally |
| Timeline rendering | D3.js, vis-timeline, Claude Artifacts | Needs structured input |

Nobody combines LLM-driven milestone extraction with interactive timeline rendering yet.

## Open Questions

- Batch size for LLM context? Full log vs. chunked windows
- Classification taxonomy: milestone, architectural decision, feature, refactor, bugfix cluster
- How to handle merge commits and branch topology
- Interactive features: zoom, filter by category, click-to-diff
- Standalone CLI tool vs. Claude Code skill vs. web app

## Notes

- Separate repo — not part of AudioJones pipeline
- Could prototype with AudioJones as the test subject (400+ commits, clear evolution arc)
- Claude Artifacts can generate interactive HTML directly, useful for quick prototyping
