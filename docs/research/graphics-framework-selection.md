---
archctl_doc_id: graphics-framework-selection
title: Graphics Framework Selection - SDL3 vs SFML vs raylib
tags: [decisions, research, graphics]
date: 2025-12-02
---

# Graphics Framework Selection

## Overview

Evaluation of SDL3, SFML 3.0, and raylib for recreating AudioThing's circular waveform visualizer. Primary constraint: simplest framework that supports custom fragment shaders and render-to-texture accumulation.

## Required Capabilities

From `audiothing-existing-impl.md`, the framework must provide:

| Capability | AudioThing Usage |
|------------|------------------|
| Custom fragment shader | `fade_blur.frag` for trail persistence |
| Render texture | Frame accumulation buffer |
| 2D vertex rendering | 10,240 point waveform curves |
| ImGui integration | Parameter controls |
| Window + input | Basic event loop |

## Framework Comparison

### raylib

**Shader workflow**: Direct GLSL. `LoadShader("vertex.vs", "fragment.fs")` returns usable `Shader` struct. No cross-compilation, no intermediate formats.

**Render texture**: `LoadRenderTexture(width, height)` returns `RenderTexture2D` with `.texture` and `.depth` attachments. Draw via `BeginTextureMode(target)`/`EndTextureMode()`.

**2D rendering**: `DrawLineStrip()` for waveforms, or custom vertex buffer via `rlgl` layer. Built-in primitives cover most cases.

**ImGui**: [rlImGui](https://github.com/raylib-extras/rlImGui) provides complete integration. Actively maintained, updated for ImGui 1.92+.

**Complexity**: Minimal. Single-header philosophy extends to shader/texture APIs. Designed explicitly for learning and prototyping.

```c
// Complete shader + render texture setup
RenderTexture2D target = LoadRenderTexture(800, 600);
Shader blur = LoadShader(0, "fade_blur.fs");

BeginTextureMode(target);
    BeginShaderMode(blur);
        DrawTexture(target.texture, 0, 0, WHITE);  // apply shader to previous frame
    EndShaderMode();
    DrawWaveform(points, count);  // draw new frame on top
EndTextureMode();
```

### SDL3

**Shader workflow**: New GPU API requires HLSL source → SDL_shadercross → backend-specific bytecode (SPIRV/DXIL/MSL). Three-stage compilation pipeline.

**Render texture**: GPU API provides framebuffer abstraction, but requires explicit pipeline/command buffer management. Basic `SDL_Renderer` has no shader support until 3.4.

**2D rendering**: GPU API uses explicit vertex buffers, pipeline state objects, render passes. More akin to Vulkan than immediate-mode 2D.

**ImGui**: Official [imgui_impl_sdlgpu3](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_sdlgpu3.cpp) backend. Well-supported.

**Complexity**: High. GPU API designed for AAA cross-platform graphics (Vulkan/Metal/D3D12 abstraction). Overkill for 2D visualizer.

```c
// SDL3 GPU shader loading (simplified)
SDL_GPUShader* shader = SDL_CreateGPUShader(device, &(SDL_GPUShaderCreateInfo){
    .code = compiled_spirv,
    .code_size = spirv_size,
    .entrypoint = "main",
    .format = SDL_GPU_SHADERFORMAT_SPIRV,
    .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
});
// Then: create pipeline, allocate command buffers, begin/end render passes...
```

### SFML 3.0

**Shader workflow**: Direct GLSL via `sf::Shader::loadFromFile()`. Same simplicity as raylib.

**Render texture**: `sf::RenderTexture` works identically to existing AudioThing code.

**2D rendering**: `sf::VertexArray` with `sf::PrimitiveType::LineStrip`. Existing code uses this.

**ImGui**: [imgui-sfml](https://github.com/SFML/imgui-sfml) integration exists but less actively maintained than rlImGui.

**Complexity**: Low. Existing AudioThing already uses SFML—zero learning curve.

**Critical limitation**: SFML 3.0 graphics module uses legacy OpenGL. Cannot support OpenGL 3.0+ core context or OpenGL ES 2.0. [SFML 4 will address this](https://duerrenberger.dev/blog/2025/01/15/sfml-3-release/), but no release date.

## Decision Matrix

| Criterion | raylib | SDL3 | SFML 3.0 |
|-----------|--------|------|----------|
| Shader simplicity | GLSL direct | HLSL→cross-compile | GLSL direct |
| Render texture | `LoadRenderTexture()` | GPU API pipelines | `sf::RenderTexture` |
| API complexity | Minimal | High | Low |
| Learning curve | Low | High | None (existing) |
| ImGui support | rlImGui (active) | Official (active) | imgui-sfml (dated) |
| OpenGL version | Modern (3.3+) | Vulkan/Metal/D3D12 | Legacy only |
| Migration effort | Medium | High | Zero |

## Decision

**raylib**

Reasoning:

1. **Shader workflow matches need**: GLSL fragment shaders load directly. The existing `fade_blur.frag` ports with minimal changes (adjust uniform names to raylib conventions).

2. **RenderTexture is trivial**: `LoadRenderTexture()` + `BeginTextureMode()` mirrors SFML's abstraction without legacy OpenGL baggage.

3. **SDL3 is overkill**: The GPU API solves AAA cross-platform shader compilation (Vulkan/Metal/D3D12). A 2D visualizer doesn't need that abstraction layer—it adds complexity without benefit.

4. **SFML has a ceiling**: Legacy OpenGL means no path forward for advanced effects. Betting on SFML 4 timeline is risky.

5. **ImGui integration is solid**: rlImGui actively maintained, updated for latest ImGui versions, includes Font Awesome support.

6. **Explicit simplicity goal**: raylib's design philosophy is "learn videogames programming"—exactly the right complexity level for a visualizer.

## Migration Notes

### From SFML to raylib

| SFML | raylib |
|------|--------|
| `sf::RenderWindow` | `InitWindow()` + main loop |
| `sf::RenderTexture` | `RenderTexture2D` |
| `sf::Shader` | `Shader` |
| `sf::VertexArray(LineStrip)` | `DrawLineStrip()` or `rlgl` |
| `sf::Event` polling | `WindowShouldClose()` + input functions |
| `window.draw(sprite)` | `DrawTexture()` |

### Shader Uniform Changes

```glsl
// SFML uniform access
uniform sampler2D texture;
uniform float fadeFactor;

// raylib uniform access (same GLSL, different binding)
uniform sampler2D texture0;  // raylib convention
uniform float fadeFactor;    // set via SetShaderValue()
```

## Open Questions

1. **rlgl for custom vertices?** raylib's low-level layer may be needed for 10K+ point waveforms if `DrawLineStrip()` has overhead.

2. **Audio capture?** raylib's audio module uses miniaudio internally—could potentially share the miniaudio context with capture code.

## References

- [raylib cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [raylib custom shaders wiki](https://github.com/raysan5/raylib/wiki/raylib-generic-uber-shader-and-custom-shaders)
- [rlImGui repository](https://github.com/raylib-extras/rlImGui)
- [SDL3 GPU API wiki](https://wiki.libsdl.org/SDL3/CategoryGPU)
- [SDL_shadercross](https://moonside.games/posts/introducing-sdl-shadercross/)
- [SFML 3.0 release notes](https://duerrenberger.dev/blog/2025/01/15/sfml-3-release/)
- [raylib shader setup tutorial (2025)](https://buscalaexcusa.com/posts/2025/raylib-how-to-setup-custom-shaders/)
