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

## Instructions

1. **Analyze Current Code**
   - Read all source files in `src/`
   - Identify modules, components, and their relationships
   - Map data flow between components
   - Note key functions and their responsibilities

2. **Update Architecture Document**
   - Update `docs/arch/architecture.md` with current state
   - Use Mermaid diagrams for visual representation
   - Reference code with `file_path:line_number` format
   - Apply `/technical-writing` skill standards

3. **Update CLAUDE.md**
   - Sync the ASCII diagram in Architecture section
   - Update "Current State" to reflect reality
   - Keep it concise (CLAUDE.md is quick reference)

4. **Validation**
   - Ensure diagrams match actual code structure
   - Verify all referenced files exist
   - Check that component names match code

## Source Files
@./src

## Current Architecture Doc
@./docs/arch/architecture.md

## CLAUDE.md Reference
@./CLAUDE.md

## Output Format

After analysis, update the docs with:
- Component overview (what each module does)
- Data flow diagram (Mermaid)
- Key interfaces between components
- File references for each component
