#include "imgui_effects_generators.h"
#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/arc_strobe.h"
#include "effects/constellation.h"
#include "effects/filaments.h"
#include "effects/glyph_field.h"
#include "effects/interference.h"
#include "effects/moire_generator.h"
#include "effects/muons.h"
#include "effects/pitch_spiral.h"
#include "effects/plasma.h"
#include "effects/scan_bars.h"
#include "effects/slashes.h"
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
static bool sectionFilaments = false;
static bool sectionSlashes = false;
static bool sectionMuons = false;
static bool sectionGlyphField = false;
static bool sectionArcStrobe = false;
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

      // Wave overlay
      ImGui::SeparatorText("Wave");
      ImGui::SliderFloat("Wave Freq##constellation", &c->waveFreq, 0.1f, 5.0f,
                         "%.2f");
      ModulatableSlider("Wave Amp##constellation", &c->waveAmp,
                        "constellation.waveAmp", "%.2f", modSources);
      ModulatableSlider("Wave Speed##constellation", &c->waveSpeed,
                        "constellation.waveSpeed", "%.2f", modSources);
      ImGui::SliderFloat("Wave Center X##constellation", &c->waveCenterX, -2.0f,
                         3.0f, "%.2f");
      ImGui::SliderFloat("Wave Center Y##constellation", &c->waveCenterY, -2.0f,
                         3.0f, "%.2f");

      // Point rendering
      ImGui::SeparatorText("Points");
      ModulatableSlider("Point Size##constellation", &c->pointSize,
                        "constellation.pointSize", "%.2f", modSources);
      ModulatableSlider("Point Bright##constellation", &c->pointBrightness,
                        "constellation.pointBrightness", "%.2f", modSources);
      ImGuiDrawColorMode(&c->pointGradient);

      // Line rendering
      ImGui::SeparatorText("Lines");
      ImGui::SliderFloat("Line Width##constellation", &c->lineThickness, 0.01f,
                         0.1f, "%.3f");
      ModulatableSlider("Max Line Len##constellation", &c->maxLineLen,
                        "constellation.maxLineLen", "%.2f", modSources);
      ModulatableSlider("Line Opacity##constellation", &c->lineOpacity,
                        "constellation.lineOpacity", "%.2f", modSources);
      ImGui::Checkbox("Interpolate Line Color##constellation",
                      &c->interpolateLineColor);
      ImGuiDrawColorMode(&c->lineGradient);

      // Triangle fill
      ImGui::SeparatorText("Triangles");
      ImGui::Checkbox("Fill Triangles##constellation", &c->fillEnabled);
      if (c->fillEnabled) {
        ModulatableSlider("Fill Opacity##constellation", &c->fillOpacity,
                          "constellation.fillOpacity", "%.2f", modSources);
        ImGui::SliderFloat("Fill Threshold##constellation", &c->fillThreshold,
                           1.0f, 4.0f, "%.1f");
      }

      // Output
      ImGui::SeparatorText("Output");
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

      // Output
      ImGui::SeparatorText("Output");
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
      ImGui::SeparatorText("Sources");
      ImGui::SliderInt("Sources##interference", &i->sourceCount, 1, 8);
      ModulatableSlider("Radius##interference", &i->baseRadius,
                        "interference.baseRadius", "%.2f", modSources);

      // Motion (DualLissajous)
      ImGui::SeparatorText("Motion");
      DrawLissajousControls(&i->lissajous, "interference",
                            "interference.lissajous", modSources, 1.0f);

      // Waves
      ImGui::SeparatorText("Waves");
      ModulatableSlider("Wave Freq##interference", &i->waveFreq,
                        "interference.waveFreq", "%.1f", modSources);
      ModulatableSlider("Wave Speed##interference", &i->waveSpeed,
                        "interference.waveSpeed", "%.2f", modSources);

      // Falloff
      ImGui::SeparatorText("Falloff");
      ImGui::Combo("Falloff##interference", &i->falloffType,
                   "None\0Inverse\0InvSquare\0Gaussian\0");
      ModulatableSlider("Falloff Strength##interference", &i->falloffStrength,
                        "interference.falloffStrength", "%.2f", modSources);

      // Boundaries
      ImGui::SeparatorText("Boundaries");
      ImGui::Checkbox("Boundaries##interference", &i->boundaries);
      if (i->boundaries) {
        ModulatableSlider("Reflection##interference", &i->reflectionGain,
                          "interference.reflectionGain", "%.2f", modSources);
      }

      // Visualization
      ImGui::SeparatorText("Visualization");
      ImGui::Combo("Visual Mode##interference", &i->visualMode,
                   "Raw\0Absolute\0Contour\0");
      if (i->visualMode == 2) {
        ImGui::SliderInt("Contours##interference", &i->contourCount, 2, 20);
      }
      ModulatableSlider("Intensity##interference", &i->visualGain,
                        "interference.visualGain", "%.2f", modSources);

      // Color
      ImGui::SeparatorText("Color");
      ImGui::Combo("Color Mode##interference", &i->colorMode,
                   "Intensity\0PerSource\0Chromatic\0");
      if (i->colorMode == 2) {
        ModulatableSlider("Chroma Spread##interference", &i->chromaSpread,
                          "interference.chromaSpread", "%.3f", modSources);
      }
      if (i->colorMode != 2) {
        ImGuiDrawColorMode(&i->color);
      }

      // Output
      ImGui::SeparatorText("Output");
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

      // Output
      ImGui::SeparatorText("Output");
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

      // Tilt
      ImGui::SeparatorText("Tilt");
      ModulatableSlider("Tilt##pitchspiral", &ps->tilt, "pitchSpiral.tilt",
                        "%.2f", modSources);
      ModulatableSliderAngleDeg("Tilt Angle##pitchspiral", &ps->tiltAngle,
                                "pitchSpiral.tiltAngle", modSources);

      // Animation
      ImGui::SeparatorText("Animation");
      ModulatableSliderSpeedDeg("Rotation Speed##pitchspiral",
                                &ps->rotationSpeed, "pitchSpiral.rotationSpeed",
                                modSources);
      ModulatableSlider("Breath Speed##pitchspiral", &ps->breathSpeed,
                        "pitchSpiral.breathSpeed", "%.2f", modSources);
      ModulatableSlider("Breath Depth##pitchspiral", &ps->breathDepth,
                        "pitchSpiral.breathDepth", "%.3f", modSources);
      ModulatableSlider("Shape Exponent##pitchspiral", &ps->shapeExponent,
                        "pitchSpiral.shapeExponent", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&ps->gradient);

      // Output
      ImGui::SeparatorText("Output");
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
  ImGui::SeparatorText(label);

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
  ModulatableSliderSpeedDeg(label, &lyr->rotationSpeed, paramId, modSources);

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
  ImGui::SeparatorText("Color");
  ImGuiDrawColorMode(&mg->gradient);
  ModulatableSlider("Color Mix##moiregen", &mg->colorIntensity,
                    "moireGenerator.colorIntensity", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ModulatableSlider("Brightness##moiregen", &mg->globalBrightness,
                    "moireGenerator.globalBrightness", "%.2f", modSources);

  ImGui::SeparatorText("Output");
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
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##spectralarcs", &sa->numOctaves, 1, 10);
  ModulatableSlider("Base Freq (Hz)##spectralarcs", &sa->baseFreq,
                    "spectralArcs.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##spectralarcs", &sa->gain, "spectralArcs.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##spectralarcs", &sa->curve, "spectralArcs.curve",
                    "%.2f", modSources);

  // Ring Layout
  ImGui::SeparatorText("Ring Layout");
  ModulatableSlider("Ring Scale##spectralarcs", &sa->ringScale,
                    "spectralArcs.ringScale", "%.2f", modSources);
  ModulatableSlider("Tilt##spectralarcs", &sa->tilt, "spectralArcs.tilt",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Tilt Angle##spectralarcs", &sa->tiltAngle,
                            "spectralArcs.tiltAngle", modSources);

  // Arc Appearance
  ImGui::SeparatorText("Arcs");
  ModulatableSlider("Arc Width##spectralarcs", &sa->arcWidth,
                    "spectralArcs.arcWidth", "%.2f", modSources);
  ModulatableSlider("Glow Intensity##spectralarcs", &sa->glowIntensity,
                    "spectralArcs.glowIntensity", "%.3f", modSources);
  ModulatableSlider("Glow Falloff##spectralarcs", &sa->glowFalloff,
                    "spectralArcs.glowFalloff", "%.1f", modSources);
  ModulatableSlider("Base Bright##spectralarcs", &sa->baseBright,
                    "spectralArcs.baseBright", "%.2f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##spectralarcs", &sa->rotationSpeed,
                            "spectralArcs.rotationSpeed", modSources);
}

static void DrawSpectralArcsOutput(SpectralArcsConfig *sa,
                                   const ModSources *modSources) {
  ImGuiDrawColorMode(&sa->gradient);

  ImGui::SeparatorText("Output");
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

static void DrawFilamentsParams(FilamentsConfig *cfg,
                                const ModSources *modSources) {
  // FFT
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##filaments", &cfg->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##filaments", &cfg->baseFreq,
                    "filaments.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##filaments", &cfg->gain, "filaments.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##filaments", &cfg->curve, "filaments.curve",
                    "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Radius##filaments", &cfg->radius, "filaments.radius",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Spread##filaments", &cfg->spread,
                            "filaments.spread", modSources);
  ModulatableSliderAngleDeg("Step Angle##filaments", &cfg->stepAngle,
                            "filaments.stepAngle", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##filaments", &cfg->glowIntensity,
                    "filaments.glowIntensity", "%.3f", modSources);
  ModulatableSlider("Falloff##filaments", &cfg->falloffExponent,
                    "filaments.falloffExponent", "%.2f", modSources);
  ModulatableSlider("Base Bright##filaments", &cfg->baseBright,
                    "filaments.baseBright", "%.2f", modSources);

  // Noise
  ImGui::SeparatorText("Noise");
  ModulatableSlider("Noise Strength##filaments", &cfg->noiseStrength,
                    "filaments.noiseStrength", "%.2f", modSources);
  ModulatableSlider("Noise Speed##filaments", &cfg->noiseSpeed,
                    "filaments.noiseSpeed", "%.1f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##filaments", &cfg->rotationSpeed,
                            "filaments.rotationSpeed", modSources);
}

static void DrawFilamentsOutput(FilamentsConfig *cfg,
                                const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##filaments", &cfg->blendIntensity,
                    "filaments.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##filaments", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsFilaments(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Filaments", categoryGlow, &sectionFilaments)) {
    const bool wasEnabled = e->filaments.enabled;
    ImGui::Checkbox("Enabled##filaments", &e->filaments.enabled);
    if (!wasEnabled && e->filaments.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_FILAMENTS_BLEND);
    }
    if (e->filaments.enabled) {
      DrawFilamentsParams(&e->filaments, modSources);
      DrawFilamentsOutput(&e->filaments, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSlashesParams(SlashesConfig *cfg,
                              const ModSources *modSources) {
  // FFT
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##slashes", &cfg->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##slashes", &cfg->baseFreq,
                    "slashes.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##slashes", &cfg->gain, "slashes.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##slashes", &cfg->curve, "slashes.curve", "%.2f",
                    modSources);

  // Timing
  ImGui::SeparatorText("Timing");
  ModulatableSlider("Tick Rate##slashes", &cfg->tickRate, "slashes.tickRate",
                    "%.1f", modSources);
  ModulatableSlider("Envelope Sharp##slashes", &cfg->envelopeSharp,
                    "slashes.envelopeSharp", "%.1f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Bar Length##slashes", &cfg->maxBarLength,
                    "slashes.maxBarLength", "%.2f", modSources);
  ModulatableSlider("Bar Thickness##slashes", &cfg->barThickness,
                    "slashes.barThickness", "%.3f", modSources);
  ModulatableSlider("Thickness Var##slashes", &cfg->thicknessVariation,
                    "slashes.thicknessVariation", "%.2f", modSources);
  ModulatableSlider("Scatter##slashes", &cfg->scatter, "slashes.scatter",
                    "%.2f", modSources);
  ModulatableSlider("Rotation Depth##slashes", &cfg->rotationDepth,
                    "slashes.rotationDepth", "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Softness##slashes", &cfg->glowSoftness,
                    "slashes.glowSoftness", "%.3f", modSources);
  ModulatableSlider("Base Bright##slashes", &cfg->baseBright,
                    "slashes.baseBright", "%.2f", modSources);
}

static void DrawSlashesOutput(SlashesConfig *cfg,
                              const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##slashes", &cfg->blendIntensity,
                    "slashes.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##slashes", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsSlashes(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Slashes", categoryGlow, &sectionSlashes)) {
    const bool wasEnabled = e->slashes.enabled;
    ImGui::Checkbox("Enabled##slashes", &e->slashes.enabled);
    if (!wasEnabled && e->slashes.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SLASHES_BLEND);
    }
    if (e->slashes.enabled) {
      DrawSlashesParams(&e->slashes, modSources);
      DrawSlashesOutput(&e->slashes, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsMuons(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Muons", categoryGlow, &sectionMuons)) {
    const bool wasEnabled = e->muons.enabled;
    ImGui::Checkbox("Enabled##muons", &e->muons.enabled);
    if (!wasEnabled && e->muons.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MUONS_BLEND);
    }
    if (e->muons.enabled) {
      MuonsConfig *m = &e->muons;

      // Raymarching
      ImGui::SeparatorText("Raymarching");
      ImGui::SliderInt("March Steps##muons", &m->marchSteps, 4, 40);
      ImGui::SliderInt("Octaves##muons", &m->turbulenceOctaves, 1, 12);
      ModulatableSlider("Turbulence##muons", &m->turbulenceStrength,
                        "muons.turbulenceStrength", "%.2f", modSources);
      ModulatableSlider("Ring Thickness##muons", &m->ringThickness,
                        "muons.ringThickness", "%.3f", modSources);
      ModulatableSlider("Camera Distance##muons", &m->cameraDistance,
                        "muons.cameraDistance", "%.1f", modSources);

      // Color
      ImGui::SeparatorText("Color");
      ModulatableSlider("Color Freq##muons", &m->colorFreq, "muons.colorFreq",
                        "%.1f", modSources);
      ModulatableSlider("Color Speed##muons", &m->colorSpeed,
                        "muons.colorSpeed", "%.2f", modSources);
      ImGuiDrawColorMode(&m->gradient);

      // Tonemap
      ImGui::SeparatorText("Tonemap");
      ModulatableSlider("Brightness##muons", &m->brightness, "muons.brightness",
                        "%.2f", modSources);
      ModulatableSlider("Exposure##muons", &m->exposure, "muons.exposure",
                        "%.0f", modSources);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##muons", &m->blendIntensity,
                        "muons.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)m->blendMode;
      if (ImGui::Combo("Blend Mode##muons", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        m->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsGlyphField(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
  if (DrawSectionBegin("Glyph Field", categoryGlow, &sectionGlyphField)) {
    const bool wasEnabled = e->glyphField.enabled;
    ImGui::Checkbox("Enabled##glyphfield", &e->glyphField.enabled);
    if (!wasEnabled && e->glyphField.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_GLYPH_FIELD_BLEND);
    }
    if (e->glyphField.enabled) {
      GlyphFieldConfig *c = &e->glyphField;

      // Grid
      ImGui::SeparatorText("Grid");
      ModulatableSlider("Grid Size##glyphfield", &c->gridSize,
                        "glyphField.gridSize", "%.1f", modSources);
      ImGui::SliderInt("Layers##glyphfield", &c->layerCount, 1, 4);
      ModulatableSlider("Layer Scale##glyphfield", &c->layerScaleSpread,
                        "glyphField.layerScaleSpread", "%.2f", modSources);
      ModulatableSlider("Layer Speed##glyphfield", &c->layerSpeedSpread,
                        "glyphField.layerSpeedSpread", "%.2f", modSources);
      ModulatableSlider("Layer Opacity##glyphfield", &c->layerOpacity,
                        "glyphField.layerOpacity", "%.2f", modSources);

      // Scroll
      ImGui::SeparatorText("Scroll");
      ImGui::Combo("Scroll Dir##glyphfield", &c->scrollDirection,
                   "Horizontal\0Vertical\0Radial\0");
      ModulatableSlider("Scroll Speed##glyphfield", &c->scrollSpeed,
                        "glyphField.scrollSpeed", "%.2f", modSources);

      // Motion
      ImGui::SeparatorText("Motion");
      ModulatableSlider("Flutter##glyphfield", &c->flutterAmount,
                        "glyphField.flutterAmount", "%.2f", modSources);
      ModulatableSlider("Flutter Speed##glyphfield", &c->flutterSpeed,
                        "glyphField.flutterSpeed", "%.1f", modSources);
      ModulatableSlider("Wave Amp##glyphfield", &c->waveAmplitude,
                        "glyphField.waveAmplitude", "%.3f", modSources);
      ModulatableSlider("Wave Freq##glyphfield", &c->waveFreq,
                        "glyphField.waveFreq", "%.1f", modSources);
      ModulatableSlider("Wave Speed##glyphfield", &c->waveSpeed,
                        "glyphField.waveSpeed", "%.2f", modSources);
      ModulatableSlider("Drift##glyphfield", &c->driftAmount,
                        "glyphField.driftAmount", "%.3f", modSources);
      ModulatableSlider("Drift Speed##glyphfield", &c->driftSpeed,
                        "glyphField.driftSpeed", "%.2f", modSources);

      // Distortion
      ImGui::SeparatorText("Distortion");
      ModulatableSlider("Band Distort##glyphfield", &c->bandDistortion,
                        "glyphField.bandDistortion", "%.2f", modSources);
      ModulatableSlider("Inversion##glyphfield", &c->inversionRate,
                        "glyphField.inversionRate", "%.2f", modSources);
      ModulatableSlider("Inversion Speed##glyphfield", &c->inversionSpeed,
                        "glyphField.inversionSpeed", "%.2f", modSources);

      // LCD
      ImGui::SeparatorText("LCD");
      ImGui::Checkbox("LCD Mode##glyphfield", &c->lcdMode);
      if (c->lcdMode) {
        ModulatableSlider("LCD Freq##glyphfield", &c->lcdFreq,
                          "glyphField.lcdFreq", "%.3f", modSources);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&c->gradient);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##glyphfield", &c->blendIntensity,
                        "glyphField.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)c->blendMode;
      if (ImGui::Combo("Blend Mode##glyphfield", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        c->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawArcStrobeParams(ArcStrobeConfig *cfg,
                                const ModSources *modSources) {
  // FFT
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##arcstrobe", &cfg->numOctaves, 1, 8);
  ImGui::SliderInt("Segments/Octave##arcstrobe", &cfg->segmentsPerOctave, 4,
                   48);
  ModulatableSlider("Base Freq (Hz)##arcstrobe", &cfg->baseFreq,
                    "arcStrobe.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##arcstrobe", &cfg->gain, "arcStrobe.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##arcstrobe", &cfg->curve, "arcStrobe.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##arcstrobe", &cfg->baseBright,
                    "arcStrobe.baseBright", "%.2f", modSources);

  // Shape
  ImGui::SeparatorText("Shape");
  ModulatableSlider("Stride##arcstrobe", &cfg->orbitOffset,
                    "arcStrobe.orbitOffset", "%.2f", modSources);
  ModulatableSlider("Line Thickness##arcstrobe", &cfg->lineThickness,
                    "arcStrobe.lineThickness", "%.3f", modSources);

  // Lissajous
  ImGui::SeparatorText("Lissajous");
  DrawLissajousControls(&cfg->lissajous, "arcstrobe", "arcStrobe.lissajous",
                        modSources, 10.0f);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##arcstrobe", &cfg->glowIntensity,
                    "arcStrobe.glowIntensity", "%.1f", modSources);

  // Strobe
  ImGui::SeparatorText("Strobe");
  ModulatableSlider("Strobe Speed##arcstrobe", &cfg->strobeSpeed,
                    "arcStrobe.strobeSpeed", "%.2f", modSources);
  ModulatableSlider("Strobe Decay##arcstrobe", &cfg->strobeDecay,
                    "arcStrobe.strobeDecay", "%.1f", modSources);
  ModulatableSlider("Strobe Boost##arcstrobe", &cfg->strobeBoost,
                    "arcStrobe.strobeBoost", "%.2f", modSources);
  ImGui::SliderInt("Strobe Stride##arcstrobe", &cfg->strobeStride, 1, 12);
}

static void DrawArcStrobeOutput(ArcStrobeConfig *cfg,
                                const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##arcstrobe", &cfg->blendIntensity,
                    "arcStrobe.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##arcstrobe", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsArcStrobe(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Arc Strobe", categoryGlow, &sectionArcStrobe)) {
    const bool wasEnabled = e->arcStrobe.enabled;
    ImGui::Checkbox("Enabled##arcstrobe", &e->arcStrobe.enabled);
    if (!wasEnabled && e->arcStrobe.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_ARC_STROBE_BLEND);
    }
    if (e->arcStrobe.enabled) {
      DrawArcStrobeParams(&e->arcStrobe, modSources);
      DrawArcStrobeOutput(&e->arcStrobe, modSources);
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

      // Output
      ImGui::SeparatorText("Output");
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
  DrawGeneratorsArcStrobe(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsFilaments(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsSlashes(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsMuons(e, modSources, Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsGlyphField(e, modSources,
                           Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsSolidColor(e, modSources,
                           Theme::GetSectionGlow(sectionIndex++));
}
