# Agent Simulation Abstraction

Extract shared infrastructure from physarum and curl_flow into a reusable agent simulation framework.

## Current Duplication

Both `physarum.cpp` and `curl_flow.cpp` duplicate:
- `LoadShaderSource()` - shader file loading with error logging
- `RGBToHSV()` - color conversion for agent coloring
- `CreateTrailMap()` / `ClearTrailMap()` - RGBA32F render texture creation
- `ProcessTrails()` - two-pass separable diffusion with decay (uses `physarum_trail.glsl`)
- Init/Uninit/Resize/Reset lifecycle patterns
- Agent buffer allocation and reinitialization
- Config application with buffer reallocation on agent count change

## Abstraction Opportunity

Create `AgentSimulation` base that handles:
- Trail map management (ping-pong textures, diffusion, decay)
- Agent SSBO lifecycle (create, resize, reinitialize)
- Common uniforms (resolution, time, diffusion scale, decay factor)
- Debug overlay rendering

Each effect (physarum, curl_flow, future effects) provides only:
- Agent compute shader with steering logic
- Effect-specific config and uniforms
- Custom initialization per agent

## Scope

~300 lines duplicated across two files. Third agent-based effect would triple the maintenance burden without abstraction.
