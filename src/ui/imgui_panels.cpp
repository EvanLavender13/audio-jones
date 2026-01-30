#include "ui/imgui_panels.h"
#include "imgui.h"
#include "ui/theme.h"

// NOLINTNEXTLINE(readability-function-size) - theme setup requires setting all
// ImGui style colors
void ImGuiApplyNeonTheme(void) {
  ImGuiStyle *style = &ImGui::GetStyle();

  // Sharp, modern geometry
  style->WindowRounding = 0.0f;
  style->FrameRounding = 2.0f;
  style->GrabRounding = 2.0f;
  style->TabRounding = 0.0f;
  style->ScrollbarRounding = 0.0f;

  // Spacing for breathing room
  style->FramePadding = ImVec2(8, 5);
  style->ItemSpacing = ImVec2(8, 6);
  style->ItemInnerSpacing = ImVec2(6, 4);
  style->WindowPadding = ImVec2(10, 10);

  // Subtle borders for definition
  style->WindowBorderSize = 1.0f;
  style->FrameBorderSize = 1.0f;
  style->TabBorderSize = 1.0f;

  // Scrollbar sizing
  style->ScrollbarSize = 12.0f;
  style->GrabMinSize = 10.0f;

  ImVec4 *colors = style->Colors;

  // Backgrounds - deep cosmic blue-black
  colors[ImGuiCol_WindowBg] = Theme::BG_DEEP;
  colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_PopupBg] = Theme::BG_MID;
  colors[ImGuiCol_FrameBg] = Theme::BG_MID;
  colors[ImGuiCol_FrameBgHovered] = Theme::BG_SURFACE;
  colors[ImGuiCol_FrameBgActive] = Theme::BG_SURFACE;

  // Title bars - subtle with cyan accent on active
  colors[ImGuiCol_TitleBg] = Theme::BG_VOID;
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.07f, 0.12f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = Theme::BG_VOID;

  // Borders - muted purple with subtle presence
  colors[ImGuiCol_Border] = Theme::BORDER;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Text - warm off-white hierarchy
  colors[ImGuiCol_Text] = Theme::TEXT_PRIMARY;
  colors[ImGuiCol_TextDisabled] = Theme::TEXT_DISABLED;

  // Buttons - cyan accented
  colors[ImGuiCol_Button] = Theme::BG_SURFACE;
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(Theme::ACCENT_CYAN_DIM.x, Theme::ACCENT_CYAN_DIM.y,
             Theme::ACCENT_CYAN_DIM.z, 0.40f);
  colors[ImGuiCol_ButtonActive] = ImVec4(
      Theme::ACCENT_CYAN.x, Theme::ACCENT_CYAN.y, Theme::ACCENT_CYAN.z, 0.50f);

  // Headers (collapsing sections) - magenta accent
  colors[ImGuiCol_Header] =
      ImVec4(Theme::ACCENT_MAGENTA_DIM.x, Theme::ACCENT_MAGENTA_DIM.y,
             Theme::ACCENT_MAGENTA_DIM.z, 0.35f);
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(Theme::ACCENT_MAGENTA.x, Theme::ACCENT_MAGENTA.y,
             Theme::ACCENT_MAGENTA.z, 0.45f);
  colors[ImGuiCol_HeaderActive] =
      ImVec4(Theme::ACCENT_MAGENTA.x, Theme::ACCENT_MAGENTA.y,
             Theme::ACCENT_MAGENTA.z, 0.55f);

  // Sliders/Grabs - cyan primary
  colors[ImGuiCol_SliderGrab] = Theme::ACCENT_CYAN_DIM;
  colors[ImGuiCol_SliderGrabActive] = Theme::ACCENT_CYAN;

  // Checkmarks and selection - cyan
  colors[ImGuiCol_CheckMark] = Theme::ACCENT_CYAN;

  // Scrollbars - subtle
  colors[ImGuiCol_ScrollbarBg] = Theme::BG_VOID;
  colors[ImGuiCol_ScrollbarGrab] = Theme::BG_SURFACE;
  colors[ImGuiCol_ScrollbarGrabHovered] = Theme::BORDER;
  colors[ImGuiCol_ScrollbarGrabActive] = Theme::ACCENT_CYAN_DIM;

  // Separators
  colors[ImGuiCol_Separator] = Theme::BORDER;
  colors[ImGuiCol_SeparatorHovered] = Theme::ACCENT_CYAN_DIM;
  colors[ImGuiCol_SeparatorActive] = Theme::ACCENT_CYAN;

  // Resize grip - orange accent
  colors[ImGuiCol_ResizeGrip] =
      ImVec4(Theme::ACCENT_ORANGE_DIM.x, Theme::ACCENT_ORANGE_DIM.y,
             Theme::ACCENT_ORANGE_DIM.z, 0.30f);
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(Theme::ACCENT_ORANGE.x, Theme::ACCENT_ORANGE.y,
             Theme::ACCENT_ORANGE.z, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = Theme::ACCENT_ORANGE;

  // Tabs - cyan active, muted inactive
  colors[ImGuiCol_Tab] = Theme::BG_VOID;
  colors[ImGuiCol_TabHovered] =
      ImVec4(Theme::ACCENT_CYAN_DIM.x, Theme::ACCENT_CYAN_DIM.y,
             Theme::ACCENT_CYAN_DIM.z, 0.50f);
  colors[ImGuiCol_TabActive] =
      ImVec4(Theme::ACCENT_CYAN_DIM.x, Theme::ACCENT_CYAN_DIM.y,
             Theme::ACCENT_CYAN_DIM.z, 0.35f);
  colors[ImGuiCol_TabUnfocused] = Theme::BG_VOID;
  colors[ImGuiCol_TabUnfocusedActive] = Theme::BG_MID;

  // Docking
  colors[ImGuiCol_DockingPreview] = ImVec4(
      Theme::ACCENT_CYAN.x, Theme::ACCENT_CYAN.y, Theme::ACCENT_CYAN.z, 0.40f);
  colors[ImGuiCol_DockingEmptyBg] = Theme::BG_VOID;

  // Plot colors for graphs
  colors[ImGuiCol_PlotLines] = Theme::ACCENT_CYAN;
  colors[ImGuiCol_PlotLinesHovered] = Theme::ACCENT_CYAN_HOVER;
  colors[ImGuiCol_PlotHistogram] = Theme::ACCENT_MAGENTA;
  colors[ImGuiCol_PlotHistogramHovered] = Theme::ACCENT_MAGENTA_HOVER;

  // Table colors
  colors[ImGuiCol_TableHeaderBg] = Theme::BG_MID;
  colors[ImGuiCol_TableBorderStrong] = Theme::BORDER;
  colors[ImGuiCol_TableBorderLight] =
      ImVec4(Theme::BORDER.x, Theme::BORDER.y, Theme::BORDER.z, 0.50f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

  // Input text selection - magenta
  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(Theme::ACCENT_MAGENTA.x, Theme::ACCENT_MAGENTA.y,
             Theme::ACCENT_MAGENTA.z, 0.35f);

  // Drag/drop and navigation
  colors[ImGuiCol_DragDropTarget] = Theme::ACCENT_ORANGE;
  colors[ImGuiCol_NavHighlight] = Theme::ACCENT_CYAN;
}

void ImGuiDrawDockspace(void) {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", NULL, flags);
  ImGui::PopStyleVar(3);

  const ImGuiID dockspaceId = ImGui::GetID("MainDockspace");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f),
                   ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::End();
}
