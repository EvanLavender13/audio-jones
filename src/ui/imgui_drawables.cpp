#include "automation/drawable_params.h"
#include "config/drawable_config.h"
#include "imgui.h"
#include "render/drawable.h"
#include "ui/drawable_type_controls.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include <stdio.h>

// Preset colors for new waveforms - from ThemeColor constants
static const Color presetColors[] = {
    ThemeColor::NEON_CYAN,          ThemeColor::NEON_MAGENTA,
    ThemeColor::NEON_ORANGE,        ThemeColor::NEON_WHITE,
    ThemeColor::NEON_CYAN_BRIGHT,   ThemeColor::NEON_MAGENTA_BRIGHT,
    ThemeColor::NEON_ORANGE_BRIGHT, ThemeColor::NEON_CYAN_DIM};

// Stable ID counter for drawables - survives reorder/delete operations
static uint32_t sNextDrawableId = 1;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires
// sequential widget calls
void ImGuiDrawDrawablesPanel(Drawable *drawables, int *count, int *selected,
                             const ModSources *sources) {
  if (!ImGui::Begin("Drawables")) {
    ImGui::End();
    return;
  }

  const int waveformCount =
      DrawableCountByType(drawables, *count, DRAWABLE_WAVEFORM);

  // Add Waveform button
  ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
  if (ImGui::Button("+ Waveform")) {
    Drawable d = {};
    d.id = sNextDrawableId++;
    d.type = DRAWABLE_WAVEFORM;
    d.path = PATH_CIRCULAR;
    d.base.color.solid = presetColors[waveformCount % 8];
    drawables[*count] = d;
    DrawableParamsRegister(&drawables[*count]);
    *selected = *count;
    (*count)++;
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  // Add Spectrum button
  ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
  if (ImGui::Button("+ Spectrum")) {
    Drawable d = {};
    d.id = sNextDrawableId++;
    d.type = DRAWABLE_SPECTRUM;
    d.path = PATH_CIRCULAR;
    d.base.color.solid = ThemeColor::NEON_MAGENTA;
    d.spectrum = SpectrumData{};
    drawables[*count] = d;
    DrawableParamsRegister(&drawables[*count]);
    *selected = *count;
    (*count)++;
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
  if (ImGui::Button("+ Shape")) {
    Drawable d = {};
    d.id = sNextDrawableId++;
    d.type = DRAWABLE_SHAPE;
    d.path = PATH_CIRCULAR;
    d.base.color.solid = ThemeColor::NEON_ORANGE;
    d.shape = ShapeData{};
    drawables[*count] = d;
    DrawableParamsRegister(&drawables[*count]);
    *selected = *count;
    (*count)++;
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(*count >= MAX_DRAWABLES);
  if (ImGui::Button("+ Trail")) {
    Drawable d = {};
    d.id = sNextDrawableId++;
    d.type = DRAWABLE_PARAMETRIC_TRAIL;
    d.path = PATH_CIRCULAR; // unused but initialize
    d.base.color.solid = ThemeColor::NEON_CYAN;
    d.parametricTrail = ParametricTrailData{};
    drawables[*count] = d;
    DrawableParamsRegister(&drawables[*count]);
    *selected = *count;
    (*count)++;
  }
  ImGui::EndDisabled();

  ImGui::Spacing();

  // Rich custom-drawn drawable list with drag-drop reorder
  if (ImGui::BeginChild("##DrawableList",
                        ImVec2(-1, 8 * ImGui::GetFrameHeightWithSpacing()),
                        ImGuiChildFlags_Borders)) {

    ImDrawList *draw = ImGui::GetWindowDrawList();

    // Suppress Selectable built-in backgrounds - we draw our own
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

    const float textH = ImGui::GetTextLineHeight();
    const float indexW = ImGui::CalcTextSize("00").x;
    const float swatchSize = 8.0f;

    int waveformIdx = 0;
    int spectrumIdx = 0;
    int shapeIdx = 0;
    int trailIdx = 0;

    for (int i = 0; i < *count; i++) {
      ImGui::PushID(i);

      const Drawable *d = &drawables[i];

      // Build per-type index and display info
      const char *typeBadge = "W";
      const char *typeName = "Waveform";
      ImU32 badgeColor = Theme::ACCENT_CYAN_U32;
      int typeIdx = 0;
      if (d->type == DRAWABLE_WAVEFORM) {
        waveformIdx++;
        typeIdx = waveformIdx;
        typeBadge = "W";
        typeName = "Waveform";
        badgeColor = Theme::ACCENT_CYAN_U32;
      } else if (d->type == DRAWABLE_SPECTRUM) {
        spectrumIdx++;
        typeIdx = spectrumIdx;
        typeBadge = "S";
        typeName = "Spectrum";
        badgeColor = Theme::ACCENT_MAGENTA_U32;
      } else if (d->type == DRAWABLE_SHAPE) {
        shapeIdx++;
        typeIdx = shapeIdx;
        typeBadge = "P";
        typeName = "Shape";
        badgeColor = Theme::ACCENT_ORANGE_U32;
      } else if (d->type == DRAWABLE_PARAMETRIC_TRAIL) {
        trailIdx++;
        typeIdx = trailIdx;
        typeBadge = "T";
        typeName = "Trail";
        badgeColor = Theme::ACCENT_CYAN_U32;
      }

      const float contentWidth = ImGui::GetContentRegionAvail().x;

      // Invisible Selectable owns the full row
      if (ImGui::Selectable("##row", *selected == i,
                            ImGuiSelectableFlags_AllowOverlap,
                            ImVec2(contentWidth, 0))) {
        *selected = i;
      }

      const ImVec2 rowMin = ImGui::GetItemRectMin();
      const ImVec2 rowMax = ImGui::GetItemRectMax();
      const float rowH = rowMax.y - rowMin.y;
      const float textY = rowMin.y + (rowH - textH) * 0.5f;
      const bool rowHovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);

      // Row background
      if (*selected == i) {
        draw->AddRectFilled(rowMin, rowMax,
                            SetColorAlpha(Theme::ACCENT_CYAN_U32, 32));
      } else if (rowHovered) {
        draw->AddRectFilled(rowMin, rowMax,
                            SetColorAlpha(Theme::ACCENT_CYAN_U32, 14));
      }

      // Resolve swatch color from drawable's color mode
      Color swatchColor;
      if (d->base.color.mode == COLOR_MODE_SOLID) {
        swatchColor = d->base.color.solid;
      } else if (d->base.color.mode == COLOR_MODE_GRADIENT &&
                 d->base.color.gradientStopCount > 0) {
        swatchColor = d->base.color.gradientStops[0].color;
      } else {
        swatchColor = {0, 255, 255, 255};
      }

      // Color swatch (8x8)
      float curX = rowMin.x + 4.0f;
      const float swatchY = rowMin.y + (rowH - swatchSize) * 0.5f;
      draw->AddRectFilled(
          ImVec2(curX, swatchY),
          ImVec2(curX + swatchSize, swatchY + swatchSize),
          IM_COL32(swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a));
      curX += swatchSize + 6.0f;

      // Index number (dim)
      char indexBuf[8];
      (void)snprintf(indexBuf, sizeof(indexBuf), "%2d", i + 1);
      draw->AddText(ImVec2(curX, textY), Theme::TEXT_DISABLED_U32, indexBuf);
      curX += indexW + 6.0f;

      // Type badge letter in accent color
      draw->AddText(ImVec2(curX, textY), badgeColor, typeBadge);
      curX += ImGui::CalcTextSize(typeBadge).x + 6.0f;

      // Name text (dimmed when disabled)
      char nameBuf[32];
      (void)snprintf(nameBuf, sizeof(nameBuf), "%s %d", typeName, typeIdx);
      const ImU32 nameColor =
          d->base.enabled ? Theme::TEXT_PRIMARY_U32 : Theme::TEXT_DISABLED_U32;
      draw->AddText(ImVec2(curX, textY), nameColor, nameBuf);

      // Drag-drop source
      if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DRAWABLE_ITEM", &i, sizeof(int));
        ImGui::Text("%s", nameBuf);
        ImGui::EndDragDropSource();
      }

      // Drag-drop target
      if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("DRAWABLE_ITEM");
        if (payload != nullptr) {
          const int srcIndex = *static_cast<const int *>(payload->Data);
          const Drawable temp = drawables[srcIndex];
          drawables[srcIndex] = drawables[i];
          drawables[i] = temp;
          if (*selected == srcIndex) {
            *selected = i;
          } else if (*selected == i) {
            *selected = srcIndex;
          }
          DrawableParamsSyncAll(drawables, *count);
        }

        draw->AddLine(ImVec2(rowMin.x, rowMax.y), ImVec2(rowMax.x, rowMax.y),
                      Theme::ACCENT_CYAN_U32, 2.0f);

        ImGui::EndDragDropTarget();
      }

      // Hover-only remove button
      if (rowHovered) {
        ImGui::SameLine(contentWidth - 20.0f);
        if (ImGui::SmallButton("x")) {
          const uint32_t deletedId = drawables[i].id;
          DrawableParamsUnregister(deletedId);
          for (int j = i; j < *count - 1; j++) {
            drawables[j] = drawables[j + 1];
          }
          (*count)--;
          if (*selected >= *count) {
            *selected = *count - 1;
          }
          DrawableParamsSyncAll(drawables, *count);
          ImGui::PopID();
          break;
        }
      }

      ImGui::PopID();
    }

    ImGui::PopStyleColor(3);
  }
  ImGui::EndChild();

  // Selected drawable settings
  if (*selected >= 0 && *selected < *count) {
    Drawable *sel = &drawables[*selected];

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Enabled toggle
    ImGui::Checkbox("Enabled", &sel->base.enabled);
    ImGui::Spacing();

    // Path selector (applies to waveform and spectrum)
    if (sel->type == DRAWABLE_WAVEFORM || sel->type == DRAWABLE_SPECTRUM) {
      const char *pathItems[] = {"Linear", "Circular"};
      int pathIdx = (int)sel->path;
      if (ImGui::Combo("Path", &pathIdx, pathItems, 2)) {
        sel->path = (DrawablePath)pathIdx;
      }
      ImGui::Spacing();
    }

    // Type-specific controls
    if (sel->type == DRAWABLE_WAVEFORM) {
      DrawWaveformControls(sel, sources);
    } else if (sel->type == DRAWABLE_SPECTRUM) {
      DrawSpectrumControls(sel, sources);
    } else if (sel->type == DRAWABLE_SHAPE) {
      DrawShapeControls(sel, sources);
    } else if (sel->type == DRAWABLE_PARAMETRIC_TRAIL) {
      DrawParametricTrailControls(sel, sources);
    }
  }

  ImGui::End();
}

void ImGuiDrawDrawablesSyncIdCounter(const Drawable *drawables, int count) {
  uint32_t maxId = 0;
  for (int i = 0; i < count; i++) {
    if (drawables[i].id > maxId) {
      maxId = drawables[i].id;
    }
  }
  sNextDrawableId = maxId + 1;
}
