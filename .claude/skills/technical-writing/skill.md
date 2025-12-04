---
name: technical-writing
description: Apply rigorous technical writing standards when creating architecture documentation, system descriptions, component descriptions, API documentation, or any technical specification text. Use when writing descriptions for systems, actors, externals, modules, components, relationships, or interfaces.
---

# Technical Writing Standards

Apply these standards to ALL technical descriptions (systems, modules, components, APIs, interfaces).

## Core Rules

### Language Mechanics
- **Active voice**: "TireModel calculates forces" NOT "Forces are calculated"
- **Present tense**: "returns null" NOT "will return null"
- **15-20 words per sentence** (max 30 before splitting)
- **Code references**: Use `file_path:line_number` format

### Precision Requirements
Replace vague terms with specific measurements:
- "fast" → "completes in <100ms"
- "many" → "up to 1000 concurrent connections"
- "handles" → "validates/transforms/routes/stores" (specify which)
- "manages" → "creates/updates/deletes/tracks" (specify which)
- "processes" → "parses/filters/aggregates/formats" (specify which)
- "communicates" → "sends HTTP POST requests to"

## Description Formats

### System (1-2 sentences)
State WHAT the system does and WHY it exists using precise technical terminology.

### Module/Component (2-3 sentences)
1. WHAT specific responsibility this owns
2. WHAT technical operations it performs
3. HOW it fits into larger architecture

### Function/Method (structured)
```
Purpose: [1 sentence transformation/effect]
Parameters: [type, constraints, purpose]
Returns: [type, meaning, range/conditions]
Behavior: [2-4 sentences of observable behavior]
Errors: [what triggers them]
```

### Relationship (2-5 words)
Be specific about what flows:
- ✅ "HTTP JSON requests", "Vehicle telemetry at 60Hz", "Validated credentials"
- ❌ "data", "information", "messages"

## Quality Checklist

Before finalizing ANY description:
- [ ] Active voice throughout
- [ ] Present tense only
- [ ] No vague verbs without specifics
- [ ] Quantities have units/ranges
- [ ] Technical terms used precisely
- [ ] First use of acronyms spelled out
- [ ] Explains WHAT (technically) and WHY (purpose)

## Examples

**BAD**: "The system handles user data and manages sessions."

**GOOD**: "The authentication service validates user credentials against PostgreSQL and issues JWT tokens with 24-hour expiration. It maintains session state in Redis with automatic cleanup after 30 minutes of inactivity."

**BAD**: "Component processes requests quickly."

**GOOD**: "RequestHandler parses incoming HTTP requests, validates JSON schema, and routes to appropriate controllers within 50ms p95 latency."

## Anti-patterns

Never write:
- "This component handles various operations"
- "The module is responsible for managing things"
- "Processes data as needed"
- "Communicates with other services"
- "Main system functionality"

Always specify WHAT operations, WHICH things, WHAT data, HOW it communicates, WHAT functionality.

**Never summarize content that already exists elsewhere:**
- Skills will be loaded automatically. Don't explain what they contain.
- Files exist in the codebase. Don't repeat their contents.
- Documentation references point to source. Don't duplicate the information.
- If content exists, reference it. Don't summarize it.

## Application

Use these standards for:
- Architecture descriptions
- API documentation
- Component specifications
- Interface contracts
- Technical decision records
- System documentation

When receiving vague input, ask:
- "What specific operations?"
- "What exact data types?"
- "Which protocol/format?"
- "What triggers this?"

**Remember**: Every word should add technical understanding. If it doesn't teach something specific about the system, cut it.