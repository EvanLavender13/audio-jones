---
title: Agent-Driven Development for AudioJones
tags: [meta, workflow, agents, skills, vibe-coding]
date: 2025-12-03
---

# Agent-Driven Development for AudioJones

## Overview

This document establishes the development philosophy and agent architecture for building AudioJones as an audio-visual toy. The approach rejects exhaustive upfront planning in favor of iterative, agent-assisted development where specialized AI personas collaborate through well-defined skills and workflows.

## The Vibe Coding Philosophy

Vibe coding, coined by Andrej Karpathy in February 2025, describes software development through natural language conversation with AI rather than line-by-line code authorship. The developer describes intent; the AI generates implementation; iteration refines the result.

### Core Principles

1. **Describe, don't dictate**: Express what you want to achieve, not how to implement it
2. **Iterate rapidly**: Generate code, evaluate results, provide feedback, repeat
3. **Trust but verify**: AI generates; human validates through tests, visuals, or behavior
4. **Context over planning**: Rich contextual information outperforms rigid specifications

### When Vibe Coding Works

| Scenario | Effectiveness |
|----------|---------------|
| Prototyping and experimentation | High |
| Exploratory feature development | High |
| Well-defined transformations | High |
| Novel algorithm invention | Low |
| Security-critical code | Low (requires extra verification) |

### The Planning Trap

The archctl approach failed because exhaustive YAML specifications create a false sense of completeness. Real software development encounters:

- Requirements that clarify only through implementation
- Technical constraints discovered during coding
- Design insights that emerge from working prototypes
- Integration issues invisible until components connect

Upfront planning optimizes for predictability at the cost of adaptability.

## Multi-Agent Architecture

Multi-agent systems distribute specialized capabilities across autonomous agents that coordinate through defined protocols. Each agent maintains local knowledge and decision-making capacity while collaborating toward shared objectives.

### Agent Composition Pattern

```
User Intent
    │
    ▼
┌─────────────────┐
│  Orchestrator   │ ◄── Routes tasks, preserves context
└────────┬────────┘
         │
    ┌────┴────┬────────────┐
    ▼         ▼            ▼
┌───────┐ ┌───────┐ ┌──────────┐
│Planner│ │ Coder │ │ Reviewer │
└───────┘ └───────┘ └──────────┘
    │         │            │
    └─────────┴────────────┘
              │
              ▼
        Aggregated Output
```

### Agent Types for AudioJones

| Agent Role | Responsibility | Invocation |
|------------|----------------|------------|
| Explorer | Searches codebase, identifies patterns, answers structural questions | Task tool with `subagent_type=Explore` |
| Planner | Decomposes features into implementation steps, identifies dependencies | EnterPlanMode or Task tool |
| Implementer | Writes code following established patterns and constraints | Direct conversation |
| Reviewer | Validates code quality, identifies bugs, suggests improvements | Task tool post-implementation |
| Researcher | Investigates external libraries, APIs, techniques | WebSearch, WebFetch |

### Coordination Mechanisms

**Task decomposition**: Complex features break into atomic implementation units. Each unit specifies inputs, outputs, and success criteria.

**Role assignment**: Agent selection based on task characteristics. Exploration tasks route to Explorer; coding tasks stay in main conversation.

**Context sharing**: Shared memory via codebase files, CLAUDE.md, and skill definitions. Agents read common artifacts rather than duplicating context.

**Result aggregation**: Outputs from multiple agents combine through human review or orchestrator synthesis.

## Claude Code Skills Architecture

Skills are model-invoked capability bundles that load dynamically based on task relevance. Unlike slash commands (user-triggered), skills activate automatically when Claude determines they apply.

### Skill Structure

```
.claude/skills/
└── skill-name/
    ├── SKILL.md      # Required: frontmatter + instructions
    ├── templates/    # Optional: code templates, examples
    └── scripts/      # Optional: automation scripts
```

### SKILL.md Format

```yaml
---
name: skill-name-lowercase
description: |
  Concise explanation of what this skill does and when to invoke it.
  Include trigger phrases and use cases for accurate discovery.
---

# Skill Instructions

Detailed guidance Claude follows when skill activates.
Include examples, constraints, and expected outputs.
```

### Skill Design Principles

**Single responsibility**: Each skill addresses one capability. "Audio visualization" is too broad; "circular waveform geometry generation" is focused.

**Explicit triggers**: Descriptions specify when to activate. Include domain keywords, task types, and file patterns.

**Minimal footprint**: Skills load on-demand. Avoid bundling unrelated functionality.

**Tested activation**: Verify skills trigger appropriately across varied prompts. Adjust descriptions based on false positives/negatives.

### Proposed Skills for AudioJones

| Skill | Description | Trigger Context |
|-------|-------------|-----------------|
| `shader-dev` | GLSL fragment shader development for visual effects | Working with `.frag` files, blur/fade effects |
| `audio-capture` | miniaudio configuration for loopback capture | Audio device setup, ring buffer management |
| `raylib-patterns` | raylib idioms for rendering, textures, shaders | Graphics initialization, render loops |
| `imgui-integration` | rlImGui setup and widget patterns | Parameter controls, UI layout |
| `technical-writing` | Documentation standards (already exists) | Creating docs, describing systems |

## Workflow Patterns

### Explore-Plan-Code-Commit

Anthropic's recommended workflow for complex tasks:

1. **Explore**: Claude reads relevant files without writing code. Understands existing patterns, identifies integration points.
2. **Plan**: Detailed implementation steps using extended thinking. No code yet—only strategy.
3. **Code**: Implementation following the plan. Iterative refinement against tests or visual targets.
4. **Commit**: Atomic commits with clear messages documenting what changed and why.

### Test-Driven Iteration

1. Define expected behavior as tests (unit, integration, or visual comparison)
2. Confirm tests fail (no false positives)
3. Claude implements until tests pass
4. Refactor while maintaining passing tests

### Visual Iteration

For UI and graphics work:

1. Provide reference image or design mock
2. Claude implements initial version
3. Take screenshot, compare to reference
4. Describe differences for Claude to address
5. Repeat 2-3 cycles until acceptable

### Multi-Claude Workflows

Parallel Claude instances enable concurrent work:

- **Code + Review**: One instance writes, another reviews
- **Independent features**: Separate git worktrees for parallel development
- **Exploration + Implementation**: One instance researches while another implements

## Context Engineering

Context engineering replaces vibe coding's informal approach with systematic context management. The quality of AI output correlates directly with context quality.

### CLAUDE.md Configuration

Create `CLAUDE.md` at repository root with:

```markdown
# AudioJones

Real-time circular waveform audio visualizer using raylib + miniaudio.

## Commands

- Build: `cmake --build build`
- Run: `./build/AudioJones`
- Test: `ctest --test-dir build`

## Code Style

- C++20, no exceptions, minimal templates
- raylib naming conventions for graphics code
- miniaudio patterns for audio code

## Architecture

- Single-threaded render loop at 60fps
- Audio callback writes to ring buffer
- Main thread reads buffer, generates geometry, renders

## Current Focus

[Update this section as work progresses]
```

### Context Preservation Strategies

**Delegate early**: Use subagents for verification tasks before context bloats.

**Clear between tasks**: `/clear` resets context when switching focus areas.

**Document discoveries**: Use `#` key to have Claude update CLAUDE.md with new learnings.

**Skill-based knowledge**: Encode domain expertise in skills rather than repeating in conversations.

## Anti-Patterns to Avoid

### Over-Specification

The archctl failure mode: defining everything before understanding anything. Specifications written without implementation feedback encode assumptions, not requirements.

### Persona Theater

Research shows role prompting ("act as a senior developer") has minimal effect on correctness. Personas influence tone and style, not accuracy. Invest in context quality over theatrical framing.

### Monolithic Conversations

Long conversations accumulate context noise. Completed tasks pollute working memory. Use `/clear`, subagents, and fresh sessions strategically.

### Blind Trust

AI-generated code requires verification. Establish automated checks (tests, linting, type checking) that catch issues before they compound.

## Implementation Strategy for AudioJones

### Phase 1: Foundation

1. Initialize raylib window with basic render loop
2. Configure miniaudio loopback capture
3. Verify audio data flows from callback to main thread
4. Display raw waveform as proof of concept

### Phase 2: Visualization

1. Implement circular waveform geometry (polar coordinates)
2. Add trail effect via render texture accumulation
3. Port `fade_blur.frag` shader to raylib conventions
4. Integrate rlImGui for parameter controls

### Phase 3: Polish

1. Add multiple waveform support
2. Implement preset save/load
3. Performance optimization
4. Visual refinements based on iteration

### No Timeline Estimates

Each phase completes when it completes. Estimation provides false precision. Focus on next concrete step, not projected completion.

## References

- [Claude Code Best Practices](https://www.anthropic.com/engineering/claude-code-best-practices) - Anthropic's official workflow guidance
- [Agent Skills Documentation](https://code.claude.com/docs/en/skills) - Skill creation and structure
- [Multi-Agent Collaboration](https://www.ibm.com/think/topics/multi-agent-collaboration) - IBM's coordination patterns
- [Vibe Coding](https://en.wikipedia.org/wiki/Vibe_coding) - Origin and definition
- [Google Agent Development Kit](https://developers.googleblog.com/en/agent-development-kit-easy-to-build-multi-agent-applications/) - Multi-agent composition
- [Claude Command Suite](https://github.com/qdhenry/Claude-Command-Suite) - Example slash command library
