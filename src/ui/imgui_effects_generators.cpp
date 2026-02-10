#include "ui/imgui_effects_generators.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"

void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources) {
  DrawGeneratorsGeometric(e, modSources);
  ImGui::Spacing();
  DrawGeneratorsFilament(e, modSources);
  ImGui::Spacing();
  DrawGeneratorsTexture(e, modSources);
  ImGui::Spacing();
  DrawGeneratorsAtmosphere(e, modSources);
}
