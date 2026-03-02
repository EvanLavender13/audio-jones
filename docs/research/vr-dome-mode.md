# VR Dome Mode

An immersive VR output mode where the viewer sits at the center of a hemisphere dome in a void. The entire 2D shader pipeline renders to a flat texture as normal, then that texture is projected onto the inner surface of the dome. Head tracking lets the viewer look around inside it — a planetarium for audio-reactive visuals.

## Classification

- **Category**: General (architecture / output mode)
- **Pipeline Position**: After the existing render pipeline output, before display
- **Compute Model**: N/A — dome mesh is trivial geometry; the expensive pipeline runs once per frame regardless

## References

- [OpenXR Tutorial — Graphics (OpenGL)](https://openxr-tutorial.com/windows/opengl/3-graphics.html) - Swapchain creation, stereo rendering loop, frame submission flow with OpenXR + OpenGL
- [OpenXR-OpenGL-Example](https://github.com/ReliaSolve/OpenXR-OpenGL-Example) - Minimal pared-down hello_xr with flattened architecture (globals, single main.cpp), stereo rendering on Windows/Linux
- [rlOpenXR](https://github.com/FireFlyForLife/rlOpenXR) - Community raylib OpenXR binding (targets raylib 4.2, last updated Oct 2022). Useful as reference architecture for how raylib's rlgl layer can interface with OpenXR. Not directly usable with raylib 5.5.
- [Geeks3D — Fish Eye, Dome and Barrel Distortion GLSL Filters](https://www.geeks3d.com/20140213/glsl-shader-library-fish-eye-and-dome-and-barrel-distortion-post-processing-filters/) - GLSL fisheye projection shader for dome UV mapping
- [Paul Bourke — Geometric Distortion for Dome Projection](https://paulbourke.net/dome/domegeom/) - Theoretical foundation for fisheye geometry distortion
- [Emmanuel Durand — Spherical Projection Through GLSL](https://emmanueldurand.net/spherical_projection/) - Single-pass spherical projection with cartesian-to-spherical coordinate conversion

## Reference Code

Fisheye projection fragment shader (from Geeks3D). Maps a rectangular texture onto a dome/fisheye view — center of texture maps to zenith, edges wrap to horizon:

```glsl
#version 120
uniform sampler2D tex0;
varying vec4 Vertex_UV;
const float PI = 3.1415926535;

void main()
{
  float aperture = 178.0;
  float apertureHalf = 0.5 * aperture * (PI / 180.0);
  float maxFactor = sin(apertureHalf);

  vec2 uv;
  vec2 xy = 2.0 * Vertex_UV.xy - 1.0;
  float d = length(xy);
  if (d < (2.0 - maxFactor))
  {
    d = length(xy * maxFactor);
    float z = sqrt(1.0 - d * d);
    float r = atan(d, z) / PI;
    float phi = atan(xy.y, xy.x);

    uv.x = r * cos(phi) + 0.5;
    uv.y = r * sin(phi) + 0.5;
  }
  else
  {
    uv = Vertex_UV.xy;
  }
  vec4 c = texture2D(tex0, uv);
  gl_FragColor = c;
}
```

Spherical coordinate conversion (from Emmanuel Durand). For a point P = (x, y, z):

```
r   = sqrt(x² + y² + z²)
phi = arcsin(z / r)
theta = arccos(x / (r * cos(phi)))

Screen mapping:
  screen_x = theta * cos(phi) / PI
  screen_y = theta * sin(phi) / PI
```

## Algorithm

### Architecture Overview

Two rendering paths coexist, selected at startup:

1. **Desktop mode** (current): pipeline texture → screen (unchanged)
2. **VR mode** (new): pipeline texture → dome mesh → OpenXR stereo swapchains → HMD

The 2D shader pipeline is completely unchanged — it produces a flat texture. VR mode adds a thin layer that projects that texture onto dome geometry rendered in stereo.

### OpenXR Integration

**Session lifecycle** (new `src/vr/` module):

1. `xrCreateInstance()` with OpenGL extension
2. `xrGetSystem()` to find the HMD
3. `xrCreateSession()` bound to raylib's existing OpenGL context
4. Create stereo swapchains at HMD-recommended resolution (one color + one depth per eye)
5. Create reference space (`XR_REFERENCE_SPACE_TYPE_LOCAL` — seated, world-locked origin)

**Per-frame loop**:

1. Run AudioJones pipeline → produces flat output texture (no change)
2. `xrWaitFrame()` → get predicted display time and `shouldRender` flag
3. `xrBeginFrame()`
4. `xrLocateViews()` → get eye poses (position + orientation) and FOV per eye
5. For each eye (2 iterations):
   - `xrAcquireSwapchainImage()` → get texture index
   - `xrWaitSwapchainImage()` → sync with compositor
   - Bind swapchain texture as framebuffer color attachment
   - Set projection matrix from eye FOV, view matrix from eye pose
   - Draw dome mesh textured with pipeline output
   - `xrReleaseSwapchainImage()`
6. `xrEndFrame()` with `XrCompositionLayerProjection`

**Desktop mirror**: Continue rendering the flat pipeline output to the desktop window for monitoring and ImGui controls. The VR user sees the dome; the desktop shows the flat view.

### Dome Mesh Generation

Generate an inverted hemisphere mesh at Init time:

- **Geometry**: Unit hemisphere (y >= 0), normals pointing inward (toward center)
- **Tessellation**: ~40×40 subdivisions (1,600 quads / 3,200 triangles) — trivially cheap
- **UV mapping**: Fisheye/azimuthal equidistant projection:
  - For vertex at position (x, y, z) on unit hemisphere:
  - `theta = atan2(z, x)` (azimuth angle, 0 to 2π)
  - `phi = acos(y)` (polar angle from zenith, 0 to π/2 for hemisphere)
  - `r = phi / (π/2)` (normalized radius, 0 at zenith to 1 at horizon)
  - `u = r * cos(theta) * 0.5 + 0.5`
  - `v = r * sin(theta) * 0.5 + 0.5`
- **Result**: Center of the pipeline texture maps to the dome zenith (straight up). Edges of the texture wrap down to the horizon. Looking up shows the heart of the visuals.

### Frame Timing

- VR compositors demand 90 FPS with strict frame prediction timing
- AudioJones currently targets 60 FPS with a 20 Hz visual update rate
- The pipeline runs once per frame regardless — VR adds only the dome mesh render per eye (< 0.1ms)
- Main concern: the shader pipeline itself must complete within ~11ms (1/90s)
- The 20 Hz visual update throttle may need adjustment or removal in VR mode to avoid judder

### UI Strategy

ImGui panels cannot render directly in VR. Options:
- **Desktop-only UI**: Controls stay on the desktop monitor window. VR shows only the dome. Simplest approach, recommended for initial implementation.
- **Future**: OpenXR overlay layer for a floating control panel in VR space.

### raylib Context Sharing

The key integration challenge: raylib owns the OpenGL context and window. OpenXR needs to:
- Share the existing GL context (not create a new one)
- Render to its own swapchain textures (not raylib's window framebuffer)
- Coexist with raylib's `rlgl` state management

This requires reaching below raylib's abstraction to call raw OpenGL for framebuffer binding when rendering to VR swapchains, then restoring raylib's state for the desktop mirror pass.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| vrEnabled | bool | — | false | Toggle VR output mode on/off |
| domeAperture | float | 120-180 | 170 | Dome coverage angle in degrees (180 = full hemisphere) |
| domeTilt | float | -45 to 45 | 0 | Tilt the dome center forward/back from zenith (degrees) |
| domeScale | float | 0.5-5.0 | 2.0 | Radius of dome in meters (affects parallax between eyes) |

## Modulation Candidates

- **domeAperture**: Narrowing/widening the visible dome creates a breathing/pulsing effect
- **domeTilt**: Tilting the dome center moves the visual focus point

### Interaction Patterns

- **domeAperture × domeTilt**: At narrow aperture, tilt sweeps a spotlight across the visuals. At wide aperture, tilt is subtle. The aperture gates how dramatic the tilt feels.

## Notes

**Performance**: The pipeline is the bottleneck, not the VR rendering. If AudioJones already runs at 90+ FPS on the target GPU, VR mode costs almost nothing extra. If it runs at 60 FPS, optimization work is needed before VR is comfortable.

**Scope**: This is a large architectural feature — OpenXR integration, new module, build system changes (linking OpenXR loader), frame timing rework. Estimate it as a multi-week effort with significant API surface to learn.

**Dependencies**: OpenXR SDK/loader must be added to the build. SteamVR must be installed as the OpenXR runtime. A VR headset is required for testing.

**Prior art**: Several VR audio visualizers exist on Steam ([VR Audio Visualizer](https://store.steampowered.com/app/601760/VR_Audio_Visualizer/), [Vision](https://store.steampowered.com/app/619550/Vision), [Chromesthesia VR](https://store.steampowered.com/app/3365440/Chromesthesia_VR_Music_Visualizer)). Vision is described as "the world's first fully customizable music visualizer made for VR" and uses spectrum analysis + beat detection — a similar approach to AudioJones but built VR-first.

**Risk**: The main technical risk is raylib OpenGL context sharing with OpenXR. rlOpenXR proved this is possible (with raylib 4.2), but raylib 5.5 may have changed its GL state management. This needs a proof-of-concept spike before committing to full implementation.
