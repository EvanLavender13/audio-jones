#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_effects_dispatch.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

// Persistent section open states
static bool sectionFlowField = false;

// Selection tracking for effect order list
static int selectedTransformEffect = -1;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires
// sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig *e, const ModSources *modSources) {
  if (!ImGui::Begin("Effects")) {
    ImGui::End();
    return;
  }

  int groupIdx = 0;

  // -------------------------------------------------------------------------
  // FEEDBACK GROUP
  // -------------------------------------------------------------------------
  DrawGroupHeader("FEEDBACK", Theme::GetSectionAccent(groupIdx++));

  ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px",
                    modSources);
  ModulatableSliderLog("Motion", &e->motionScale, "effects.motionScale", "%.3f",
                       modSources);
  ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
  ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);

  ImGui::Spacing();

  if (DrawSectionBegin("Flow Field", Theme::GetSectionGlow(0),
                       &sectionFlowField)) {
    ImGui::SeparatorText("Base");
    ModulatableSlider("Zoom##base", &e->flowField.zoomBase,
                      "flowField.zoomBase", "%.4f", modSources);
    ModulatableSliderSpeedDeg("Spin##base", &e->flowField.rotationSpeed,
                              "flowField.rotationSpeed", modSources);
    ModulatableSlider("DX##base", &e->flowField.dxBase, "flowField.dxBase",
                      "%.4f", modSources);
    ModulatableSlider("DY##base", &e->flowField.dyBase, "flowField.dyBase",
                      "%.4f", modSources);

    ImGui::SeparatorText("Radial");
    ModulatableSlider("Zoom##radial", &e->flowField.zoomRadial,
                      "flowField.zoomRadial", "%.4f", modSources);
    ModulatableSliderSpeedDeg("Spin##radial", &e->flowField.rotationSpeedRadial,
                              "flowField.rotationSpeedRadial", modSources);
    ModulatableSlider("DX##radial", &e->flowField.dxRadial,
                      "flowField.dxRadial", "%.4f", modSources);
    ModulatableSlider("DY##radial", &e->flowField.dyRadial,
                      "flowField.dyRadial", "%.4f", modSources);

    ImGui::SeparatorText("Angular");
    ModulatableSlider("Zoom##angular", &e->flowField.zoomAngular,
                      "flowField.zoomAngular", "%.4f", modSources);
    ImGui::SliderInt("Zoom Freq", &e->flowField.zoomAngularFreq, 1, 8);
    ModulatableSliderSpeedDeg("Spin##angular", &e->flowField.rotAngular,
                              "flowField.rotAngular", modSources);
    ImGui::SliderInt("Spin Freq", &e->flowField.rotAngularFreq, 1, 8);
    ModulatableSlider("DX##angular", &e->flowField.dxAngular,
                      "flowField.dxAngular", "%.4f", modSources);
    ImGui::SliderInt("DX Freq", &e->flowField.dxAngularFreq, 1, 8);
    ModulatableSlider("DY##angular", &e->flowField.dyAngular,
                      "flowField.dyAngular", "%.4f", modSources);
    ImGui::SliderInt("DY Freq", &e->flowField.dyAngularFreq, 1, 8);

    ImGui::SeparatorText("Center");
    ModulatableSlider("CX", &e->flowField.cx, "flowField.cx", "%.2f",
                      modSources);
    ModulatableSlider("CY", &e->flowField.cy, "flowField.cy", "%.2f",
                      modSources);

    ImGui::SeparatorText("Stretch");
    ModulatableSlider("SX", &e->flowField.sx, "flowField.sx", "%.3f",
                      modSources);
    ModulatableSlider("SY", &e->flowField.sy, "flowField.sy", "%.3f",
                      modSources);

    ImGui::SeparatorText("Warp");
    ModulatableSlider("Warp", &e->proceduralWarp.warp, "proceduralWarp.warp",
                      "%.2f", modSources);
    ModulatableSlider("Warp Speed", &e->proceduralWarp.warpSpeed,
                      "proceduralWarp.warpSpeed", "%.2f", modSources);
    ModulatableSlider("Warp Scale", &e->proceduralWarp.warpScale,
                      "proceduralWarp.warpScale", "%.1f", modSources);

    ImGui::SeparatorText("Gradient Flow");
    ModulatableSlider("Strength", &e->feedbackFlow.strength,
                      "feedbackFlow.strength", "%.1f px", modSources);
    ModulatableSliderAngleDeg("Flow Angle", &e->feedbackFlow.flowAngle,
                              "feedbackFlow.flowAngle", modSources);
    ModulatableSlider("Scale", &e->feedbackFlow.scale, "feedbackFlow.scale",
                      "%.1f", modSources);
    ModulatableSlider("Threshold", &e->feedbackFlow.threshold,
                      "feedbackFlow.threshold", "%.3f", modSources);
    DrawSectionEnd();
  }

  // -------------------------------------------------------------------------
  // OUTPUT GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("OUTPUT", Theme::GetSectionAccent(groupIdx++));

  ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
  ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

  // -------------------------------------------------------------------------
  // SIMULATIONS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("SIMULATIONS", Theme::GetSectionAccent(groupIdx++));
  DrawEffectCategory(e, modSources, 9);

  // -------------------------------------------------------------------------
  // GENERATORS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("GENERATORS", Theme::GetSectionAccent(groupIdx++));
  DrawEffectCategory(e, modSources, 16); // Cymatics
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 13); // Field
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 11); // Filament
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 10); // Geometric
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 15); // Scatter
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 12); // Texture

  // -------------------------------------------------------------------------
  // TRANSFORMS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("TRANSFORMS", Theme::GetSectionAccent(groupIdx++));

  // Pipeline list - shows only enabled effects
  if (ImGui::BeginListBox("##PipelineList", ImVec2(-FLT_MIN, 120))) {
    const float listWidth = ImGui::GetContentRegionAvail().x;
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    int visibleRow = 0;

    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
      const TransformEffectType type = e->transformOrder[i];
      if (!IsTransformEnabled(e, type)) {
        continue;
      }

      const char *name = TransformEffectName(type);
      const EffectCategory cat = EffectDescriptorCategory(type);
      const bool isSelected = (selectedTransformEffect == i);

      ImGui::PushID(i);

      // Get row bounds for background drawing
      const ImVec2 rowMin = ImGui::GetCursorScreenPos();
      const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
      const ImVec2 rowMax = ImVec2(rowMin.x + listWidth, rowMin.y + rowHeight);

      // Alternating row background (subtle)
      if (visibleRow % 2 == 1) {
        drawList->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 8));
      }

      // Full-width selectable (provides highlight and drag source)
      if (ImGui::Selectable("", isSelected, ImGuiSelectableFlags_AllowOverlap,
                            ImVec2(listWidth, 0))) {
        selectedTransformEffect = i;
      }

      // Drag source
      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        ImGui::SetDragDropPayload("TRANSFORM_ORDER", &i, sizeof(int));
        ImGui::Text("%s", name);
        ImGui::EndDragDropSource();
      }

      // Drop target with cyan highlight
      if (ImGui::BeginDragDropTarget()) {
        // Draw cyan highlight on drop target
        drawList->AddRectFilled(rowMin, rowMax,
                                SetColorAlpha(Theme::ACCENT_CYAN_U32, 60));
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("TRANSFORM_ORDER")) {
          const int srcIdx = *(const int *)payload->Data;
          if (srcIdx != i) {
            // Move: remove from srcIdx, insert at i
            const TransformEffectType moving = e->transformOrder[srcIdx];
            if (srcIdx < i) {
              // Shift left: items between src and dst move down
              for (int j = srcIdx; j < i; j++) {
                e->transformOrder[j] = e->transformOrder[j + 1];
              }
            } else {
              // Shift right: items between dst and src move up
              for (int j = srcIdx; j > i; j--) {
                e->transformOrder[j] = e->transformOrder[j - 1];
              }
            }
            e->transformOrder[i] = moving;
            selectedTransformEffect = i;
          }
        }
        ImGui::EndDragDropTarget();
      }

      // Overlay content on same line
      ImGui::SameLine(4);

      // Drag handle (dimmed)
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(
                                               Theme::TEXT_SECONDARY_U32));
      ImGui::Text("::");
      ImGui::PopStyleColor();
      ImGui::SameLine();

      // Effect name
      ImGui::Text("%s", name);

      // Category badge (colored, right-aligned) - uses same color cycle as
      // section headers
      ImGui::SameLine(listWidth - 35);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::ColorConvertU32ToFloat4(
                                Theme::GetSectionAccent(cat.sectionIndex)));
      ImGui::Text("%s", cat.badge);
      ImGui::PopStyleColor();

      ImGui::PopID();
      visibleRow++;
    }

    if (visibleRow == 0) {
      ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_SECONDARY);
      ImGui::Text("No effects enabled");
      ImGui::PopStyleColor();
    }

    ImGui::EndListBox();
  }

  ImGui::SeparatorText("Trails Blend");
  ModulatableSlider("Blend Intensity##accumComposite", &e->accumBlendIntensity,
                    "effects.accumBlendIntensity", "%.2f", modSources);
  int blendModeInt = (int)e->accumBlendMode;
  if (ImGui::Combo("Blend Mode##accumComposite", &blendModeInt,
                   BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
    e->accumBlendMode = (EffectBlendMode)blendModeInt;
  }

  // Transform subcategories (extracted to imgui_effects_transforms.cpp)
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 0);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 1);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 2);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 3);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 4);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 5);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 14);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 6);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 7);
  ImGui::Spacing();
  DrawEffectCategory(e, modSources, 8);

  ImGui::End();
}
