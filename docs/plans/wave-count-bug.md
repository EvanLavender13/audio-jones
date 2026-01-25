# Wave Count Mismatch Bug

Claude sometimes states a wave count in the summary that contradicts its own dependency table.

## Example (kuwahara.md planning session)

The dependency table produced:

| Wave | Phases | Rationale |
|------|--------|-----------|
| 1 | Phase 1, Phase 2 | No dependencies |
| 2 | Phase 3, Phase 4, Phase 5 | All depend on Wave 1 phases |

Summary text incorrectly stated: "5 phases, **3 waves**"

Correct answer: **2 waves**.

## Pattern

The table analysis is correct — phases with no dependencies form Wave 1, phases whose dependencies are all in Wave 1 form Wave 2. The error occurs in the natural-language summary sentence, which invents a higher number despite the table being right above it.

## Where this matters

The `/implement` command uses wave counts to schedule parallel execution. An inflated wave count could cause confusion about expected parallelism or sequential ordering.

## Reproduction

Ask Claude to plan a feature, then summarize with wave breakdown. Compare the stated count against the actual table rows. The mismatch appears intermittently — the table is reliably correct, the prose number is not.
