---
name: raylib
description: Reference raylib's API before implementing custom graphics, input, or audio solutions. Consult the cheatsheet to find existing functions that solve common problems.
---

# raylib Development

Check raylib's built-in functions before writing custom implementations.

## Cheatsheet First

**Reference**: https://www.raylib.com/cheatsheet/cheatsheet.html

Before implementing any rendering, input, or audio feature:
1. Identify the module (shapes, textures, text, models, shaders, audio)
2. Scan that module's functions in the cheatsheet
3. Only write custom code if no raylib function exists

Use `WebFetch` to retrieve and search the cheatsheet when uncertain.

## Module Categories

| Module | Covers | Common Functions |
|--------|--------|------------------|
| **rcore** | Window, input, timing | `GetFrameTime`, `IsKeyPressed`, `GetMousePosition` |
| **rshapes** | 2D primitives | `DrawLineEx`, `DrawRectangleRounded`, `DrawCircleSector` |
| **rtextures** | Images, textures | `DrawTexturePro`, `LoadRenderTexture`, `BeginTextureMode` |
| **rtext** | Font rendering | `DrawTextEx`, `MeasureTextEx` |
| **rmodels** | 3D geometry | `DrawMesh`, `LoadModel` |
| **raudio** | Sound, music | `LoadSound`, `PlaySound`, `SetSoundVolume` |
| **rlgl** | Low-level OpenGL | Use only when raylib lacks the feature |

## Before You Implement

Check these functions first for common tasks:

| Task | Check First |
|------|-------------|
| Thick lines | `DrawLineEx(start, end, thickness, color)` |
| Rounded rectangles | `DrawRectangleRounded`, `DrawRectangleRoundedLines` |
| Rotated/scaled sprites | `DrawTexturePro(texture, source, dest, origin, rotation, tint)` |
| Bezier curves | `DrawLineBezier`, `DrawLineBezierQuad`, `DrawLineBezierCubic` |
| Render to texture | `LoadRenderTexture`, `BeginTextureMode`, `EndTextureMode` |
| Screen capture | `TakeScreenshot`, `LoadImageFromScreen` |
| Shader uniforms | `GetShaderLocation`, `SetShaderValue` |
| Delta time | `GetFrameTime()` returns seconds since last frame |

## Naming Conventions

raylib uses consistent verb prefixes:

- `Load*` / `Unload*` — Resource lifecycle
- `Begin*` / `End*` — Scoped state (drawing, shader, texture mode)
- `Draw*` — Immediate rendering
- `Get*` / `Set*` — Property access
- `Is*` — Boolean queries (`IsKeyPressed`, `IsWindowResized`)
- `Check*` — Collision detection

## rlgl: Last Resort

`rlgl` exposes raw OpenGL. Use only when:
- raylib provides no built-in function
- Performance requires batched custom geometry
- Custom vertex attributes needed

For simple shapes, `rshapes` functions outperform hand-rolled rlgl.

## Verification

When implementing graphics features, verify:
- [ ] Searched cheatsheet for existing function
- [ ] Checked `rshapes` before `rlgl`
- [ ] Used `DrawTexturePro` for any sprite transforms
- [ ] Paired all `Begin*/End*` calls
