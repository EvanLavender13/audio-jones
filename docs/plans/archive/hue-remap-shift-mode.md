# Hue Remap Shift Mode

Add a mode toggle to the Hue Remap effect. In **Replace mode** (current default), the gradient LUT determines output hue. In **Shift mode**, the gradient is ignored and the effect performs a spatially-varying hue rotation — like Color Grade's hue shift but with all of Hue Remap's spatial blend/shift controls.

## Design

### Types

Add to `HueRemapConfig`:
- `bool shiftMode = false;` — false = Replace (current behavior), true = Shift (hue rotation)

Add to `HueRemapEffect`:
- `int shiftModeLoc;` — cached uniform location

Add to `HUE_REMAP_CONFIG_FIELDS` macro: `shiftMode`

### Algorithm

In the shader, the existing Replace path is unchanged. The new Shift path replaces lines 162-167:

```glsl
// Replace mode (existing):
float t = fract(hsv.x + shift + shiftField);
vec3 remappedRGB = texture(texture1, vec2(t, 0.5)).rgb;
vec3 remappedHSV = rgb2hsv(remappedRGB);
vec3 result = hsv2rgb(vec3(remappedHSV.x, hsv.y, hsv.z));

// Shift mode (new):
float newHue = fract(hsv.x + shift + shiftField);
vec3 result = hsv2rgb(vec3(newHue, hsv.y, hsv.z));
```

The blend mask and spatial fields work identically in both modes.

### UI Behavior

- Toggle checkbox labeled `"Hue Shift Mode"` appears after the Enabled checkbox, before the gradient editor
- When `shiftMode` is true, the gradient editor (`ImGuiDrawColorMode`) is hidden (not just disabled)
- All other controls (Core, Blend Spatial, Shift Spatial, Noise Field) remain visible in both modes

---

## Tasks

### Wave 1: Config and effect struct

#### Task 1.1: Add shiftMode field to config and effect struct

**Files**: `src/effects/hue_remap.h`

**Do**:
- Add `bool shiftMode = false;` to `HueRemapConfig` after `enabled`
- Add `int shiftModeLoc;` to `HueRemapEffect` after `gradientLUTLoc`
- Add `shiftMode` to `HUE_REMAP_CONFIG_FIELDS` macro

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Shader, C++, and UI (parallel — no file overlap)

#### Task 2.1: Add shiftMode branch to shader

**Files**: `shaders/hue_remap.fs`

**Do**:
- Add `uniform int shiftMode;` after the existing uniforms
- In `main()`, branch on `shiftMode`: if true, compute `vec3 result = hsv2rgb(vec3(fract(hsv.x + shift + shiftField), hsv.y, hsv.z));` — skip the LUT sample entirely. If false, keep existing code unchanged.

**Verify**: Shader is valid GLSL (runtime check only — no build needed).

#### Task 2.2: Cache and bind shiftMode uniform

**Files**: `src/effects/hue_remap.cpp`

**Do**:
- In `HueRemapEffectInit`: cache `e->shiftModeLoc = GetShaderLocation(e->shader, "shiftMode");` after the existing location caches
- In `HueRemapEffectSetup`: bind the uniform as `int sm = cfg->shiftMode ? 1 : 0;` then `SetShaderValue(e->shader, e->shiftModeLoc, &sm, SHADER_UNIFORM_INT);` — place before the existing shift uniform binding

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Add shift mode toggle to UI

**Files**: `src/ui/imgui_effects_color.cpp`

**Do**:
- In `DrawColorHueRemap`, after the Enabled checkbox and before `ImGuiDrawColorMode(&hr->gradient)`:
  - Add `ImGui::Checkbox("Hue Shift Mode##hueremap", &hr->shiftMode);`
- Wrap `ImGuiDrawColorMode(&hr->gradient);` in `if (!hr->shiftMode) { ... }` so the gradient editor is hidden in shift mode

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Replace mode (shiftMode off): behaves identically to current effect
- [ ] Shift mode (shiftMode on): hue rotation with spatial variation, no gradient involved
- [ ] Gradient editor hidden when shift mode enabled
- [ ] Preset save/load preserves shiftMode field
