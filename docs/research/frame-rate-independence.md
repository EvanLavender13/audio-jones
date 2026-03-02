# Frame-Rate Independence

Systematic audit and fix pass to decouple all visual/simulation timing from the 60fps frame rate lock, enabling VR support at 90fps (or arbitrary target rates) without behavioral changes.

## Classification

- **Category**: General (architecture / timing infrastructure)
- **Pipeline Position**: Cross-cutting — affects feedback loop, simulations, transform effects, analysis smoothing, and UI profiling
- **Prerequisite for**: VR Dome Mode

## References

- [Fix Your Timestep!](https://gafferongames.com/post/fix_your_timestep/) - Canonical article on frame-rate-independent game loops
- [Framerate Independence](https://www.construct.net/en/tutorials/framerate-independence-2) - Practical guide to dt-scaling patterns

## Current State

The codebase has a **split personality** on timing:

| System | Status | Pattern |
|--------|--------|---------|
| Feedback decay (blur, trails) | Correct | `exp(-0.693147 * dt / halfLife)` |
| Feedback rotation | Correct | `rotationSpeed * currentDeltaTime` |
| Feedback zoom/translate/stretch/desaturate/flow/warp | **Broken** | Per-frame, no dt scaling |
| LFO processing | Correct | Receives `deltaTime` |
| Particle Life force/position integration | Correct | Passes `deltaTime` as `timeStep` uniform |
| Particle Life momentum damping | **Broken** | `vel *= momentum` per frame |
| Audio capture / FFT | Correct | Audio-rate timing, decoupled from frame rate |
| Beat detection / band energy | Correct | Uses `audioHopTime` from sample rate |
| All other simulations (5 of 6) | **Broken** | Per-dispatch with no dt scaling |
| Transform effect time | **Broken** | `globalTick * 0.016f` hardcodes 1/60 |
| Simulation deposits | **Broken** | Fixed amount per dispatch, brightness scales with fps |
| Glitch effect timing | **Broken** | `frame` counter and `t * 60.0` seed hash |
| texBrightness attenuation | **Broken** | Per-frame multiplicative decay in feedback loop |
| Waveform history envelope | **Broken** | Fixed alpha per frame |
| deltaTime capping | **Missing** | No protection against large dt spikes |
| Profiler / UI budget display | **Broken** | Cosmetic but misleading |

## Issues by Severity

### HIGH — Visible behavioral change at non-60fps

#### H1. Transform animation time — hardcoded tick-to-seconds conversion

**File:** `src/render/render_pipeline.cpp:290`
```cpp
const float t = (float)globalTick * 0.016f;
```
`0.016f` ≈ 1/60. This value feeds `pe->transformTime`, which drives Lissajous animation and all transform effect `time` uniforms. `globalTick` increments at 20Hz (inside `UpdateVisuals`), so the effective rate is `20 * 0.016 = 0.32` real seconds per second — likely intentional for aesthetic speed, but the relationship is fragile and hardcoded.

**Fix:** Replace frame-tick conversion with accumulated real time. Either:
- Accumulate `transformTime += deltaTime` directly, or
- Pass real elapsed time from `GetTime()` instead of deriving from tick count

#### H2. Feedback zoom — per-frame multiplicative compounding

**File:** `src/render/shader_setup.cpp:36-38`
```cpp
float zoomEff = 1.0f + (ff->zoomBase - 1.0f) * ms;
```
**Shader:** `shaders/feedback.fs:83` — `uv *= zoom;`

`zoomBase` default is `0.995`. Applied multiplicatively per frame: at 60fps compounds to `0.995^60 ≈ 0.74` per second; at 90fps `0.995^90 ≈ 0.64` per second — 14% more zoom. `zoomRadial` (line 41-43) has the same issue as an additive per-frame offset.

**Fix:** Convert to rate-based: `zoomEff = powf(zoomBase, deltaTime * REFERENCE_FPS)` on CPU side.

#### H3. Feedback translation — per-frame drift without deltaTime

**File:** `src/render/shader_setup.cpp:54-65`
```cpp
float dxBaseEff = ff->dxBase * ms;
float dxRadialEff = ff->dxRadial * ms;
float dyBaseEff = ff->dyBase * ms;
float dyRadialEff = ff->dyRadial * ms;
```
**Shader:** `shaders/feedback.fs:108` — `uv += vec2(dx, dy);`

Translation applied per frame. At 90fps the feedback drifts 50% faster. Note: `rotationSpeed` IS correctly deltaTime-scaled (lines 46-47), making this inconsistency more visible.

**Fix:** Scale by `deltaTime / referenceDt` on CPU side, matching the pattern used for rotation.

#### H4. Feedback stretch — per-frame multiplicative without deltaTime

**File:** `src/render/shader_setup.cpp:85-90`
```cpp
float sxEff = 1.0f + (ff->sx - 1.0f) * ms;
float syEff = 1.0f + (ff->sy - 1.0f) * ms;
```
**Shader:** `shaders/feedback.fs:86-87` — `uv.x /= max(sx, 0.001); uv.y /= max(sy, 0.001);`

Same compounding issue as zoom. At 90fps the stretch distortion accumulates faster.

**Fix:** Same `powf(value, deltaTime * REFERENCE_FPS)` conversion as zoom.

#### H5. Feedback desaturate — per-frame color mix

**File:** `src/config/effect_config.h:280`
```cpp
float feedbackDesaturate = 0.05f; // Fade toward dark gray per frame (0.0-0.2)
```
**Shader:** `shaders/feedback.fs:151` — `finalColor.rgb = mix(finalColor.rgb, vec3(luma * 0.3), desaturate);`

Per-frame `mix()` in the feedback loop. Comment acknowledges "per frame". At 90fps desaturates 50% faster.

**Fix:** Scale to `1.0f - powf(1.0f - desaturate, deltaTime * REFERENCE_FPS)`.

#### H6. Feedback flow strength — per-frame displacement

**File:** `src/render/shader_setup.cpp:68-70`
```cpp
float flowStrengthEff = pe->effects.feedbackFlow.strength * ms;
```
**Shader:** `shaders/feedback.fs:136` — `uv += flowDir * gradMag * feedbackFlowStrength * texelSize;`

Luminance gradient flow displacement applied per frame. Twice the displacement per second at 120fps.

**Fix:** Scale by `deltaTime / referenceDt` on CPU side.

#### H7. Feedback warp — per-frame procedural displacement

**File:** `src/render/shader_setup.cpp:116-117`
```cpp
float warpEff = pe->effects.proceduralWarp.warp * ms;
```
**Shader:** `shaders/feedback.fs:98-101`
```glsl
uv.x += warp * 0.0035 * sin(warpTime * 0.333 + ...);
uv.y += warp * 0.0035 * cos(warpTime * 0.375 - ...);
```

`warpTime` accumulation IS deltaTime-based, but the displacement *amplitude* per frame is not scaled. Total warp distortion accumulates faster at higher fps.

**Fix:** Scale `warpEff` by `deltaTime / referenceDt` on CPU side.

#### H8. texBrightness — per-frame multiplicative attenuation in feedback loop

**File:** `src/config/drawable_config.h:71-72`
```cpp
float texBrightness = 0.9f; // 10% attenuation per frame prevents brightness accumulation
```
**Shader:** `shaders/shape_texture.fs:21` — `sampled.rgb *= texBrightness;`

Textured shapes sample from the accumulation buffer and draw back with this multiplier. At 60fps: `0.9^60 ≈ 0.002` per second of trail. At 90fps: `0.9^90 ≈ 0.00004`. Trail lifetime changes dramatically.

**Fix:** Convert to `powf(texBrightness, deltaTime * REFERENCE_FPS)` on CPU side.

#### H9. Physarum — movement and deposit per-dispatch

**File:** `shaders/physarum_agents.glsl:341, :450`
```glsl
pos += moveDir * agentStep;                          // line 341
vec3 newColor = current.rgb + depositColor * depositAmount;  // line 450
```
**CPU:** `src/simulation/physarum.cpp:243` — `stepSize` passed as-is, no dt scaling.

**Fix:** Scale `stepSize` and `depositAmount` by `deltaTime / referenceDt` on CPU before passing to shader.

#### H10. Boids — velocity and position update per-dispatch

**File:** `shaders/boids_agents.glsl:221`
```glsl
selfPos += selfVel;
```
Steering forces accumulated with fixed coefficients (lines 169-176), velocity clamped, then added directly to position. No dt scaling.

**CPU:** `src/simulation/boids.cpp:252-254` — `maxSpeed`, `minSpeed`, `depositAmount` passed as-is.

**Fix:** Same pattern — scale on CPU side by `deltaTime / referenceDt`.

#### H11. Curl Flow — step size and respawn per-dispatch

**File:** `shaders/curl_flow_agents.glsl:214`
```glsl
pos = wrapPosition(pos + smoothedDir * stepSize);
```
**CPU:** `src/simulation/curl_flow.cpp:276` — `stepSize` passed directly.

Additionally, `respawnProbability` (line 180-181) is a per-frame chance — at 90fps, 50% more agents respawn per second.

**Fix:** Scale `stepSize` by `deltaTime / referenceDt`. Scale `respawnProbability` by `deltaTime / referenceDt`.

#### H12. Attractor Flow — RK4 integration step count scales with fps

**File:** `shaders/attractor_agents.glsl:274`
```glsl
pos = rk4Step(pos, timeScale);
```
One RK4 step per dispatch. At 90fps → 50% more steps/second. `timeScale` is user-facing config (default 0.01), not multiplied by frame dt.

**Fix:** Multiply `timeScale` by `deltaTime / referenceDt` on CPU side.

#### H13. Curl Advection — entire field update ignores deltaTime

**File:** `src/simulation/curl_advection.cpp:188`
```cpp
void CurlAdvectionUpdate(CurlAdvection *ca, float /* deltaTime */, ...)
```
The `deltaTime` parameter is explicitly discarded. All field operations (curl, divergence, laplacian, pressure, self-advection) run once per dispatch with fixed coefficients.

**Fix:** Restore `deltaTime` usage. Scale field update coefficients by `deltaTime / referenceDt`.

#### H14. Simulation deposit amounts — trail brightness scales with fps

**Files:**
- `shaders/physarum_agents.glsl:450`
- `shaders/boids_agents.glsl:239`
- `shaders/curl_flow_agents.glsl:230`
- `shaders/attractor_agents.glsl:307`

All add `depositColor * depositAmount` per dispatch. Since dispatch runs once per frame, total deposit per second ∝ frame rate. Trail decay IS frame-rate independent (half-life formula), so the steady-state brightness shifts.

**Fix:** Scale deposit amounts by `deltaTime / referenceDt` alongside the movement fixes (H9-H12).

#### H15. Particle Life momentum — per-frame velocity damping

**File:** `shaders/particle_life_agents.glsl:150`
```glsl
vel *= momentum;
```
**Config:** `src/simulation/particle_life.h:29` — `float momentum = 0.8f; // Velocity retention per frame (0-1)`

Force integration and position update ARE correctly deltaTime-scaled (`vel += totalForce * timeStep; pos += vel * timeStep;`), but the damping applied before them is per-frame. At 60fps: `0.8^60 ≈ 1.5e-6` per second. At 90fps: `0.8^90 ≈ 1.2e-9`. Particle behavior changes dramatically.

**Fix:** Pass `deltaTime` to shader, use `pow(momentum, deltaTime * REFERENCE_FPS)`.

#### H16. Glitch frame counter — per-frame timing

**File:** `shaders/glitch.fs`
- Line 178, 186: `t * 60.0` hardcoded in row/column slice seed — assumes 60fps
- Line 162: `frame / datamoshSpeed` — datamosh resolution cycles per frame
- Line 261: `frame % (blockMaskMaxSize - ...)` — block mask cycles per frame
- Line 265: `frame / 6` — block pattern changes every 6 frames (100ms at 60fps, 67ms at 90fps)

**CPU:** `src/effects/glitch.cpp` — `e->frame++` increments per frame.

**Fix:** Replace `frame` uniform with a time-based approach. Convert `t * 60.0` to use a `referenceFps` uniform or precompute on CPU.

### MEDIUM — Noticeable but less critical

#### M1. Waveform history envelope — fixed alpha per frame

**File:** `src/analysis/analysis_pipeline.cpp:139-142`
```cpp
const float ALPHA = 0.1f;
pipeline->waveformEnvelope += ALPHA * (peakSigned - pipeline->waveformEnvelope);
```
Called every frame (comment on line 223 confirms: "60fps for smoother gradients"). At 90fps the envelope converges 50% faster.

**Fix:** Convert to frame-rate-independent EMA: `alpha = 1.0f - expf(-deltaTime / tau)` where `tau` is the desired time constant. Current `ALPHA = 0.1` at 60fps ≈ `tau = 0.15s`.

#### M2. No deltaTime capping — spiral-of-death vulnerability

**File:** `src/main.cpp:207-208`
```cpp
const float deltaTime = GetFrameTime();
ctx->updateAccumulator += deltaTime;
```
If the frame takes abnormally long (window minimized, breakpoint, GPU stall), `deltaTime` could be very large. All per-frame accumulations (rotation, LFO, modulation) get a huge spike. No `fminf(deltaTime, 0.1f)` guard.

**Fix:** Clamp deltaTime: `const float deltaTime = fminf(GetFrameTime(), 0.1f);`

#### M3. Profiler EMA smoothing — per-frame without dt correction

**File:** `src/render/profiler.h:9-10`, `src/render/profiler.cpp:59`
```cpp
#define PROFILER_SMOOTHING 0.05f
*smoothed = *smoothed + PROFILER_SMOOTHING * (ms - *smoothed);
```
Converges faster at higher fps. Cosmetic only.

**Fix:** Same EMA conversion as M1, or accept as cosmetic variance.

#### M4. DrawInterval — hardcoded 20Hz conversion

**File:** `src/render/drawable.cpp:133-134`
```cpp
const uint8_t interval = (uint8_t)(d->base.drawInterval * 20.0f + 0.5f);
```
Converts seconds to ticks assuming 20Hz. Duplicates the `TICK_RATE_HZ` constant from `ui_units.h`. If the 20Hz rate ever changes, this breaks silently.

**Fix:** Use `TICK_RATE_HZ` constant instead of hardcoded `20.0f`.

### LOW — Cosmetic or already gated

#### L1. Frame budget bar hardcodes 16.67ms

**File:** `src/ui/imgui_analysis.cpp:242-257`
```cpp
const float budgetMs = 16.67f;
```
Purely display. Would show wrong budget percentage at 90fps.

**Fix:** Replace with `1000.0f / targetFps`.

#### L2. FFT max magnitude decay — gated behind 20Hz timer

**File:** `src/main.cpp:150-151`
```cpp
const float decayFactor = 0.99f;
pe->fftMaxMagnitude = pe->fftMaxMagnitude * decayFactor;
```
Runs inside the 20Hz `updateAccumulator` gate, not per-frame. Stable regardless of fps.

**No fix needed** — already rate-limited.

#### L3. AVG_DECAY comment misleading

**File:** `src/analysis/smoothing.h:6-7`
```cpp
// Running average decay factor (0.999 = ~1 second time constant at 60Hz)
#define AVG_DECAY 0.999f
```
Comment says 60Hz but callers run at audio rate (per FFT hop), not frame rate.

**Fix:** Update comment to reflect actual call rate.

#### L4. 20Hz-gated smoothing — spectrum bars and waveform EMA

**Files:**
- `src/render/spectrum_bars.cpp:101-103` — smoothing factor `0.8` per 20Hz tick
- `src/render/drawable.cpp:68-71` — `waveformMotionScale` as raw EMA alpha per 20Hz tick

Both are currently stable because they run inside the 20Hz accumulator, not per-frame. Fragile if the accumulator is ever refactored.

**Fix:** Low priority. Could convert to time-based EMA, but the 20Hz gate makes this stable for now.

## Fix Strategy

### Approach: Reference-dt scaling

Rather than rewriting physics to use continuous-time integration (high risk of behavioral changes), use a **reference delta-time** approach:

```
REFERENCE_FPS = 60.0f
REFERENCE_DT  = 1.0f / 60.0f
dtScale = actualDeltaTime / REFERENCE_DT
```

**Three patterns cover all cases:**

| Pattern | Use for | Formula |
|---------|---------|---------|
| Linear scaling | Additive per-frame values (step sizes, deposits, translations, displacement) | `value * dtScale` |
| Exponential scaling | Multiplicative per-frame values (zoom, stretch, damping, brightness) | `powf(value, dtScale)` |
| EMA correction | Fixed-alpha smoothing | `1.0f - expf(-deltaTime / tau)` |

**Why not fixed timestep?** The simulations use GPU compute dispatches. Running multiple dispatches per frame to maintain a fixed sim rate would be complex and expensive. The reference-dt approach is simpler and sufficient for the 60→90fps jump.

**Why not full continuous-time?** The current parameters were tuned at 60fps. A pure `* deltaTime` conversion would require retuning every default value. Reference-dt scaling preserves existing defaults.

### Implementation Order

1. **Add constants** — `REFERENCE_FPS` and `REFERENCE_DT` to `src/config/constants.h`
2. **Add deltaTime clamp** — cap at 100ms in main loop (M2)
3. **Fix feedback system** (H2-H7) — largest surface area, highest user visibility. All fixes are CPU-side in `shader_setup.cpp`, no shader changes needed
4. **Fix texBrightness** (H8) — CPU-side scaling before passing to shader
5. **Fix transform time** (H1) — single file, high visibility, easy to verify
6. **Fix simulations** (H9-H14) — add `deltaTime` uniform to compute shaders, scale on CPU side
7. **Fix Particle Life momentum** (H15) — pass `deltaTime` to shader, use `pow()` for damping
8. **Fix glitch timing** (H16) — replace `frame` counter with time-based approach
9. **Fix EMA smoothing** (M1, M3) — convert to time-based alpha
10. **Fix cosmetic** (L1, L3, M4) — trivial label/comment updates
11. **Change `SetTargetFPS()`** — the final toggle, do last so all fixes are in place
12. **Verify** — A/B test at 60fps and 90fps, confirm identical visual behavior

### Scope Boundary

This pass does NOT address:
- The 20Hz visual update throttle (already frame-rate independent)
- Audio capture timing (already correct)
- VR stereo rendering or projection (separate feature)
- Display sync / vsync behavior (raylib/platform concern)

## Notes

- **Feedback is the biggest surprise** — only rotation uses `deltaTime`. Zoom, translation, stretch, desaturate, flow, and warp are all per-frame. This means the entire feedback look-and-feel changes at non-60fps rates.
- **Particle Life is *almost* the gold standard** — force integration is correct, but the momentum damping (`vel *= momentum`) is per-frame. One line away from being fully frame-rate independent.
- **Trail decay is already correct everywhere** — the `exp(-0.693147 * dt / halfLife)` formula is used consistently across all trail maps, feedback blur, attractor lines, muons, and fireworks.
- **The 20Hz update gate is a separate concern** — `TICK_RATE_HZ` controls how often drawables and FFT textures update. This is intentionally decoupled from frame rate and should remain so.
- **VR typically requires 90fps minimum** — the Oculus/Meta and SteamVR runtimes enforce this. Some headsets target 120fps. The reference-dt approach handles any target rate.
- **Testing at lower fps matters too** — if the app drops frames under load, reference-dt scaling prevents the "slow motion" effect where simulations crawl during frame drops.
- **Total scope: ~16 distinct fixes** across feedback setup, 5 simulation compute shaders, transform time, glitch timing, analysis smoothing, and UI cosmetics. Most fixes are CPU-side parameter scaling — only Particle Life and glitch require shader changes.
