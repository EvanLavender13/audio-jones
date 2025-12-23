# Modulation System Architecture

Research notes for a MilkDrop-inspired parameter modulation system.

## Problem Statement

Current architecture has hardcoded effect sequencing and manual audio-reactive wiring. Each new audio-reactive feature requires code changes. No general "route X to Y" capability exists.

## MilkDrop's Approach

MilkDrop presets are mini-programs with three equation layers:

### 1. Per-Frame Equations
Execute once per frame. Animate parameters over time.
```
zoom = zoom + 0.1 * sin(time);
wave_r = 0.5 + 0.5 * bass;
```

### 2. Per-Vertex Equations
Execute at each mesh vertex. Enable spatial variation.
```
zoom = zoom + rad * 0.1;  // More zoom at edges (perspective)
rot = rot + ang * 0.05;   // Rotation varies by angle
```

### 3. Built-in Variables
| Category | Variables |
|----------|-----------|
| Audio | `bass`, `mid`, `treb` (instant), `bass_att`, `mid_att`, `treb_att` (smoothed) |
| Time | `time`, `fps`, `frame` |
| Spatial | `x`, `y`, `rad` (distance from center), `ang` (angle from center) |
| Screen | `aspectx`, `aspecty`, `pixelsx`, `pixelsy` |

### 4. Q Variables (q1-q32)
Bridge data between equation types. Computed in per-frame, read in per-vertex/shaders.

## Proposed AudioJones Architecture

### Modulation Sources
| Source | Type | Description |
|--------|------|-------------|
| `bass`, `mid`, `treb` | Audio | Frequency band energy (0.0-2.0, 1.0 = normal) |
| `bass_smooth`, etc. | Audio | Attenuated/smoothed variants |
| `beat` | Trigger | Pulse on beat detection (0→1 decay) |
| `beat_count` | Counter | Increments each beat |
| `lfo1`-`lfo4` | Time | Configurable LFO waveforms |
| `time` | Time | Seconds since start |
| `noise` | Random | Per-frame random value |
| `rad`, `ang` | Spatial | For per-vertex/shader use |

### Modulation Targets
Any numeric effect parameter:
- Feedback: `zoom`, `rotation`, `desaturate`
- Warp: `warpStrength`, `warpScale`
- Waveform: `radius`, `amplitude`, `thickness`, `spawnRate`
- Post: `blur`, `chromatic`, `kaleidoSegments`

### Routing Table
```
source      → target        → amount → curve
bass        → zoom          → 0.05   → linear
beat        → waveformSpawn → 1.0    → trigger
lfo1        → rotation      → 0.02   → sine
treb_smooth → chromatic     → 10.0   → linear
```

### Event System
| Event | Trigger | Action |
|-------|---------|--------|
| Beat | `beat > threshold` | Spawn waveform, pulse effect |
| Threshold | `bass > 1.5` | Trigger one-shot animation |
| Periodic | Every N frames | Spawn, color shift |

## Waveform Emitter Modes

Instead of one global waveform draw:

| Mode | Behavior |
|------|----------|
| Continuous | Draw every frame (current) |
| Beat-synced | Spawn on beat detection |
| Periodic | Spawn every N frames |
| Triggered | Spawn on threshold crossing |

Each spawn creates a "ring" that enters the feedback loop and travels.

## Implementation Phases

### Phase 1: Audio Variables
- Expose `bass`, `mid`, `treb` as first-class variables
- Add smoothed variants with configurable attack/decay
- Make available to existing effects

### Phase 2: Modulation Routing
- Define routing table data structure
- UI for adding source → target mappings
- Runtime evaluation each frame

### Phase 3: Per-Vertex Support
- Pass spatial variables (`rad`, `ang`) to shaders
- Enable per-vertex equations in feedback shader
- Perspective zoom as spatial modulation example

### Phase 4: Event System
- Beat trigger with configurable threshold
- One-shot animations
- Waveform spawn modes

## Key Insight

MilkDrop's power comes from **composability**: any audio feature can drive any visual parameter through simple math expressions. The current AudioJones architecture requires code changes for each new audio-reactive behavior. A routing system would make this configuration instead of code.

## Unique Generative Systems

AudioJones has generative systems that MilkDrop lacks:

### Physarum Simulation
- 100k+ autonomous agents with sensing, turning, depositing
- Species/hue-based behavior (agents prefer similar-colored trails)
- FFT frequency modulation drives agent repulsion
- Waveform → trailmap injection (waveforms feed simulation as "food")
- Bidirectional coupling with accumulation buffer

### Voronoi Overlay
- Animated Worley noise cells
- Edge detection creates organic cell boundaries
- Adds texture/structure to feedback content

### The Depth Problem

These systems create rich "color transportation" - dynamic animated backgrounds. But they render **flat**. The original tunnel effect question stemmed from wanting these generative layers to feel immersive, like traveling through them.

Potential approaches:
- **Per-vertex perspective** in feedback shader affects physarum trails too
- **Layered rendering** - physarum at multiple depth scales
- **Agent depth parameter** - agents spawn at varying "distances", render with perspective
- **3D physarum** - agents in 3D space, projected to 2D with depth

A modulation system would enhance these systems:
```
bass        → depositAmount   (more trail on bass)
beat        → stepSize        (agents rush on beats)
treb        → sensorAngle     (behavior shifts with highs)
rad         → agentSpeed      (spatial variation)
```

Currently these are partially hardcoded (`stepBeatModulation`, `frequencyModulation`). Routing would make them discoverable.

## Sources

- [MilkDrop Preset Authoring Guide](https://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html)
- [MilkDrop 3 GitHub](https://github.com/milkdrop2077/MilkDrop3)
