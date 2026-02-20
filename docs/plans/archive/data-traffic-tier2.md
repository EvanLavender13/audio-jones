# Data Traffic — Tier 2 Behaviors

Add six cell-level animation behaviors to the existing Data Traffic generator: Heartbeat, Per-Cell Twitch, Split/Merge, Phase Shift, Doorstop Spring (Position), and Doorstop Spring (Width). Each behavior modifies cell position, width, brightness, or color within the existing per-cell loop and is controlled by a probability param (what fraction of cells/lanes are affected) and an intensity/rate param.

**Research**: `docs/research/data-traffic-behaviors.md` (Tier 2 section, behaviors 3-8)

## Design

### Config Fields

Add to `DataTrafficConfig` in the Behaviors section, after existing `glowRadius`:

| Field | Type | Default | Range | Comment |
|-------|------|---------|-------|---------|
| `heartbeatProb` | float | 0.0f | 0.0-1.0 | Fraction of cells with heartbeat pulse |
| `heartbeatRate` | float | 1.0f | 0.3-2.0 | Heartbeat speed multiplier |
| `twitchProb` | float | 0.0f | 0.0-1.0 | Fraction of cells that twitch |
| `twitchIntensity` | float | 0.5f | 0.0-1.0 | Twitch amplitude scale |
| `splitProb` | float | 0.0f | 0.0-1.0 | Fraction of cells that momentarily shrink |
| `mergeProb` | float | 0.0f | 0.0-1.0 | Fraction of cells that momentarily expand |
| `phaseShiftProb` | float | 0.0f | 0.0-1.0 | Fraction of lanes with speed kick |
| `phaseShiftIntensity` | float | 0.5f | 0.0-1.0 | Phase shift kick strength |
| `springProb` | float | 0.0f | 0.0-1.0 | Fraction of cells with position spring |
| `springIntensity` | float | 0.5f | 0.0-1.0 | Position spring displacement scale |
| `widthSpringProb` | float | 0.0f | 0.0-1.0 | Fraction of cells with width spring |
| `widthSpringIntensity` | float | 0.5f | 0.0-1.0 | Width spring displacement scale |

### Effect Struct Fields

Add to `DataTrafficEffect`, after existing `glowRadiusLoc`:

```
int heartbeatProbLoc, heartbeatRateLoc;
int twitchProbLoc, twitchIntensityLoc;
int splitProbLoc, mergeProbLoc;
int phaseShiftProbLoc, phaseShiftIntensityLoc;
int springProbLoc, springIntensityLoc;
int widthSpringProbLoc, widthSpringIntensityLoc;
```

### Shader Uniforms

12 new `uniform float` declarations matching the config field names above.

### Shader Algorithm

All behaviors use the same hash-seeded probability gate pattern established by Tier 1 breathing. Each block is gated by `if (seed < uniformProb)` so disabled behaviors (prob = 0) cost nothing.

#### Heartbeat (behavior 3) — in cell loop, modifies brightness + color

Dual-Gaussian pulse (lub-dub) with red tint. Applied per-cell after `epochBrightness`:

Declare `float heartBeat = 0.0;` before the gate. Inside the gate, compute the dual-Gaussian and update brightness. After `cCol` is computed (after both color/grayscale branches), apply the red tint:

```glsl
// Before the heartbeat gate:
float heartBeat = 0.0;
float heartSeed = h21(vec2(idx * 11.1, float(laneIdx) + 678.0));
if (heartSeed < heartbeatProb) {
    float hRate = mix(0.8, 1.4, h21(vec2(idx + 1500.0, float(laneIdx)))) * heartbeatRate;
    float heartPhase = fract(time * hRate + heartSeed * 10.0);
    float lub = exp(-pow((heartPhase - 0.1) * 15.0, 2.0));
    float dub = exp(-pow((heartPhase - 0.25) * 18.0, 2.0)) * 0.7;
    heartBeat = max(lub, dub);
    brightness = max(brightness, heartBeat);
}

// After cCol is computed (after both isColorCell branches):
if (heartBeat > 0.3) {
    cCol = mix(cCol, vec3(0.9, 0.1, 0.15) * brightness, heartBeat * 0.6);
}
```

#### Per-Cell Twitch (behavior 4) — in cell loop, modifies jitteredCenter

Multi-frequency sinusoidal position oscillation with burst envelope:

```glsl
float twitchSeed = h21(vec2(idx * 7.3, float(laneIdx) * 3.1 + 55.0));
if (twitchSeed < twitchProb) {
    float twitchRate = mix(8.0, 25.0, h21(vec2(idx + 500.0, float(laneIdx))));
    float twitchAmp = slotWidth * mix(0.08, 0.25, h21(vec2(idx + 600.0, float(laneIdx)))) * twitchIntensity;
    float twitch = sin(time * twitchRate + idx * 3.7) * 0.6
                 + sin(time * twitchRate * 1.7 + idx * 5.1) * 0.3
                 + sin(time * twitchRate * 3.1 + idx * 9.3) * 0.1;
    float burstSeed = h21(vec2(idx + 700.0, float(laneIdx)));
    float burst = smoothstep(0.3, 0.5, sin(time * mix(0.3, 0.8, burstSeed) + burstSeed * 6.28));
    jitteredCenter += twitch * twitchAmp * burst;
}
```

#### Split/Merge (behavior 5) — in cell loop, modifies halfW

Uses existing width epoch variables (`widthEpoch`, `widthProgress`):

The reference uses the existing width epoch rate `wER = mix(.15, .5, rh6)` per-cell. In our shader, `widthEpoch` is already computed per-cell. Use it plus a per-cell hash for the split/merge phase seed (`s3` equivalent):

```glsl
// Split — shrink to near-zero
float spS = h21(vec2(idx + widthEpoch * 11.3, float(laneIdx) + 77.0));
if (spS < splitProb) {
    float smRate = mix(0.15, 0.5, h21(vec2(idx * 0.4217, float(laneIdx) + 57.3)));
    float smSeed = h21(vec2(idx + 200.0, float(laneIdx)));
    float p = fract(time * smRate + smSeed * 12.0);
    float c = smoothstep(0.0, 0.1, p) * (1.0 - smoothstep(0.5, 0.8, p));
    halfW *= mix(1.0, 0.05, c);
}
// Merge — expand wide (only if not splitting)
float mS = h21(vec2(idx + widthEpoch * 5.7, float(laneIdx) + 33.0));
if (mS < mergeProb && spS >= splitProb) {
    float smRate = mix(0.15, 0.5, h21(vec2(idx * 0.4217, float(laneIdx) + 57.3)));
    float smSeed = h21(vec2(idx + 200.0, float(laneIdx)));
    float p = fract(time * smRate + smSeed * 12.0);
    float e2 = smoothstep(0.0, 0.2, p) * (1.0 - smoothstep(0.6, 0.9, p));
    halfW *= mix(1.0, 2.5, e2);
}
```

#### Phase Shift (behavior 6) — in `laneScrolledX`, modifies speed

Row-level speed kick with damped spring wobble. Add before the `return` in `laneScrolledX`:

The reference modifies `r.speed` (velocity), but our shader has no row-state struct — `laneScrolledX` returns a position offset directly. To achieve the same visual effect (lanes jolting forward then wobbling back), apply the kick as a position displacement scaled by `cellWidth * spacing` so it's proportional to cell size:

```glsl
float phaseSeed = h11(laneF * 0.371 + 222.0);
if (phaseSeed < phaseShiftProb) {
    float phaseRate = mix(0.08, 0.2, h11(laneF * 0.621 + 333.0));
    float phaseEpoch = floor(time * phaseRate + phaseSeed * 20.0);
    float phaseT = fract(time * phaseRate + phaseSeed * 20.0);
    float phaseDir = h11(laneF * 0.441 + phaseEpoch * 7.7) > 0.5 ? 1.0 : -1.0;
    float phaseKick = 0.0;
    if (phaseT < 0.15) {
        float snapT2 = phaseT / 0.15;
        phaseKick = phaseDir * 8.0 * (1.0 - snapT2);
    } else if (phaseT < 0.6) {
        float wobT = (phaseT - 0.15) / 0.45;
        phaseKick = phaseDir * 3.0 * exp(-4.0 * wobT) * cos(wobT * 40.0);
    }
    scrolledX += phaseKick * phaseShiftIntensity * cellWidth * spacing;
}
```

Place this in `laneScrolledX` before the `return` statement. `cellWidth` and `spacing` are already uniforms accessible in that scope.

#### Doorstop Spring Position (behavior 7) — in cell loop, modifies jitteredCenter

```glsl
float springChance = h21(vec2(idx * 4.1, float(laneIdx) + 123.0));
if (springChance < springProb) {
    float springRate = mix(0.15, 0.5, h21(vec2(idx + 800.0, float(laneIdx))));
    float s2 = h21(vec2(idx + 200.0, float(laneIdx)));
    float springEpoch = floor(time * springRate + s2 * 10.0);
    float springT = fract(time * springRate + s2 * 10.0);
    float target = (h21(vec2(idx + springEpoch * 5.1, float(laneIdx) + 90.0)) - 0.5) * slotWidth * 5.0 * springIntensity;
    float kick = exp(-3.0 * springT);
    float wobble = cos(springT * 50.0);
    jitteredCenter += target * kick * wobble;
}
```

#### Doorstop Spring Width (behavior 8) — in cell loop, modifies halfW

```glsl
float springWSeed = h21(vec2(idx * 3.3, float(laneIdx) + 155.0));
if (springWSeed < widthSpringProb) {
    float swRate = mix(0.15, 0.4, h21(vec2(idx + 900.0, float(laneIdx))));
    float s4 = h21(vec2(idx + 400.0, float(laneIdx)));
    float swEpoch = floor(time * swRate + s4 * 8.0);
    float swT = fract(time * swRate + s4 * 8.0);
    float wKick = mix(0.3, 3.0, h21(vec2(idx + swEpoch * 7.7, float(laneIdx) + 111.0)));
    float wDecay = exp(-3.5 * swT);
    float wWobble = cos(swT * 40.0);
    float wMul = 1.0 + (wKick - 1.0) * wDecay * wWobble * widthSpringIntensity;
    halfW *= max(wMul, 0.02);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| heartbeatProb | float | 0.0-1.0 | 0.0 | Yes | Heartbeat Prob |
| heartbeatRate | float | 0.3-2.0 | 1.0 | Yes | Heartbeat Rate |
| twitchProb | float | 0.0-1.0 | 0.0 | Yes | Twitch Prob |
| twitchIntensity | float | 0.0-1.0 | 0.5 | Yes | Twitch Intensity |
| splitProb | float | 0.0-1.0 | 0.0 | Yes | Split Prob |
| mergeProb | float | 0.0-1.0 | 0.0 | Yes | Merge Prob |
| phaseShiftProb | float | 0.0-1.0 | 0.0 | Yes | Phase Shift Prob |
| phaseShiftIntensity | float | 0.0-1.0 | 0.5 | Yes | Phase Shift Intensity |
| springProb | float | 0.0-1.0 | 0.0 | Yes | Spring Prob |
| springIntensity | float | 0.0-1.0 | 0.5 | Yes | Spring Intensity |
| widthSpringProb | float | 0.0-1.0 | 0.0 | Yes | Width Spring Prob |
| widthSpringIntensity | float | 0.0-1.0 | 0.5 | Yes | Width Spring Intensity |

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Add Tier 2 config and effect struct fields

**Files**: `src/effects/data_traffic.h`
**Creates**: Config fields and uniform location fields that all other tasks depend on

**Do**:

1. Add 12 config fields to `DataTrafficConfig` after `glowRadius`, in a sub-group comment `// Behaviors — Tier 2`. Use the field names, types, defaults, and range comments from the Config Fields table above.

2. Add 12 uniform location `int` fields to `DataTrafficEffect` after `glowRadiusLoc`:
   - `heartbeatProbLoc`, `heartbeatRateLoc`
   - `twitchProbLoc`, `twitchIntensityLoc`
   - `splitProbLoc`, `mergeProbLoc`
   - `phaseShiftProbLoc`, `phaseShiftIntensityLoc`
   - `springProbLoc`, `springIntensityLoc`
   - `widthSpringProbLoc`, `widthSpringIntensityLoc`

3. Add all 12 new fields to the `DATA_TRAFFIC_CONFIG_FIELDS` macro, after `glowRadius` and before `baseFreq`.

**Verify**: `cmake.exe --build build` compiles (new fields unused is fine).

---

### Wave 2: Implementation (3 parallel tasks)

#### Task 2.1: Shader — add Tier 2 behavior code

**Files**: `shaders/data_traffic.fs`
**Depends on**: Wave 1 complete (uniform names must match config fields)

**Do**:

1. Add 12 `uniform float` declarations after the existing `glowRadius` uniform, matching config field names exactly.

2. **Phase Shift** (behavior 6): In the `laneScrolledX` function, before the `return` statement, add the phase shift code from the Algorithm section. The kick adds to the returned scroll value as `phaseKick * phaseShiftIntensity * cellWidth * spacing`.

3. **Heartbeat** (behavior 3): Inside the cell loop (`for (int dc = -2; dc <= 2; dc++)`), after `epochBrightness` and the 3% flash check but BEFORE the `if (isColorCell)` branch, add the heartbeat probability gate. It modifies `brightness`. Then after `cCol` is computed (after both the color and grayscale branches), add the red tint mix when `heartBeat > 0.3`. Use a variable declared before the gate (e.g., `float heartBeat = 0.0;`) so it's available after the if-block.

4. **Per-Cell Twitch** (behavior 4): After the existing jitter offset line (`float jitterOffset = ...`), add the twitch code. It adds to `jitteredCenter`. Reference code uses `sp` for spacing — here use `slotWidth`.

5. **Split/Merge** (behavior 5): After `halfW` is computed (after width variation), add split then merge. Split and merge are mutually exclusive per-cell (merge checks `spS >= splitProb`). Both use `widthEpoch` already computed on the lines above.

6. **Doorstop Spring Position** (behavior 7): After twitch, add position spring code. Also modifies `jitteredCenter`. Uses `slotWidth` for `sp`.

7. **Doorstop Spring Width** (behavior 8): After split/merge, add width spring code. Modifies `halfW`.

Follow the GLSL from the Algorithm section. All behavior blocks are independent and additive on position, multiplicative on width.

**Verify**: Shader compiles at runtime (restart app).

---

#### Task 2.2: C++ — uniform binding and param registration

**Files**: `src/effects/data_traffic.cpp`
**Depends on**: Wave 1 complete

**Do**:

1. In `DataTrafficEffectInit`: Add 12 `GetShaderLocation` calls after the existing `glowRadiusLoc` line, one per new uniform. Pattern: `e->heartbeatProbLoc = GetShaderLocation(e->shader, "heartbeatProb");`

2. In `DataTrafficEffectSetup`: Add 12 `SetShaderValue` calls after the existing `glowRadius` binding, one per new config field. Pattern: `SetShaderValue(e->shader, e->heartbeatProbLoc, &cfg->heartbeatProb, SHADER_UNIFORM_FLOAT);`

3. In `DataTrafficRegisterParams`: Add 12 `ModEngineRegisterParam` calls after the existing `glowRadius` registration. Use `"dataTraffic.<fieldName>"` IDs with ranges matching the Parameters table.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: UI — add Tier 2 behavior sliders

**Files**: `src/ui/imgui_effects_gen_texture.cpp`
**Depends on**: Wave 1 complete

**Do**:

In `DrawGeneratorsDataTraffic`, in the Behaviors section (after the existing Glow Radius slider, before `ImGui::Spacing()`), add 12 `ModulatableSlider` calls for the new params. Group visually with blank lines between behavior pairs:

```
Heartbeat Prob, Heartbeat Rate
(blank line)
Twitch Prob, Twitch Intensity
(blank line)
Split Prob, Merge Prob
(blank line)
Phase Shift Prob, Phase Shift Intensity
(blank line)
Spring Prob, Spring Intensity
(blank line)
Width Spring Prob, Width Spring Intensity
```

All use `"%.2f"` format. ImGui ID suffix: `##datatraffic`. Param ID prefix: `"dataTraffic."`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 11 new sliders visible in Data Traffic > Behaviors section
- [ ] All probability sliders default to 0.0 (behaviors off by default)
- [ ] Twitch: set twitchProb to ~0.3 — cells jitter in bursts
- [ ] Split: set splitProb to ~0.3 — cells shrink momentarily
- [ ] Merge: set mergeProb to ~0.3 — cells expand momentarily
- [ ] Fission: set fissionProb to ~0.3 — cells divide into two sub-cells that drift apart
- [ ] Phase Shift: set phaseShiftProb to ~0.3 — lanes jolt forward/backward
- [ ] Spring: set springProb to ~0.3 — cells bounce to random positions
- [ ] Width Spring: set widthSpringProb to ~0.3 — cell widths wobble
- [ ] Multiple behaviors stack cleanly when enabled simultaneously

---

## Implementation Notes

### Heartbeat removed

Heartbeat (behavior 3) was cut during review — the dual-Gaussian lub-dub pulse with red tint didn't fit the Data Traffic aesthetic. All heartbeat fields, uniforms, shader code, and UI sliders were removed.

### Split/Merge restored to reference behavior

The plan's original split/merge algorithm matched the reference (tier 2 behavior 5) but was misnamed — "split" implied cell division when it's actually just a momentary width shrink. The implementation now faithfully matches the reference: split shrinks `halfW` to 5% and merge expands to 250%, both using width-epoch-seeded envelopes. They are independent (no mutual exclusion gate).

### Fission added from tier 3

Fission (tier 3 behavior 10) was added as a separate behavior. Unlike split (width-only), fission renders **two sub-cells** via dual `boxCov` coverage: each sub-cell shrinks to ~45% width and drifts in opposite directions from the original center. The phase envelope ramps up (0→0.2), holds (0.2→0.7), then ramps down (0.7→0.9). This required modifying the coverage computation rather than just `halfW`/`jitteredCenter`, since a single cell's position can only be in one place. Uses its own hash seed (`idx * 6.6, laneIdx + 345.0`) distinct from split/merge.

### Field count: 11 (not 12)

Removing heartbeat (2 fields) and adding fission (1 field) nets 11 new fields total: twitchProb, twitchIntensity, splitProb, mergeProb, fissionProb, phaseShiftProb, phaseShiftIntensity, springProb, springIntensity, widthSpringProb, widthSpringIntensity.
