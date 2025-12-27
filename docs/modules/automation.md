# Automation Module

> Part of [AudioJones](../architecture.md)

## Purpose

Provides parameter animation through LFO oscillators and audio-reactive modulation routing. Maps 8 normalized sources (bass/mid/treb/beat/LFO1-4) to any registered parameter via the modulation engine.

## Files

- `src/automation/lfo.h` - LFO state and processing API
- `src/automation/lfo.cpp` - Waveform generation (sine, triangle, saw, square, sample-hold)
- `src/automation/modulation_engine.h` - Modulation routing engine API
- `src/automation/modulation_engine.cpp` - Parameter registration, route management, offset calculation
- `src/automation/mod_sources.h` - ModSources aggregation and enum definitions
- `src/automation/mod_sources.cpp` - Source normalization from analysis outputs
- `src/automation/param_registry.h` - Parameter definition registry for min/max ranges
- `src/automation/param_registry.cpp` - Parameter registration and lookup

## Function Reference

| Function | Purpose |
|----------|---------|
| `LFOStateInit` | Clears phase to 0, sets random initial held value |
| `LFOProcess` | Advances phase by rate * deltaTime, generates output (-1 to 1) |
| `ModEngineInit` | Initializes routing tables (call once at startup) |
| `ModEngineUninit` | Clears all routes and parameter registrations |
| `ModEngineRegisterParam` | Registers parameter pointer with ID and min/max range |
| `ModEngineSetRoute` | Creates modulation route from source to parameter |
| `ModEngineRemoveRoute` | Removes route by parameter ID |
| `ModEngineHasRoute` | Checks if parameter has active route |
| `ModEngineGetRoute` | Retrieves route configuration by parameter ID |
| `ModEngineUpdate` | Applies modulation offsets to all registered parameters |
| `ModEngineGetOffset` | Returns computed modulation offset for parameter |
| `ModEngineSetBase` | Sets base value for parameter (used for preset loading) |
| `ModEngineGetRouteCount` | Returns number of active routes (for serialization) |
| `ModEngineGetRouteByIndex` | Retrieves route by index (for iteration) |
| `ModEngineClearRoutes` | Removes all routes |
| `ModEngineWriteBaseValues` | Writes base values to parameter pointers (for preset save) |
| `ModSourcesInit` | Zeros all source values |
| `ModSourcesUpdate` | Updates sources from band energies, beat, and LFO outputs |
| `ModSourceGetName` | Returns display name for ModSource enum |
| `ModSourceGetColor` | Returns ImGui color for ModSource visualization |
| `ParamRegistryInit` | Registers all effect parameters with min/max ranges |
| `ParamRegistryGet` | Retrieves parameter definition by ID string |

## Types

### LFOState

| Field | Type | Description |
|-------|------|-------------|
| `phase` | `float` | Current position in cycle (0.0-1.0) |
| `currentOutput` | `float` | Last computed output (-1.0 to 1.0) |
| `heldValue` | `float` | Random value for sample-hold waveform |

### ModSource

| Value | Description |
|-------|-------------|
| `MOD_SOURCE_BASS` | Bass band energy (0-1) |
| `MOD_SOURCE_MID` | Mid band energy (0-1) |
| `MOD_SOURCE_TREB` | Treble band energy (0-1) |
| `MOD_SOURCE_BEAT` | Beat intensity (0-1) |
| `MOD_SOURCE_LFO1` | LFO 1 output (normalized 0-1) |
| `MOD_SOURCE_LFO2` | LFO 2 output (normalized 0-1) |
| `MOD_SOURCE_LFO3` | LFO 3 output (normalized 0-1) |
| `MOD_SOURCE_LFO4` | LFO 4 output (normalized 0-1) |

### ModSources

| Field | Type | Description |
|-------|------|-------------|
| `values[MOD_SOURCE_COUNT]` | `float` | Normalized source values (8 total, 0-1) |

### ModCurve

| Value | Description |
|-------|-------------|
| `MOD_CURVE_LINEAR` | Linear response |
| `MOD_CURVE_EXP` | Exponential response |
| `MOD_CURVE_SQUARED` | Squared response |

### ModRoute

| Field | Type | Description |
|-------|------|-------------|
| `paramId[64]` | `char` | Unique parameter identifier string |
| `source` | `int` | ModSource enum value |
| `amount` | `float` | Modulation depth (-1.0 to +1.0) |
| `curve` | `int` | ModCurve enum value |

### ParamDef

| Field | Type | Description |
|-------|------|-------------|
| `min` | `float` | Minimum parameter value |
| `max` | `float` | Maximum parameter value |

## Data Flow

1. **Entry:** `LFOConfig` for each LFO, `BandEnergies` from analysis, `BeatDetector` intensity
2. **Transform (LFO):** Phase accumulation, waveform lookup → output (-1 to 1)
3. **Transform (ModSources):** Aggregate band energies, beat, and LFO outputs → 8 normalized sources (0-1)
4. **Transform (ModEngine):** Apply curve, multiply by amount, scale to parameter range
5. **Exit:** Computed offsets applied to registered parameter pointers every frame
