# Prune Disabled Modulator Serialization

Stop serializing disabled `LFOConfig` and `ModBusConfig` entries to preset JSON. Disabled slots are dropped from the file; missing slots default-construct on load (which is `enabled = false` plus default field values).

The slot index is significant (LFO1..LFO8 and BUS1..BUS8 are distinct mod sources), so the array form is replaced with a sparse JSON object keyed by stringified slot index.

**Research**: none (mechanical serialization change).

## Design

### Format change

Before:

```json
"lfos": [
  {"enabled": false, "rate": 0.1, "waveform": 0, "phaseOffset": 0.0},
  {"enabled": false, "rate": 0.1, "waveform": 0, "phaseOffset": 0.0},
  {"enabled": true,  "rate": 0.5, "waveform": 1, "phaseOffset": 0.0},
  ... 5 more disabled entries ...
],
"modBuses": [ ... 8 entries, often all disabled ... ]
```

After:

```json
"lfos": {
  "2": {"enabled": true, "rate": 0.5, "waveform": 1, "phaseOffset": 0.0}
},
"modBuses": {}
```

### Types

No struct changes. `Preset::lfos[NUM_LFOS]` and `Preset::modBuses[NUM_MOD_BUSES]` remain dense in memory; only the JSON shape changes.

### Algorithm

`to_json(json& j, const Preset& p)`:

```cpp
j["lfos"] = json::object();
for (int i = 0; i < NUM_LFOS; i++) {
  if (p.lfos[i].enabled) {
    j["lfos"][std::to_string(i)] = p.lfos[i];
  }
}
j["modBuses"] = json::object();
for (int i = 0; i < NUM_MOD_BUSES; i++) {
  if (p.modBuses[i].enabled) {
    j["modBuses"][std::to_string(i)] = p.modBuses[i];
  }
}
```

`from_json(const json& j, Preset& p)`:

```cpp
if (j.contains("lfos")) {
  for (const auto& [key, value] : j["lfos"].items()) {
    const int slot = std::stoi(key);
    if (slot >= 0 && slot < NUM_LFOS) {
      p.lfos[slot] = value.get<LFOConfig>();
    }
  }
}
if (j.contains("modBuses")) {
  for (const auto& [key, value] : j["modBuses"].items()) {
    const int slot = std::stoi(key);
    if (slot >= 0 && slot < NUM_MOD_BUSES) {
      p.modBuses[slot] = value.get<ModBusConfig>();
    }
  }
}
```

Notes:
- `Preset` is default-constructed inside `j.get<Preset>()` before `from_json` runs, so unmentioned slots are already `LFOConfig{}` / `ModBusConfig{}` (i.e. disabled with default fields). No reset loop needed.
- `std::stoi` propagates exceptions; the outer `try/catch` in `PresetLoad` already handles parse failures by returning `false`.

### Parameters

None.

### Constants

None.

---

## Tasks

### Wave 1

#### Task 1.1: Replace LFO/ModBus serialization with sparse keyed objects

**Files**: `src/config/preset.cpp`

**Do**:
- In `to_json(json& j, const Preset& p)`: replace the two `json::array()` blocks (lines 106-113) with the keyed-object writes from the Algorithm section. Only push entries whose `enabled` is `true`.
- In `from_json(const json& j, Preset& p)`: replace the two array-iteration blocks (lines 133-145) with the `.items()` loops from the Algorithm section.
- Leave everything else in `preset.cpp` untouched. `PresetFromAppConfigs` / `PresetToAppConfigs` continue to copy the full dense arrays in memory.
- Do not add backwards-compatibility code, legacy key fallbacks, or comments referring to the old format.

**Verify**:
- `cmake.exe --build build` compiles with no warnings.
- Save a preset with all LFOs/buses disabled: the resulting JSON shows `"lfos": {}` and `"modBuses": {}`.
- Save a preset with one LFO and one bus enabled: only those slots appear, keyed by their slot index.
- Reload either preset: enabled slots restore to their saved values; disabled slots remain default-disabled.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Empty case: all-disabled preset writes `"lfos": {}` and `"modBuses": {}`
- [ ] Sparse case: only enabled slots are present, keyed by slot index as strings
- [ ] Round-trip: save then load reproduces the in-memory state for enabled slots
- [ ] Disabled slot tweaks are intentionally lost on save (matches design decision)
