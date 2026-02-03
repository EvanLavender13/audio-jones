#include "automation/mod_sources.h"
#include "config/domain_warp_config.h"
#include "config/effect_config.h"
#include "config/radial_pulse_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionSineWarp = false;
static bool sectionTextureWarp = false;
static bool sectionGradientFlow = false;
static bool sectionWaveRipple = false;
static bool sectionMobius = false;
static bool sectionChladniWarp = false;
static bool sectionCircuitBoard = false;
static bool sectionDomainWarp = false;
static bool sectionSurfaceWarp = false;
static bool sectionInterferenceWarp = false;
static bool sectionCorridorWarp = false;
static bool sectionRadialPulse = false;
static bool sectionFftRadialWarp = false;

static void DrawWarpSine(EffectConfig *e, const ModSources *modSources,
                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("Sine Warp", categoryGlow, &sectionSineWarp)) {
    const bool wasEnabled = e->sineWarp.enabled;
    ImGui::Checkbox("Enabled##sineWarp", &e->sineWarp.enabled);
    if (!wasEnabled && e->sineWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SINE_WARP);
    }
    if (e->sineWarp.enabled) {
      ImGui::SliderInt("Octaves##sineWarp", &e->sineWarp.octaves, 1, 8);
      ModulatableSlider("Strength##sineWarp", &e->sineWarp.strength,
                        "sineWarp.strength", "%.2f", modSources);
      SliderAngleDeg("Anim Rate##sineWarp", &e->sineWarp.animRate, -180.0f,
                     180.0f, "%.1f °/s");
      ModulatableSliderAngleDeg("Octave Rotation##sineWarp",
                                &e->sineWarp.octaveRotation,
                                "sineWarp.octaveRotation", modSources);
      ImGui::Checkbox("Radial Mode##sineWarp", &e->sineWarp.radialMode);
      ImGui::Checkbox("Depth Blend##sineWarp", &e->sineWarp.depthBlend);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpTexture(EffectConfig *e, const ModSources *modSources,
                            const ImU32 categoryGlow) {
  if (DrawSectionBegin("Texture Warp", categoryGlow, &sectionTextureWarp)) {
    const bool wasEnabled = e->textureWarp.enabled;
    ImGui::Checkbox("Enabled##texwarp", &e->textureWarp.enabled);
    if (!wasEnabled && e->textureWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_TEXTURE_WARP);
    }
    if (e->textureWarp.enabled) {
      const char *channelModeNames[] = {
          "RG",          "RB",   "GB", "Luminance", "LuminanceSplit",
          "Chrominance", "Polar"};
      int channelMode = (int)e->textureWarp.channelMode;
      if (ImGui::Combo("Channel Mode##texwarp", &channelMode, channelModeNames,
                       7)) {
        e->textureWarp.channelMode = (TextureWarpChannelMode)channelMode;
      }
      ModulatableSlider("Strength##texwarp", &e->textureWarp.strength,
                        "textureWarp.strength", "%.3f", modSources);
      ImGui::SliderInt("Iterations##texwarp", &e->textureWarp.iterations, 1, 8);

      if (TreeNodeAccented("Directional##texwarp", categoryGlow)) {
        ModulatableSliderAngleDeg("Ridge Angle##texwarp",
                                  &e->textureWarp.ridgeAngle,
                                  "textureWarp.ridgeAngle", modSources);
        ModulatableSlider("Anisotropy##texwarp", &e->textureWarp.anisotropy,
                          "textureWarp.anisotropy", "%.2f", modSources);
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Noise##texwarp", categoryGlow)) {
        ModulatableSlider("Noise Amount##texwarp", &e->textureWarp.noiseAmount,
                          "textureWarp.noiseAmount", "%.2f", modSources);
        ImGui::SliderFloat("Noise Scale##texwarp", &e->textureWarp.noiseScale,
                           1.0f, 20.0f, "%.1f");
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawWarpGradientFlow(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Gradient Flow", categoryGlow, &sectionGradientFlow)) {
    const bool wasEnabled = e->gradientFlow.enabled;
    ImGui::Checkbox("Enabled##gradflow", &e->gradientFlow.enabled);
    if (!wasEnabled && e->gradientFlow.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_GRADIENT_FLOW);
    }
    if (e->gradientFlow.enabled) {
      ModulatableSlider("Strength##gradflow", &e->gradientFlow.strength,
                        "gradientFlow.strength", "%.3f", modSources);
      ImGui::SliderInt("Iterations##gradflow", &e->gradientFlow.iterations, 1,
                       8);
      ModulatableSlider("Edge Weight##gradflow", &e->gradientFlow.edgeWeight,
                        "gradientFlow.edgeWeight", "%.2f", modSources);
      ImGui::Checkbox("Random Direction##gradflow",
                      &e->gradientFlow.randomDirection);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpWaveRipple(EffectConfig *e, const ModSources *modSources,
                               const ImU32 categoryGlow) {
  if (DrawSectionBegin("Wave Ripple", categoryGlow, &sectionWaveRipple)) {
    const bool wasEnabled = e->waveRipple.enabled;
    ImGui::Checkbox("Enabled##waveripple", &e->waveRipple.enabled);
    if (!wasEnabled && e->waveRipple.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_WAVE_RIPPLE);
    }
    if (e->waveRipple.enabled) {
      ImGui::SliderInt("Octaves##waveripple", &e->waveRipple.octaves, 1, 4);
      ModulatableSlider("Strength##waveripple", &e->waveRipple.strength,
                        "waveRipple.strength", "%.3f", modSources);
      ImGui::SliderFloat("Anim Rate##waveripple", &e->waveRipple.animRate, 0.0f,
                         5.0f, "%.2f rad/s");
      ModulatableSlider("Frequency##waveripple", &e->waveRipple.frequency,
                        "waveRipple.frequency", "%.1f", modSources);
      ModulatableSlider("Steepness##waveripple", &e->waveRipple.steepness,
                        "waveRipple.steepness", "%.2f", modSources);
      ModulatableSlider("Decay##waveripple", &e->waveRipple.decay,
                        "waveRipple.decay", "%.1f", modSources);
      ModulatableSlider("Center Hole##waveripple", &e->waveRipple.centerHole,
                        "waveRipple.centerHole", "%.2f", modSources);
      if (TreeNodeAccented("Origin##waveripple", categoryGlow)) {
        ModulatableSlider("X##waveripple", &e->waveRipple.originX,
                          "waveRipple.originX", "%.2f", modSources);
        ModulatableSlider("Y##waveripple", &e->waveRipple.originY,
                          "waveRipple.originY", "%.2f", modSources);
        DrawLissajousControls(&e->waveRipple.originLissajous,
                              "waveripple_origin", NULL, NULL, 5.0f);
        TreeNodeAccentedPop();
      }
      ImGui::Checkbox("Shading##waveripple", &e->waveRipple.shadeEnabled);
      if (e->waveRipple.shadeEnabled) {
        ModulatableSlider("Shade Intensity##waveripple",
                          &e->waveRipple.shadeIntensity,
                          "waveRipple.shadeIntensity", "%.2f", modSources);
      }
    }
    DrawSectionEnd();
  }
}

static void DrawWarpMobius(EffectConfig *e, const ModSources *modSources,
                           const ImU32 categoryGlow) {
  if (DrawSectionBegin("Mobius", categoryGlow, &sectionMobius)) {
    const bool wasEnabled = e->mobius.enabled;
    ImGui::Checkbox("Enabled##mobius", &e->mobius.enabled);
    if (!wasEnabled && e->mobius.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOBIUS);
    }
    if (e->mobius.enabled) {
      ModulatableSlider("Spiral Tightness##mobius", &e->mobius.spiralTightness,
                        "mobius.spiralTightness", "%.2f", modSources);
      ModulatableSlider("Zoom Factor##mobius", &e->mobius.zoomFactor,
                        "mobius.zoomFactor", "%.2f", modSources);
      ModulatableSliderAngleDeg("Anim Rate##mobius", &e->mobius.animRate,
                                "mobius.animRate", modSources, "%.1f °/s");
      if (TreeNodeAccented("Fixed Points##mobius", categoryGlow)) {
        ModulatableSlider("Point 1 X##mobius", &e->mobius.point1X,
                          "mobius.point1X", "%.2f", modSources);
        ModulatableSlider("Point 1 Y##mobius", &e->mobius.point1Y,
                          "mobius.point1Y", "%.2f", modSources);
        ModulatableSlider("Point 2 X##mobius", &e->mobius.point2X,
                          "mobius.point2X", "%.2f", modSources);
        ModulatableSlider("Point 2 Y##mobius", &e->mobius.point2Y,
                          "mobius.point2Y", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
      if (TreeNodeAccented("Point 1 Motion##mobius", categoryGlow)) {
        DrawLissajousControls(&e->mobius.point1Lissajous, "mobius_p1", NULL,
                              NULL, 5.0f);
        TreeNodeAccentedPop();
      }
      if (TreeNodeAccented("Point 2 Motion##mobius", categoryGlow)) {
        DrawLissajousControls(&e->mobius.point2Lissajous, "mobius_p2", NULL,
                              NULL, 5.0f);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawWarpChladniWarp(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Chladni Warp", categoryGlow, &sectionChladniWarp)) {
    const bool wasEnabled = e->chladniWarp.enabled;
    ImGui::Checkbox("Enabled##chladni", &e->chladniWarp.enabled);
    if (!wasEnabled && e->chladniWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CHLADNI_WARP);
    }
    if (e->chladniWarp.enabled) {
      ChladniWarpConfig *cw = &e->chladniWarp;

      ModulatableSlider("N (X Mode)##chladni", &cw->n, "chladniWarp.n", "%.1f",
                        modSources);
      ModulatableSlider("M (Y Mode)##chladni", &cw->m, "chladniWarp.m", "%.1f",
                        modSources);
      ImGui::SliderFloat("Plate Size##chladni", &cw->plateSize, 0.5f, 2.0f,
                         "%.2f");
      ModulatableSlider("Strength##chladni", &cw->strength,
                        "chladniWarp.strength", "%.3f", modSources);

      const char *warpModeNames[] = {"Toward Nodes", "Along Nodes",
                                     "Intensity"};
      ImGui::Combo("Mode##chladni", &cw->warpMode, warpModeNames, 3);

      if (TreeNodeAccented("Animation##chladni", categoryGlow)) {
        ImGui::SliderFloat("Anim Rate##chladni", &cw->animRate, 0.0f, 2.0f,
                           "%.2f rad/s");
        ModulatableSlider("Range##chladni", &cw->animRange,
                          "chladniWarp.animRange", "%.1f", modSources);
        TreeNodeAccentedPop();
      }

      ImGui::Checkbox("Pre-Fold (Symmetry)##chladni", &cw->preFold);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpDomainWarp(EffectConfig *e, const ModSources *modSources,
                               const ImU32 categoryGlow) {
  if (DrawSectionBegin("Domain Warp", categoryGlow, &sectionDomainWarp)) {
    const bool wasEnabled = e->domainWarp.enabled;
    ImGui::Checkbox("Enabled##domainwarp", &e->domainWarp.enabled);
    if (!wasEnabled && e->domainWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_DOMAIN_WARP);
    }
    if (e->domainWarp.enabled) {
      DomainWarpConfig *dw = &e->domainWarp;

      ModulatableSlider("Strength##domainwarp", &dw->warpStrength,
                        "domainWarp.warpStrength", "%.3f", modSources);
      ImGui::SliderFloat("Scale##domainwarp", &dw->warpScale, 1.0f, 10.0f,
                         "%.1f");
      ImGui::SliderInt("Iterations##domainwarp", &dw->warpIterations, 1, 3);
      ModulatableSlider("Falloff##domainwarp", &dw->falloff,
                        "domainWarp.falloff", "%.2f", modSources);
      ModulatableSliderAngleDeg("Drift Speed##domainwarp", &dw->driftSpeed,
                                "domainWarp.driftSpeed", modSources,
                                "%.1f °/s");
      ModulatableSliderAngleDeg("Drift Angle##domainwarp", &dw->driftAngle,
                                "domainWarp.driftAngle", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpSurfaceWarp(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Surface Warp", categoryGlow, &sectionSurfaceWarp)) {
    const bool wasEnabled = e->surfaceWarp.enabled;
    ImGui::Checkbox("Enabled##surfacewarp", &e->surfaceWarp.enabled);
    if (!wasEnabled && e->surfaceWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SURFACE_WARP);
    }
    if (e->surfaceWarp.enabled) {
      ModulatableSlider("Intensity##surfacewarp", &e->surfaceWarp.intensity,
                        "surfaceWarp.intensity", "%.2f", modSources);
      ModulatableSliderAngleDeg("Angle##surfacewarp", &e->surfaceWarp.angle,
                                "surfaceWarp.angle", modSources);
      ModulatableSliderAngleDeg(
          "Rotation Speed##surfacewarp", &e->surfaceWarp.rotationSpeed,
          "surfaceWarp.rotationSpeed", modSources, "%.1f °/s");
      ModulatableSlider("Scroll Speed##surfacewarp",
                        &e->surfaceWarp.scrollSpeed, "surfaceWarp.scrollSpeed",
                        "%.2f", modSources);
      ModulatableSlider("Depth Shade##surfacewarp", &e->surfaceWarp.depthShade,
                        "surfaceWarp.depthShade", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpInterferenceWarp(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Interference Warp", categoryGlow,
                       &sectionInterferenceWarp)) {
    const bool wasEnabled = e->interferenceWarp.enabled;
    ImGui::Checkbox("Enabled##intfwarp", &e->interferenceWarp.enabled);
    if (!wasEnabled && e->interferenceWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_INTERFERENCE_WARP);
    }
    if (e->interferenceWarp.enabled) {
      ModulatableSlider("Amplitude##intfwarp", &e->interferenceWarp.amplitude,
                        "interferenceWarp.amplitude", "%.3f", modSources);
      ModulatableSlider("Scale##intfwarp", &e->interferenceWarp.scale,
                        "interferenceWarp.scale", "%.1f", modSources);
      ImGui::SliderInt("Axes##intfwarp", &e->interferenceWarp.axes, 2, 8);
      ModulatableSliderAngleDeg(
          "Axis Rotation##intfwarp", &e->interferenceWarp.axisRotationSpeed,
          "interferenceWarp.axisRotationSpeed", modSources, "%.1f °/s");
      ImGui::SliderInt("Harmonics##intfwarp", &e->interferenceWarp.harmonics, 8,
                       256);
      ModulatableSlider("Decay##intfwarp", &e->interferenceWarp.decay,
                        "interferenceWarp.decay", "%.2f", modSources);
      ModulatableSlider("Speed##intfwarp", &e->interferenceWarp.speed,
                        "interferenceWarp.speed", "%.4f", modSources);
      ImGui::SliderFloat("Drift##intfwarp", &e->interferenceWarp.drift, 1.0f,
                         3.0f, "%.2f");
    }
    DrawSectionEnd();
  }
}

static void DrawWarpCircuitBoard(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Circuit Board", categoryGlow, &sectionCircuitBoard)) {
    const bool wasEnabled = e->circuitBoard.enabled;
    ImGui::Checkbox("Enabled##circuitboard", &e->circuitBoard.enabled);
    if (!wasEnabled && e->circuitBoard.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CIRCUIT_BOARD);
    }
    if (e->circuitBoard.enabled) {
      ImGui::SliderFloat("Pattern X##circuitboard", &e->circuitBoard.patternX,
                         1.0f, 10.0f, "%.1f");
      ImGui::SliderFloat("Pattern Y##circuitboard", &e->circuitBoard.patternY,
                         1.0f, 10.0f, "%.1f");
      ImGui::SliderInt("Iterations##circuitboard", &e->circuitBoard.iterations,
                       3, 12);
      ModulatableSlider("Scale##circuitboard", &e->circuitBoard.scale,
                        "circuitBoard.scale", "%.2f", modSources);
      ModulatableSlider("Fold Phase##circuitboard", &e->circuitBoard.offset,
                        "circuitBoard.offset", "%.3f", modSources);
      ImGui::SliderFloat("Scale Decay##circuitboard",
                         &e->circuitBoard.scaleDecay, 1.01f, 1.2f, "%.3f");
      ModulatableSlider("Strength##circuitboard", &e->circuitBoard.strength,
                        "circuitBoard.strength", "%.2f", modSources);
      ModulatableSlider("Scroll Speed##circuitboard",
                        &e->circuitBoard.scrollSpeed,
                        "circuitBoard.scrollSpeed", "%.2f", modSources);
      ModulatableSliderAngleDeg("Scroll Angle##circuitboard",
                                &e->circuitBoard.scrollAngle,
                                "circuitBoard.scrollAngle", modSources);
      ImGui::Checkbox("Chromatic##circuitboard", &e->circuitBoard.chromatic);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpCorridorWarp(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Corridor Warp", categoryGlow, &sectionCorridorWarp)) {
    const bool wasEnabled = e->corridorWarp.enabled;
    ImGui::Checkbox("Enabled##corridorwarp", &e->corridorWarp.enabled);
    if (!wasEnabled && e->corridorWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CORRIDOR_WARP);
    }
    if (e->corridorWarp.enabled) {
      ModulatableSlider("Horizon##corridorwarp", &e->corridorWarp.horizon,
                        "corridorWarp.horizon", "%.2f", modSources);
      ModulatableSlider("Perspective##corridorwarp",
                        &e->corridorWarp.perspectiveStrength,
                        "corridorWarp.perspectiveStrength", "%.2f", modSources);

      const char *modeNames[] = {"Floor", "Ceiling", "Corridor"};
      ImGui::Combo("Mode##corridorwarp", &e->corridorWarp.mode, modeNames, 3);

      ModulatableSliderAngleDeg(
          "View Rotation##corridorwarp", &e->corridorWarp.viewRotationSpeed,
          "corridorWarp.viewRotationSpeed", modSources, "%.1f °/s");
      ModulatableSliderAngleDeg(
          "Plane Rotation##corridorwarp", &e->corridorWarp.planeRotationSpeed,
          "corridorWarp.planeRotationSpeed", modSources, "%.1f °/s");
      ModulatableSlider("Tile Density##corridorwarp", &e->corridorWarp.scale,
                        "corridorWarp.scale", "%.1f", modSources);
      ModulatableSlider("Scroll Speed##corridorwarp",
                        &e->corridorWarp.scrollSpeed,
                        "corridorWarp.scrollSpeed", "%.2f", modSources);
      ModulatableSlider("Strafe Speed##corridorwarp",
                        &e->corridorWarp.strafeSpeed,
                        "corridorWarp.strafeSpeed", "%.2f", modSources);
      ModulatableSlider("Fog Strength##corridorwarp",
                        &e->corridorWarp.fogStrength,
                        "corridorWarp.fogStrength", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpRadialPulse(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Radial Pulse", categoryGlow, &sectionRadialPulse)) {
    const bool wasEnabled = e->radialPulse.enabled;
    ImGui::Checkbox("Enabled##radpulse", &e->radialPulse.enabled);
    if (!wasEnabled && e->radialPulse.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_PULSE);
    }
    if (e->radialPulse.enabled) {
      RadialPulseConfig *rp = &e->radialPulse;

      ModulatableSlider("Radial Freq##radpulse", &rp->radialFreq,
                        "radialPulse.radialFreq", "%.1f", modSources);
      ModulatableSlider("Radial Amp##radpulse", &rp->radialAmp,
                        "radialPulse.radialAmp", "%.3f", modSources);
      ImGui::SliderInt("Segments##radpulse", &rp->segments, 2, 16);
      ModulatableSlider("Swirl##radpulse", &rp->angularAmp,
                        "radialPulse.angularAmp", "%.3f", modSources);
      ModulatableSlider("Petal##radpulse", &rp->petalAmp,
                        "radialPulse.petalAmp", "%.2f", modSources);
      ImGui::SliderFloat("Phase Speed##radpulse", &rp->phaseSpeed, -5.0f, 5.0f,
                         "%.2f");
      ModulatableSliderAngleDeg("Spiral Twist##radpulse", &rp->spiralTwist,
                                "radialPulse.spiralTwist", modSources);
      ImGui::SliderInt("Octaves##radpulse", &rp->octaves, 1, 8);
      ModulatableSliderAngleDeg("Octave Rotation##radpulse",
                                &rp->octaveRotation,
                                "radialPulse.octaveRotation", modSources);
      ImGui::Checkbox("Depth Blend##radpulse", &rp->depthBlend);
    }
    DrawSectionEnd();
  }
}

static void DrawWarpFftRadialWarp(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("FFT Radial Warp", categoryGlow,
                       &sectionFftRadialWarp)) {
    const bool wasEnabled = e->fftRadialWarp.enabled;
    ImGui::Checkbox("Enabled##fftradialwarp", &e->fftRadialWarp.enabled);
    if (!wasEnabled && e->fftRadialWarp.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_FFT_RADIAL_WARP);
    }
    if (e->fftRadialWarp.enabled) {
      ModulatableSlider("Intensity##fftradialwarp", &e->fftRadialWarp.intensity,
                        "fftRadialWarp.intensity", "%.3f", modSources);
      ModulatableSlider("Freq Start##fftradialwarp",
                        &e->fftRadialWarp.freqStart, "fftRadialWarp.freqStart",
                        "%.2f", modSources);
      ModulatableSlider("Freq End##fftradialwarp", &e->fftRadialWarp.freqEnd,
                        "fftRadialWarp.freqEnd", "%.2f", modSources);
      ModulatableSlider("Max Radius##fftradialwarp",
                        &e->fftRadialWarp.maxRadius, "fftRadialWarp.maxRadius",
                        "%.2f", modSources);
      ImGui::SliderInt("Segments##fftradialwarp", &e->fftRadialWarp.segments, 1,
                       16);
      ModulatableSliderAngleDeg("Phase##fftradialwarp",
                                &e->fftRadialWarp.pushPullPhase,
                                "fftRadialWarp.pushPullPhase", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawWarpCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(1);
  DrawCategoryHeader("Warp", categoryGlow);
  DrawWarpSine(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpTexture(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpGradientFlow(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpWaveRipple(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpMobius(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpChladniWarp(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpCircuitBoard(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpDomainWarp(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpSurfaceWarp(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpInterferenceWarp(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpCorridorWarp(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpRadialPulse(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawWarpFftRadialWarp(e, modSources, categoryGlow);
}
