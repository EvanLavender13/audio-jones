#include "imgui_effects_generators.h"
#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/constellation.h"
#include "effects/interference.h"
#include "effects/moire_generator.h"
#include "effects/pitch_spiral.h"
#include "effects/plasma.h"
#include "effects/scan_bars.h"
#include "effects/solid_color.h"
#include "effects/spectral_arcs.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionConstellation = false;
static bool sectionPlasma = false;
static bool sectionInterference = false;
static bool sectionScanBars = false;
static bool sectionPitchSpiral = false;
static bool sectionMoireGenerator = false;
static bool sectionSpectralArcs = false;
static bool sectionSolidColor = false;

static void DrawGeneratorsConstellation(EffectConfig *e,
                                        const ModSources *modSources,
                                        const ImU32 categoryGlow) {
  if (DrawSectionBegin("Constellation", categoryGlow, &sectionConstellation)) {
    const bool wasEnabled = e->constellation.enabled;
    ImGui::Checkbox("Enabled##constellation", &e->constellation.enabled);
    if (!wasEnabled && e->constellation.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CONSTELLATION_BLEND);
    }
    if (e->constellation.enabled) {
      ConstellationConfig *c = &e->constellation;

      // Grid and animation
      ModulatableSlider("Grid Scale##constellation", &c->gridScale,
                        "constellation.gridScale", "%.1f", modSources);
      ModulatableSlider("Anim Speed##constellation", &c->animSpeed,
                        "constellation.animSpeed", "%.2f", modSources);
      ModulatableSlider("Wander##constellation", &c->wanderAmp,
                        "constellation.wanderAmp", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Radial wave
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Radial Wave");
      ImGui::Spacing();
      ImGui::SliderFloat("Radial Freq##constellation", &c->radialFreq, 0.1f,
                         5.0f, "%.2f");
      ModulatableSlider("Radial Amp##constellation", &c->radialAmp,
                        "constellation.radialAmp", "%.2f", modSources);
      ModulatableSlider("Radial Speed##constellation", &c->radialSpeed,
                        "constellation.radialSpeed", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Point rendering
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Points");
      ImGui::Spacing();
      ModulatableSlider("Point Size##constellation", &c->pointSize,
                        "constellation.pointSize", "%.2f", modSources);
      ModulatableSlider("Point Bright##constellation", &c->pointBrightness,
                        "constellation.pointBrightness", "%.2f", modSources);
      ImGuiDrawColorMode(&c->pointGradient);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Line rendering
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Lines");
      ImGui::Spacing();
      ImGui::SliderFloat("Line Width##constellation", &c->lineThickness, 0.01f,
                         0.1f, "%.3f");
      ModulatableSlider("Max Line Len##constellation", &c->maxLineLen,
                        "constellation.maxLineLen", "%.2f", modSources);
      ModulatableSlider("Line Opacity##constellation", &c->lineOpacity,
                        "constellation.lineOpacity", "%.2f", modSources);
      ImGui::Checkbox("Interpolate Line Color##constellation",
                      &c->interpolateLineColor);
      ImGuiDrawColorMode(&c->lineGradient);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##constellation", &c->blendIntensity,
                        "constellation.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)c->blendMode;
      if (ImGui::Combo("Blend Mode##constellation", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        c->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsPlasma(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Plasma", categoryGlow, &sectionPlasma)) {
    const bool wasEnabled = e->plasma.enabled;
    ImGui::Checkbox("Enabled##plasma", &e->plasma.enabled);
    if (!wasEnabled && e->plasma.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PLASMA_BLEND);
    }
    if (e->plasma.enabled) {
      PlasmaConfig *p = &e->plasma;

      // Bolt configuration
      ImGui::SliderInt("Bolt Count##plasma", &p->boltCount, 1, 8);
      ImGui::SliderInt("Layers##plasma", &p->layerCount, 1, 3);
      ImGui::SliderInt("Octaves##plasma", &p->octaves, 1, 10);
      ImGui::Combo("Falloff##plasma", &p->falloffType, "Sharp\0Linear\0Soft\0");

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Animation
      ModulatableSlider("Drift Speed##plasma", &p->driftSpeed,
                        "plasma.driftSpeed", "%.2f", modSources);
      ModulatableSlider("Drift Amount##plasma", &p->driftAmount,
                        "plasma.driftAmount", "%.2f", modSources);
      ModulatableSlider("Anim Speed##plasma", &p->animSpeed, "plasma.animSpeed",
                        "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Appearance
      ModulatableSlider("Displacement##plasma", &p->displacement,
                        "plasma.displacement", "%.2f", modSources);
      ModulatableSlider("Glow Radius##plasma", &p->glowRadius,
                        "plasma.glowRadius", "%.3f", modSources);
      ModulatableSlider("Brightness##plasma", &p->coreBrightness,
                        "plasma.coreBrightness", "%.2f", modSources);
      ModulatableSlider("Flicker##plasma", &p->flickerAmount,
                        "plasma.flickerAmount", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&p->gradient);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##plasma", &p->blendIntensity,
                        "plasma.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)p->blendMode;
      if (ImGui::Combo("Blend Mode##plasma", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        p->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsInterference(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Interference", categoryGlow, &sectionInterference)) {
    const bool wasEnabled = e->interference.enabled;
    ImGui::Checkbox("Enabled##interference", &e->interference.enabled);
    if (!wasEnabled && e->interference.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_INTERFERENCE_BLEND);
    }
    if (e->interference.enabled) {
      InterferenceConfig *i = &e->interference;

      // Sources
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Sources");
      ImGui::Spacing();
      ImGui::SliderInt("Sources##interference", &i->sourceCount, 1, 8);
      ModulatableSlider("Radius##interference", &i->baseRadius,
                        "interference.baseRadius", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Motion (DualLissajous)
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Motion");
      ImGui::Spacing();
      DrawLissajousControls(&i->lissajous, "interference",
                            "interference.lissajous", modSources, 1.0f);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Waves
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Waves");
      ImGui::Spacing();
      ModulatableSlider("Wave Freq##interference", &i->waveFreq,
                        "interference.waveFreq", "%.1f", modSources);
      ModulatableSlider("Wave Speed##interference", &i->waveSpeed,
                        "interference.waveSpeed", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Falloff
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Falloff");
      ImGui::Spacing();
      ImGui::Combo("Falloff##interference", &i->falloffType,
                   "None\0Inverse\0InvSquare\0Gaussian\0");
      ModulatableSlider("Falloff Strength##interference", &i->falloffStrength,
                        "interference.falloffStrength", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Boundaries
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Boundaries");
      ImGui::Spacing();
      ImGui::Checkbox("Boundaries##interference", &i->boundaries);
      if (i->boundaries) {
        ModulatableSlider("Reflection##interference", &i->reflectionGain,
                          "interference.reflectionGain", "%.2f", modSources);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Visualization
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Visualization");
      ImGui::Spacing();
      ImGui::Combo("Visual Mode##interference", &i->visualMode,
                   "Raw\0Absolute\0Contour\0");
      if (i->visualMode == 2) {
        ImGui::SliderInt("Contours##interference", &i->contourCount, 2, 20);
      }
      ModulatableSlider("Intensity##interference", &i->visualGain,
                        "interference.visualGain", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Color");
      ImGui::Spacing();
      ImGui::Combo("Color Mode##interference", &i->colorMode,
                   "Intensity\0PerSource\0Chromatic\0");
      if (i->colorMode == 2) {
        ModulatableSlider("Chroma Spread##interference", &i->chromaSpread,
                          "interference.chromaSpread", "%.3f", modSources);
      }
      if (i->colorMode != 2) {
        ImGuiDrawColorMode(&i->color);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##interference", &i->blendIntensity,
                        "interference.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)i->blendMode;
      if (ImGui::Combo("Blend Mode##interference", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        i->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsScanBars(EffectConfig *e,
                                   const ModSources *modSources,
                                   const ImU32 categoryGlow) {
  if (DrawSectionBegin("Scan Bars", categoryGlow, &sectionScanBars)) {
    const bool wasEnabled = e->scanBars.enabled;
    ImGui::Checkbox("Enabled##scanbars", &e->scanBars.enabled);
    if (!wasEnabled && e->scanBars.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SCAN_BARS_BLEND);
    }
    if (e->scanBars.enabled) {
      ScanBarsConfig *sb = &e->scanBars;

      ImGui::Combo("Mode##scanbars", &sb->mode, "Linear\0Spokes\0Rings\0");
      if (sb->mode == 0) {
        ModulatableSliderAngleDeg("Angle##scanbars", &sb->angle,
                                  "scanBars.angle", modSources);
      }
      ModulatableSlider("Bar Density##scanbars", &sb->barDensity,
                        "scanBars.barDensity", "%.1f", modSources);
      ModulatableSlider("Convergence##scanbars", &sb->convergence,
                        "scanBars.convergence", "%.2f", modSources);
      ModulatableSlider("Conv. Frequency##scanbars", &sb->convergenceFreq,
                        "scanBars.convergenceFreq", "%.1f", modSources);
      ModulatableSlider("Conv. Offset##scanbars", &sb->convergenceOffset,
                        "scanBars.convergenceOffset", "%.2f", modSources);
      ModulatableSlider("Sharpness##scanbars", &sb->sharpness,
                        "scanBars.sharpness", "%.3f", modSources);
      ModulatableSlider("Scroll Speed##scanbars", &sb->scrollSpeed,
                        "scanBars.scrollSpeed", "%.2f", modSources);
      ModulatableSlider("Color Speed##scanbars", &sb->colorSpeed,
                        "scanBars.colorSpeed", "%.2f", modSources);
      ModulatableSlider("Chaos Frequency##scanbars", &sb->chaosFreq,
                        "scanBars.chaosFreq", "%.1f", modSources);
      ModulatableSlider("Chaos Intensity##scanbars", &sb->chaosIntensity,
                        "scanBars.chaosIntensity", "%.2f", modSources);
      ModulatableSlider("Snap Amount##scanbars", &sb->snapAmount,
                        "scanBars.snapAmount", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&sb->gradient);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##scanbars", &sb->blendIntensity,
                        "scanBars.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)sb->blendMode;
      if (ImGui::Combo("Blend Mode##scanbars", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        sb->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsPitchSpiral(EffectConfig *e,
                                      const ModSources *modSources,
                                      const ImU32 categoryGlow) {
  if (DrawSectionBegin("Pitch Spiral", categoryGlow, &sectionPitchSpiral)) {
    const bool wasEnabled = e->pitchSpiral.enabled;
    ImGui::Checkbox("Enabled##pitchspiral", &e->pitchSpiral.enabled);
    if (!wasEnabled && e->pitchSpiral.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PITCH_SPIRAL_BLEND);
    }
    if (e->pitchSpiral.enabled) {
      PitchSpiralConfig *ps = &e->pitchSpiral;

      ModulatableSlider("Base Freq (Hz)##pitchspiral", &ps->baseFreq,
                        "pitchSpiral.baseFreq", "%.1f", modSources);
      ModulatableSlider("Ring Spacing##pitchspiral", &ps->spiralSpacing,
                        "pitchSpiral.spiralSpacing", "%.3f", modSources);
      ModulatableSlider("Line Width##pitchspiral", &ps->lineWidth,
                        "pitchSpiral.lineWidth", "%.3f", modSources);
      ModulatableSlider("AA Softness##pitchspiral", &ps->blur,
                        "pitchSpiral.blur", "%.3f", modSources);
      ModulatableSlider("Gain##pitchspiral", &ps->gain, "pitchSpiral.gain",
                        "%.1f", modSources);
      ModulatableSlider("Contrast##pitchspiral", &ps->curve,
                        "pitchSpiral.curve", "%.2f", modSources);
      ImGui::SliderInt("Octaves##pitchspiral", &ps->numTurns, 4, 12);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Tilt
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Tilt");
      ImGui::Spacing();
      ModulatableSlider("Tilt##pitchspiral", &ps->tilt, "pitchSpiral.tilt",
                        "%.2f", modSources);
      ModulatableSliderAngleDeg("Tilt Angle##pitchspiral", &ps->tiltAngle,
                                "pitchSpiral.tiltAngle", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&ps->gradient);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##pitchspiral", &ps->blendIntensity,
                        "pitchSpiral.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)ps->blendMode;
      if (ImGui::Combo("Blend Mode##pitchspiral", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        ps->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawMoireLayerControls(MoireLayerConfig *lyr, int n,
                                   const ModSources *modSources) {
  char label[64];
  char paramId[64];

  (void)snprintf(label, sizeof(label), "Layer %d", n);
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s",
                     label);
  ImGui::Spacing();

  (void)snprintf(label, sizeof(label), "Frequency##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.frequency",
                 n);
  ModulatableSlider(label, &lyr->frequency, paramId, "%.1f", modSources);

  (void)snprintf(label, sizeof(label), "Angle##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.angle", n);
  ModulatableSliderAngleDeg(label, &lyr->angle, paramId, modSources);

  (void)snprintf(label, sizeof(label), "Rotation Speed##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId),
                 "moireGenerator.layer%d.rotationSpeed", n);
  ModulatableSliderAngleDeg(label, &lyr->rotationSpeed, paramId, modSources,
                            "%.1f °/s");

  (void)snprintf(label, sizeof(label), "Warp##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.warpAmount",
                 n);
  ModulatableSlider(label, &lyr->warpAmount, paramId, "%.3f", modSources);

  (void)snprintf(label, sizeof(label), "Scale##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.scale", n);
  ModulatableSlider(label, &lyr->scale, paramId, "%.2f", modSources);

  (void)snprintf(label, sizeof(label), "Phase##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.phase", n);
  ModulatableSliderAngleDeg(label, &lyr->phase, paramId, modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

static void DrawMoireOutputControls(MoireGeneratorConfig *mg,
                                    const ModSources *modSources) {
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Color");
  ImGui::Spacing();
  ImGuiDrawColorMode(&mg->gradient);
  ModulatableSlider("Color Mix##moiregen", &mg->colorIntensity,
                    "moireGenerator.colorIntensity", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ModulatableSlider("Brightness##moiregen", &mg->globalBrightness,
                    "moireGenerator.globalBrightness", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Output");
  ImGui::Spacing();
  ModulatableSlider("Blend Intensity##moiregen", &mg->blendIntensity,
                    "moireGenerator.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)mg->blendMode;
  if (ImGui::Combo("Blend Mode##moiregen", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    mg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsMoireGenerator(EffectConfig *e,
                                         const ModSources *modSources,
                                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("Moire Generator", categoryGlow,
                       &sectionMoireGenerator)) {
    const bool wasEnabled = e->moireGenerator.enabled;
    ImGui::Checkbox("Enabled##moiregen", &e->moireGenerator.enabled);
    if (!wasEnabled && e->moireGenerator.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOIRE_GENERATOR_BLEND);
    }
    if (e->moireGenerator.enabled) {
      MoireGeneratorConfig *mg = &e->moireGenerator;

      ImGui::Combo("Pattern##moiregen", &mg->patternMode,
                   "Stripes\0Circles\0Grid\0");
      ImGui::SliderInt("Layers##moiregen", &mg->layerCount, 2, 4);
      ImGui::Checkbox("Sharp##moiregen", &mg->sharpMode);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      MoireLayerConfig *layers[] = {&mg->layer0, &mg->layer1, &mg->layer2,
                                    &mg->layer3};
      for (int n = 0; n < mg->layerCount; n++) {
        DrawMoireLayerControls(layers[n], n, modSources);
      }

      DrawMoireOutputControls(mg, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSpectralArcsParams(SpectralArcsConfig *sa,
                                   const ModSources *modSources) {
  // FFT
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "FFT");
  ImGui::Spacing();
  ImGui::SliderInt("Octaves##spectralarcs", &sa->numOctaves, 1, 10);
  ModulatableSlider("Base Freq (Hz)##spectralarcs", &sa->baseFreq,
                    "spectralArcs.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##spectralarcs", &sa->gain, "spectralArcs.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##spectralarcs", &sa->curve, "spectralArcs.curve",
                    "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Ring Layout
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                     "Ring Layout");
  ImGui::Spacing();
  ModulatableSlider("Ring Scale##spectralarcs", &sa->ringScale,
                    "spectralArcs.ringScale", "%.2f", modSources);
  ModulatableSlider("Tilt##spectralarcs", &sa->tilt, "spectralArcs.tilt",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Tilt Angle##spectralarcs", &sa->tiltAngle,
                            "spectralArcs.tiltAngle", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Arc Appearance
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Arcs");
  ImGui::Spacing();
  ModulatableSlider("Arc Width##spectralarcs", &sa->arcWidth,
                    "spectralArcs.arcWidth", "%.2f", modSources);
  ModulatableSlider("Glow Intensity##spectralarcs", &sa->glowIntensity,
                    "spectralArcs.glowIntensity", "%.3f", modSources);
  ModulatableSlider("Glow Falloff##spectralarcs", &sa->glowFalloff,
                    "spectralArcs.glowFalloff", "%.1f", modSources);
  ModulatableSlider("Base Bright##spectralarcs", &sa->baseBright,
                    "spectralArcs.baseBright", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Animation
  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                     "Animation");
  ImGui::Spacing();
  ModulatableSliderAngleDeg("Rotation Speed##spectralarcs", &sa->rotationSpeed,
                            "spectralArcs.rotationSpeed", modSources,
                            "%.1f °/s");
}

static void DrawSpectralArcsOutput(SpectralArcsConfig *sa,
                                   const ModSources *modSources) {
  ImGuiDrawColorMode(&sa->gradient);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Output");
  ImGui::Spacing();
  ModulatableSlider("Blend Intensity##spectralarcs", &sa->blendIntensity,
                    "spectralArcs.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)sa->blendMode;
  if (ImGui::Combo("Blend Mode##spectralarcs", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    sa->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsSpectralArcs(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Spectral Arcs", categoryGlow, &sectionSpectralArcs)) {
    const bool wasEnabled = e->spectralArcs.enabled;
    ImGui::Checkbox("Enabled##spectralarcs", &e->spectralArcs.enabled);
    if (!wasEnabled && e->spectralArcs.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SPECTRAL_ARCS_BLEND);
    }
    if (e->spectralArcs.enabled) {
      SpectralArcsConfig *sa = &e->spectralArcs;
      DrawSpectralArcsParams(sa, modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      DrawSpectralArcsOutput(sa, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsSolidColor(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Solid Color", categoryGlow, &sectionSolidColor)) {
    const bool wasEnabled = e->solidColor.enabled;
    ImGui::Checkbox("Enabled##solidcolor", &e->solidColor.enabled);
    if (!wasEnabled && e->solidColor.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SOLID_COLOR);
    }
    if (e->solidColor.enabled) {
      SolidColorConfig *sc = &e->solidColor;

      ImGuiDrawColorMode(&sc->color);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Output
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Output");
      ImGui::Spacing();
      ModulatableSlider("Blend Intensity##solidcolor", &sc->blendIntensity,
                        "solidColor.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)sc->blendMode;
      if (ImGui::Combo("Blend Mode##solidcolor", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        sc->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources,
                            int &sectionIndex) {
  DrawGeneratorsConstellation(e, modSources,
                              Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsPlasma(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsInterference(e, modSources,
                             Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsScanBars(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsPitchSpiral(e, modSources,
                            Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsMoireGenerator(e, modSources,
                               Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsSpectralArcs(e, modSources,
                             Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsSolidColor(e, modSources,
                           Theme::GetSectionGlow(sectionIndex++));
}
