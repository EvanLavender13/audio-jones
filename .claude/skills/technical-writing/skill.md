---
name: technical-writing
description: Apply rigorous technical writing standards to any technical prose - code comments, documentation, research notes, planning docs, READMEs, or specifications. Use when clarity and precision matter.
---

# Technical Writing Standards

Apply these standards to ALL technical writing.

## Core Rules

### Language Mechanics
- **Active voice**: "The parser validates input" NOT "Input is validated"
- **Present tense**: "returns null" NOT "will return null"
- **15-20 words per sentence** (max 30 before splitting)
- **Code references**: Use `file_path:line_number` format

### Precision Requirements
Replace vague terms with specifics:
- "fast" → "completes in <100ms"
- "many" → "up to 1000 items"
- "handles" → "validates/transforms/routes/stores" (specify which)
- "manages" → "creates/updates/deletes/tracks" (specify which)
- "processes" → "parses/filters/aggregates/formats" (specify which)

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

- Don't summarize content that exists elsewhere
- Reference files instead of repeating their contents
- If documentation exists, link to it

**Remember**: Every word should add understanding. If it doesn't teach something specific, cut it.
