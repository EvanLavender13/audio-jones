# Unified Drawable System (Phase 3)

Unify waveforms and spectrum into a single `Drawable` array with per-drawable path (circular/linear) control. Removes global `EffectConfig.circular` toggle. Prepares for Phase 4 shapes.

## Current State

Existing files and hook points:
- `src/config/waveform_config.h` - WaveformConfig with x, y, feedbackPhase, color (delete)
- `src/config/spectrum_bars_config.h` - SpectrumConfig with x, y, feedbackPhase, color (delete)
- `src/config/effect_config.h:18` - Global `bool circular` toggle (remove field)
- `src/config/preset.h:21-23` - Separate `waveforms[]` + `spectrum` (replace with `drawables[]`)
- `src/main.cpp:31-33` - AppContext has separate arrays (replace)
- `src/main.cpp:138-172` - `RenderWaveformsWithPhase()` switch on global circular (extract to drawable.cpp)
- `src/render/waveform.cpp:179,216` - Draw functions take `WaveformConfig*` (change to `Drawable*`)
- `src/render/spectrum_bars.cpp:115,174` - Draw functions take `SpectrumConfig*` (change to `Drawable*`)

---

## Phase 1: Config Unification

**Goal**: Create unified drawable config structs.

**Create**:
- `src/config/drawable_config.h` - DrawableType enum, DrawablePath enum, DrawableBase struct, WaveformData struct, SpectrumData struct, Drawable struct with union

**Delete**:
- `src/config/waveform_config.h`
- `src/config/spectrum_bars_config.h`

**Modify**:
- `src/config/effect_config.h` - Remove `bool circular` field
- Update all includes that reference deleted files

**Struct definitions**:

```cpp
typedef enum { DRAWABLE_WAVEFORM, DRAWABLE_SPECTRUM } DrawableType;
typedef enum { PATH_LINEAR, PATH_CIRCULAR } DrawablePath;

struct DrawableBase {
    bool enabled = true;
    float x = 0.5f;
    float y = 0.5f;
    float rotationSpeed = 0.0f;
    float rotationOffset = 0.0f;
    float feedbackPhase = 1.0f;
    ColorConfig color;
};

struct WaveformData {
    float amplitudeScale = 0.35f;
    int thickness = 2;
    float smoothness = 5.0f;
    float radius = 0.25f;
};

struct SpectrumData {
    float innerRadius = 0.15f;
    float barHeight = 0.25f;
    float barWidth = 0.8f;
    float smoothing = 0.8f;
    float minDb = 10.0f;
    float maxDb = 50.0f;
};

struct Drawable {
    DrawableType type = DRAWABLE_WAVEFORM;
    DrawablePath path = PATH_CIRCULAR;
    DrawableBase base;
    union {
        WaveformData waveform;
        SpectrumData spectrum;
    };
};
```

**Done when**: Project compiles with new header, old headers deleted.

---

## Phase 2: Drawable Dispatch Module

**Goal**: Create drawable.cpp dispatch layer to keep rendering logic out of main.cpp.

**Create**:
- `src/render/drawable.h` - DrawableState struct, lifecycle and render functions
- `src/render/drawable.cpp` - Implementation with dispatch loop

**Key functions**:
- `DrawableStateInit()` / `DrawableStateUninit()` - Lifecycle
- `DrawableProcessWaveforms()` - Updates waveform buffers for all DRAWABLE_WAVEFORM
- `DrawableProcessSpectrum()` - Updates spectrum bars for DRAWABLE_SPECTRUM
- `DrawableRenderAll()` - Dispatch loop for pre/post feedback rendering
- `DrawableValidate()` - Enforce one spectrum max

**DrawableState holds**:
- `WaveformPipeline waveformPipeline` - Embedded
- `SpectrumBars* spectrumBars` - Pointer (single instance)

**Render dispatch pattern**:
```cpp
void DrawableRenderAll(const DrawableState* state, const RenderContext* ctx,
                       const Drawable* drawables, int count,
                       uint64_t tick, bool isPreFeedback)
{
    for (int i = 0; i < count; i++) {
        if (!drawables[i].base.enabled) continue;
        float opacity = isPreFeedback
            ? (1.0f - drawables[i].base.feedbackPhase)
            : drawables[i].base.feedbackPhase;
        if (opacity < 0.001f) continue;

        switch (drawables[i].type) {
            case DRAWABLE_WAVEFORM:
                DrawableRenderWaveform(state, ctx, &drawables[i], i, tick, opacity);
                break;
            case DRAWABLE_SPECTRUM:
                DrawableRenderSpectrum(state, ctx, &drawables[i], tick, opacity);
                break;
        }
    }
}
```

**Done when**: Module compiles, exports declared functions.

---

## Phase 3: Update Draw Functions

**Goal**: Change waveform and spectrum draw functions to accept `const Drawable*` directly.

**Modify**:
- `src/render/waveform.h` - Change signatures
- `src/render/waveform.cpp:179-271` - Access `d->base.*` and `d->waveform.*`
- `src/render/spectrum_bars.h` - Change signatures
- `src/render/spectrum_bars.cpp:115-211` - Access `d->base.*` and `d->spectrum.*`

**Example signature change**:
```cpp
// Old:
void DrawWaveformCircular(float* samples, int count, RenderContext* ctx,
                          WaveformConfig* cfg, uint64_t tick, float opacity);

// New:
void DrawWaveformCircular(float* samples, int count, RenderContext* ctx,
                          const Drawable* d, uint64_t tick, float opacity);
```

**Example access change**:
```cpp
// Old:
const float centerX = cfg->x * ctx->screenW;
const float amplitude = ctx->minDim * cfg->amplitudeScale;
Color segColor = ColorFromConfig(&cfg->color, t, opacity);

// New:
const float centerX = d->base.x * ctx->screenW;
const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
Color segColor = ColorFromConfig(&d->base.color, t, opacity);
```

**Done when**: All draw functions compile with new signatures.

---

## Phase 4: Main Context Integration

**Goal**: Replace separate arrays in AppContext with unified drawables array.

**Modify**:
- `src/config/preset.h` - Replace `waveforms[]` + `spectrum` with `Drawable drawables[MAX_DRAWABLES]`
- `src/config/app_configs.h` - Update pointer types
- `src/main.cpp:24-40` - Update AppContext struct
- `src/main.cpp:42-60` - Update AppContextUninit
- `src/main.cpp:71-99` - Update AppContextInit to use DrawableStateInit
- `src/main.cpp:105-117` - Replace UpdateVisuals with DrawableProcess calls
- `src/main.cpp:136-224` - Replace RenderWaveformsWithPhase/Full with DrawableRenderAll calls

**AppContext changes**:
```cpp
typedef struct AppContext {
    AnalysisPipeline analysis;
    DrawableState* drawableState;  // Replaces waveformPipeline + spectrumBars
    PostEffect* postEffect;
    AudioCapture* capture;
    AudioConfig audio;

    Drawable drawables[MAX_DRAWABLES];  // Replaces waveforms[] + spectrum
    int drawableCount;
    int selectedDrawable;

    // ... rest unchanged
} AppContext;
```

**Done when**: Application runs with unified drawable array.

---

## Phase 5: Preset Serialization

**Goal**: Update preset serialization for new Drawable format.

**Modify**:
- `src/config/preset.cpp` - Add custom to_json/from_json for Drawable

**Serialization pattern**:
```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrawableBase,
    enabled, x, y, rotationSpeed, rotationOffset, feedbackPhase, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveformData,
    amplitudeScale, thickness, smoothness, radius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectrumData,
    innerRadius, barHeight, barWidth, smoothing, minDb, maxDb)

// Custom for union handling
static void to_json(json& j, const Drawable& d) {
    j["type"] = d.type;
    j["path"] = d.path;
    j["base"] = d.base;
    if (d.type == DRAWABLE_WAVEFORM) {
        j["waveform"] = d.waveform;
    } else {
        j["spectrum"] = d.spectrum;
    }
}

static void from_json(const json& j, Drawable& d) {
    d = Drawable{};
    d.type = j.value("type", DRAWABLE_WAVEFORM);
    d.path = j.value("path", PATH_CIRCULAR);
    d.base = j.value("base", DrawableBase{});
    if (d.type == DRAWABLE_WAVEFORM && j.contains("waveform")) {
        d.waveform = j["waveform"].get<WaveformData>();
    } else if (d.type == DRAWABLE_SPECTRUM && j.contains("spectrum")) {
        d.spectrum = j["spectrum"].get<SpectrumData>();
    }
}
```

**Preset struct update**:
```cpp
struct Preset {
    char name[PRESET_NAME_MAX];
    EffectConfig effects;  // circular field removed
    AudioConfig audio;
    Drawable drawables[MAX_DRAWABLES];
    int drawableCount;
    ModulationConfig modulation;
    LFOConfig lfos[4];
};
```

**Done when**: New presets save/load correctly with new format.

---

## Phase 6: UI Panel Updates

**Goal**: Update UI panels to work with unified drawable array.

**Modify**:
- `src/ui/imgui_waveforms.cpp` - Rename to imgui_drawables.cpp, filter by type
- `src/ui/imgui_spectrum.cpp` - Merge into drawable panel or find spectrum in array
- `src/ui/imgui_panels.h` - Update function signatures

**UI changes**:
- Drawable list shows type indicator (Waveform/Spectrum)
- Per-drawable path toggle (Linear/Circular dropdown)
- Controls filter based on `drawable->type`

**Done when**: UI controls modify correct drawable fields.

---

## Phase 7: Manual Preset Migration

**Goal**: Update existing preset JSON files to new format.

**For each preset in `presets/`**:
1. Read old format:
   ```json
   {
     "effects": { "circular": true, ... },
     "waveformCount": 2,
     "waveforms": [ {...}, {...} ],
     "spectrum": { "enabled": true, ... }
   }
   ```

2. Convert to new format:
   ```json
   {
     "effects": { ... },  // circular removed
     "drawableCount": 3,
     "drawables": [
       { "type": 0, "path": 1, "base": {...}, "waveform": {...} },
       { "type": 0, "path": 1, "base": {...}, "waveform": {...} },
       { "type": 1, "path": 1, "base": {...}, "spectrum": {...} }
     ]
   }
   ```

**Conversion rules**:
- `type: 0` = DRAWABLE_WAVEFORM, `type: 1` = DRAWABLE_SPECTRUM
- `path: 0` = PATH_LINEAR, `path: 1` = PATH_CIRCULAR
- Apply old `effects.circular` value to all drawables' `path`
- Move shared fields (x, y, rotation*, feedbackPhase, color) into `base`
- Move type-specific fields into `waveform` or `spectrum`
- Only include spectrum drawable if old `spectrum.enabled == true`

**Done when**: All preset files updated, load without errors.

---

## Phase 8: Testing and Validation

**Goal**: Verify everything works correctly.

**Test cases**:
- [ ] Load converted preset, verify visual output matches previous
- [ ] Create preset with mixed paths (circular waveform + linear spectrum)
- [ ] Verify per-drawable feedbackPhase still works (pre/post feedback split)
- [ ] Add 8 waveforms + 1 spectrum, verify no crashes
- [ ] Verify spectrum limit enforced (only one DRAWABLE_SPECTRUM allowed)
- [ ] Save preset, reload, verify round-trip

**Done when**: All test cases pass.
