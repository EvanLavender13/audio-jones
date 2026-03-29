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
static void EntryDisplayName(const char *path, char *out, int outSize) {
  const char *slash = strrchr(path, '/');
  const char *name = (slash != nullptr) ? slash + 1 : path;

  strncpy(out, name, outSize - 1);
  out[outSize - 1] = '\0';

  char *dot = strrchr(out, '.');
  if (dot != nullptr && strcmp(dot, ".json") == 0) {
    *dot = '\0';
  }
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
  const float cursorY = pos.y + 2.0f;
  const float cursorX = pos.x + 8.0f;

  const bool empty = (playlist.entryCount == 0);
  const bool noActive = (playlist.activeIndex < 0);

  // Prev button
  ImGui::SetCursorScreenPos(ImVec2(cursorX, cursorY));
  const bool prevDisabled = empty || noActive || playlist.activeIndex <= 0;
  if (prevDisabled) {
    ImGui::BeginDisabled();
  }
  if (ImGui::ArrowButton("##prev", ImGuiDir_Left)) {
    ImGuiPlaylistAdvance(-1, configs);
  }
  if (prevDisabled) {
    ImGui::EndDisabled();
  }

  ImGui::SameLine();

  // Next button
  const bool nextDisabled =
      empty || noActive || playlist.activeIndex >= playlist.entryCount - 1;
  if (nextDisabled) {
    ImGui::BeginDisabled();
  }
  if (ImGui::ArrowButton("##next", ImGuiDir_Right)) {
    ImGuiPlaylistAdvance(+1, configs);
  }
  if (nextDisabled) {
    ImGui::EndDisabled();
  }

  ImGui::SameLine();

  // Preset name (centered in remaining space)
  const float afterButtons = ImGui::GetCursorScreenPos().x;
  const float availWidth = (pos.x + width) - afterButtons;

  if (empty) {
    // Empty playlist
    const char *emptyText = "(empty)";
    const float textW = ImGui::CalcTextSize(emptyText).x;
    const char *counterText = "0 / 0";
    const float counterW = ImGui::CalcTextSize(counterText).x;
    const float totalW = textW + 8.0f + counterW;
    const float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextDisabled("%s", emptyText);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterText);
  } else if (noActive) {
    // No preset active
    const char *noText = "No preset loaded";
    const float textW = ImGui::CalcTextSize(noText).x;
    char counterBuf[32];
    (void)snprintf(counterBuf, sizeof(counterBuf), "- / %d",
                   playlist.entryCount);
    const float counterW = ImGui::CalcTextSize(counterBuf).x;
    const float totalW = textW + 8.0f + counterW;
    const float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextDisabled("%s", noText);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterBuf);
  } else {
    // Active preset
    char name[PRESET_PATH_MAX];
    EntryDisplayName(playlist.entries[playlist.activeIndex], name,
                     PRESET_PATH_MAX);
    const float textW = ImGui::CalcTextSize(name).x;
    char counterBuf[32];
    (void)snprintf(counterBuf, sizeof(counterBuf), "%d / %d",
                   playlist.activeIndex + 1, playlist.entryCount);
    const float counterW = ImGui::CalcTextSize(counterBuf).x;
    const float totalW = textW + 8.0f + counterW;
    const float centerX = afterButtons + (availWidth - totalW) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, cursorY));
    ImGui::TextColored(Theme::ACCENT_CYAN, "%s", name);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", counterBuf);
  }
}

static void DrawSetlist(AppConfigs *configs) {
  if (!ImGui::BeginChild("##setlist",
                         ImVec2(-1, PLAYLIST_SETLIST_ROWS *
                                        ImGui::GetFrameHeightWithSpacing()),
                         ImGuiChildFlags_Borders)) {
    ImGui::EndChild();
    return;
  }

  ImDrawList *draw = ImGui::GetWindowDrawList();

  if (playlist.entryCount == 0) {
    // Empty state - centered hint text
    const char *hint = "Click + Add Current to build a setlist";
    const float textW = ImGui::CalcTextSize(hint).x;
    const float windowW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowW - textW) * 0.5f);
    ImGui::TextDisabled("%s", hint);
    ImGui::EndChild();
    return;
  }

  // Suppress all Selectable built-in backgrounds - we draw our own
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

  const float textH = ImGui::GetTextLineHeight();
  const float indexW = ImGui::CalcTextSize("00").x;
  const float markerW = 10.0f;
  const float triSize = 6.0f;

  for (int i = 0; i < playlist.entryCount; i++) {
    ImGui::PushID(i);

    const bool isActive = (i == playlist.activeIndex);
    char displayName[PRESET_PATH_MAX];
    EntryDisplayName(playlist.entries[i], displayName, PRESET_PATH_MAX);
    const float contentWidth = ImGui::GetContentRegionAvail().x;

    // Single invisible Selectable owns the full row - drives interaction
    const bool clicked =
        ImGui::Selectable("##row", false,
                          ImGuiSelectableFlags_AllowDoubleClick |
                              ImGuiSelectableFlags_AllowOverlap,
                          ImVec2(contentWidth, 0));

    // Use the Selectable's actual rect for everything
    const ImVec2 rowMin = ImGui::GetItemRectMin();
    const ImVec2 rowMax = ImGui::GetItemRectMax();
    const float rowH = rowMax.y - rowMin.y;
    const float textY = rowMin.y + (rowH - textH) * 0.5f;
    const bool rowHovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);

    // Row background
    if (isActive) {
      draw->AddRectFilled(rowMin, rowMax,
                          SetColorAlpha(Theme::ACCENT_CYAN_U32, 32));
    } else if (rowHovered) {
      draw->AddRectFilled(rowMin, rowMax,
                          SetColorAlpha(Theme::ACCENT_CYAN_U32, 14));
    }

    // Index number
    char indexBuf[8];
    (void)snprintf(indexBuf, sizeof(indexBuf), "%2d", i + 1);
    draw->AddText(ImVec2(rowMin.x + 4.0f, textY), Theme::TEXT_DISABLED_U32,
                  indexBuf);

    // Play marker triangle for active row
    const float nameX = rowMin.x + 4.0f + indexW + 8.0f + markerW + 4.0f;
    if (isActive) {
      const float triX = rowMin.x + 4.0f + indexW + 8.0f;
      const float triCY = rowMin.y + rowH * 0.5f;
      draw->AddTriangleFilled(ImVec2(triX, triCY - triSize * 0.5f),
                              ImVec2(triX, triCY + triSize * 0.5f),
                              ImVec2(triX + triSize, triCY),
                              Theme::ACCENT_CYAN_U32);
    }

    // Preset name
    const ImU32 nameColor =
        isActive ? Theme::ACCENT_CYAN_U32 : Theme::TEXT_PRIMARY_U32;
    draw->AddText(ImVec2(nameX, textY), nameColor, displayName);

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
      if (payload != nullptr) {
        const int srcIndex = *static_cast<const int *>(payload->Data);
        PlaylistSwap(&playlist, srcIndex, i);
      }

      draw->AddLine(ImVec2(rowMin.x, rowMax.y), ImVec2(rowMax.x, rowMax.y),
                    Theme::ACCENT_CYAN_U32, 2.0f);

      ImGui::EndDragDropTarget();
    }

    // Remove button - AllowOverlap lets this receive clicks over the Selectable
    if (rowHovered) {
      ImGui::SameLine(contentWidth - 20.0f);
      if (ImGui::SmallButton("x")) {
        PlaylistRemove(&playlist, i);
        ImGui::PopID();
        break;
      }
    }

    ImGui::PopID();
  }

  ImGui::PopStyleColor(3);

  ImGui::EndChild();
}

static void DrawManageBar() {
  const float width = ImGui::GetContentRegionAvail().x;

  if (saving) {
    // Inline save flow
    const float inputWidth = width * 0.7f;
    ImGui::SetNextItemWidth(inputWidth);
    if (focusSaveInput) {
      ImGui::SetKeyboardFocusHere(0);
      focusSaveInput = false;
    }
    const bool submitted =
        ImGui::InputText("##playlistSave", saveNameBuf, PRESET_NAME_MAX,
                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::SmallButton("OK") || submitted) {
      if (saveNameBuf[0] != '\0') {
        strncpy(playlist.name, saveNameBuf, PRESET_NAME_MAX - 1);
        playlist.name[PRESET_NAME_MAX - 1] = '\0';
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, PRESET_PATH_MAX, "%s/%s.json", PLAYLIST_DIR,
                       saveNameBuf);
        if (!PlaylistSave(&playlist, filepath)) {
          TraceLog(LOG_WARNING, "PLAYLIST: Failed to save: %s", filepath);
        }
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
          (void)snprintf(filepath, PRESET_PATH_MAX, "%s/%s", PLAYLIST_DIR,
                         playlistFiles[i]);
          PlaylistLoad(&playlist, filepath);
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
  const bool noPresetLoaded = (loadedPath[0] == '\0');
  if (noPresetLoaded) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("+ Add Current")) {
    PlaylistAdd(&playlist, loadedPath);
  }
  if (noPresetLoaded) {
    ImGui::EndDisabled();
  }

  // Right-aligned Save and Load buttons
  const float saveW =
      ImGui::CalcTextSize("Save").x + ImGui::GetStyle().FramePadding.x * 2;
  const float loadW =
      ImGui::CalcTextSize("Load").x + ImGui::GetStyle().FramePadding.x * 2;
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float rightX = width - saveW - loadW - spacing;

  ImGui::SameLine(rightX);
  if (ImGui::Button("Save##playlist")) {
    saving = true;
    focusSaveInput = true;
    if (playlist.name[0] != '\0') {
      strncpy(saveNameBuf, playlist.name, PRESET_NAME_MAX - 1);
      saveNameBuf[PRESET_NAME_MAX - 1] = '\0';
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

void ImGuiDrawPlaylistSection(AppConfigs *configs) {
  if (!initialized) {
    playlist = PlaylistDefault();
    RefreshPlaylistFiles();
    initialized = true;
  }

  ImGui::SeparatorText("Playlist");

  DrawTransportStrip(configs);
  DrawSetlist(configs);
  DrawManageBar();
}

void ImGuiPlaylistAdvance(int direction, AppConfigs *configs) {
  if (!PlaylistAdvance(&playlist, direction)) {
    return;
  }

  if (fs::exists(playlist.entries[playlist.activeIndex])) {
    ImGuiLoadPreset(playlist.entries[playlist.activeIndex], configs);
  } else {
    TraceLog(LOG_WARNING, "PLAYLIST: Preset file not found: %s",
             playlist.entries[playlist.activeIndex]);
  }
}
