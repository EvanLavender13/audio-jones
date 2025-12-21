#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/theme.h"
#include "config/effect_config.h"

// Persistent section open states
static bool sectionWarp = false;
static bool sectionVoronoi = false;
static bool sectionPhysarum = false;
static bool sectionLFO = false;

void ImGuiDrawEffectsPanel(EffectConfig* e)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    // Core effects - Cyan header
    ImGui::TextColored(Theme::ACCENT_CYAN, "Core Effects");
    ImGui::Spacing();
    ImGui::SliderInt("Blur", &e->baseBlurScale, 0, 4);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ImGui::SliderInt("Bloom", &e->beatBlurScale, 0, 5);
    ImGui::SliderInt("Chroma", &e->chromaticMaxOffset, 0, 50);
    ImGui::SliderFloat("Zoom", &e->feedbackZoom, 0.9f, 1.0f, "%.3f");
    ImGui::SliderFloat("Rotation", &e->feedbackRotation, -0.02f, 0.02f, "%.4f rad");
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);
    ImGui::SliderInt("Kaleido", &e->kaleidoSegments, 1, 12);

    ImGui::Spacing();

    // Domain Warp - Cyan accent
    if (SectionScope warp{"Domain Warp", Theme::GLOW_CYAN, &sectionWarp}) {
        ImGui::SliderFloat("Strength", &e->warpStrength, 0.0f, 0.05f);
        if (e->warpStrength > 0.0f) {
            ImGui::SliderFloat("Scale##warp", &e->warpScale, 1.0f, 20.0f);
            ImGui::SliderInt("Octaves", &e->warpOctaves, 1, 5);
            ImGui::SliderFloat("Lacunarity", &e->warpLacunarity, 1.5f, 3.0f);
            ImGui::SliderFloat("Speed##warp", &e->warpSpeed, 0.1f, 2.0f);
        }
    }

    ImGui::Spacing();

    // Voronoi - Magenta accent
    if (SectionScope vor{"Voronoi", Theme::GLOW_MAGENTA, &sectionVoronoi}) {
        ImGui::SliderFloat("Intensity", &e->voronoiIntensity, 0.0f, 1.0f);
        if (e->voronoiIntensity > 0.0f) {
            ImGui::SliderFloat("Scale##vor", &e->voronoiScale, 5.0f, 50.0f);
            ImGui::SliderFloat("Speed##vor", &e->voronoiSpeed, 0.1f, 2.0f);
            ImGui::SliderFloat("Edge", &e->voronoiEdgeWidth, 0.01f, 0.1f);
        }
    }

    ImGui::Spacing();

    // Physarum - Orange accent
    if (SectionScope phys{"Physarum", Theme::GLOW_ORANGE, &sectionPhysarum}) {
        ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
        if (e->physarum.enabled) {
            ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);
            ImGui::SliderFloat("Sensor Dist", &e->physarum.sensorDistance, 1.0f, 100.0f);
            ImGui::SliderFloat("Sensor Angle", &e->physarum.sensorAngle, 0.0f, 6.28f, "%.2f rad");
            ImGui::SliderFloat("Turn Angle", &e->physarum.turningAngle, 0.0f, 6.28f, "%.2f rad");
            ImGui::SliderFloat("Step Size", &e->physarum.stepSize, 0.1f, 100.0f);
            ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
            ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);
            ImGuiDrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
    }

    ImGui::Spacing();

    // Rotation LFO - Cyan accent (cycle back)
    if (SectionScope lfo{"Rotation LFO", Theme::GLOW_CYAN, &sectionLFO}) {
        ImGui::Checkbox("Enabled##lfo", &e->rotationLFO.enabled);
        if (e->rotationLFO.enabled) {
            ImGui::SliderFloat("Rate", &e->rotationLFO.rate, 0.01f, 1.0f, "%.2f Hz");
            const char* waveforms[] = {"Sine", "Triangle", "Saw", "Square", "S&H"};
            ImGui::Combo("Waveform", &e->rotationLFO.waveform, waveforms, 5);
        }
    }

    ImGui::End();
}
