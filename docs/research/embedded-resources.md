# Embedded Resources

Package shader and font files directly into the executable binary so the release distribution is a single `.exe` with no `shaders/` or `fonts/` folder. Debug builds retain file-based loading for iteration speed.

## Classification

- **Category**: General (build system / resource packaging)
- **Pipeline Position**: N/A — affects resource loading at init, not rendering

## References

- [CMake Resource Embedding (codegenes.net)](https://www.codegenes.net/blog/embed-resources-eg-shader-code-images-into-executable-library-with-cmake/) - CMake `file(READ)` + hex conversion to generate C arrays
- [raylib Resource Embedding Discussion](https://github.com/raysan5/raylib/discussions/2152) - Community approaches for raylib projects
- [raylib API: LoadShaderFromMemory](https://www.raylib.com/cheatsheet/cheatsheet.html) - `LoadShaderFromMemory(const char* vsCode, const char* fsCode)` — loads shader from source strings

## Algorithm

### Current Loading Patterns

Three resource loading patterns exist in the codebase:

| Resource Type | Count | Current API | Replacement API |
|--------------|-------|-------------|-----------------|
| Fragment shaders (`.fs`) | 107 | `LoadShader(NULL, "shaders/name.fs")` | `LoadShaderFromMemory(NULL, source)` |
| Compute shaders (`.glsl`) | 10 | `SimLoadShaderSource(path)` → `rlCompileShader(source, RL_COMPUTE_SHADER)` | Same, but source comes from embedded array |
| Font (`.ttf`) | 1 | `io.Fonts->AddFontFromFileTTF(path, size)` | `io.Fonts->AddFontFromMemoryTTF(data, len, size)` |

### Approach: CMake Code Generation + Compile-Time Switch

**Step 1 — CMake generates embedded resource source files at configure time:**

A CMake function globs all `.fs`, `.glsl`, and `.ttf` files. For each file it:
- Reads the file content with `file(READ ... HEX)` (binary) or `file(READ ...)` (text)
- Generates a C variable: `const char shaders_feedback_fs[]` for text, `const unsigned char fonts_roboto_medium_ttf[]` for binary
- Generates a lookup function `GetEmbeddedResource(const char* path)` mapping path strings (e.g., `"shaders/feedback.fs"`) to the embedded data pointer
- A second function `GetEmbeddedResourceSize(const char* path)` returns byte count (needed for binary resources like fonts)
- Output: `${CMAKE_BINARY_DIR}/generated/embedded_resources.h` and `.cpp`

**Step 2 — Thin wrapper functions dispatch by build config:**

A new header `src/render/resource_loader.h` provides:

```
Shader LoadShaderResource(const char* vsPath, const char* fsPath);
const char* LoadShaderSourceResource(const char* path);
```

- When `EMBED_RESOURCES` is defined: looks up the embedded string and calls `LoadShaderFromMemory` or returns the embedded pointer
- When `EMBED_RESOURCES` is not defined: delegates to `LoadShader` or `SimLoadShaderSource` (current behavior)

**Step 3 — CMake wires it up:**

- `add_compile_definitions` adds `EMBED_RESOURCES` for Release builds only
- The generated `.cpp` is added to the source list
- Debug builds behave exactly as today — no generated file compiled in

**Step 4 — Migrate call sites:**

Mechanical replacement across ~125 `.cpp` files:
- `LoadShader(0, "shaders/foo.fs")` → `LoadShaderResource(0, "shaders/foo.fs")`
- `LoadShader(NULL, "shaders/foo.fs")` → `LoadShaderResource(NULL, "shaders/foo.fs")`
- `SimLoadShaderSource(path)` → `LoadShaderSourceResource(path)` (already returns `const char*`; caller uses it with `rlCompileShader`)
- Font loading in `main.cpp` gets an `#ifdef EMBED_RESOURCES` block

### Font Memory Ownership

`AddFontFromMemoryTTF` takes ownership of the data pointer by default (it calls `free`). Since the embedded font is a static array, pass `AddFontFromMemoryCompressedTTF` or set `ImFontConfig::FontDataOwnedByAtlas = false` to prevent ImGui from trying to free static memory.

### Compute Shader Wrinkle

`SimLoadShaderSource` currently returns `char*` from `LoadFileText`, and callers free it with `UnloadFileText`. The embedded path returns `const char*` to static data that must NOT be freed. `LoadShaderSourceResource` returns `const char*`; callers must be updated to skip the `UnloadFileText` call when using embedded resources (or the wrapper can `strdup` the embedded source so the existing free pattern still works).

The simpler approach: `LoadShaderSourceResource` always returns a `malloc`'d copy. In Debug mode it calls `LoadFileText` (already `malloc`'d). In Release mode it `strdup`s the embedded string. Callers always free with `UnloadFileText`/`free` as they do today.

## Parameters

N/A — this is a build-system feature, not a visual effect.

## Notes

- **Binary size impact**: ~107 shader files averaging ~2-4 KB each ≈ 200-400 KB added to the executable. The font is ~170 KB. Total overhead is under 1 MB — negligible for a Release build.
- **Configure-time dependency**: CMake regenerates the embedded source when shader files change, triggering recompilation of the generated `.cpp`. Individual shader edits don't trigger full rebuilds — only the generated file recompiles.
- **No third-party dependency**: Uses only CMake built-in `file()` commands. No need for `cmrc`, `xxd`, or external tools.
- **Incremental migration**: The wrapper functions work whether `EMBED_RESOURCES` is defined or not. Files can be migrated one at a time — both `LoadShader` and `LoadShaderResource` coexist until migration is complete.
- **Compute shaders already use in-memory compilation**: The simulation modules call `rlCompileShader(source, RL_COMPUTE_SHADER)` with a source string. Only the source-loading step changes.
