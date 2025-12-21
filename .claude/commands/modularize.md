---
description: Extract modules from monolithic files using multi-agent cohesion analysis
argument-hint: File path or pattern (e.g., src/main.cpp or src/*.cpp)
---

# Module Extraction

You are extracting cohesive modules from source files. This is a multi-phase process with user approval gates.

## Why Multi-Agent

Module extraction requires different analytical perspectives:
- **Cohesion analysis** - Finding code that belongs together (multiple signals)
- **Dependency mapping** - Understanding coupling and blockers
- **Interface design** - Creating clean APIs

Running these in parallel with specialized agents produces better results than sequential single-agent analysis.

## Core Rules

- **User approval required** before extraction (Phase 4)
- **Build verification** after each extraction
- **Use TodoWrite** to track all phases
- **Parallel agents** where possible

---

## Phase 1: Discovery

**Goal**: Identify target files and extraction goals

Target: $ARGUMENTS

**Actions**:
1. Create todo list with all 7 phases
2. Validate target file(s) exist
3. Get file sizes (NLOC) to prioritize
4. Ask user if unclear:
   - Which file(s) to analyze?
   - Any specific modules they want extracted?
   - Any modules they want to keep together?

If multiple large files, ask which to start with.

---

## Phase 2: Cohesion Analysis

**Goal**: Find module candidates through multiple lenses

**Actions**:
1. Launch 2-3 cohesion-analyzer agents in parallel with different signals:
   - `"Analyze [file] for DATA cohesion - functions grouped by struct/data type"`
   - `"Analyze [file] for FUNCTIONAL cohesion - functions implementing features"`
   - `"Analyze [file] for PREFIX cohesion - functions with shared naming prefixes"`

2. Collect results from all agents
3. Merge overlapping candidates (same functions found by multiple signals = strong candidate)
4. Create consolidated candidate list

---

## Phase 3: Dependency Mapping

**Goal**: Identify coupling issues before extraction

**Actions**:
1. Launch dependency-mapper agent:
   - `"Map dependencies for these proposed modules in [file]: [list candidates from Phase 2]"`

2. Review results for:
   - Circular dependencies (BLOCKER - must resolve)
   - Shared state ownership
   - Extraction order constraints
   - High coupling warnings

3. If circular dependencies found:
   - Present options to user
   - May need to merge candidates or extract interfaces

---

## Phase 4: Candidate Review (USER APPROVAL)

**Goal**: Get user approval on extraction plan

**CRITICAL**: Do not proceed without explicit approval.

**Actions**:
1. Present consolidated findings:

```
## Module Extraction Candidates

### 1. [ModuleName] (RECOMMENDED)
- Functions: N
- Cohesion: X% (found by: data, prefix)
- Coupling: Low/Medium/High
- Files affected: list

### 2. [ModuleName]
...

## Dependency Issues
- [Any blockers or warnings]

## Extraction Order
1. First module (no dependencies)
2. Second module (depends on #1)
...
```

2. Ask user:
   - Which modules to extract?
   - Any changes to proposed boundaries?
   - Proceed with extraction or just produce plan?

3. Wait for explicit approval before Phase 5

---

## Phase 5: Interface Design

**Goal**: Design clean public APIs for approved modules

**Actions**:
1. For each approved module, launch interface-designer agent:
   - `"Design interface for [ModuleName] module from [file]. Functions: [list]. Primary struct: [name]"`

2. Review proposed interfaces for:
   - Init/Uninit pattern compliance
   - Minimal surface area
   - Breaking changes to callers

3. Present interfaces to user for review (quick confirmation, not full approval gate)

---

## Phase 6: Extraction

**Goal**: Perform the actual extraction

**Actions**:
For each module (in dependency order):

1. Create header file: `src/[modulename].h`
   - Include guard
   - Struct definition
   - Public function declarations
   - Necessary includes

2. Create source file: `src/[modulename].cpp`
   - Include header
   - Static helper declarations
   - Function implementations (moved from original)

3. Update original file:
   - Add `#include "[modulename].h"`
   - Remove extracted code
   - Update any changed signatures at call sites

4. Update `CMakeLists.txt`:
   - Add new source file to build

5. Build and verify:
   ```bash
   cmake.exe --build build --config Release 2>&1 | head -50
   ```

6. If build fails:
   - Fix issues
   - Re-verify
   - Do not proceed to next module until current one builds

---

## Phase 7: Verification & Documentation

**Goal**: Confirm success and update docs

**Actions**:
1. Final build verification
2. Run `/sync-architecture` to update architecture docs
3. Summary to user:
   - Modules extracted
   - Files created
   - Any manual follow-up needed

---

## Extraction Patterns

### Header Template

```cpp
#ifndef MODULENAME_H
#define MODULENAME_H

// Required includes
#include <stdint.h>

struct ModuleName {
    // Config (set before Init)
    int param = 0;

    // State (managed internally)
    float *data = NULL;
};

void ModuleNameInit(ModuleName *self);
void ModuleNameUninit(ModuleName *self);
// Other public functions

#endif
```

### Source Template

```cpp
#include "modulename.h"

// Static helpers (private)
static void HelperFunction(ModuleName *self) {
    // Implementation
}

// Public functions
void ModuleNameInit(ModuleName *self) {
    // Initialize
}

void ModuleNameUninit(ModuleName *self) {
    // Cleanup
}
```

---

## Anti-patterns to Avoid

| Mistake | Why It Fails |
|---------|--------------|
| Extracting by file size alone | Creates arbitrary boundaries |
| One function per file | Fragments related code |
| Exposing all functions | No encapsulation benefit |
| Skipping user approval | May extract wrong modules |
| Ignoring circular deps | Creates compile errors |
| "Utils" modules | No cohesion |

---

## Output Constraints

- Always verify builds after extraction
- Update CMakeLists.txt
- Run /sync-architecture when done
- If extraction is too large (>3 modules), offer to write a plan instead
