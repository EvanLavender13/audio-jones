# Shader Prompt Integrity

Prevents the implement skill's orchestrator from fabricating shader algorithm details by closing the information gap at both ends: plans must inline shader formulas (upstream), and the orchestrator must stop if it encounters external references for algorithm content (downstream).

## Classification

- **Category**: General (skill/workflow architecture)
- **Affected Files**: `.claude/skills/feature-plan/SKILL.md`, `.claude/skills/implement/SKILL.md`
- **Chosen Approach**: Both upstream + downstream — belt and suspenders

## References

- `ORCHESTRATOR_FAILURE_LOG.md` — Documents circuit board SDF fabrication failure
- `docs/plans/archive/circuit-board.md` — Plan that inlines full GLSL (works correctly)
- `docs/plans/archive/signal-frames.md` — Plan that references research doc for SDF functions (vulnerable pattern)

## Problem

When a plan says "see research doc for algorithm" instead of inlining formulas, the orchestrator perceives an information gap. It fills that gap by fabricating algorithm details from memory. The fabricated content compiles but produces wrong visual output.

Failure chain:
1. Plan says "algorithm in research doc"
2. Orchestrator writes agent prompt, paraphrases or invents formulas
3. Agent implements what the orchestrator told it
4. Shader compiles, output looks wrong

The implement skill already says "paste verbatim" and "do NOT enrich." The orchestrator ignores these instructions because the gap feels like something that should be filled. Natural language instructions cannot prevent this reliably.

## Solution

### Upstream Fix (feature-plan skill)

The Algorithm section in plans must inline all shader formulas, SDF functions, and key GLSL logic. No deferring to research docs for algorithm content.

Current feature-plan line 231 says: "Shader code is fair game here; C++ function bodies are not." This allows inlining but does not require it. Change to require it.

Add to Phase 7 (Write Design Section):
- When the design includes a fragment shader, the Algorithm section MUST contain all mathematical formulas, SDF functions, and core GLSL logic that the shader agent needs
- "See research doc" is NOT acceptable for algorithm content — the plan is the single source of truth for agents
- The plan's Algorithm section IS the specification. Research docs are upstream references that feed INTO the plan, not runtime references that agents consult

Add to Phase 9 (Write Plan Document), Task instructions for shader tasks:
- Shader task "Do" sections must say "Implement the Algorithm section" not "Implement from research doc"
- All formulas the agent needs are already in the plan's Algorithm section

Add to Red Flags table:
- "I'll reference the research doc for the algorithm" -> "NO. Inline the formulas. The plan is the single source of truth."

### Downstream Fix (implement skill)

If the orchestrator encounters a task that references an external document for algorithm content (e.g., "from the research doc," "as specified in docs/research/"), it STOPS and reports the plan as incomplete instead of fabricating.

Add to Phase 3a (Prepare Agent Prompts):
- If a task's Do section references a research doc or external file for algorithm/formula content: STOP. Tell user "Task [name] references [doc] for algorithm details. The plan should inline these. Run /feature-plan to fix."
- This is a hard stop, not a warning. Do not proceed. Do not summarize the referenced document.

Add to Red Flags table:
- "The task says 'from research doc' — I'll include the algorithm" -> "STOP. The plan is incomplete. Tell the user."

## Notes

- This fix does NOT affect non-shader tasks. C++ boilerplate tasks (config structs, UI panels, param registration) follow templates and don't need algorithm content.
- Plans that already inline shader code (like circuit-board.md) are unaffected.
- The feature-plan's Phase 10 (Research Fidelity Check) still runs — it verifies the plan's inlined algorithm matches the research doc, catching copy errors.
- This makes the plan slightly longer but eliminates an entire class of orchestrator failures.
