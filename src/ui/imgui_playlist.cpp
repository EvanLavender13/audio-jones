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
  const char *name = slash ? slash + 1 : path;

  strncpy(out, name, outSize - 1);
  out[outSize - 1] = '\0';

  char *dot = strrchr(out, '.');
  if (dot && strcmp(dot, ".json") == 0) {
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
    char name[PRESET_PATH_MAX];
    EntryDisplayName(playlist.entries[playlist.activeIndex], name,
                     PRESET_PATH_MAX);
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
  if (!ImGui::BeginChild("##setlist",
                         ImVec2(-1, PLAYLIST_SETLIST_ROWS *
                                        ImGui::GetFrameHeightWithSpacing()),
                         true)) {
    ImGui::EndChild();
    return;
  }

  ImDrawList *draw = ImGui::GetWindowDrawList();

  if (playlist.entryCount == 0) {
    // Empty state - centered hint text
    const char *hint = "Click + Add Current to build a setlist";
    float textW = ImGui::CalcTextSize(hint).x;
    float windowW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowW - textW) * 0.5f);
    ImGui::TextDisabled("%s", hint);
    ImGui::EndChild();
    return;
  }

  // Suppress all Selectable built-in backgrounds — we draw our own
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

  float textH = ImGui::GetTextLineHeight();
  float padY = ImGui::GetStyle().FramePadding.y;
  float indexW = ImGui::CalcTextSize("00").x;
  float markerW = 10.0f;
  float triSize = 6.0f;

  for (int i = 0; i < playlist.entryCount; i++) {
    ImGui::PushID(i);

    bool isActive = (i == playlist.activeIndex);
    char displayName[PRESET_PATH_MAX];
    EntryDisplayName(playlist.entries[i], displayName, PRESET_PATH_MAX);
    float contentWidth = ImGui::GetContentRegionAvail().x;

    // Single invisible Selectable owns the full row — drives interaction
    bool clicked = ImGui::Selectable("##row", false,
                                     ImGuiSelectableFlags_AllowDoubleClick |
                                         ImGuiSelectableFlags_AllowOverlap,
                                     ImVec2(contentWidth, 0));

    // Use the Selectable's actual rect for everything
    ImVec2 rowMin = ImGui::GetItemRectMin();
    ImVec2 rowMax = ImGui::GetItemRectMax();
    float rowH = rowMax.y - rowMin.y;
    float textY = rowMin.y + (rowH - textH) * 0.5f;
    bool rowHovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);

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
    snprintf(indexBuf, sizeof(indexBuf), "%2d", i + 1);
    draw->AddText(ImVec2(rowMin.x + 4.0f, textY), Theme::TEXT_DISABLED_U32,
                  indexBuf);

    // Play marker triangle for active row
    float nameX = rowMin.x + 4.0f + indexW + 8.0f + markerW + 4.0f;
    if (isActive) {
      float triX = rowMin.x + 4.0f + indexW + 8.0f;
      float triCY = rowMin.y + rowH * 0.5f;
      draw->AddTriangleFilled(ImVec2(triX, triCY - triSize * 0.5f),
                              ImVec2(triX, triCY + triSize * 0.5f),
                              ImVec2(triX + triSize, triCY),
                              Theme::ACCENT_CYAN_U32);
    }

    // Preset name
    ImU32 nameColor =
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
      if (payload) {
        int srcIndex = *(const int *)payload->Data;
        PlaylistSwap(&playlist, srcIndex, i);
      }

      draw->AddLine(ImVec2(rowMin.x, rowMax.y), ImVec2(rowMax.x, rowMax.y),
                    Theme::ACCENT_CYAN_U32, 2.0f);

      ImGui::EndDragDropTarget();
    }

    // Remove button — AllowOverlap lets this receive clicks over the Selectable
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
        if (!PlaylistSave(&playlist, filepath))
          TraceLog(LOG_WARNING, "PLAYLIST: Failed to save: %s", filepath);
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
  DrawManageBar(configs);
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
