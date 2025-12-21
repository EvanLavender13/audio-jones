#ifndef IMGUI_PANELS_H
#define IMGUI_PANELS_H

#include "imgui.h"

struct EffectConfig;
struct ColorConfig;
struct WaveformConfig;
struct SpectrumConfig;
struct AudioConfig;
struct BeatDetector;
struct BandEnergies;
struct BandConfig;
struct AppConfigs;

// Call once after rlImGuiSetup() - applies Neon Eclipse synthwave theme
void ImGuiApplyNeonTheme(void);

// Draw main dockspace covering viewport (transparent, passthrough to visualization)
void ImGuiDrawDockspace(void);

// ---------------------------------------------------------------------------
// Reusable drawing helpers for themed widgets
// ---------------------------------------------------------------------------

// Draw a vertical gradient rectangle with optional border
void DrawGradientBox(ImVec2 pos, ImVec2 size, ImU32 topColor, ImU32 bottomColor, float rounding = 0.0f);

// Draw an expanded glow rectangle behind an element
void DrawGlow(ImVec2 pos, ImVec2 size, ImU32 glowColor, float expand = 2.0f);

// Draw a collapsible section header with accent bar; returns true if section is open
bool DrawSectionHeader(const char* label, ImU32 accentColor, bool* isOpen);

// RAII helper for section content with consistent indent/spacing
struct SectionScope {
    bool open;
    SectionScope(const char* label, ImU32 accentColor, bool* isOpen);
    ~SectionScope();
    operator bool() const { return open; }
};

// ---------------------------------------------------------------------------
// Shared widgets
// ---------------------------------------------------------------------------

void ImGuiDrawColorMode(ColorConfig* color);

// Panel draw functions
void ImGuiDrawEffectsPanel(EffectConfig* cfg);
void ImGuiDrawWaveformsPanel(WaveformConfig* waveforms, int* count, int* selected);
void ImGuiDrawSpectrumPanel(SpectrumConfig* cfg);
void ImGuiDrawAudioPanel(AudioConfig* cfg);
void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* config);
void ImGuiDrawPresetPanel(AppConfigs* configs);

#endif // IMGUI_PANELS_H
