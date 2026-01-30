#ifndef IMGUI_PANELS_H
#define IMGUI_PANELS_H

#include "imgui.h"

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

// Draw an expanded glow rectangle behind an element
void DrawGlow(ImVec2 pos, ImVec2 size, ImU32 glowColor, float expand = 2.0f);

// Draw a non-collapsible group header with horizontal accent line (pipeline
// category)
void DrawGroupHeader(const char *label, ImU32 accentColor);

// Draw a non-collapsible category header with accent bar and colored text
// (transform sub-category)
void DrawCategoryHeader(const char *label, ImU32 accentColor);

// Draw a collapsible section header with accent bar; returns true if section is
// open
bool DrawSectionHeader(const char *label, ImU32 accentColor, bool *isOpen);

// Begin/End pair for collapsible sections with consistent indent/spacing
bool DrawSectionBegin(const char *label, ImU32 accentColor, bool *isOpen);
void DrawSectionEnd(void);

// SliderFloat with automatic tooltip on hover
bool SliderFloatWithTooltip(const char *label, float *value, float min,
                            float max, const char *format, const char *tooltip);

// TreeNode with accent bar spanning expanded content
// Use TreeNodeAccentedPop() instead of ImGui::TreePop() to draw the accent bar
bool TreeNodeAccented(const char *label, ImU32 accentColor);
void TreeNodeAccentedPop(void);

// Toggle button that flips a float intensity between 0.0f and 1.0f
// Updates modulation engine base value when changed
bool IntensityToggleButton(const char *label, float *intensity,
                           const char *paramId, ImU32 activeColor);

// ---------------------------------------------------------------------------
// Shared widgets
// ---------------------------------------------------------------------------

void ImGuiDrawColorMode(ColorConfig *color);

// Panel draw functions
void ImGuiDrawEffectsPanel(EffectConfig *cfg, const ModSources *modSources);
void ImGuiDrawDrawablesPanel(Drawable *drawables, int *count, int *selected,
                             const ModSources *sources);
void ImGuiDrawDrawablesSyncIdCounter(const Drawable *drawables, int count);
void ImGuiDrawAudioPanel(AudioConfig *cfg);
void ImGuiDrawAnalysisPanel(BeatDetector *beat, BandEnergies *bands,
                            const AudioFeatures *features,
                            const Profiler *profiler);
void ImGuiDrawPresetPanel(AppConfigs *configs);
void ImGuiDrawLFOPanel(LFOConfig *configs, const LFOState *states,
                       const ModSources *sources);

#endif // IMGUI_PANELS_H
