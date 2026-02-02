#include "imgui_effects_generators.h"
#include "automation/mod_sources.h"
#include "config/constellation_config.h"
#include "config/effect_config.h"
#include "config/interference_config.h"
#include "config/plasma_config.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionConstellation = false;
static bool sectionPlasma = false;
static bool sectionInterference = false;

static void DrawGeneratorsConstellation(EffectConfig *e,
                                        const ModSources *modSources,
                                        const ImU32 categoryGlow) {
  if (DrawSectionBegin("Constellation", categoryGlow, &sectionConstellation)) {
    ImGui::Checkbox("Enabled##constellation", &e->constellation.enabled);
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
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsPlasma(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  if (DrawSectionBegin("Plasma", categoryGlow, &sectionPlasma)) {
    ImGui::Checkbox("Enabled##plasma", &e->plasma.enabled);
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
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsInterference(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Interference", categoryGlow, &sectionInterference)) {
    ImGui::Checkbox("Enabled##interference", &e->interference.enabled);
    if (e->interference.enabled) {
      InterferenceConfig *i = &e->interference;

      // Sources
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Sources");
      ImGui::Spacing();
      ImGui::SliderInt("Sources##interference", &i->sourceCount, 1, 8);
      ModulatableSlider("Radius##interference", &i->baseRadius,
                        "interference.baseRadius", "%.2f", modSources);
      ModulatableSliderAngleDeg("Pattern Angle##interference", &i->patternAngle,
                                "interference.patternAngle", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Motion (DualLissajous)
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Motion");
      ImGui::Spacing();
      DualLissajousConfig *lj = &i->lissajous;
      ModulatableSlider("Amplitude##interference", &lj->amplitude,
                        "interference.lissajous.amplitude", "%.3f", modSources);
      ModulatableSlider("Freq X1##interference", &lj->freqX1,
                        "interference.lissajous.freqX1", "%.3f", modSources);
      ModulatableSlider("Freq Y1##interference", &lj->freqY1,
                        "interference.lissajous.freqY1", "%.3f", modSources);
      ModulatableSlider("Freq X2##interference", &lj->freqX2,
                        "interference.lissajous.freqX2", "%.3f", modSources);
      ModulatableSlider("Freq Y2##interference", &lj->freqY2,
                        "interference.lissajous.freqY2", "%.3f", modSources);
      ModulatableSlider("Offset X2##interference", &lj->offsetX2,
                        "interference.lissajous.offsetX2", "%.2f", modSources);
      ModulatableSlider("Offset Y2##interference", &lj->offsetY2,
                        "interference.lissajous.offsetY2", "%.2f", modSources);

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
        ImGui::SliderInt("Contours##interference", &i->contourCount, 0, 20);
      }
      ModulatableSlider("Intensity##interference", &i->visualGain,
                        "interference.visualGain", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Chromatic
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Chromatic");
      ImGui::Spacing();
      ImGui::Checkbox("Chromatic##interference", &i->chromatic);
      if (i->chromatic) {
        ModulatableSlider("Chroma Spread##interference", &i->chromaSpread,
                          "interference.chromaSpread", "%.3f", modSources);
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "Color");
      ImGui::Spacing();
      ImGui::Combo("Color Mode##interference", &i->colorMode,
                   "Intensity\0PerSource\0");
      ImGuiDrawColorMode(&i->color);
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
}
