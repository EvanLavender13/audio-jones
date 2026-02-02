#include "imgui_effects_generators.h"
#include "automation/mod_sources.h"
#include "config/constellation_config.h"
#include "config/effect_config.h"
#include "config/plasma_config.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"

static bool sectionConstellation = false;
static bool sectionPlasma = false;

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

void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources,
                            int &sectionIndex) {
  DrawGeneratorsConstellation(e, modSources,
                              Theme::GetSectionGlow(sectionIndex++));
  ImGui::Spacing();
  DrawGeneratorsPlasma(e, modSources, Theme::GetSectionGlow(sectionIndex++));
}
