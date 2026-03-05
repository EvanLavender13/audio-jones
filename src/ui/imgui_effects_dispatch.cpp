#include "ui/imgui_effects_dispatch.h"

#include "automation/mod_sources.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"

#include <stdlib.h>
#include <string.h>

// Section toggle state: one bool per TransformEffectType
bool g_effectSectionOpen[TRANSFORM_EFFECT_COUNT] = {};

// Maps categorySectionIndex to display name and glow color index
struct CategoryInfo {
  const char *name;
  int glowIndex;
};

static const CategoryInfo CATEGORY_INFO[] = {
    {/* 0  */ "Symmetry", 0},  {/* 1  */ "Warp", 1},
    {/* 2  */ "Cellular", 2},  {/* 3  */ "Motion", 3},
    {/* 4  */ "Painterly", 4}, {/* 5  */ "Print", 5},
    {/* 6  */ "Retro", 7},     {/* 7  */ "Optical", 8},
    {/* 8  */ "Color", 9},     {/* 9  */ "Simulation", 4},
    {/* 10 */ "Geometric", 2}, {/* 11 */ "Filament", 1},
    {/* 12 */ "Texture", 4},   {/* 13 */ "Field", 0},
    {/* 14 */ "Novelty", 6},   {/* 15 */ "Scatter", 3},
    {/* 16 */ "Cymatics", 5},
};

static const int CATEGORY_INFO_COUNT =
    (int)(sizeof(CATEGORY_INFO) / sizeof(CATEGORY_INFO[0]));

void DrawEffectCategory(EffectConfig *e, const ModSources *modSources,
                        int sectionIndex) {
  if (sectionIndex < 0 || sectionIndex >= CATEGORY_INFO_COUNT)
    return;
  const CategoryInfo &cat = CATEGORY_INFO[sectionIndex];
  if (cat.name == nullptr)
    return;

  ImU32 categoryGlow = Theme::GetSectionGlow(cat.glowIndex);
  DrawCategoryHeader(cat.name, categoryGlow);

  // Collect effects in this section, then sort alphabetically by name
  int indices[TRANSFORM_EFFECT_COUNT];
  int count = 0;
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (EFFECT_DESCRIPTORS[i].categorySectionIndex == sectionIndex)
      indices[count++] = i;
  }
  qsort(indices, count, sizeof(int), [](const void *a, const void *b) -> int {
    return strcmp(EFFECT_DESCRIPTORS[*(const int *)a].name,
                  EFFECT_DESCRIPTORS[*(const int *)b].name);
  });

  for (int j = 0; j < count; j++) {
    int i = indices[j];
    const EffectDescriptor &desc = EFFECT_DESCRIPTORS[i];
    bool *enabled = (bool *)((char *)e + desc.enabledOffset);

    ImGui::PushID(desc.type);
    if (DrawSectionBegin(desc.name, categoryGlow,
                         &g_effectSectionOpen[desc.type], *enabled)) {
      bool wasEnabled = *enabled;
      ImGui::Checkbox("Enabled", enabled);
      if (!wasEnabled && *enabled) {
        MoveTransformToEnd(&e->transformOrder, desc.type);
      }
      if (*enabled && desc.drawParams != nullptr) {
        desc.drawParams(e, modSources, categoryGlow);
      }
      if (*enabled && desc.drawOutput != nullptr) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        desc.drawOutput(e, modSources);
      }
      DrawSectionEnd();
    }
    ImGui::PopID();
    ImGui::Spacing();
  }
}
