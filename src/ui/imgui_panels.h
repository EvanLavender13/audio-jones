#ifndef IMGUI_PANELS_H
#define IMGUI_PANELS_H

struct EffectConfig;
struct ColorConfig;
struct WaveformConfig;
struct SpectrumConfig;
struct AudioConfig;
struct BeatDetector;
struct BandEnergies;
struct BandConfig;
struct AppConfigs;

// Call once after rlImGuiSetup()
void ImGuiApplyDarkTheme(void);

// Draw main dockspace covering viewport (transparent, passthrough to visualization)
void ImGuiDrawDockspace(void);

// Shared widgets
void ImGuiDrawColorMode(ColorConfig* color);

// Panel draw functions
void ImGuiDrawEffectsPanel(EffectConfig* cfg);
void ImGuiDrawWaveformsPanel(WaveformConfig* waveforms, int* count, int* selected);
void ImGuiDrawSpectrumPanel(SpectrumConfig* cfg);
void ImGuiDrawAudioPanel(AudioConfig* cfg);
void ImGuiDrawAnalysisPanel(BeatDetector* beat, BandEnergies* bands, BandConfig* config);
void ImGuiDrawPresetPanel(AppConfigs* configs);

#endif // IMGUI_PANELS_H
