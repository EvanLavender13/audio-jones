#include "config/app_configs.h"
#include "config/playlist.h"
#include "imgui.h"
#include "raylib.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include <filesystem>
#include <stdio.h>
#include <string.h>

namespace fs = std::filesystem;

// Runtime playlist state (file-local, mirrors imgui_presets.cpp pattern)
static Playlist playlist;
static bool initialized = false;
static bool saving = false;
static bool loading = false;
static bool focusSaveInput = false;
static char saveNameBuf[PRESET_NAME_MAX] = "";
static char playlistFiles[MAX_PLAYLIST_ENTRIES][PRESET_PATH_MAX];
static int playlistFileCount = 0;

static void RefreshPlaylistFiles(void) {
  playlistFileCount = PlaylistListFiles(playlistFiles, MAX_PLAYLIST_ENTRIES);
}

// Extract display name from a preset path (filename without .json extension)
static const char *EntryDisplayName(const char *path) {
  const char *slash = strrchr(path, '/');
  const char *name = slash ? slash + 1 : path;

  // Strip .json extension for display
  static char buf[PRESET_PATH_MAX];
  strncpy(buf, name, PRESET_PATH_MAX - 1);
  buf[PRESET_PATH_MAX - 1] = '\0';

  char *dot = strrchr(buf, '.');
  if (dot && strcmp(dot, ".json") == 0) {
    *dot = '\0';
  }
  return buf;
}

static void DrawTransportStrip(AppConfigs *configs) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  const float width = ImGui::GetContentRegionAvail().x;
  const float height = ImGui::GetFrameHeight() + 4.0f;

  // Full-width background tint
  draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                      SetColorAlpha(Theme::ACCENT_CYAN_U32, 21));

  // Left-edge accent bar (2px, L-bracket motif)
  draw->AddRectFilled(pos, ImVec2(pos.x + 2.0f, pos.y + height),
                      Theme::ACCENT_CYAN_U32);

  // Top rim-light
  draw->AddLine(pos, ImVec2(pos.x + width, pos.y),
                SetColorAlpha(Theme::ACCENT_CYAN_U32, 60), 1.0f);

  // Reserve space for the strip
  ImGui::Dummy(ImVec2(width, height));

  // Draw controls inside the strip area
  float cursorY = pos.y + 2.0f;
  float cursorX = pos.x + 8.0f;

  bool empty = (playlist.entryCount == 0);
  bool noActive = (playlist.activeIndex < 0);

  // Prev button
  ImGui::SetCursorScreenPos(ImVec2(cursorX, cursorY));
  bool prevDisabled = empty || noActive || playlist.activeIndex <= 0;
  if (prevDisabled)
    ImGui::BeginDisabled();
  if (ImGui::ArrowButton("##prev", ImGuiDir_Left))
    ImGuiPlaylistAdvance(-1, configs);
  if (prevDisabled)
    ImGui::EndDisabled();

  ImGui::SameLine();

  // Next button
  bool nextDisabled =
      empty || noActive || playlist.activeIndex >= playlist.entryCount - 1;
  if (nextDisabled)
    ImGui::BeginDisabled();
  if (ImGui::ArrowButton("##next", ImGuiDir_Right))
    ImGuiPlaylistAdvance(+1, configs);
  if (nextDisabled)
    ImGui::EndDisabled();

  ImGui::SameLine();

  // Preset name (centered in remaining space)
  float afterButtons = ImGui::GetCursorScreenPos().x;
  float availWidth = (pos.x + width) - afterButtons;

  if (empty) {
    // Empty playlist
    const char *emptyText = "(empty)";
    float textW = ImGui::CalcTextSize(emptyText).x;
    const char *counterText = "0 / 0";
    float counterW = ImGui::CalcTextSize(counterText).x;
    float totalW = textW + 8.0f + counterW;
    float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextDisabled("%s", emptyText);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterText);
  } else if (noActive) {
    // No preset active
    const char *noText = "No preset loaded";
    float textW = ImGui::CalcTextSize(noText).x;
    char counterBuf[32];
    snprintf(counterBuf, sizeof(counterBuf), "- / %d", playlist.entryCount);
    float counterW = ImGui::CalcTextSize(counterBuf).x;
    float totalW = textW + 8.0f + counterW;
    float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextDisabled("%s", noText);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterBuf);
  } else {
    // Active preset
    const char *name = EntryDisplayName(playlist.entries[playlist.activeIndex]);
    float textW = ImGui::CalcTextSize(name).x;
    char counterBuf[32];
    snprintf(counterBuf, sizeof(counterBuf), "%d / %d",
             playlist.activeIndex + 1, playlist.entryCount);
    float counterW = ImGui::CalcTextSize(counterBuf).x;
    float totalW = textW + 8.0f + counterW;
    float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextColored(Theme::ACCENT_CYAN, "%s", name);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterBuf);
  }
}

static void DrawSetlist(AppConfigs *configs) {
  if (!ImGui::BeginChild(
          "##setlist", ImVec2(-1, -ImGui::GetFrameHeightWithSpacing()), true)) {
    ImGui::EndChild();
    return;
  }

  ImDrawList *draw = ImGui::GetWindowDrawList();

  if (playlist.entryCount == 0) {
    // Empty state - centered hint text
    const char *hint = "Drag presets here or click + Add Current";
    float textW = ImGui::CalcTextSize(hint).x;
    float windowW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowW - textW) * 0.5f);
    ImGui::TextDisabled("%s", hint);
    ImGui::EndChild();
    return;
  }

  for (int i = 0; i < playlist.entryCount; i++) {
    ImGui::PushID(i);

    bool isActive = (i == playlist.activeIndex);
    const char *displayName = EntryDisplayName(playlist.entries[i]);
    float contentWidth = ImGui::GetContentRegionAvail().x;

    // Active row background highlight
    if (isActive) {
      ImVec2 rowMin = ImGui::GetCursorScreenPos();
      ImVec2 rowMax = ImVec2(rowMin.x + contentWidth,
                             rowMin.y + ImGui::GetTextLineHeight() +
                                 ImGui::GetStyle().FramePadding.y * 2);
      draw->AddRectFilled(rowMin, rowMax,
                          SetColorAlpha(Theme::ACCENT_CYAN_U32, 32));
    }

    // Index column
    char indexBuf[8];
    snprintf(indexBuf, sizeof(indexBuf), "%2d", i + 1);
    ImGui::TextDisabled("%s", indexBuf);
    ImGui::SameLine();

    // Play marker for active row
    if (isActive) {
      ImVec2 triPos = ImGui::GetCursorScreenPos();
      float triSize = 6.0f;
      float triCenterY = triPos.y + ImGui::GetTextLineHeight() * 0.5f;
      draw->AddTriangleFilled(ImVec2(triPos.x, triCenterY - triSize * 0.5f),
                              ImVec2(triPos.x, triCenterY + triSize * 0.5f),
                              ImVec2(triPos.x + triSize, triCenterY),
                              Theme::ACCENT_CYAN_U32);
      ImGui::Dummy(ImVec2(triSize + 4.0f, 0));
      ImGui::SameLine();
    } else {
      // Blank space to maintain alignment with active row triangle
      ImGui::Dummy(ImVec2(10.0f, 0));
      ImGui::SameLine();
    }

    // Selectable for the row
    float selectableWidth = contentWidth - ImGui::GetCursorPos().x +
                            ImGui::GetStyle().WindowPadding.x;
    if (isActive) {
      ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN);
    }

    bool clicked = ImGui::Selectable(displayName, isActive,
                                     ImGuiSelectableFlags_AllowDoubleClick,
                                     ImVec2(selectableWidth - 30.0f, 0));

    if (isActive) {
      ImGui::PopStyleColor();
    }

    bool rowHovered = ImGui::IsItemHovered();

    // Double-click to load
    if (clicked && ImGui::IsMouseDoubleClicked(0)) {
      if (fs::exists(playlist.entries[i])) {
        ImGuiLoadPreset(playlist.entries[i], configs);
        playlist.activeIndex = i;
      } else {
        TraceLog(LOG_WARNING, "PLAYLIST: Preset file not found: %s",
                 playlist.entries[i]);
      }
    }

    // Drag-drop source
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("PLAYLIST_ITEM", &i, sizeof(int));
      ImGui::Text("%s", displayName);
      ImGui::EndDragDropSource();
    }

    // Drag-drop target
    if (ImGui::BeginDragDropTarget()) {
      const ImGuiPayload *payload =
          ImGui::AcceptDragDropPayload("PLAYLIST_ITEM");
      if (payload) {
        int srcIndex = *(const int *)payload->Data;
        PlaylistSwap(&playlist, srcIndex, i);
      }

      // Drop indicator line
      ImVec2 lineMin = ImGui::GetCursorScreenPos();
      draw->AddLine(ImVec2(lineMin.x, lineMin.y),
                    ImVec2(lineMin.x + contentWidth, lineMin.y),
                    Theme::ACCENT_CYAN_U32, 2.0f);

      ImGui::EndDragDropTarget();
    }

    // Remove button (only on hover)
    if (rowHovered) {
      ImGui::SameLine(contentWidth - 20.0f);
      if (ImGui::SmallButton("x")) {
        PlaylistRemove(&playlist, i);
        ImGui::PopID();
        break; // List changed, stop iterating
      }
    }

    ImGui::PopID();
  }

  ImGui::EndChild();
}

static void DrawManageBar(AppConfigs *configs) {
  float width = ImGui::GetContentRegionAvail().x;

  if (saving) {
    // Inline save flow
    float inputWidth = width * 0.7f;
    ImGui::SetNextItemWidth(inputWidth);
    if (focusSaveInput) {
      ImGui::SetKeyboardFocusHere(0);
      focusSaveInput = false;
    }
    bool submitted =
        ImGui::InputText("##playlistSave", saveNameBuf, PRESET_NAME_MAX,
                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::SmallButton("OK") || submitted) {
      if (saveNameBuf[0] != '\0') {
        strncpy(playlist.name, saveNameBuf, PRESET_NAME_MAX - 1);
        playlist.name[PRESET_NAME_MAX - 1] = '\0';
        char filepath[PRESET_PATH_MAX];
        snprintf(filepath, PRESET_PATH_MAX, "%s/%s.json", PLAYLIST_DIR,
                 saveNameBuf);
        PlaylistSave(&playlist, filepath);
        RefreshPlaylistFiles();
      }
      saving = false;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("x")) {
      saving = false;
    }
    return;
  }

  if (loading) {
    // Inline load flow
    const char *currentName =
        playlist.name[0] != '\0' ? playlist.name : "Select...";
    if (ImGui::BeginCombo("Playlist", currentName)) {
      for (int i = 0; i < playlistFileCount; i++) {
        if (ImGui::Selectable(playlistFiles[i])) {
          char filepath[PRESET_PATH_MAX];
          snprintf(filepath, PRESET_PATH_MAX, "%s/%s", PLAYLIST_DIR,
                   playlistFiles[i]);
          PlaylistLoad(&playlist, filepath);
          playlist.activeIndex = -1;
          loading = false;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("x")) {
      loading = false;
    }
    return;
  }

  // Normal button row
  const char *loadedPath = ImGuiGetLoadedPresetPath();
  bool noPresetLoaded = (loadedPath[0] == '\0');
  if (noPresetLoaded)
    ImGui::BeginDisabled();
  if (ImGui::Button("+ Add Current")) {
    PlaylistAdd(&playlist, loadedPath);
  }
  if (noPresetLoaded)
    ImGui::EndDisabled();

  // Right-aligned Save and Load buttons
  float saveW =
      ImGui::CalcTextSize("Save").x + ImGui::GetStyle().FramePadding.x * 2;
  float loadW =
      ImGui::CalcTextSize("Load").x + ImGui::GetStyle().FramePadding.x * 2;
  float spacing = ImGui::GetStyle().ItemSpacing.x;
  float rightX = width - saveW - loadW - spacing;

  ImGui::SameLine(rightX);
  if (ImGui::Button("Save")) {
    saving = true;
    focusSaveInput = true;
    if (playlist.name[0] != '\0') {
      strncpy(saveNameBuf, playlist.name, PRESET_NAME_MAX);
    } else {
      memset(saveNameBuf, 0, PRESET_NAME_MAX);
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    loading = true;
    RefreshPlaylistFiles();
  }
}

void ImGuiDrawPlaylistPanel(AppConfigs *configs) {
  if (!initialized) {
    playlist = PlaylistDefault();
    RefreshPlaylistFiles();
    initialized = true;
  }

  if (!ImGui::Begin("Playlist")) {
    ImGui::End();
    return;
  }

  DrawTransportStrip(configs);
  DrawSetlist(configs);
  DrawManageBar(configs);

  ImGui::End();
}

void ImGuiPlaylistAdvance(int direction, AppConfigs *configs) {
  if (!PlaylistAdvance(&playlist, direction))
    return;

  if (fs::exists(playlist.entries[playlist.activeIndex])) {
    ImGuiLoadPreset(playlist.entries[playlist.activeIndex], configs);
  } else {
    TraceLog(LOG_WARNING, "PLAYLIST: Preset file not found: %s",
             playlist.entries[playlist.activeIndex]);
  }
}
