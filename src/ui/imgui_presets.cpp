#include "config/app_configs.h"
#include "config/preset.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include <filesystem>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// UI state for preset panel (folder browser and selection).
static PresetEntry entries[MAX_PRESET_ENTRIES];
static int entryCount = 0;
static char presetName[PRESET_NAME_MAX] = "Default";
static bool initialized = false;

// Folder browsing state
static char currentDir[PRESET_PATH_MAX] = PRESET_DIR;
static char loadedPresetPath[PRESET_PATH_MAX] = "";
static bool creatingFolder = false;
static bool focusFolderInput = false;
static char folderNameBuf[PRESET_NAME_MAX] = "";
static bool savingPreset = false;
static bool focusSaveInput = false;
static char savePresetBuf[PRESET_NAME_MAX] = "";

static void RefreshPresetList(void) {
  entryCount = PresetListEntries(currentDir, entries, MAX_PRESET_ENTRIES);
}

// Split a path string by '/' into segments
static std::vector<std::string> SplitPath(const char *path) {
  std::vector<std::string> segments;
  const std::string s(path);
  size_t start = 0;
  while (start < s.size()) {
    const size_t pos = s.find('/', start);
    if (pos == std::string::npos) {
      segments.push_back(s.substr(start));
      break;
    }
    if (pos > start) {
      segments.push_back(s.substr(start, pos - start));
    }
    start = pos + 1;
  }
  return segments;
}

// Build path from segments [0..endIndex] inclusive
static std::string BuildPath(const std::vector<std::string> &segments,
                             int endIndex) {
  std::string path;
  for (int i = 0; i <= endIndex; i++) {
    if (i > 0) {
      path += "/";
    }
    path += segments[i];
  }
  return path;
}

// Navigate to a directory path and refresh the entry list
static void NavigateTo(const char *path) {
  (void)snprintf(currentDir, PRESET_PATH_MAX, "%s", path);
  savingPreset = false;
  RefreshPresetList();
}

const char *ImGuiGetLoadedPresetPath(void) { return loadedPresetPath; }

// Load a preset file and apply it to app configs
void ImGuiLoadPreset(const char *filepath, AppConfigs *configs) {
  Preset p;
  if (PresetLoad(&p, filepath)) {
    strncpy(presetName, p.name, PRESET_NAME_MAX);
    presetName[PRESET_NAME_MAX - 1] = '\0';
    PresetToAppConfigs(&p, configs);
    PostEffectClearFeedback(configs->postEffect);
    strncpy(loadedPresetPath, filepath, PRESET_PATH_MAX);
    loadedPresetPath[PRESET_PATH_MAX - 1] = '\0';
    savingPreset = false;
  }
}

static void DrawBreadcrumbs(void) {
  std::vector<std::string> segments = SplitPath(currentDir);

  // Style: transparent buttons with cyan text
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.0f, 0.9f, 0.95f, 0.15f));
  ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN);

  if (segments.size() > 1) {
    if (ImGui::SmallButton("<")) {
      const std::string newPath = BuildPath(segments, (int)segments.size() - 2);
      NavigateTo(newPath.c_str());
    }
    ImGui::SameLine();
  }

  for (int i = 0; i < (int)segments.size() - 1; i++) {
    ImGui::PushID(i);
    if (ImGui::SmallButton(segments[i].c_str())) {
      const std::string newPath = BuildPath(segments, i);
      NavigateTo(newPath.c_str());
    }
    ImGui::PopID();
    ImGui::SameLine();

    // Delimiter - restore style for TextDisabled, then re-push
    ImGui::PopStyleColor(3);
    ImGui::TextDisabled(">");
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.0f, 0.9f, 0.95f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN);
    ImGui::SameLine();
  }

  ImGui::PopStyleColor(3);

  // Current segment - white, not clickable
  ImGui::TextColored(Theme::TEXT_PRIMARY, "%s", segments.back().c_str());

  ImGui::Separator();
}

static void DrawPresetList(AppConfigs *configs) {
  const float controlsH = ImGui::GetFrameHeightWithSpacing();
  const float playlistH =
      (PLAYLIST_SETLIST_ROWS + 4) * ImGui::GetFrameHeightWithSpacing();
  const float reserveBelow = controlsH + playlistH;

  if (!ImGui::BeginChild("##presetList", ImVec2(-1, -reserveBelow),
                         ImGuiChildFlags_Borders)) {
    ImGui::EndChild();
    return;
  }

  // Count folders and presets for separator logic
  bool hasFolders = false;
  bool hasPresets = false;
  for (int i = 0; i < entryCount; i++) {
    if (entries[i].isFolder) {
      hasFolders = true;
    } else {
      hasPresets = true;
    }
  }

  // Folder rows
  for (int i = 0; i < entryCount; i++) {
    if (!entries[i].isFolder) {
      continue;
    }

    char label[PRESET_PATH_MAX + 4];
    (void)snprintf(label, sizeof(label), "> %s", entries[i].name);

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_CYAN);
    if (ImGui::Selectable(label)) {
      char newDir[PRESET_PATH_MAX];
      (void)snprintf(newDir, PRESET_PATH_MAX, "%s/%s", currentDir,
                     entries[i].name);
      NavigateTo(newDir);
      ImGui::PopStyleColor();
      break;
    }
    ImGui::PopStyleColor();
  }

  // Inline folder creation
  if (creatingFolder) {
    ImGui::TextColored(Theme::ACCENT_CYAN, ">");
    ImGui::SameLine();
    if (focusFolderInput) {
      ImGui::SetKeyboardFocusHere(0);
      focusFolderInput = false;
    }
    if (ImGui::InputText("##newfolder", folderNameBuf, PRESET_NAME_MAX,
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
      if (folderNameBuf[0] != '\0') {
        const std::string folderPath =
            std::string(currentDir) + "/" + folderNameBuf;
        fs::create_directory(folderPath);
        RefreshPresetList();
      }
      creatingFolder = false;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      creatingFolder = false;
    }
  }

  // Separator between folders and presets
  if ((hasFolders || creatingFolder) && hasPresets) {
    ImGui::SeparatorText("");
  }

  // Preset rows
  for (int i = 0; i < entryCount; i++) {
    if (entries[i].isFolder) {
      continue;
    }

    char filepath[PRESET_PATH_MAX];
    (void)snprintf(filepath, PRESET_PATH_MAX, "%s/%s.json", currentDir,
                   entries[i].name);

    const bool isLoaded = (strcmp(filepath, loadedPresetPath) == 0);

    char label[PRESET_PATH_MAX + 4];
    if (isLoaded) {
      (void)snprintf(label, sizeof(label), "* %s", entries[i].name);
    } else {
      (void)snprintf(label, sizeof(label), "%s", entries[i].name);
    }

    if (isLoaded) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.3f, 1.0f));
    }

    if (ImGui::Selectable(label)) {
      ImGuiLoadPreset(filepath, configs);
    }

    if (isLoaded) {
      ImGui::PopStyleColor();
    }
  }

  // Empty state
  if (entryCount == 0 && !creatingFolder) {
    const float textWidth = ImGui::CalcTextSize("(empty)").x;
    const float windowWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         (windowWidth - textWidth) * 0.5f);
    ImGui::TextDisabled("(empty)");
  }

  ImGui::EndChild();
}

static void DrawPresetControls(const AppConfigs *configs) {
  const float width = ImGui::GetContentRegionAvail().x;

  if (savingPreset) {
    // Inline save flow - replaces button row
    const float inputWidth = width * 0.7f;
    ImGui::SetNextItemWidth(inputWidth);
    if (focusSaveInput) {
      ImGui::SetKeyboardFocusHere(0);
      focusSaveInput = false;
    }
    const bool submitted =
        ImGui::InputText("##presetSave", savePresetBuf, PRESET_NAME_MAX,
                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::SmallButton("OK") || submitted) {
      if (savePresetBuf[0] != '\0') {
        char filepath[PRESET_PATH_MAX];
        (void)snprintf(filepath, PRESET_PATH_MAX, "%s/%s.json", currentDir,
                       savePresetBuf);
        Preset p;
        strncpy(p.name, savePresetBuf, PRESET_NAME_MAX);
        p.name[PRESET_NAME_MAX - 1] = '\0';
        strncpy(presetName, savePresetBuf, PRESET_NAME_MAX);
        PresetFromAppConfigs(&p, configs);
        if (PresetSave(&p, filepath)) {
          strncpy(loadedPresetPath, filepath, PRESET_PATH_MAX);
          loadedPresetPath[PRESET_PATH_MAX - 1] = '\0';
          RefreshPresetList();
        }
      }
      savingPreset = false;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("x")) {
      savingPreset = false;
    }
    return;
  }

  if (ImGui::Button("Save")) {
    savingPreset = true;
    focusSaveInput = true;
    strncpy(savePresetBuf, presetName, PRESET_NAME_MAX);
  }

  ImGui::SameLine();

  if (ImGui::Button("+ Folder")) {
    creatingFolder = true;
    focusFolderInput = true;
    memset(folderNameBuf, 0, PRESET_NAME_MAX);
  }

  ImGui::SameLine();

  if (ImGui::Button("Refresh")) {
    RefreshPresetList();
  }
}

void ImGuiDrawPresetPanel(AppConfigs *configs) {
  if (!initialized) {
    RefreshPresetList();
    initialized = true;
  }

  if (!ImGui::Begin("Presets")) {
    ImGui::End();
    return;
  }

  DrawBreadcrumbs();
  DrawPresetList(configs);
  DrawPresetControls(configs);
  ImGuiDrawPlaylistSection(configs);

  ImGui::End();
}
