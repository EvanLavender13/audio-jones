# Fix Preset Modulation Bleed Bug

Modulated parameter values bleed between presets when switching. Switching SOLO → ICEY retains SOLO's `flowField.rotBase` because `ModulationConfigToEngine` clears routes before syncing bases, overwriting newly-loaded preset values with old stored bases.

## Current State

The bug lives in one function:
- `src/config/modulation_config.cpp:76-87` - `ModulationConfigToEngine` calls `ClearRoutes` before `SyncBases`

Supporting context:
- `src/config/preset.cpp:257-264` - `PresetToAppConfigs` writes preset values, then calls `ModulationConfigToEngine`
- `src/automation/modulation_engine.cpp:173-184` - `ModEngineClearRoutes` writes stored bases to param pointers
- `src/automation/modulation_engine.cpp:195-202` - `ModEngineSyncBases` reads param pointers into base storage

## Bug Mechanism

```
Switching from SOLO (has modulation on rotBase) to ICEY (no modulation):

1. PresetToAppConfigs writes ICEY's rotBase (0.0025) to *configs->effects
2. ModulationConfigToEngine:
   a. ModEngineClearRoutes() iterates SOLO's routes
      - Writes SOLO's meta.base (0.01) to *meta.ptr
      - OVERWRITES ICEY's 0.0025 with SOLO's 0.01
   b. ModEngineSyncBases() reads *meta.ptr (now 0.01) into meta.base
   c. No routes to restore (ICEY has none)
3. Result: rotBase = 0.01 (SOLO's value) instead of 0.0025 (ICEY's value)
```

## Phase 1: Fix Operation Order

**Goal**: Sync bases before clearing routes so new preset values are captured.

**Modify**:
- `src/config/modulation_config.cpp:78-79` - Swap the order of `ModEngineSyncBases()` and `ModEngineClearRoutes()`

**Current code**:
```cpp
void ModulationConfigToEngine(const ModulationConfig* config)
{
    ModEngineClearRoutes();    // Writes OLD bases, destroying NEW preset values
    ModEngineSyncBases();      // Captures the OLD values
    ...
}
```

**Fixed code**:
```cpp
void ModulationConfigToEngine(const ModulationConfig* config)
{
    ModEngineSyncBases();      // Capture NEW preset values into base storage FIRST
    ModEngineClearRoutes();    // Now writes NEW bases (no effect, values match)
    ...
}
```

**Done when**: Switching SOLO → ICEY shows ICEY's low rotation speed, not SOLO's high rotation.

---

## Phase 2: Verify Fix

**Goal**: Confirm the fix works across multiple preset combinations.

**Test cases**:
1. ICEY → SOLO → ICEY: rotBase returns to ICEY's 0.0025
2. SOLO → ICEY → SOLO: rotBase returns to SOLO's 0.01 with modulation active
3. STAYINNIT → SOLO → STAYINNIT: Similar flow with different preset
4. Test with multiple modulated params: SOLO modulates rotBase, zoomBase, chromaticOffset

**Done when**: All test cases pass, no parameter bleed between presets.
