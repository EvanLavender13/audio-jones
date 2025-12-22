# Physarum Waveform Coupling

Research on Physarum interaction with the waveform visualization.

## Current Architecture

With `accumSenseBlend = 1.0`, agents sense `accumTexture` which contains:
- Fresh waveform geometry (drawn each frame)
- Blurred/decayed history from previous frames (`blur_v.fs` applies Gaussian blur + exponential decay)

The "temporal persistence" problem described earlier is **already solved** by the existing blur/decay pipeline on `accumTexture`. Waveform leaves trails that persist according to `halfLife`.

## What Actually Happens

```
1. Feedback pass: zoom/rotate accumTexture
2. Blur pass: Gaussian blur + exponential decay (halfLife)
3. Physarum update: agents sense accumTexture, deposit to trailMap
4. Waveform drawn: fresh geometry added to accumTexture
```

Agents sensing `accumTexture` see both:
- Fresh waveform (high intensity)
- Decayed waveform history (lower intensity, blurred)

## If It's Not Working Well

The issue isn't lack of persistence. Possible causes:

| Symptom | Likely Cause |
|---------|--------------|
| Agents ignore waveform | `accumSenseBlend` too low, or waveform intensity too dim relative to trails |
| Agents cluster on old positions | `halfLife` too long, decay trails dominate fresh waveform |
| Agents scatter randomly | `halfLife` too short, not enough gradient to follow |
| Agents form networks ignoring waveform | Trail self-attraction stronger than waveform attraction |

## Parameters to Tune

- `accumSenseBlend`: 0 = trails only, 1 = accumTexture only
- `halfLife` (blur pass): Controls how long waveform "echoes" persist
- `depositAmount`: Agent trail strength relative to waveform brightness
- `sensorDistance`: How far ahead agents look

## The TrailMap vs AccumTexture Question

Current design:
- Agents deposit to `trailMap` (separate texture)
- Agents sense blend of `trailMap` + `accumTexture`

Alternative (true injection):
- Agents deposit to `accumTexture` directly
- Single texture contains both waveform and agent trails
- Simpler, but couples Physarum lifecycle to post-effect pipeline

## Sources

- `src/render/post_effect.cpp:107-170` - feedback/blur pipeline
- `shaders/blur_v.fs` - decay implementation
- `shaders/physarum_agents.glsl:105-110` - sensing blend
