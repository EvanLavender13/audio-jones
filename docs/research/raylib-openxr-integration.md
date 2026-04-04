# raylib + OpenXR GL Integration

Technical deep-dive on sharing raylib's OpenGL context with OpenXR for stereo swapchain rendering. Covers context extraction, FBO binding, rlgl state management, and the frame handoff protocol.

## Classification

- **Category**: General (architecture reference)
- **Pipeline Position**: N/A — integration layer between raylib and OpenXR
- **Relation**: Companion document to [VR Dome Mode](vr-dome-mode.md)

## References

- [rlxr](https://github.com/caszuu/rlxr) - C single-header raylib OpenXR binding (updated Dec 2025, targets raylib 5.5). Per-eye manual rendering approach. Best current reference.
- [rlOpenXR](https://github.com/FireFlyForLife/rlOpenXR) - C++ raylib OpenXR binding (targets raylib 4.2). Uses rlgl stereo mode. Useful for stereo batch architecture reference.
- [hello_xr OpenGL plugin](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/tests/hello_xr/graphicsplugin_opengl.cpp) - Khronos official OpenXR OpenGL example. FBO attachment and frame submission patterns.
- [KHeresy openxr-simple-example](https://github.com/KHeresy/openxr-simple-example/blob/master/main.cpp) - Minimal OpenXR + OpenGL with `glFinish()` sync pattern
- [OpenXR 1.1 Spec — XrGraphicsBindingOpenGLWin32KHR](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrGraphicsBindingOpenGLWin32KHR.html) - Session binding struct, threading rules
- [OpenXR 1.1 Spec — Threading](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#khr_opengl_enable-threading) - GL context threading constraints for OpenXR calls
- [raylib_opengl_interop.c](https://github.com/raysan5/raylib/blob/master/examples/others/raylib_opengl_interop.c) - Official raylib example for raw GL alongside rlgl
- [raylib PR #2424](https://github.com/raysan5/raylib/pull/2424) - VR stereo viewport fix (rlOpenXR-discovered, merged into raylib)
- [SteamVR GL state contamination](https://github.com/ValveSoftware/SteamVR-for-Linux/issues/799) - SteamVR leaves GL errors after `xrEndFrame()`

## Reference Code

### GL Context Extraction (from rlxr)

Platform-specific retrieval of the GL context raylib created internally. Called after `InitWindow()`.

```c
// Windows — wglGetCurrentContext returns raylib's HGLRC
#ifdef XR_USE_PLATFORM_WIN32
if ((ctx.win32 = wglGetCurrentContext())) {
    rlglInfo.win32.hDC = wglGetCurrentDC();
    rlglInfo.win32.hGLRC = ctx.win32;
}
#endif

// Linux/Xlib
#ifdef XR_USE_PLATFORM_XLIB
if ((ctx.xlib = glXGetCurrentContext())) {
    rlglInfo.xlib.xDisplay = XOpenDisplay(NULL);
    rlglInfo.xlib.glxContext = ctx.xlib;
    rlglInfo.xlib.glxDrawable = glXGetCurrentDrawable();
}
#endif
```

### OpenXR Session Binding (from rlOpenXR)

Passes the extracted handles to OpenXR session creation.

```cpp
auto graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR{
    .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
    .next = nullptr,
    .hDC = wrapped_wglGetCurrentDC(),
    .hGLRC = wrapped_wglGetCurrentContext()
};
// Pass via XrSessionCreateInfo::next to xrCreateSession()
```

### Per-Eye FBO Setup (from rlxr)

Init-time FBO creation with app-created depth buffer.

```c
for (int i = 0; i < rlxr.viewCount; i++) {
    rlxr.viewBufs[i].framebuffer = rlLoadFramebuffer();

    if (!rlxr.depthSupported) {
        rlxr.viewBufs[i].depthRenderBuffer = rlLoadTextureDepth(
            rlxr.viewProps[i].recommendedImageRectWidth,
            rlxr.viewProps[i].recommendedImageRectHeight, true);
        rlFramebufferAttach(rlxr.viewBufs[i].framebuffer,
            rlxr.viewBufs[i].depthRenderBuffer,
            RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    }
}
```

### Per-Frame Eye Rendering (from rlxr)

Acquire swapchain image, attach to FBO, set matrices, render, release.

```c
// BeginView():
rlDrawRenderBatchActive();   // Flush pending rlgl draws
rlViewport(0, 0, w, h);
rlScissor(0, 0, w, h);
rlEnableFramebuffer(view->framebuffer);
rlSetFramebufferWidth(w);
rlSetFramebufferHeight(h);

// Attach acquired swapchain image as color target
rlFramebufferAttach(view->framebuffer, view->colorImages[colorAcquiredIndex].image,
    RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

// Set projection from OpenXR FOV
Matrix xr_proj;
XrMatrix4x4f_CreateProjectionFov(&xr_proj, GRAPHICS_OPENGL,
    rlxr.views[index].fov, rlGetCullDistanceNear(), rlGetCullDistanceFar());
rlSetMatrixProjection(xr_proj);

// Set view matrix from eye pose
Matrix xr_view = MatrixInvert(
    MatrixMultiply(QuaternionToMatrix(quat), MatrixTranslate(pos.x, pos.y, pos.z)));
rlSetMatrixModelview(xr_view);

// ... draw scene ...

// EndView():
rlDrawRenderBatchActive();
rlDisableFramebuffer();
```

### Desktop Mirror Blit (from rlOpenXR)

Raw GL blit from swapchain FBO to window.

```cpp
void rlOpenXRBlitToWindow(RLOpenXREye eye, bool keep_aspect_ratio) {
    rlDisableFramebuffer();
    ClearBackground(BLACK);
    glBlitNamedFramebuffer(s_xr->active_fbo, 0,
        src.offset.x, src.offset.y,
        src.offset.x + src.extent.width, src.offset.y + src.extent.height,
        dest.offset.x, dest.offset.y,
        dest.offset.x + dest.extent.width, dest.offset.y + dest.extent.height,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    rlEnableFramebuffer(s_xr->active_fbo);
}
```

## Algorithm

### 1. GL Context Extraction

raylib does not expose the OpenGL context handle through its public API. `GetWindowHandle()` returns `HWND` (Windows) / `GLFWwindow*` (Linux), not the GL context. The GLFW platform layer stores `GLFWwindow*` as file-static — inaccessible from outside `rcore_desktop_glfw.c`.

**Solution**: After `InitWindow()`, raylib's GL context is current on the calling thread. Standard OS calls retrieve it directly:

| Platform | Context Handle | Device/Display Handle |
|----------|---------------|----------------------|
| Windows | `wglGetCurrentContext()` → `HGLRC` | `wglGetCurrentDC()` → `HDC` |
| Linux (X11) | `glXGetCurrentContext()` → `GLXContext` | `glXGetCurrentDisplay()` → `Display*` |

No raylib patching required. Both rlOpenXR and rlxr use this approach. The handles are passed to `XrGraphicsBindingOpenGLWin32KHR` (or the Xlib equivalent) for `xrCreateSession()`. OpenXR uses the existing context — it does not create a separate one.

### 2. Swapchain FBO Binding

OpenXR swapchain images are `GLuint` texture names (in `XrSwapchainImageOpenGLKHR.image`). The application creates its own FBO and re-attaches the acquired image each frame.

**Init-time**:
- `rlLoadFramebuffer()` — creates an empty FBO (raylib 5.5 signature takes no args)
- Attach an app-created depth renderbuffer via `rlFramebufferAttach()` with `RL_ATTACHMENT_DEPTH` + `RL_ATTACHMENT_RENDERBUFFER`
- A separate depth swapchain is optional (enables runtime reprojection via `XR_KHR_composition_layer_depth` but is unnecessary for dome projection of a 2D texture)

**Per-frame per-eye**:
1. `xrAcquireSwapchainImage()` → get image index
2. `xrWaitSwapchainImage()` → block until runtime releases the image for writing
3. `rlFramebufferAttach(fbo, swapchainImages[index].image, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0)` — attach as color target
4. `rlEnableFramebuffer(fbo)` — bind
5. `rlSetFramebufferWidth(w)` / `rlSetFramebufferHeight(h)` — **critical**: rlgl does not auto-track FBO dimensions. Without this, stereo viewport calculations use the window size, producing incorrect output (see [PR #2424](https://github.com/raysan5/raylib/pull/2424))
6. Render (draw dome mesh textured with pipeline output, or blit pipeline texture)
7. `glFinish()` — defensive GPU sync before releasing to compositor
8. `xrReleaseSwapchainImage()`

**Alternative — blit instead of draw**: Since AudioJones already produces a flat render texture, it can `glBlitFramebuffer()` from the pipeline output FBO to the swapchain FBO instead of drawing dome geometry. This is the simplest path for a flat projection but does not support the dome curvature — dome mesh rendering is needed for the hemisphere effect.

### 3. rlgl State Protocol

rlgl maintains a software-side batch renderer. Raw GL calls between rlgl calls can corrupt state. The safe handoff protocol:

**Before VR rendering** — flush the batch:
```
rlDrawRenderBatchActive()
```
After this call, rlgl is in a clean state: `glUseProgram(0)`, `glBindTexture(GL_TEXTURE_2D, 0)`, VAO unbound. This is the handoff point for raw GL or OpenXR operations.

**What rlgl tracks internally** (`rlglData` struct in `rlgl.h`):

| State | Tracked? | Notes |
|-------|----------|-------|
| Matrix stack (projection, modelview) | Yes (software) | Does not use GL 1.x matrix calls. Uploads MVP via `glUniformMatrix4fv` at draw time. Use `rlSetMatrixProjection()` / `rlSetMatrixModelview()` to restore. |
| Current shader | Yes (`currentShaderId`) | Only activated inside `rlDrawRenderBatch()`. After flush, `glUseProgram(0)` is called. |
| Current FBO | **No** | `rlEnableFramebuffer()` / `rlDisableFramebuffer()` are thin `glBindFramebuffer` wrappers. No cached value — queries via `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)`. |
| Framebuffer dimensions | Yes (manual) | `rlSetFramebufferWidth/Height()` must be called when binding a non-window FBO. Not auto-updated. |
| Blend mode | Yes (`currentBlendMode`) | Changes trigger batch flush. |
| Viewport | **No** | `rlViewport()` is a pass-through to `glViewport()`. |
| Texture bindings | Yes (per-batch) | Reset to defaults after each batch flush. |
| Stereo state | Yes | `rlEnableStereoRender()`, projection/view offset matrices. |

**After VR rendering** — restore for desktop mirror pass:
1. `rlDisableFramebuffer()` — rebind window FBO (FBO 0)
2. `rlViewport(0, 0, windowWidth, windowHeight)` — restore window viewport
3. `rlSetFramebufferWidth(windowWidth)` / `rlSetFramebufferHeight(windowHeight)` — restore cached dimensions
4. Resume normal raylib/rlgl drawing for the desktop mirror and ImGui

**After `xrEndFrame()`** — drain the GL error queue:
```c
while (glGetError() != GL_NO_ERROR) {}
```
SteamVR is known to leave `GL_INVALID_VALUE` errors from its internal compositor operations. Meta's OpenXR runtime may exhibit similar behavior. Defensive drain regardless of runtime.

### 4. Two Integration Architectures

**rlxr approach (recommended for AudioJones)**: Manual per-eye rendering. The application loops over eyes, sets matrices, and renders into each swapchain FBO independently. For AudioJones this means: run pipeline once → produce flat texture → for each eye: bind swapchain FBO, draw dome mesh textured with pipeline output using that eye's view/projection matrix, release.

**rlOpenXR approach**: Uses rlgl's built-in `rlEnableStereoRender()` which causes the batch renderer to draw everything twice with different viewports and view offsets. Constructs a fake `RenderTexture2D` from swapchain textures and uses `BeginTextureMode()`. Elegant for scene rendering but awkward for AudioJones where the "scene" is a single textured mesh — the per-eye approach is more direct.

### 5. raylib API Differences (4.2 → 5.5)

| API | raylib 4.2 | raylib 5.5 |
|-----|-----------|-----------|
| `rlLoadFramebuffer()` | `(int width, int height)` | `(void)` — dimensions managed separately |
| `rlSetFramebufferWidth/Height()` | Not available | New — required for correct FBO viewport |
| `rlGetCullDistanceNear/Far()` | Not available | New — used by rlxr for projection matrix |
| `rlEnableStereoRender()` | Present | Unchanged |
| `rlSetMatrixProjectionStereo()` | Present | Unchanged |
| `rlFramebufferAttach()` | Present | Unchanged |
| Platform layer | Single `rcore.c` | Split into `rcore_desktop_glfw.c`, etc. |

rlxr already uses the 5.5 API. rlOpenXR code samples need the `rlLoadFramebuffer` signature adjusted.

## Notes

**Threading**: The GL context must be current on the thread calling the 10 OpenXR GL-dependent functions (`xrCreateSession`, `xrBeginFrame`, `xrEndFrame`, `xrAcquireSwapchainImage`, etc.). `xrWaitFrame` is explicitly exempt — it can run on a separate thread. AudioJones is single-threaded, so this is not a concern.

**OpenXR does not create a separate GL context**: It uses the application's existing context directly. The spec states the application "becomes subject to restrictions on use of that context" during the listed calls. In practice this just means don't rebind the context to another thread mid-frame.

**Swapchain format negotiation**: Query available formats via `xrEnumerateSwapchainFormats()`. Prefer `GL_SRGB8_ALPHA8` for correct gamma handling. Fallback order: `GL_SRGB8_ALPHA8` → `GL_RGB10_A2` → `GL_RGBA16F` → `GL_RGBA8`.

**Single vs. double-wide swapchain**: rlOpenXR creates one double-wide swapchain (both eyes side-by-side) and uses rlgl stereo to split it. rlxr creates separate per-eye swapchains. The per-eye approach is more standard across OpenXR implementations.

**Target runtime**: Meta Quest 3 via Quest Link (USB or Air Link) using Meta's OpenXR runtime. SteamVR is a secondary option via Steam Link. All OpenXR API calls in this doc are runtime-agnostic - no code changes needed between runtimes.

**Desktop mirror**: Continue rendering the flat pipeline output to the desktop window for monitoring and ImGui. The VR pass is additive — it reads the pipeline's output texture and projects it into the swapchain FBOs. The desktop window rendering path is unchanged.
