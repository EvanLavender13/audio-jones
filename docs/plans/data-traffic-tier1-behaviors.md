# Data Traffic — Tier 1 Behaviors (Row Flash + Breathing)

Add two simple cell-level animation behaviors to the Data Traffic generator: Row Flash (random full-brightness flash on entire rows) and Breathing (sinusoidal row gap oscillation). Both are probability-gated and shader-only — no new render passes or textures.

**Research**: `docs/research/data-traffic-behaviors.md` (Tier 1 section)

<!-- Intentional deviations from research:
  - Row Flash: no `fog` term — our shader has no fog variable (reference Shadertoy scene-level context)
  - Row Flash: `flashRate` renamed to `flashProb` — matches probability-gating pattern
  - Breathing: `breathRate` is a global multiplier on per-lane random rate, not a direct replacement — preserves per-lane variety
-->

## Design

### Types

Add to `DataTrafficConfig` (new "Behaviors" group):

```
// Behaviors
float flashProb = 0.0f;      // Fraction of rows that flash (0.0-1.0)
float flashIntensity = 0.3f;  // Flash brightness boost (0.0-1.0)
float breathProb = 0.0f;      // Fraction of lanes that breathe (0.0-1.0)
float breathRate = 0.25f;     // Breathing oscillation speed (0.05-0.5)
```

Add to `DataTrafficEffect`:

```
int flashProbLoc;
int flashIntensityLoc;
int breathProbLoc;
int breathRateLoc;
```

### Algorithm

Both behaviors operate on per-lane hashes already available in the shader.

**Row Flash** — applied after the cell loop, before `finalColor` output:

```glsl
// Row flash: probabilistic full-brightness accent
float rfl = h21(vec2(float(laneIdx), floor(time * 1.5)));
if (rfl > (1.0 - flashProb)) {
    color = mix(color, vec3(1.0), flashIntensity);
}
```

The hash is re-seeded each `floor(time * 1.5)` frame, giving ~1.5 flashes/sec at the random-selection rate. `flashProb` controls what fraction of lanes flash in any given frame. At 0.0 no lanes flash (behavior off). At 1.0 all lanes flash every frame.

**Breathing** — applied to `gapMask` computation, replacing the fixed `gapSize` for selected lanes:

```glsl
// Breathing: sinusoidal gap oscillation on selected lanes
float breathSeed = h11(float(laneIdx) * 0.531 + 99.0);
float effectiveGap = gapSize;
if (breathSeed < breathProb) {
    float bRate = mix(0.15, 0.4, h11(float(laneIdx) * 0.831 + 111.0));
    float breath = sin(time * bRate * breathRate * 6.28318) * 0.5 + 0.5;
    effectiveGap = mix(0.02, 0.35, breath);
}
float gapMask = smoothstep(0.0, effectiveGap * 0.5, withinLane)
              * smoothstep(1.0, 1.0 - effectiveGap * 0.5, withinLane);
```

`breathProb` gates which lanes breathe (hash < threshold). `breathRate` scales the per-lane oscillation speed. Each lane gets a slightly different rate from `bRate` for visual variety.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| flashProb | float | 0.0-1.0 | 0.0 | yes | Flash Prob |
| flashIntensity | float | 0.0-1.0 | 0.3 | yes | Flash Intensity |
| breathProb | float | 0.0-1.0 | 0.0 | yes | Breath Prob |
| breathRate | float | 0.05-0.5 | 0.25 | yes | Breath Rate |

All default to 0.0 probability (off) so existing presets are unaffected.

---

## Tasks

### Wave 1: Config + Shader (no file overlap)

#### Task 1.1: Add config fields and uniforms

**Files**: `src/effects/data_traffic.h`, `src/effects/data_traffic.cpp`

**Do**:
- Add 4 config fields to `DataTrafficConfig` under a `// Behaviors` comment, after the existing `sparkIntensity` field. Fields: `flashProb`, `flashIntensity`, `breathProb`, `breathRate` with defaults and range comments per the Design section.
- Update `DATA_TRAFFIC_CONFIG_FIELDS` macro to include the 4 new fields.
- Add 4 `int` location fields to `DataTrafficEffect` struct: `flashProbLoc`, `flashIntensityLoc`, `breathProbLoc`, `breathRateLoc`.
- In `DataTrafficEffectInit`: add 4 `GetShaderLocation` calls for `"flashProb"`, `"flashIntensity"`, `"breathProb"`, `"breathRate"`.
- In `DataTrafficEffectSetup`: add 4 `SetShaderValue` calls binding the config fields to the uniform locations.
- In `DataTrafficRegisterParams`: add 4 `ModEngineRegisterParam` calls with ranges matching the parameter table.

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.2: Add shader behaviors

**Files**: `shaders/data_traffic.fs`

**Do**:
- Add 4 uniforms: `uniform float flashProb;`, `uniform float flashIntensity;`, `uniform float breathProb;`, `uniform float breathRate;` alongside existing uniforms.
- **Breathing**: Before the existing `gapMask` computation (line ~53), implement the breathing behavior per the Algorithm section. The breathing code computes `effectiveGap` which replaces `gapSize` in the `smoothstep` calls for `gapMask`.
- **Row Flash**: After the spark detection loop and before `finalColor = vec4(color, 1.0);` (line ~186), implement the row flash behavior per the Algorithm section.

**Verify**: Shader has no syntax errors visible on app launch.

#### Task 1.3: Add UI sliders

**Files**: `src/ui/imgui_effects_gen_texture.cpp`

**Do**:
- In `DrawGeneratorsDataTraffic`, after the Animation section's last slider (`Spark Intensity`) and before the `ImGui::Spacing(); ImGui::Separator();` block, add a new `ImGui::SeparatorText("Behaviors");` section.
- Add 4 `ModulatableSlider` calls in order: Flash Prob, Flash Intensity, Breath Prob, Breath Rate. Use `"%.2f"` format and `"##datatraffic"` suffix. Param IDs: `"dataTraffic.flashProb"`, `"dataTraffic.flashIntensity"`, `"dataTraffic.breathProb"`, `"dataTraffic.breathRate"`.

**Verify**: `cmake.exe --build build` compiles.

---

## Implementation Notes

Changes made after initial implementation:

### Row Flash removed, replaced with brightness variation
- `flashProb`/`flashIntensity` deleted from config, uniforms, UI, shader, and param registration
- All cells now get epoch-based brightness from the reference shader: 25% off, 20% dim (0.05-0.2), 25% medium (0.25-0.55), 30% bright (0.65-1.0), plus 3% per-frame full-flash chance
- Color cells multiply FFT magnitude on top of epoch brightness; gray cells show epoch brightness directly with a slight warm tint when bright
- This replaces the flat 0.03-0.05 gray that non-color cells previously had

### Breath rate accumulation fix
- `breathRate` no longer sent as a shader uniform — changing it caused instant phase jumps
- C++ side accumulates `breathPhase += breathRate * deltaTime` in `DataTrafficEffectSetup`
- Shader receives `breathPhase` and uses `sin(breathPhase * bRate * 6.28318)` — per-lane variety preserved via `bRate`, rate changes are smooth

### Proximity glow added
- New params: `glowIntensity` (0.0-1.0, default 0.0), `glowRadius` (0.5-5.0, default 2.5)
- 3×7 loop in shader: checks cells in current lane ± 1, cell slots ± 3
- Cells emit glow if they're color cells or have epoch brightness > 0.4
- Gaussian distance falloff with aspect-corrected Y distance
- Glow color: LUT color for color cells (scaled by brightness), grayscale for gray cells

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] App launches, Data Traffic renders with rich brightness variation at default settings
- [x] Raising Breath Prob shows lanes pulsing wider/narrower (smooth rate changes)
- [x] Raising Glow Intensity shows colored light bleeding between lanes
- [x] Brightness variation and glow combine without visual artifacts
