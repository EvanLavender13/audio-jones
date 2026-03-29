#ifndef IMGUI_PANELS_H
#define IMGUI_PANELS_H

#include "imgui.h"

#define PLAYLIST_SETLIST_ROWS 8

struct EffectConfig;
struct ColorConfig;
struct Drawable;
struct AudioConfig;
struct BeatDetector;
struct BandEnergies;
struct AudioFeatures;
struct Profiler;
struct AppConfigs;
struct ModSources;
struct LFOConfig;
struct LFOState;

// Call once after rlImGuiSetup() - applies Neon Eclipse synthwave theme
void ImGuiApplyNeonTheme(void);

// Draw main dockspace covering viewport (transparent, passthrough to
// visualization)
void ImGuiDrawDockspace(void);

// ---------------------------------------------------------------------------
// Reusable drawing helpers for themed widgets
// ---------------------------------------------------------------------------

// Draw a vertical gradient rectangle with optional border
void DrawGradientBox(ImVec2 pos, ImVec2 size, ImU32 topColor, ImU32 bottomColor,
                     float rounding = 0.0f);

// Draw a non-collapsible group header with horizontal accent line (pipeline
// category)
void DrawGroupHeader(const char *label, ImU32 accentColor);

// Draw a non-collapsible category header with accent bar and colored text
// (transform sub-category)
void DrawCategoryHeader(const char *label, ImU32 accentColor);

// Draw a collapsible section header with accent bar; returns true if section is
// open
bool DrawSectionHeader(const char *label, ImU32 accentColor, bool *isOpen,
                       bool isEnabled = true);

// Begin/End pair for collapsible sections with consistent indent/spacing
bool DrawSectionBegin(const char *label, ImU32 accentColor, bool *isOpen,
                      bool isEnabled = true);
void DrawSectionEnd(void);

// Begin/End pair for always-visible compact module containers (LFO slots)
void DrawModuleStripBegin(const char *label, ImU32 accentColor, bool *enabled);
void DrawModuleStripEnd(void);

// TreeNode with accent bar spanning expanded content
// Use TreeNodeAccentedPop() instead of ImGui::TreePop() to draw the accent bar
bool TreeNodeAccented(const char *label, ImU32 accentColor);
void TreeNodeAccentedPop(void);

// ---------------------------------------------------------------------------
// Shared widgets
// ---------------------------------------------------------------------------

void ImGuiDrawColorMode(ColorConfig *color);

// Panel draw functions
void ImGuiDrawEffectsPanel(EffectConfig *e, const ModSources *modSources);
void ImGuiDrawDrawablesPanel(Drawable *drawables, int *count, int *selected,
                             const ModSources *sources);
void ImGuiDrawDrawablesSyncIdCounter(const Drawable *drawables, int count);
void ImGuiDrawAudioPanel(AudioConfig *cfg);
void ImGuiDrawAnalysisPanel(const BeatDetector *beat, const BandEnergies *bands,
                            const AudioFeatures *features,
                            const Profiler *profiler);
void ImGuiDrawPresetPanel(AppConfigs *configs);
const char *ImGuiGetLoadedPresetPath(void);
void ImGuiLoadPreset(const char *filepath, AppConfigs *configs);
void ImGuiDrawPlaylistSection(AppConfigs *configs);
void ImGuiPlaylistAdvance(int direction, AppConfigs *configs);
void ImGuiDrawLFOPanel(LFOConfig *configs, const LFOState *states,
                       const ModSources *sources);

#endif // IMGUI_PANELS_H
