# RayCameraConfig Shared Struct

Embeddable camera configuration for raymarched generators. Extracts repeated camera parameters (zoom, drift, rotation) into a single shared struct with a reusable UI widget, following the same pattern as `DualLissajousConfig` and `ColorConfig`.

## Classification

- **Category**: General (architecture / shared infrastructure)
- **Scope**: New shared config header, UI widget, param registration helper. Existing generators adopt incrementally.

## References

- `src/config/dual_lissajous_config.h` — Existing shared embeddable config pattern with inline update function
- `src/render/color_config.h` — Existing shared config with manual serialization
- `src/ui/ui_units.h` — Home for `DrawLissajousControls()` and other reusable UI widgets

## Current State

Camera-like parameters are duplicated across raymarched generators with inconsistent naming:

| Generator | Zoom/Distance | Drift | Rotation |
|-----------|--------------|-------|----------|
| Laser Dance | *(hardcoded)* | *(none)* | *(none)* |
| Prism Shatter | `orbitRadius`, `fov` | circular orbit (`cameraSpeed`) | implicit from orbit |
| Vortex | `cameraDistance` | *(none)* | `rotationSpeed` |
| Shell | `cameraDistance` | *(none)* | `phaseX/Y/Z` |
| Muons | `cameraDistance` | *(none)* | `phaseX/Y/Z`, `drift` |

After the Laser Dance Camera enhancement, Laser Dance will have `zoom`, `DualLissajousConfig drift`, and `rotationSpeed` — the cleanest version of this pattern. That becomes the template.

## Algorithm

### Struct Design

```
src/config/ray_camera_config.h
```

```c
struct RayCameraConfig {
  float zoom = 1.0f;           // Focal length / FOV (0.5-3.0)
  float rotationSpeed = 0.0f;  // View rotation rate rad/s (-PI_F to PI_F)
  DualLissajousConfig drift;   // Camera position drift path
};
```

The struct embeds `DualLissajousConfig` for drift. Each consumer overrides drift defaults via designated initializers to match their coordinate space:

```c
// Laser Dance: large cosine field, wide drift
RayCameraConfig camera = {.drift = {.amplitude = 16.0f, .freqX1 = 0.07f, .freqY1 = 0.05f}};

// Vortex: smaller volume, tighter drift
RayCameraConfig camera = {.zoom = 6.0f, .drift = {.amplitude = 2.0f, .freqX1 = 0.03f, .freqY1 = 0.02f}};
```

### Update Function

Inline in the header, same pattern as `DualLissajousUpdate()`:

```c
struct RayCameraState {
  float angle;     // Accumulated rotation
  float driftX;    // Current drift offset X
  float driftY;    // Current drift offset Y
};

inline void RayCameraUpdate(RayCameraConfig *cfg, RayCameraState *state, float deltaTime) {
  state->angle += cfg->rotationSpeed * deltaTime;
  DualLissajousUpdate(&cfg->drift, deltaTime, 0.0f, &state->driftX, &state->driftY);
}
```

Each generator's Setup function calls `RayCameraUpdate()` then passes `zoom`, `driftX`, `driftY`, `angle` as uniforms.

### UI Widget

In `src/ui/ui_units.h`:

```c
inline void DrawRayCameraControls(RayCameraConfig *cfg, const char *idSuffix,
                                  const char *paramPrefix, const ModSources *modSources,
                                  float amplitudeMax = 32.0f) {
  // zoom slider
  // rotationSpeed slider (angle/deg per sec)
  // DrawLissajousControls() for drift
}
```

### Serialization

Define `RAY_CAMERA_CONFIG_FIELDS` macro. Add manual or macro-based `to_json`/`from_json` in `effect_serialization.cpp`, similar to existing `DualLissajousConfig` handling.

### Param Registration Helper

```c
inline void RayCameraRegisterParams(const char *prefix, RayCameraConfig *cfg) {
  // Registers zoom, rotationSpeed, drift.amplitude, drift.motionSpeed
}
```

## Adoption Strategy

1. **Build the struct** with Laser Dance as first consumer (extract from the camera enhancement)
2. **Retrofit Vortex** — replace `cameraDistance` + `rotationSpeed` with embedded `RayCameraConfig`, add drift capability for free
3. **Retrofit Shell/Muons** — replace `cameraDistance` + `phaseX/Y/Z` with `RayCameraConfig`, mapping phase offsets to drift frequencies
4. **Prism Shatter** — leave alone or adapt later; its orbit camera is structurally different

Each adoption is a small independent PR. The struct exists and works after step 1; steps 2-4 are optional follow-ups.

## Notes

- `RayCameraState` is stored in the effect's runtime struct (e.g., `LaserDanceEffect`), not in the config. Config is user-facing params; state is accumulators.
- The shader side is not standardized — each generator applies drift/rotation differently depending on its ray setup. The shared part is the CPU config, update, and UI.
- `zoom` semantics vary: Laser Dance uses it as focal length in ray direction, Vortex uses it as camera distance along Z. The field name stays `zoom` but the shader interpretation is per-effect.
- No migration/compat code. When retrofitting an existing generator, rename the old fields and update all references in one pass.
