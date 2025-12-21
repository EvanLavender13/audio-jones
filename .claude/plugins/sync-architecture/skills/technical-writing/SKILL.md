---
name: technical-writing
description: This skill should be used when the user asks to "write documentation", "improve clarity", "apply technical writing standards", "review my writing", "make this clearer", or when generating code comments, READMEs, architecture docs, or specifications. Provides rigorous standards for precision and clarity in technical prose.
---

# Technical Writing Standards

Apply these standards to all technical writing: code comments, documentation, research notes, planning docs, READMEs, and specifications.

## Core Rules

### Language Mechanics

- **Active voice**: "The parser validates input" NOT "Input is validated"
- **Present tense**: "returns null" NOT "will return null"
- **15-20 words per sentence** (max 30 before splitting)
- **Code references**: Use `file_path:line_number` format

### Precision Requirements

Replace vague terms with specifics:

| Vague | Specific |
|-------|----------|
| "fast" | "completes in <100ms" |
| "many" | "up to 1000 items" |
| "handles" | "validates/transforms/routes/stores" |
| "manages" | "creates/updates/deletes/tracks" |
| "processes" | "parses/filters/aggregates/formats" |

## Writing Types

### Code Comments

```c
// BAD: Handle the data
// GOOD: Parse JSON response and extract sample values into ring buffer
```

- Explain WHY, not WHAT (code shows what)
- One line for simple logic, block for complex algorithms
- Reference related code: `// See also: waveform.c:45`

### Research/Planning Docs

- State findings, not opinions
- Include sources or evidence
- Mark unknowns explicitly: "TBD", "Needs investigation"
- Separate facts from conclusions

### Architecture Docs

- WHAT the component does (1 sentence)
- HOW it fits into the system (1 sentence)
- Reference implementation: `src/audio.c:120`

### Function/API Documentation

```
Purpose: [1 sentence - what transformation/effect]
Parameters: [type, constraints]
Returns: [type, conditions]
Errors: [what triggers them]
```

## Quality Checklist

Before finalizing:

- [ ] Active voice throughout
- [ ] Present tense only
- [ ] No vague verbs without specifics
- [ ] Quantities have units/ranges
- [ ] First use of acronyms spelled out

## Examples

**BAD**: "This handles user data and manages sessions."

**GOOD**: "Validates user credentials against the database and issues JWT tokens with 24-hour expiration."

**BAD**: "Component processes requests quickly."

**GOOD**: "Parses incoming HTTP requests and routes to controllers within 50ms p95."

## Anti-patterns

Never write:

- "handles various operations"
- "responsible for managing things"
- "processes data as needed"
- "main functionality"

Always specify WHAT operations, WHICH things, WHAT data.

## No Duplication

- Reference files instead of repeating their contents
- If documentation exists, link to it
- Avoid summarizing content that exists elsewhere

**Remember**: Every word should add understanding. If it doesn't teach something specific, cut it.
