#include "imgui_effects_generators.h"
#include "automation/mod_sources.h"
#include "config/constellation_config.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"

static bool sectionConstellation = false;

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
      ImGui::SliderFloat("Glow Scale##constellation", &c->glowScale, 50.0f,
                         500.0f, "%.0f");
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

void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(10);
  DrawCategoryHeader("Generators", categoryGlow);
  DrawGeneratorsConstellation(e, modSources, categoryGlow);
}
