# Voronoi Modulation

Add audio-reactive modulation to all 4 Voronoi parameters. Refactor inline Voronoi fields into a dedicated `VoronoiConfig` struct with an `enabled` toggle, matching the Physarum pattern.

## Current State

- `src/config/effect_config.h:27-31` - Voronoi params inline: voronoiIntensity, voronoiScale, voronoiSpeed, voronoiEdgeWidth
- `src/ui/imgui_effects.cpp:36-44` - Plain `ImGui::SliderFloat` calls, hidden when intensity==0
- `src/automation/param_registry.cpp:11-26` - Static PARAM_TABLE for modulatable params
- `src/render/render_pipeline.cpp:184-188` - Voronoi pass gated by `intensity > 0`
- `presets/*.json` - Store inline voronoi fields at EffectConfig level

## Phase 1: VoronoiConfig Struct

**Goal**: Create dedicated config struct matching Physarum pattern.

**Create**:
- `src/config/voronoi_config.h` - New header with VoronoiConfig struct

**Struct definition**:
```
VoronoiConfig {
    bool enabled = false
    float intensity = 0.5f    // Range: 0.1-1.0 (enabled handles "off")
    float scale = 15.0f       // Range: 5-50
    float speed = 0.5f        // Range: 0.1-2.0
    float edgeWidth = 0.05f   // Range: 0.01-0.1
}
```

**Done when**: Header compiles standalone.

---

## Phase 2: EffectConfig Integration

**Goal**: Replace inline voronoi fields with struct member.

**Modify**:
- `src/config/effect_config.h`:
  - Add `#include "voronoi_config.h"`
  - Remove lines 27-31 (4 inline voronoi floats)
  - Add `VoronoiConfig voronoi;` member

**Done when**: Project compiles (expect errors in files accessing old fields).

---

## Phase 3: Preset Serialization

**Goal**: JSON serializes nested VoronoiConfig struct.

**Modify**:
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VoronoiConfig, enabled, intensity, scale, speed, edgeWidth)` before EffectConfig macro
  - Update EffectConfig macro: replace `voronoiScale, voronoiIntensity, voronoiSpeed, voronoiEdgeWidth` with `voronoi`

**Done when**: New preset saves with nested `"voronoi": {...}` JSON object.

---

## Phase 4: Render Pipeline Update

**Goal**: Use `voronoi.enabled` to gate render pass.

**Modify**:
- `src/render/render_pipeline.cpp:40-52` (SetupVoronoi):
  - Change `pe->effects.voronoiScale` → `pe->effects.voronoi.scale`
  - Change `pe->effects.voronoiIntensity` → `pe->effects.voronoi.intensity`
  - Change `pe->effects.voronoiSpeed` → `pe->effects.voronoi.speed`
  - Change `pe->effects.voronoiEdgeWidth` → `pe->effects.voronoi.edgeWidth`

- `src/render/render_pipeline.cpp:184-188`:
  - Change condition from `voronoiIntensity > 0.0f` to `voronoi.enabled`

**Done when**: Voronoi effect renders when enabled=true.

---

## Phase 5: Param Registry Wiring

**Goal**: Register 4 Voronoi params for modulation.

**Modify**:
- `src/automation/param_registry.cpp:11-26` - Add to PARAM_TABLE:
  - `{"voronoi.intensity", {0.1f, 1.0f}}`
  - `{"voronoi.scale", {5.0f, 50.0f}}`
  - `{"voronoi.speed", {0.1f, 2.0f}}`
  - `{"voronoi.edgeWidth", {0.01f, 0.1f}}`

- `src/automation/param_registry.cpp:32-47` - Add to targets array:
  - `&effects->voronoi.intensity`
  - `&effects->voronoi.scale`
  - `&effects->voronoi.speed`
  - `&effects->voronoi.edgeWidth`

**Done when**: `ModEngineGetParam("voronoi.scale")` returns valid pointer.

---

## Phase 6: UI Integration

**Goal**: Voronoi panel uses checkbox + ModulatableSlider.

**Modify**:
- `src/ui/imgui_effects.cpp:36-44` - Replace Voronoi section:
  - Add `ImGui::Checkbox("Enabled##vor", &e->voronoi.enabled);`
  - Wrap sliders in `if (e->voronoi.enabled)` block
  - Replace 4 `ImGui::SliderFloat` with `ModulatableSlider`:
    - Intensity: paramId `"voronoi.intensity"`, format `"%.2f"`
    - Scale: paramId `"voronoi.scale"`, format `"%.1f"`
    - Speed: paramId `"voronoi.speed"`, format `"%.2f"`
    - Edge Width: paramId `"voronoi.edgeWidth"`, format `"%.3f"`

**Done when**: Diamond indicators appear on all 4 sliders, modulation popup works.

---

## Phase 7: Preset Migration

**Goal**: Manually update existing preset files to new structure.

**For each preset in `presets/*.json`**:
1. Find old inline fields: `voronoiScale`, `voronoiIntensity`, `voronoiSpeed`, `voronoiEdgeWidth`
2. Determine if Voronoi was "enabled" (any field had non-default values)
3. Replace with nested structure:
   ```json
   "voronoi": {
     "enabled": true/false,
     "intensity": <old voronoiIntensity or 0.5 if was 0>,
     "scale": <old voronoiScale>,
     "speed": <old voronoiSpeed>,
     "edgeWidth": <old voronoiEdgeWidth>
   }
   ```
4. Remove old inline voronoi fields

**Done when**: All presets load without warnings, Voronoi state preserved.

---

## File Summary

**Create (1 file)**:
- `src/config/voronoi_config.h`

**Modify (4 files)**:
- `src/config/effect_config.h`
- `src/config/preset.cpp`
- `src/render/render_pipeline.cpp`
- `src/automation/param_registry.cpp`
- `src/ui/imgui_effects.cpp`

**Update manually**:
- `presets/*.json` - Migrate to nested voronoi structure
