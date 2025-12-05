---
description: Extract modules from monolithic files using cohesion and coupling analysis
allowed-tools:
  - Read
  - Edit
  - Write
  - Bash
  - Glob
  - Grep
  - TodoWrite
  - EnterPlanMode
  - SlashCommand
  - Skill
---

# Module Extraction

Analyze source files for module extraction opportunities. Identify cohesive code clusters, extract to separate files with clean interfaces.

## Extraction Signals

Scan for these indicators that code belongs in a separate module:

| Signal | What to Look For | Priority |
|--------|------------------|----------|
| **Data cohesion** | Functions operating on the same struct/data type | High |
| **Functional cohesion** | Functions implementing a single feature or use case | High |
| **File size** | Files exceeding 500 NLOC | Medium |
| **Prefix clustering** | Functions sharing a naming prefix (`audio_*`, `render_*`) | Medium |
| **Include clustering** | Groups of related `#include` directives | Low |
| **Change coupling** | Code sections that change together in commits | Low |

## Module Boundary Heuristics

A code cluster qualifies as a module when:

1. **Internal cohesion exceeds external coupling** - Functions within the cluster call each other more than they call outside code
2. **Data ownership is clear** - The cluster owns specific state that others access through it
3. **Interface surface is small** - Fewer than 10 functions need public exposure
4. **Responsibility is nameable** - You can describe what it does in 3-5 words without "and"

## Instructions

### 1. Analyze Target File

Read the target file and identify:
- All struct definitions and their associated functions
- Function call graph (which functions call which)
- Shared state (globals, static variables)
- Logical groupings by naming convention

### 2. Identify Candidate Modules

Group code by cohesion signals. For each candidate:
- List functions that belong together
- Identify the data they operate on
- Note dependencies on code outside the group
- Name the module by its single responsibility

Reject candidates where:
- Fewer than 3 functions (too small)
- More than 30% of calls go outside the module (too coupled)
- No clear data ownership (responsibility unclear)

### 3. Design Interfaces

For each accepted module, define:

**Public interface** (goes in header):
- Opaque type declaration: `typedef struct ModuleName ModuleName;`
- Constructor: `ModuleName *module_create(...);`
- Destructor: `void module_destroy(ModuleName *self);`
- Essential operations (minimize surface area)

**Private implementation** (stays in .c):
- Struct definition with all fields
- Helper functions (prefix with `_` or declare `static`)
- Internal state management

### 4. Plan Extraction

Use `EnterPlanMode` if:
- More than 2 modules need extraction
- Circular dependencies exist between candidates
- Shared state requires ownership decision

Otherwise, extract directly:

1. Create `src/modulename.h` with public interface
2. Create `src/modulename.c` with implementation
3. Move functions, preserving internal call relationships
4. Update original file to `#include` new header
5. Replace direct struct access with interface calls

### 5. Apply C Interface Patterns

**Opaque pointer pattern** (preferred for stateful modules):
```c
// header: audio.h
typedef struct Audio Audio;
Audio *audio_create(const AudioConfig *config);
void audio_destroy(Audio *self);
float audio_get_sample(Audio *self, int index);
```

**Static functions** (for implementation details):
```c
// source: audio.c
static void update_ring_buffer(Audio *self) { ... }
```

**Config structs** (for complex initialization):
```c
typedef struct {
    int sample_rate;
    int buffer_size;
} AudioConfig;
```

### 6. Verify Extraction

After each module extraction:
- Build compiles without errors
- No function signature changes in calling code (interface stability)
- Original functionality preserved

```bash
cmake.exe --build build --config Release 2>&1 | head -50
```

### 7. Update Documentation

After all extractions complete, run:
```
/sync-architecture
```

## Anti-patterns

**Avoid these extraction mistakes:**

| Mistake | Why It Fails |
|---------|--------------|
| Extracting by file size alone | Creates arbitrary boundaries, not cohesive modules |
| One function per file | Fragments related code, increases coupling |
| Exposing all functions | No encapsulation benefit, interface too wide |
| Circular module dependencies | Indicates wrong boundary placement |
| "Utils" or "helpers" modules | Dumping ground with no cohesion |

## Reference

Principles derived from:
- Constantine, Structured Design: Cohesion/coupling metrics
- Fowler, Refactoring: Extract Class technique
- SOLID: Single Responsibility Principle
- Memfault: [Opaque Pointers in C](https://interrupt.memfault.com/blog/opaque-pointers)
