#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/glyph_field.h"
#include "effects/interference.h"
#include "effects/moire_generator.h"
#include "effects/motherboard.h"
#include "effects/plasma.h"
#include "effects/scan_bars.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_effects_generators.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionPlasma = false;
static bool sectionInterference = false;
static bool sectionMoireGenerator = false;
static bool sectionMotherboard = false;
static bool sectionScanBars = false;
static bool sectionGlyphField = false;

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

      // Audio
      ImGui::SeparatorText("Audio");
      ImGui::SliderInt("Octaves##scanbars", &sb->numOctaves, 1, 8);
      ModulatableSlider("Base Freq (Hz)##scanbars", &sb->baseFreq,
                        "scanBars.baseFreq", "%.1f", modSources);
      ModulatableSlider("Gain##scanbars", &sb->gain, "scanBars.gain", "%.1f",
                        modSources);
      ModulatableSlider("Contrast##scanbars", &sb->curve, "scanBars.curve",
                        "%.2f", modSources);
      ModulatableSlider("Base Bright##scanbars", &sb->baseBright,
                        "scanBars.baseBright", "%.2f", modSources);

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

      // Audio
      ImGui::SeparatorText("Audio");
      ImGui::SliderInt("Octaves##glyphfield", &c->numOctaves, 1, 8);
      ModulatableSlider("Base Freq (Hz)##glyphfield", &c->baseFreq,
                        "glyphField.baseFreq", "%.1f", modSources);
      ModulatableSlider("Gain##glyphfield", &c->gain, "glyphField.gain", "%.1f",
                        modSources);
      ModulatableSlider("Contrast##glyphfield", &c->curve, "glyphField.curve",
                        "%.2f", modSources);
      ModulatableSlider("Base Bright##glyphfield", &c->baseBright,
                        "glyphField.baseBright", "%.2f", modSources);

      // Scroll
      ImGui::SeparatorText("Scroll");
      ImGui::Combo("Scroll Dir##glyphfield", &c->scrollDirection,
                   "Horizontal\0Vertical\0Radial\0");
      ModulatableSlider("Scroll Speed##glyphfield", &c->scrollSpeed,
                        "glyphField.scrollSpeed", "%.2f", modSources);

      // Stutter
      ImGui::SeparatorText("Stutter");
      ModulatableSlider("Stutter##glyphfield", &c->stutterAmount,
                        "glyphField.stutterAmount", "%.2f", modSources);
      ModulatableSlider("Stutter Speed##glyphfield", &c->stutterSpeed,
                        "glyphField.stutterSpeed", "%.2f", modSources);
      ModulatableSlider("Discrete##glyphfield", &c->stutterDiscrete,
                        "glyphField.stutterDiscrete", "%.2f", modSources);

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

static void DrawGeneratorsMotherboard(EffectConfig *e,
                                      const ModSources *modSources,
                                      const ImU32 categoryGlow) {
  if (DrawSectionBegin("Motherboard", categoryGlow, &sectionMotherboard)) {
    const bool wasEnabled = e->motherboard.enabled;
    ImGui::Checkbox("Enabled##motherboard", &e->motherboard.enabled);
    if (!wasEnabled && e->motherboard.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOTHERBOARD_BLEND);
    }
    if (e->motherboard.enabled) {
      MotherboardConfig *cfg = &e->motherboard;

      // Audio
      ImGui::SeparatorText("Audio");
      ImGui::SliderInt("Octaves##motherboard", &cfg->numOctaves, 1, 8);
      ModulatableSlider("Base Freq (Hz)##motherboard", &cfg->baseFreq,
                        "motherboard.baseFreq", "%.1f", modSources);
      ModulatableSlider("Gain##motherboard", &cfg->gain, "motherboard.gain",
                        "%.1f", modSources);
      ModulatableSlider("Contrast##motherboard", &cfg->curve,
                        "motherboard.curve", "%.2f", modSources);
      ModulatableSlider("Base Bright##motherboard", &cfg->baseBright,
                        "motherboard.baseBright", "%.2f", modSources);

      // Geometry
      ImGui::SeparatorText("Geometry");
      ImGui::SliderInt("Iterations##motherboard", &cfg->iterations, 4, 16);
      ModulatableSlider("Range X##motherboard", &cfg->rangeX,
                        "motherboard.rangeX", "%.2f", modSources);
      ModulatableSlider("Range Y##motherboard", &cfg->rangeY,
                        "motherboard.rangeY", "%.2f", modSources);
      ModulatableSlider("Size##motherboard", &cfg->size, "motherboard.size",
                        "%.2f", modSources);
      ModulatableSlider("Fall Off##motherboard", &cfg->fallOff,
                        "motherboard.fallOff", "%.2f", modSources);
      ModulatableSliderAngleDeg("Rotation##motherboard", &cfg->rotAngle,
                                "motherboard.rotAngle", modSources);

      // Glow
      ImGui::SeparatorText("Glow");
      ModulatableSliderLog("Glow Intensity##motherboard", &cfg->glowIntensity,
                           "motherboard.glowIntensity", "%.3f", modSources);
      ModulatableSlider("Accent##motherboard", &cfg->accentIntensity,
                        "motherboard.accentIntensity", "%.3f", modSources);

      // Animation
      ImGui::SeparatorText("Animation");
      ModulatableSliderSpeedDeg("Rotation Speed##motherboard",
                                &cfg->rotationSpeed,
                                "motherboard.rotationSpeed", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&cfg->gradient);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##motherboard", &cfg->blendIntensity,
                        "motherboard.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)cfg->blendMode;
      if (ImGui::Combo("Blend Mode##motherboard", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        cfg->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

void DrawGeneratorsTexture(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(2);
  DrawCategoryHeader("Texture", categoryGlow);
  DrawGeneratorsPlasma(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsInterference(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsMoireGenerator(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsScanBars(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsGlyphField(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsMotherboard(e, modSources, categoryGlow);
}
