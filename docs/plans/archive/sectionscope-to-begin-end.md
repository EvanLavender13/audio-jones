# Convert SectionScope to Begin/End Functions

Remove the RAII-style SectionScope struct and replace with C-style DrawSectionBegin/DrawSectionEnd function pair for consistency with the rest of the codebase.

## Current State

- `src/ui/imgui_panels.h:36-41` - SectionScope struct declaration
- `src/ui/imgui_widgets.cpp:89-104` - SectionScope constructor/destructor implementation
- 11 usage sites across 3 files using `if (SectionScope name{...}) { }` pattern

## Phase 1: Update API

**Goal**: Replace SectionScope struct with Begin/End function pair.

**Modify**:
- `src/ui/imgui_panels.h` - Remove SectionScope struct, add function declarations:
  ```c
  bool DrawSectionBegin(const char* label, ImU32 accentColor, bool* isOpen);
  void DrawSectionEnd(void);
  ```
- `src/ui/imgui_widgets.cpp` - Replace constructor/destructor with function implementations

**Done when**: Header declares Begin/End functions, implementation compiles.

---

## Phase 2: Update Usage Sites

**Goal**: Convert all SectionScope usages to Begin/End pattern.

**Pattern change**:
```cpp
// Before (RAII)
if (SectionScope section{"Label", Theme::GLOW_CYAN, &sectionOpen}) {
    // content
}

// After (Begin/End)
if (DrawSectionBegin("Label", Theme::GLOW_CYAN, &sectionOpen)) {
    // content
    DrawSectionEnd();
}
```

**Modify**:
- `src/ui/imgui_waveforms.cpp` - 3 usages (lines 85, 95, 103)
- `src/ui/imgui_spectrum.cpp` - 4 usages (lines 30, 39, 48, 56)
- `src/ui/imgui_effects.cpp` - 4 usages (lines 34, 47, 59, 80)

**Done when**: All usages converted, build succeeds, UI sections work correctly.
