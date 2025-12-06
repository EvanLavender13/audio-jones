---
description: Regenerate architecture documentation from current code state
allowed-tools:
  - Read
  - Write
  - Glob
  - Grep
  - Bash
  - Skill
---

# Sync Architecture Documentation

Analyze the current codebase and update architecture documentation to match the actual implementation.

## Critical: This is a PATCH operation

- Only change content that is **factually incorrect or missing**
- Preserve existing wording if it accurately describes the code
- Do NOT rephrase sentences that are already correct
- If nothing changed in the code, the document should have **zero diff**
- Compare existing docs to code before making any edits

## Instructions

1. **Load Standards**
   - Invoke the `technical-writing` skill before writing any documentation
   - Invoke the `architecture-diagrams` skill before creating Mermaid diagrams

2. **Analyze Current Code**
   - Read all source files in `src/`
   - Identify modules, components, and their relationships
   - Map data flow between components
   - Note key functions and their responsibilities

3. **Update Architecture Document**
   - Update `docs/architecture.md` with current state
   - Use Mermaid diagrams for visual representation
   - Reference code by file path only, no line numbers

4. **Update CLAUDE.md**
   - Update "Current State" to reflect reality
   - Keep it concise (CLAUDE.md is quick reference)

5. **Validation**
   - Ensure diagrams match actual code structure
   - Verify all referenced files exist
   - Check that component names match code

## Source Files
@./src

## Current Architecture Doc
@./docs/architecture.md

## CLAUDE.md Reference
@./CLAUDE.md

## Output Format

After analysis, update the docs with:
- Component overview (what each module does)
- Data flow diagram (Mermaid)
- Key interfaces between components
- File references for each component
