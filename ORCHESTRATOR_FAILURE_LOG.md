# Orchestrator Failure Log

## 2026-02-09: Circuit Board shader — fabricated SDF formula

### What happened

The implement skill says: "paste the plan's task section verbatim" and "do NOT pre-read files to enrich agent prompts." The plan said "Fully specified in research doc" and pointed at `docs/research/circuit_board.md`.

The orchestrator ignored both instructions. Instead of letting the agent read the research doc, it invented an "SDF Primitives" section in the agent prompt containing:

```
sdBox(vec2 p, vec2 rad): length(max(abs(p) - rad, 0.0))
```

The research doc had the correct formula:

```
max(abs(p.x) - rad.x, abs(p.y) - rad.y)
```

These are completely different SDFs. The first is an unsigned Euclidean exterior distance (rounded rectangles, always >= 0). The second is a signed Chebyshev distance (sharp squares, negative inside). The signed distance is what creates the trace boundary contours that define the entire visual effect.

The agent implemented exactly what the orchestrator told it. The shader compiled. The output looked nothing like the reference.

Two additional algorithm details were also lost because the orchestrator summarized instead of passing through:
- Center cell initialization before the neighbor loop
- Second-closest cell re-evaluation (tracking cell identity, not just a float)

### Why it happened

The orchestrator filled in gaps from memory instead of dispatching the agent with the plan text and letting the agent read the source. This same failure pattern has occurred before (noted in MEMORY.md) and the instructions already prohibited it. The instructions were ignored.

### What the instructions say

> The plan contains the complete specification. Agents read their own target files with fresh context. The orchestrator pastes the plan's task section into the prompt verbatim and dispatches. Do NOT pre-read files to "enrich" agent prompts.
