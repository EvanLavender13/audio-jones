# Effect Modules Migration - Batch 1 (Warp Effects)

Migrate 12 warp effects from scattered PostEffect fields into self-contained modules at `src/effects/`. Each effect becomes a single header containing config struct, runtime state, and lifecycle functions, plus a cpp file with the implementation.

**Effects in this batch**: texture_warp, wave_ripple, mobius, gradient_flow, chladni_warp, domain_warp, surface_warp, interference_warp, corridor_warp, fft_radial_warp, circuit_board, radial_pulse

<!-- Intentional deviation: radial_pulse is listed under Batch 2 (Symmetry) in research doc, but its setup function lives in shader_setup_warp.cpp and it's a polar UV displacement effect, so it fits better with warp effects. -->

---

## Specification

### Module Pattern

Each effect module follows the same pattern established by sine_warp (Batch 0):

```cpp
// src/effects/<name>.h
#ifndef <NAME>_H
#define <NAME>_H

#include "raylib.h"
#include <stdbool.h>

// Config struct (user-facing parameters, serialized in presets)
struct <Name>Config { ... };

// Runtime state (shader + uniforms + animation accumulators)
typedef struct <Name>Effect {
  Shader shader;
  int <uniform>Loc;  // One per shader uniform
  float <state>;     // Animation accumulators
} <Name>Effect;

// Lifecycle functions
bool <Name>EffectInit(<Name>Effect* e);
void <Name>EffectSetup(<Name>Effect* e, const <Name>Config* cfg, float deltaTime);
void <Name>EffectUninit(<Name>Effect* e);
<Name>Config <Name>ConfigDefault(void);

#endif
```

---

### Effect Specifications

#### 1. TextureWarp

**Config** (from `src/config/texture_warp_config.h`):
```cpp
enum class TextureWarpChannelMode {
  RG = 0, RB = 1, GB = 2, Luminance = 3, LuminanceSplit = 4, Chrominance = 5, Polar = 6
};

struct TextureWarpConfig {
  bool enabled = false;
  float strength = 0.05f;
  int iterations = 3;
  TextureWarpChannelMode channelMode = TextureWarpChannelMode::RG;
  float ridgeAngle = 0.0f;
  float anisotropy = 0.0f;
  float noiseAmount = 0.0f;
  float noiseScale = 5.0f;
};
```

**Runtime**:
```cpp
typedef struct TextureWarpEffect {
  Shader shader;
  int strengthLoc;
  int iterationsLoc;
  int channelModeLoc;
  int ridgeAngleLoc;
  int anisotropyLoc;
  int noiseAmountLoc;
  int noiseScaleLoc;
} TextureWarpEffect;
```

**Setup logic**: No time accumulation. Direct uniform mapping from config.

---

#### 2. WaveRipple

**Config** (from `src/config/wave_ripple_config.h`):
```cpp
#include "config/dual_lissajous_config.h"

struct WaveRippleConfig {
  bool enabled = false;
  int octaves = 2;
  float strength = 0.02f;
  float speed = 1.0f;  // RENAMED from nothing - was using pe->waveRippleTime accumulation
  float frequency = 8.0f;
  float steepness = 0.0f;
  float decay = 5.0f;
  float centerHole = 0.0f;
  float originX = 0.5f;
  float originY = 0.5f;
  DualLissajousConfig originLissajous;
  bool shadeEnabled = false;
  float shadeIntensity = 0.2f;
};
```

**Runtime**:
```cpp
typedef struct WaveRippleEffect {
  Shader shader;
  int timeLoc;
  int octavesLoc;
  int strengthLoc;
  int frequencyLoc;
  int steepnessLoc;
  int decayLoc;
  int centerHoleLoc;
  int originLoc;
  int shadeEnabledLoc;
  int shadeIntensityLoc;
  float time;  // Animation accumulator
} WaveRippleEffect;
```

**Setup logic**:
- Accumulate `time += cfg->speed * deltaTime`
- Compute origin via `DualLissajousUpdate` if `originLissajous.amplitude > 0`, else use static `originX/Y`
- Set all uniforms

---

#### 3. Mobius

**Config** (from `src/config/mobius_config.h`):
```cpp
#include "config/dual_lissajous_config.h"

struct MobiusConfig {
  bool enabled = false;
  float point1X = 0.3f;
  float point1Y = 0.5f;
  float point2X = 0.7f;
  float point2Y = 0.5f;
  float spiralTightness = 0.0f;
  float zoomFactor = 0.0f;
  float speed = 1.0f;
  DualLissajousConfig point1Lissajous;
  DualLissajousConfig point2Lissajous;
};
```

**Runtime**:
```cpp
typedef struct MobiusEffect {
  Shader shader;
  int timeLoc;
  int point1Loc;
  int point2Loc;
  int spiralTightnessLoc;
  int zoomFactorLoc;
  float time;
  float currentPoint1[2];
  float currentPoint2[2];
} MobiusEffect;
```

**Setup logic**:
- Accumulate `time += cfg->speed * deltaTime`
- Compute points via `DualLissajousUpdate` for each point
- Set all uniforms

---

#### 4. GradientFlow

**Config** (from `src/config/gradient_flow_config.h`):
```cpp
struct GradientFlowConfig {
  bool enabled = false;
  float strength = 0.01f;
  int iterations = 8;
  float edgeWeight = 1.0f;
  bool randomDirection = false;
};
```

**Runtime**:
```cpp
typedef struct GradientFlowEffect {
  Shader shader;
  int resolutionLoc;
  int strengthLoc;
  int iterationsLoc;
  int edgeWeightLoc;
  int randomDirectionLoc;
} GradientFlowEffect;
```

**Setup logic**: No time accumulation. Direct uniform mapping.

---

#### 5. ChladniWarp

**Config** (from `src/config/chladni_warp_config.h`):
```cpp
struct ChladniWarpConfig {
  bool enabled = false;
  float n = 4.0f;
  float m = 3.0f;
  float plateSize = 1.0f;
  float strength = 0.1f;
  int warpMode = 0;
  float animRate = 0.0f;  // Will be RENAMED to speed
  float animRange = 0.0f;
  bool preFold = false;
};
```

**Field rename**: `animRate` -> `speed` for consistency with other effects.

**Runtime**:
```cpp
typedef struct ChladniWarpEffect {
  Shader shader;
  int nLoc;
  int mLoc;
  int plateSizeLoc;
  int strengthLoc;
  int modeLoc;
  int animPhaseLoc;
  int animRangeLoc;
  int preFoldLoc;
  float phase;  // Animation accumulator
} ChladniWarpEffect;
```

**Setup logic**:
- Accumulate `phase += cfg->speed * deltaTime`
- Set all uniforms

---

#### 6. DomainWarp

**Config** (from `src/config/domain_warp_config.h`):
```cpp
struct DomainWarpConfig {
  bool enabled = false;
  float warpStrength = 0.1f;
  float warpScale = 4.0f;
  int warpIterations = 2;
  float falloff = 0.5f;
  float driftSpeed = 0.0f;
  float driftAngle = 0.0f;
};
```

**Runtime**:
```cpp
typedef struct DomainWarpEffect {
  Shader shader;
  int warpStrengthLoc;
  int warpScaleLoc;
  int warpIterationsLoc;
  int falloffLoc;
  int timeOffsetLoc;
  float drift;  // Accumulated drift distance
} DomainWarpEffect;
```

**Setup logic**:
- Accumulate `drift += cfg->driftSpeed * deltaTime`
- Compute `timeOffset = {cos(driftAngle) * drift, sin(driftAngle) * drift}`
- Set all uniforms

---

#### 7. SurfaceWarp

**Config** (from `src/config/surface_warp_config.h`):
```cpp
struct SurfaceWarpConfig {
  bool enabled = false;
  float intensity = 0.5f;
  float angle = 0.0f;
  float rotationSpeed = 0.0f;
  float scrollSpeed = 0.5f;
  float depthShade = 0.3f;
};
```

**Runtime**:
```cpp
typedef struct SurfaceWarpEffect {
  Shader shader;
  int intensityLoc;
  int angleLoc;
  int rotationLoc;
  int scrollOffsetLoc;
  int depthShadeLoc;
  float rotation;      // Accumulated rotation
  float scrollOffset;  // Accumulated scroll
} SurfaceWarpEffect;
```

**Setup logic**:
- Accumulate `rotation += cfg->rotationSpeed * deltaTime`
- Accumulate `scrollOffset += cfg->scrollSpeed * deltaTime`
- Set all uniforms

---

#### 8. InterferenceWarp

**Config** (from `src/config/interference_warp_config.h`):
```cpp
struct InterferenceWarpConfig {
  bool enabled = false;
  float amplitude = 0.1f;
  float scale = 2.0f;
  int axes = 3;
  float axisRotationSpeed = 0.0f;
  int harmonics = 64;
  float decay = 1.0f;
  float speed = 0.0003f;
  float drift = 2.0f;
};
```

**Runtime**:
```cpp
typedef struct InterferenceWarpEffect {
  Shader shader;
  int timeLoc;
  int amplitudeLoc;
  int scaleLoc;
  int axesLoc;
  int axisRotationLoc;
  int harmonicsLoc;
  int decayLoc;
  int driftLoc;
  float time;          // Animation accumulator
  float axisRotation;  // Rotation accumulator
} InterferenceWarpEffect;
```

**Setup logic**:
- Accumulate `time += cfg->speed * deltaTime`
- Accumulate `axisRotation += cfg->axisRotationSpeed * deltaTime`
- Set all uniforms

---

#### 9. CorridorWarp

**Config** (from `src/config/corridor_warp_config.h`):
```cpp
enum CorridorWarpMode {
  CORRIDOR_WARP_FLOOR = 0,
  CORRIDOR_WARP_CEILING = 1,
  CORRIDOR_WARP_CORRIDOR = 2
};

struct CorridorWarpConfig {
  bool enabled = false;
  float horizon = 0.5f;
  float perspectiveStrength = 1.0f;
  int mode = CORRIDOR_WARP_CORRIDOR;
  float viewRotationSpeed = 0.0f;
  float planeRotationSpeed = 0.0f;
  float scale = 2.0f;
  float scrollSpeed = 0.5f;
  float strafeSpeed = 0.0f;
  float fogStrength = 1.0f;
};
```

**Runtime**:
```cpp
typedef struct CorridorWarpEffect {
  Shader shader;
  int resolutionLoc;
  int horizonLoc;
  int perspectiveStrengthLoc;
  int modeLoc;
  int viewRotationLoc;
  int planeRotationLoc;
  int scaleLoc;
  int scrollOffsetLoc;
  int strafeOffsetLoc;
  int fogStrengthLoc;
  float viewRotation;
  float planeRotation;
  float scrollOffset;
  float strafeOffset;
} CorridorWarpEffect;
```

**Setup logic**:
- Accumulate 4 state values (view rotation, plane rotation, scroll, strafe)
- Set all uniforms including resolution

---

#### 10. FftRadialWarp

**Config** (from `src/config/fft_radial_warp_config.h`):
```cpp
struct FftRadialWarpConfig {
  bool enabled = false;
  float intensity = 0.1f;
  float freqStart = 0.0f;
  float freqEnd = 0.5f;
  float maxRadius = 0.7f;
  float freqCurve = 1.0f;
  float bassBoost = 0.0f;
  int segments = 4;
  float pushPullBalance = 0.5f;
  float pushPullSmoothness = 0.0f;
  float phaseSpeed = 0.0f;
};
```

**Runtime**:
```cpp
typedef struct FftRadialWarpEffect {
  Shader shader;
  int resolutionLoc;
  int fftTextureLoc;
  int intensityLoc;
  int freqStartLoc;
  int freqEndLoc;
  int maxRadiusLoc;
  int freqCurveLoc;
  int bassBoostLoc;
  int segmentsLoc;
  int pushPullBalanceLoc;
  int pushPullSmoothnessLoc;
  int phaseOffsetLoc;
  float phaseAccum;  // Auto-rotation accumulator
} FftRadialWarpEffect;
```

**Setup logic**:
- Accumulate `phaseAccum += cfg->phaseSpeed * deltaTime`
- Compute `phaseOffset = fmod(phaseAccum, TWO_PI)`
- Bind FFT texture (passed as parameter or accessed via PostEffect)
- Set all uniforms

**Note**: This effect requires access to the FFT texture. The Setup function needs an additional `Texture2D fftTexture` parameter:
```cpp
void FftRadialWarpEffectSetup(FftRadialWarpEffect* e, const FftRadialWarpConfig* cfg,
                               float deltaTime, Texture2D fftTexture);
```

---

#### 11. CircuitBoard

**Config** (from `src/config/circuit_board_config.h`):
```cpp
struct CircuitBoardConfig {
  bool enabled = false;
  float patternX = 7.0f;
  float patternY = 5.0f;
  int iterations = 6;
  float scale = 1.4f;
  float offset = 0.16f;
  float scaleDecay = 1.05f;
  float strength = 0.5f;
  float scrollSpeed = 0.0f;
  float scrollAngle = 0.0f;
  bool chromatic = false;
};
```

**Runtime**:
```cpp
typedef struct CircuitBoardEffect {
  Shader shader;
  int patternConstLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;
  int scaleDecayLoc;
  int strengthLoc;
  int scrollOffsetLoc;
  int rotationAngleLoc;
  int chromaticLoc;
  float scrollOffset;  // Accumulated scroll
} CircuitBoardEffect;
```

**Setup logic**:
- Accumulate `scrollOffset += cfg->scrollSpeed * deltaTime`
- Set all uniforms

---

#### 12. RadialPulse

**Config** (from `src/config/radial_pulse_config.h`):
```cpp
struct RadialPulseConfig {
  bool enabled = false;
  float radialFreq = 8.0f;
  float radialAmp = 0.05f;
  int segments = 6;
  float angularAmp = 0.1f;
  float petalAmp = 0.0f;
  float phaseSpeed = 1.0f;
  float spiralTwist = 0.0f;
  int octaves = 1;
  float octaveRotation = 0.0f;
  bool depthBlend = false;
};
```

**Runtime**:
```cpp
typedef struct RadialPulseEffect {
  Shader shader;
  int radialFreqLoc;
  int radialAmpLoc;
  int segmentsLoc;
  int angularAmpLoc;
  int petalAmpLoc;
  int phaseLoc;
  int spiralTwistLoc;
  int octavesLoc;
  int octaveRotationLoc;
  int depthBlendLoc;
  float time;  // Animation accumulator
} RadialPulseEffect;
```

**Setup logic**:
- Accumulate `time += cfg->phaseSpeed * deltaTime`
- Set all uniforms

---

## Tasks

### Wave 1: Create Effect Modules

All 12 effect modules can be created in parallel since they have no file dependencies on each other.

#### Task 1.1: Create texture_warp module

**Files**: `src/effects/texture_warp.h`, `src/effects/texture_warp.cpp`

**Creates**: `TextureWarpConfig`, `TextureWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Move `TextureWarpChannelMode` enum from config header
3. Define `TextureWarpConfig` struct per spec
4. Define `TextureWarpEffect` struct per spec
5. Declare lifecycle functions
6. Implement in cpp:
   - `TextureWarpEffectInit`: Load `"shaders/texture_warp.fs"`, cache 7 uniform locs
   - `TextureWarpEffectSetup`: Direct uniform mapping, cast channelMode to int
   - `TextureWarpEffectUninit`: Unload shader
   - `TextureWarpConfigDefault`: Return defaults

**Verify**: Files compile standalone with raylib headers.

---

#### Task 1.2: Create wave_ripple module

**Files**: `src/effects/wave_ripple.h`, `src/effects/wave_ripple.cpp`

**Creates**: `WaveRippleConfig`, `WaveRippleEffect`, lifecycle functions

**Build**:
1. Create header with include guards
2. `#include "raylib.h"`, `#include <stdbool.h>`, `#include "config/dual_lissajous_config.h"`
3. Define `WaveRippleConfig` struct per spec (keep `DualLissajousConfig originLissajous` field)
4. Define `WaveRippleEffect` struct per spec
5. Declare lifecycle functions
6. Implement in cpp:
   - Init: Load shader, cache 10 uniform locs, init `time = 0.0f`
   - Setup: Accumulate time, compute origin via Lissajous if amplitude > 0, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.3: Create mobius module

**Files**: `src/effects/mobius.h`, `src/effects/mobius.cpp`

**Creates**: `MobiusConfig`, `MobiusEffect`, lifecycle functions

**Build**:
1. Create header with include guards
2. `#include "raylib.h"`, `#include <stdbool.h>`, `#include "config/dual_lissajous_config.h"`
3. Define `MobiusConfig` struct per spec
4. Define `MobiusEffect` struct per spec (include `currentPoint1[2]` and `currentPoint2[2]`)
5. Declare lifecycle functions
6. Implement in cpp:
   - Init: Load shader, cache 5 uniform locs, init time and point arrays
   - Setup: Accumulate time, compute points via Lissajous, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.4: Create gradient_flow module

**Files**: `src/effects/gradient_flow.h`, `src/effects/gradient_flow.cpp`

**Creates**: `GradientFlowConfig`, `GradientFlowEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `GradientFlowConfig` struct per spec
3. Define `GradientFlowEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 5 uniform locs (including resolution)
   - Setup: Direct uniform mapping, convert bool to int
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.5: Create chladni_warp module

**Files**: `src/effects/chladni_warp.h`, `src/effects/chladni_warp.cpp`

**Creates**: `ChladniWarpConfig`, `ChladniWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `ChladniWarpConfig` struct per spec (**rename `animRate` to `speed`**)
3. Define `ChladniWarpEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 8 uniform locs, init `phase = 0.0f`
   - Setup: Accumulate `phase += cfg->speed * deltaTime`, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults (speed=0.0f)

**Verify**: Files compile.

---

#### Task 1.6: Create domain_warp module

**Files**: `src/effects/domain_warp.h`, `src/effects/domain_warp.cpp`

**Creates**: `DomainWarpConfig`, `DomainWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `DomainWarpConfig` struct per spec
3. Define `DomainWarpEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 5 uniform locs, init `drift = 0.0f`
   - Setup: Accumulate drift, compute timeOffset vec2 using cos/sin of driftAngle, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.7: Create surface_warp module

**Files**: `src/effects/surface_warp.h`, `src/effects/surface_warp.cpp`

**Creates**: `SurfaceWarpConfig`, `SurfaceWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `SurfaceWarpConfig` struct per spec
3. Define `SurfaceWarpEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 5 uniform locs, init `rotation = 0.0f`, `scrollOffset = 0.0f`
   - Setup: Accumulate both rotation and scrollOffset, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.8: Create interference_warp module

**Files**: `src/effects/interference_warp.h`, `src/effects/interference_warp.cpp`

**Creates**: `InterferenceWarpConfig`, `InterferenceWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `InterferenceWarpConfig` struct per spec
3. Define `InterferenceWarpEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 9 uniform locs, init `time = 0.0f`, `axisRotation = 0.0f`
   - Setup: Accumulate time and axisRotation, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.9: Create corridor_warp module

**Files**: `src/effects/corridor_warp.h`, `src/effects/corridor_warp.cpp`

**Creates**: `CorridorWarpConfig`, `CorridorWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Move `CorridorWarpMode` enum from config header
3. Define `CorridorWarpConfig` struct per spec
4. Define `CorridorWarpEffect` struct per spec
5. Declare lifecycle functions (Setup needs screenWidth, screenHeight params)
6. Implement in cpp:
   - Init: Load shader, cache 10 uniform locs, init 4 accumulators to 0
   - Setup: Accumulate 4 values, set resolution and all uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Signature**: `void CorridorWarpEffectSetup(..., int screenWidth, int screenHeight)`

**Verify**: Files compile.

---

#### Task 1.10: Create fft_radial_warp module

**Files**: `src/effects/fft_radial_warp.h`, `src/effects/fft_radial_warp.cpp`

**Creates**: `FftRadialWarpConfig`, `FftRadialWarpEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `FftRadialWarpConfig` struct per spec
3. Define `FftRadialWarpEffect` struct per spec
4. Declare lifecycle functions (Setup needs fftTexture param)
5. Implement in cpp:
   - Init: Load shader, cache 12 uniform locs, init `phaseAccum = 0.0f`
   - Setup: Accumulate phase, compute phaseOffset with fmod, bind fftTexture, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Signature**: `void FftRadialWarpEffectSetup(..., Texture2D fftTexture)`

**Verify**: Files compile.

---

#### Task 1.11: Create circuit_board module

**Files**: `src/effects/circuit_board.h`, `src/effects/circuit_board.cpp`

**Creates**: `CircuitBoardConfig`, `CircuitBoardEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `CircuitBoardConfig` struct per spec
3. Define `CircuitBoardEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 9 uniform locs, init `scrollOffset = 0.0f`
   - Setup: Accumulate scrollOffset, pack patternX/Y into vec2, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

#### Task 1.12: Create radial_pulse module

**Files**: `src/effects/radial_pulse.h`, `src/effects/radial_pulse.cpp`

**Creates**: `RadialPulseConfig`, `RadialPulseEffect`, lifecycle functions

**Build**:
1. Create header with include guards, `#include "raylib.h"`, `#include <stdbool.h>`
2. Define `RadialPulseConfig` struct per spec
3. Define `RadialPulseEffect` struct per spec
4. Declare lifecycle functions
5. Implement in cpp:
   - Init: Load shader, cache 10 uniform locs, init `time = 0.0f`
   - Setup: Accumulate `time += cfg->phaseSpeed * deltaTime`, convert bools to ints, set uniforms
   - Uninit: Unload shader
   - Default: Return defaults

**Verify**: Files compile.

---

### Wave 2: Integration Updates

All integration tasks can run in parallel since they touch different files.

#### Task 2.1: Update CMakeLists.txt

**Files**: `CMakeLists.txt`

**Depends on**: Wave 1 complete

**Build**:
1. Add 12 new cpp files to `EFFECTS_SOURCES`:
   ```cmake
   set(EFFECTS_SOURCES
       src/effects/sine_warp.cpp
       src/effects/texture_warp.cpp
       src/effects/wave_ripple.cpp
       src/effects/mobius.cpp
       src/effects/gradient_flow.cpp
       src/effects/chladni_warp.cpp
       src/effects/domain_warp.cpp
       src/effects/surface_warp.cpp
       src/effects/interference_warp.cpp
       src/effects/corridor_warp.cpp
       src/effects/fft_radial_warp.cpp
       src/effects/circuit_board.cpp
       src/effects/radial_pulse.cpp
   )
   ```

**Verify**: `cmake.exe -G Ninja -B build -S .` succeeds.

---

#### Task 2.2: Update effect_config.h

**Files**: `src/config/effect_config.h`

**Depends on**: Wave 1 complete

**Build**:
1. Remove includes for migrated config headers:
   - `#include "texture_warp_config.h"`
   - `#include "wave_ripple_config.h"`
   - `#include "mobius_config.h"`
   - `#include "gradient_flow_config.h"`
   - `#include "chladni_warp_config.h"`
   - `#include "domain_warp_config.h"`
   - `#include "surface_warp_config.h"`
   - `#include "interference_warp_config.h"`
   - `#include "corridor_warp_config.h"`
   - `#include "fft_radial_warp_config.h"`
   - `#include "circuit_board_config.h"`
   - `#include "radial_pulse_config.h"`
2. Add includes for new effect modules:
   - `#include "effects/texture_warp.h"`
   - `#include "effects/wave_ripple.h"`
   - `#include "effects/mobius.h"`
   - `#include "effects/gradient_flow.h"`
   - `#include "effects/chladni_warp.h"`
   - `#include "effects/domain_warp.h"`
   - `#include "effects/surface_warp.h"`
   - `#include "effects/interference_warp.h"`
   - `#include "effects/corridor_warp.h"`
   - `#include "effects/fft_radial_warp.h"`
   - `#include "effects/circuit_board.h"`
   - `#include "effects/radial_pulse.h"`

**Verify**: Header parses correctly.

---

#### Task 2.3: Update post_effect.h

**Files**: `src/render/post_effect.h`

**Depends on**: Wave 1 complete

**Build**:
1. Add includes for all 12 effect modules after existing `#include "effects/sine_warp.h"`
2. Remove shader fields: `textureWarpShader`, `waveRippleShader`, `mobiusShader`, `gradientFlowShader`, `chladniWarpShader`, `domainWarpShader`, `surfaceWarpShader`, `interferenceWarpShader`, `corridorWarpShader`, `fftRadialWarpShader`, `circuitBoardShader`, `radialPulseShader`
3. Remove all uniform location fields for these 12 effects (~80 int fields)
4. Remove animation state fields: `waveRippleTime`, `mobiusTime`, `currentMobiusPoint1[2]`, `currentMobiusPoint2[2]`, `chladniWarpPhase`, `domainWarpDrift`, `surfaceWarpRotation`, `surfaceWarpScrollOffset`, `interferenceWarpTime`, `interferenceWarpAxisRotation`, `corridorWarpViewRotation`, `corridorWarpPlaneRotation`, `corridorWarpScrollOffset`, `corridorWarpStrafeOffset`, `fftRadialWarpPhaseAccum`, `circuitBoardScrollOffset`, `radialPulseTime`, `currentWaveRippleOrigin[2]`
5. Add effect module fields after `SineWarpEffect sineWarp`:
   ```cpp
   TextureWarpEffect textureWarp;
   WaveRippleEffect waveRipple;
   MobiusEffect mobius;
   GradientFlowEffect gradientFlow;
   ChladniWarpEffect chladniWarp;
   DomainWarpEffect domainWarp;
   SurfaceWarpEffect surfaceWarp;
   InterferenceWarpEffect interferenceWarp;
   CorridorWarpEffect corridorWarp;
   FftRadialWarpEffect fftRadialWarp;
   CircuitBoardEffect circuitBoard;
   RadialPulseEffect radialPulse;
   ```

**Verify**: Header parses correctly.

---

#### Task 2.4: Update post_effect.cpp

**Files**: `src/render/post_effect.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `LoadPostEffectShaders`:
   - Remove 12 `LoadShader` calls for migrated effects
   - Remove corresponding entries from the return condition
2. In `GetShaderUniformLocations`:
   - Remove all `GetShaderLocation` calls for the 12 migrated effects (~80 lines)
3. In `PostEffectInit`:
   - After `SineWarpEffectInit`, add Init calls for all 12 effects:
     ```cpp
     if (!TextureWarpEffectInit(&pe->textureWarp)) { /* error */ }
     if (!WaveRippleEffectInit(&pe->waveRipple)) { /* error */ }
     // ... etc for all 12
     ```
4. Remove state initializations for migrated effects (time accumulators, etc.)
5. In `PostEffectUninit`:
   - Remove `UnloadShader` calls for migrated effects
   - Add Uninit calls for all 12 effects

**Verify**: Compiles.

---

#### Task 2.5: Update shader_setup.cpp

**Files**: `src/render/shader_setup.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `GetTransformEffect`, update shader pointer for each migrated effect:
   - `TRANSFORM_TEXTURE_WARP`: `&pe->textureWarp.shader`
   - `TRANSFORM_WAVE_RIPPLE`: `&pe->waveRipple.shader`
   - `TRANSFORM_MOBIUS`: `&pe->mobius.shader`
   - `TRANSFORM_GRADIENT_FLOW`: `&pe->gradientFlow.shader`
   - `TRANSFORM_CHLADNI_WARP`: `&pe->chladniWarp.shader`
   - `TRANSFORM_DOMAIN_WARP`: `&pe->domainWarp.shader`
   - `TRANSFORM_SURFACE_WARP`: `&pe->surfaceWarp.shader`
   - `TRANSFORM_INTERFERENCE_WARP`: `&pe->interferenceWarp.shader`
   - `TRANSFORM_CORRIDOR_WARP`: `&pe->corridorWarp.shader`
   - `TRANSFORM_FFT_RADIAL_WARP`: `&pe->fftRadialWarp.shader`
   - `TRANSFORM_CIRCUIT_BOARD`: `&pe->circuitBoard.shader`
   - `TRANSFORM_RADIAL_PULSE`: `&pe->radialPulse.shader`

**Verify**: Compiles.

---

#### Task 2.6: Update shader_setup_warp.cpp

**Files**: `src/render/shader_setup_warp.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Add includes for all 12 effect modules
2. Replace each Setup function body with a single call to the effect's Setup function:
   ```cpp
   void SetupTextureWarp(PostEffect *pe) {
     TextureWarpEffectSetup(&pe->textureWarp, &pe->effects.textureWarp);
   }
   void SetupWaveRipple(PostEffect *pe) {
     WaveRippleEffectSetup(&pe->waveRipple, &pe->effects.waveRipple, pe->currentDeltaTime);
   }
   void SetupMobius(PostEffect *pe) {
     MobiusEffectSetup(&pe->mobius, &pe->effects.mobius, pe->currentDeltaTime);
   }
   void SetupGradientFlow(PostEffect *pe) {
     GradientFlowEffectSetup(&pe->gradientFlow, &pe->effects.gradientFlow);
   }
   void SetupChladniWarp(PostEffect *pe) {
     ChladniWarpEffectSetup(&pe->chladniWarp, &pe->effects.chladniWarp, pe->currentDeltaTime);
   }
   void SetupDomainWarp(PostEffect *pe) {
     DomainWarpEffectSetup(&pe->domainWarp, &pe->effects.domainWarp, pe->currentDeltaTime);
   }
   void SetupSurfaceWarp(PostEffect *pe) {
     SurfaceWarpEffectSetup(&pe->surfaceWarp, &pe->effects.surfaceWarp, pe->currentDeltaTime);
   }
   void SetupInterferenceWarp(PostEffect *pe) {
     InterferenceWarpEffectSetup(&pe->interferenceWarp, &pe->effects.interferenceWarp, pe->currentDeltaTime);
   }
   void SetupCorridorWarp(PostEffect *pe) {
     CorridorWarpEffectSetup(&pe->corridorWarp, &pe->effects.corridorWarp, pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
   }
   void SetupFftRadialWarp(PostEffect *pe) {
     FftRadialWarpEffectSetup(&pe->fftRadialWarp, &pe->effects.fftRadialWarp, pe->currentDeltaTime, pe->fftTexture);
   }
   void SetupCircuitBoard(PostEffect *pe) {
     CircuitBoardEffectSetup(&pe->circuitBoard, &pe->effects.circuitBoard, pe->currentDeltaTime);
   }
   void SetupRadialPulse(PostEffect *pe) {
     RadialPulseEffectSetup(&pe->radialPulse, &pe->effects.radialPulse, pe->currentDeltaTime);
   }
   ```

**Verify**: Compiles.

---

#### Task 2.7: Update preset.cpp

**Files**: `src/config/preset.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. Find NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE for ChladniWarpConfig
2. Change `animRate` to `speed` in the macro field list
3. No changes needed for other effects (field names unchanged)

**Verify**: Compiles.

---

#### Task 2.8: Update imgui_effects_warp.cpp

**Files**: `src/ui/imgui_effects_warp.cpp`

**Depends on**: Wave 1 complete

**Build**:
1. In `DrawWarpChladniWarp` function:
   - Change `&e->chladniWarp.animRate` to `&e->chladniWarp.speed`
   - Update slider label from `"Anim Rate##chladniWarp"` to `"Speed##chladniWarp"`

**Verify**: Compiles.

---

#### Task 2.9: Delete old config headers

**Files**: 12 config header files

**Depends on**: Task 2.2 complete

**Build**:
1. Delete the following files:
   - `src/config/texture_warp_config.h`
   - `src/config/wave_ripple_config.h`
   - `src/config/mobius_config.h`
   - `src/config/gradient_flow_config.h`
   - `src/config/chladni_warp_config.h`
   - `src/config/domain_warp_config.h`
   - `src/config/surface_warp_config.h`
   - `src/config/interference_warp_config.h`
   - `src/config/corridor_warp_config.h`
   - `src/config/fft_radial_warp_config.h`
   - `src/config/circuit_board_config.h`
   - `src/config/radial_pulse_config.h`

**Verify**: Files no longer exist.

---

#### Task 2.10: Migrate presets

**Files**: `presets/*.json`

**Depends on**: Wave 1 complete

**Build**:
1. Search all JSON files for `chladniWarp` containing `animRate`
2. For each match, rename `"animRate"` to `"speed"`

**Verify**: `grep -r "animRate" presets/` returns no matches for chladniWarp.

---

### Wave 3: Validation

#### Task 3.1: Build and runtime test

**Files**: None (validation only)

**Depends on**: All Wave 2 tasks complete

**Build**:
1. `cmake.exe --build build`
2. Launch application
3. Test each of the 12 migrated effects:
   - Enable effect
   - Verify animation (if applicable)
   - Verify sliders update visual
   - Disable effect
4. Load a preset that uses warp effects
5. Verify preset loads correctly

**Verify**: No crashes, effects render identically to before migration.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S .` succeeds
- [ ] `cmake.exe --build build` succeeds with no warnings in new files
- [ ] 12 config headers deleted from `src/config/`
- [ ] 12 effect modules exist in `src/effects/`
- [ ] All 12 warp effects render correctly at runtime
- [ ] Presets containing warp effects load correctly
- [ ] No presets contain `chladniWarp.animRate` (migrated to `speed`)
