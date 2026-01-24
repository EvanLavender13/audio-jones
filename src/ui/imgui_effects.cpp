#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/theme.h"
#include "ui/ui_units.h"
#include "ui/modulatable_slider.h"
#include "config/effect_config.h"
#include "automation/mod_sources.h"
#include "simulation/bounds_mode.h"

// Persistent section open states (simulations only - transforms in imgui_effects_transforms.cpp)
static bool sectionPhysarum = false;
static bool sectionCurlFlow = false;
static bool sectionCurlAdvection = false;
static bool sectionAttractorFlow = false;
static bool sectionBoids = false;
static bool sectionCymatics = false;
static bool sectionFlowField = false;

// Selection tracking for effect order list
static int selectedTransformEffect = -1;

// Category badge and section index for pipeline list (indices match Draw*Category section colors)
struct TransformCategory {
    const char* badge;
    int sectionIndex;
};

static TransformCategory GetTransformCategory(TransformEffectType type) {
    switch (type) {
        // Symmetry - section 0
        case TRANSFORM_KALEIDOSCOPE:
        case TRANSFORM_KIFS:
        case TRANSFORM_POINCARE_DISK:
        case TRANSFORM_RADIAL_PULSE:
        case TRANSFORM_MANDELBOX:
        case TRANSFORM_TRIANGLE_FOLD:
        case TRANSFORM_MOIRE_INTERFERENCE:
            return {"SYM", 0};
        // Warp - section 1
        case TRANSFORM_SINE_WARP:
        case TRANSFORM_TEXTURE_WARP:
        case TRANSFORM_GRADIENT_FLOW:
        case TRANSFORM_WAVE_RIPPLE:
        case TRANSFORM_MOBIUS:
        case TRANSFORM_CHLADNI_WARP:
        case TRANSFORM_DOMAIN_WARP:
        case TRANSFORM_PHYLLOTAXIS_WARP:
            return {"WARP", 1};
        // Cellular - section 2
        case TRANSFORM_VORONOI:
        case TRANSFORM_LATTICE_FOLD:
        case TRANSFORM_PHYLLOTAXIS:
            return {"CELL", 2};
        // Motion - section 3
        case TRANSFORM_INFINITE_ZOOM:
        case TRANSFORM_RADIAL_STREAK:
        case TRANSFORM_DROSTE_ZOOM:
        case TRANSFORM_DENSITY_WAVE_SPIRAL:
            return {"MOT", 3};
        // Style - section 4
        case TRANSFORM_PIXELATION:
        case TRANSFORM_GLITCH:
        case TRANSFORM_TOON:
        case TRANSFORM_HEIGHTFIELD_RELIEF:
        case TRANSFORM_ASCII_ART:
        case TRANSFORM_OIL_PAINT:
        case TRANSFORM_WATERCOLOR:
        case TRANSFORM_NEON_GLOW:
        case TRANSFORM_CROSS_HATCHING:
        case TRANSFORM_BOKEH:
        case TRANSFORM_BLOOM:
        case TRANSFORM_PENCIL_SKETCH:
        case TRANSFORM_MATRIX_RAIN:
        case TRANSFORM_IMPRESSIONIST:
            return {"STY", 4};
        // Color - section 5
        case TRANSFORM_COLOR_GRADE:
        case TRANSFORM_FALSE_COLOR:
        case TRANSFORM_HALFTONE:
        case TRANSFORM_PALETTE_QUANTIZATION:
            return {"COL", 5};
        // Simulation - section 6
        case TRANSFORM_PHYSARUM_BOOST:
        case TRANSFORM_CURL_FLOW_BOOST:
        case TRANSFORM_CURL_ADVECTION_BOOST:
        case TRANSFORM_ATTRACTOR_FLOW_BOOST:
        case TRANSFORM_BOIDS_BOOST:
        case TRANSFORM_CYMATICS_BOOST:
            return {"SIM", 6};
        default:
            return {"???", 0};
    }
}

// Shared blend mode options for simulation effects
static const char* BLEND_MODES[] = {
    "Boost", "Tinted Boost", "Screen", "Mix", "Soft Light",
    "Overlay", "Color Burn", "Linear Burn", "Vivid Light", "Linear Light",
    "Pin Light", "Difference", "Negation", "Subtract", "Reflect", "Phoenix"
};
static const int BLEND_MODE_COUNT = 16;

// Bounds mode options for simulations
static const char* PHYSARUM_BOUNDS_MODES[] = {
    "Toroidal", "Reflect", "Redirect", "Scatter", "Random",
    "Fixed Home", "Orbit", "Species Orbit", "Multi-Home", "Antipodal"
};
static const int PHYSARUM_BOUNDS_MODE_COUNT = 10;
static const char* BOIDS_BOUNDS_MODES[] = { "Toroidal", "Soft Repulsion" };

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig* e, const ModSources* modSources)
{
    if (!ImGui::Begin("Effects")) {
        ImGui::End();
        return;
    }

    // -------------------------------------------------------------------------
    // FEEDBACK GROUP
    // -------------------------------------------------------------------------
    DrawGroupHeader("FEEDBACK", Theme::ACCENT_CYAN_U32);

    ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);
    ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
    ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);

    ImGui::Spacing();

    if (DrawSectionBegin("Flow Field", Theme::GetSectionGlow(0), &sectionFlowField)) {
        ImGui::SeparatorText("Base");
        ModulatableSlider("Zoom##base", &e->flowField.zoomBase,
                          "flowField.zoomBase", "%.4f", modSources);
        ModulatableSliderAngleDeg("Spin##base", &e->flowField.rotationSpeed,
                                  "flowField.rotationSpeed", modSources, "%.1f °/s");
        ModulatableSlider("DX##base", &e->flowField.dxBase,
                          "flowField.dxBase", "%.4f", modSources);
        ModulatableSlider("DY##base", &e->flowField.dyBase,
                          "flowField.dyBase", "%.4f", modSources);

        ImGui::SeparatorText("Radial");
        ModulatableSlider("Zoom##radial", &e->flowField.zoomRadial,
                          "flowField.zoomRadial", "%.4f", modSources);
        ModulatableSliderAngleDeg("Spin##radial", &e->flowField.rotationSpeedRadial,
                                  "flowField.rotationSpeedRadial", modSources, "%.1f °/s");
        ModulatableSlider("DX##radial", &e->flowField.dxRadial,
                          "flowField.dxRadial", "%.4f", modSources);
        ModulatableSlider("DY##radial", &e->flowField.dyRadial,
                          "flowField.dyRadial", "%.4f", modSources);

        ImGui::SeparatorText("Angular");
        ModulatableSlider("Zoom##angular", &e->flowField.zoomAngular,
                          "flowField.zoomAngular", "%.4f", modSources);
        ImGui::SliderInt("Zoom Freq", &e->flowField.zoomAngularFreq, 1, 8);
        ModulatableSliderAngleDeg("Spin##angular", &e->flowField.rotAngular,
                                  "flowField.rotAngular", modSources);
        ImGui::SliderInt("Spin Freq", &e->flowField.rotAngularFreq, 1, 8);
        ModulatableSlider("DX##angular", &e->flowField.dxAngular,
                          "flowField.dxAngular", "%.4f", modSources);
        ImGui::SliderInt("DX Freq", &e->flowField.dxAngularFreq, 1, 8);
        ModulatableSlider("DY##angular", &e->flowField.dyAngular,
                          "flowField.dyAngular", "%.4f", modSources);
        ImGui::SliderInt("DY Freq", &e->flowField.dyAngularFreq, 1, 8);

        ImGui::SeparatorText("Center");
        ModulatableSlider("CX", &e->flowField.cx,
                          "flowField.cx", "%.2f", modSources);
        ModulatableSlider("CY", &e->flowField.cy,
                          "flowField.cy", "%.2f", modSources);

        ImGui::SeparatorText("Stretch");
        ModulatableSlider("SX", &e->flowField.sx,
                          "flowField.sx", "%.3f", modSources);
        ModulatableSlider("SY", &e->flowField.sy,
                          "flowField.sy", "%.3f", modSources);

        ImGui::SeparatorText("Warp");
        ModulatableSlider("Warp", &e->proceduralWarp.warp,
                          "proceduralWarp.warp", "%.2f", modSources);
        ModulatableSlider("Warp Speed", &e->proceduralWarp.warpSpeed,
                          "proceduralWarp.warpSpeed", "%.2f", modSources);
        ModulatableSlider("Warp Scale", &e->proceduralWarp.warpScale,
                          "proceduralWarp.warpScale", "%.1f", modSources);

        ImGui::SeparatorText("Gradient Flow");
        ModulatableSlider("Strength", &e->feedbackFlow.strength,
                          "feedbackFlow.strength", "%.1f px", modSources);
        ModulatableSliderAngleDeg("Flow Angle", &e->feedbackFlow.flowAngle,
                                  "feedbackFlow.flowAngle", modSources);
        ModulatableSlider("Scale", &e->feedbackFlow.scale,
                          "feedbackFlow.scale", "%.1f", modSources);
        ModulatableSlider("Threshold", &e->feedbackFlow.threshold,
                          "feedbackFlow.threshold", "%.3f", modSources);
        DrawSectionEnd();
    }

    // -------------------------------------------------------------------------
    // OUTPUT GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("OUTPUT", Theme::ACCENT_MAGENTA_U32);

    ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);
    ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
    ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

    // -------------------------------------------------------------------------
    // SIMULATIONS GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("SIMULATIONS", Theme::ACCENT_ORANGE_U32);

    int simIdx = 0;
    if (DrawSectionBegin("Physarum", Theme::GetSectionGlow(simIdx++), &sectionPhysarum)) {
        ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
        if (e->physarum.enabled) {
            ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);

            ImGui::SeparatorText("Bounds");
            int boundsMode = (int)e->physarum.boundsMode;
            if (ImGui::Combo("Bounds Mode##phys", &boundsMode, PHYSARUM_BOUNDS_MODES, PHYSARUM_BOUNDS_MODE_COUNT)) {
                e->physarum.boundsMode = (PhysarumBoundsMode)boundsMode;
            }
            if (boundsMode == PHYSARUM_BOUNDS_REDIRECT || boundsMode == PHYSARUM_BOUNDS_MULTI_HOME) {
                ImGui::Checkbox("Respawn", &e->physarum.respawnMode);
            }
            if (boundsMode == PHYSARUM_BOUNDS_MULTI_HOME) {
                ImGui::SliderInt("Attractors", &e->physarum.attractorCount, 2, 8);
            }
            if (boundsMode == PHYSARUM_BOUNDS_SPECIES_ORBIT) {
                ModulatableSlider("Orbit Offset", &e->physarum.orbitOffset,
                                  "physarum.orbitOffset", "%.2f", modSources);
            }

            ImGui::SeparatorText("Sensing");
            ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                              "physarum.sensorDistance", "%.1f px", modSources);
            ModulatableSlider("Sensor Variance", &e->physarum.sensorDistanceVariance,
                              "physarum.sensorDistanceVariance", "%.1f px", modSources);
            ModulatableSliderAngleDeg("Sensor Angle", &e->physarum.sensorAngle,
                                      "physarum.sensorAngle", modSources);
            ModulatableSliderAngleDeg("Turn Angle", &e->physarum.turningAngle,
                                      "physarum.turningAngle", modSources);
            ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);

            ImGui::SeparatorText("Movement");
            ModulatableSlider("Step Size", &e->physarum.stepSize,
                              "physarum.stepSize", "%.1f px", modSources);
            ModulatableSlider("Levy Alpha", &e->physarum.levyAlpha,
                              "physarum.levyAlpha", "%.1f", modSources);
            ModulatableSlider("Gravity", &e->physarum.gravityStrength,
                              "physarum.gravityStrength", "%.2f", modSources);
            ImGui::Checkbox("Vector Steering", &e->physarum.vectorSteering);

            ImGui::SeparatorText("Species");
            ModulatableSlider("Repulsion", &e->physarum.repulsionStrength,
                              "physarum.repulsionStrength", "%.2f", modSources);
            ModulatableSlider("Sampling Exp", &e->physarum.samplingExponent,
                              "physarum.samplingExponent", "%.1f", modSources);

            ImGui::SeparatorText("Trail");
            ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
            ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);

            ImGui::SeparatorText("Output");
            ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->physarum.blendMode;
            if (ImGui::Combo("Blend Mode", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->physarum.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->physarum.color);
            ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Curl Flow", Theme::GetSectionGlow(simIdx++), &sectionCurlFlow)) {
        ImGui::Checkbox("Enabled##curl", &e->curlFlow.enabled);
        if (e->curlFlow.enabled) {
            ImGui::SliderInt("Agents##curl", &e->curlFlow.agentCount, 1000, 1000000);
            ImGui::SliderFloat("Frequency", &e->curlFlow.noiseFrequency, 0.001f, 0.1f, "%.4f");
            ImGui::SliderFloat("Evolution", &e->curlFlow.noiseEvolution, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Momentum", &e->curlFlow.momentum, 0.0f, 0.99f, "%.2f");
            ImGui::SliderFloat("Density Influence", &e->curlFlow.trailInfluence, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Sense Blend##curl", &e->curlFlow.accumSenseBlend, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Gradient Radius", &e->curlFlow.gradientRadius, 1.0f, 32.0f, "%.0f px");
            ImGui::SliderFloat("Step Size##curl", &e->curlFlow.stepSize, 0.5f, 5.0f, "%.1f px");
            ImGui::SliderFloat("Deposit##curl", &e->curlFlow.depositAmount, 0.01f, 0.2f, "%.3f");
            ImGui::SliderFloat("Decay##curl", &e->curlFlow.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##curl", &e->curlFlow.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost##curl", &e->curlFlow.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->curlFlow.blendMode;
            if (ImGui::Combo("Blend Mode##curl", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->curlFlow.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->curlFlow.color);
            ImGui::Checkbox("Debug##curl", &e->curlFlow.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Attractor Flow", Theme::GetSectionGlow(simIdx++), &sectionAttractorFlow)) {
        ImGui::Checkbox("Enabled##attr", &e->attractorFlow.enabled);
        if (e->attractorFlow.enabled) {
            const char* attractorTypes[] = {"Lorenz", "Rossler", "Aizawa", "Thomas"};
            int attractorType = (int)e->attractorFlow.attractorType;
            if (ImGui::Combo("Attractor##attr", &attractorType, attractorTypes, 4)) {
                e->attractorFlow.attractorType = (AttractorType)attractorType;
            }
            ImGui::SliderInt("Agents##attr", &e->attractorFlow.agentCount, 10000, 500000);
            ImGui::SliderFloat("Time Scale", &e->attractorFlow.timeScale, 0.001f, 0.1f, "%.3f");
            ImGui::SliderFloat("Scale##attr", &e->attractorFlow.attractorScale, 0.005f, 0.1f, "%.3f");
            ImGui::SliderFloat("X##attr", &e->attractorFlow.x, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Y##attr", &e->attractorFlow.y, 0.0f, 1.0f, "%.2f");
            ModulatableSliderAngleDeg("Angle X##attr", &e->attractorFlow.rotationAngleX,
                                      "attractorFlow.rotationAngleX", modSources);
            ModulatableSliderAngleDeg("Angle Y##attr", &e->attractorFlow.rotationAngleY,
                                      "attractorFlow.rotationAngleY", modSources);
            ModulatableSliderAngleDeg("Angle Z##attr", &e->attractorFlow.rotationAngleZ,
                                      "attractorFlow.rotationAngleZ", modSources);
            ModulatableSliderAngleDeg("Spin X##attr", &e->attractorFlow.rotationSpeedX,
                                      "attractorFlow.rotationSpeedX", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Spin Y##attr", &e->attractorFlow.rotationSpeedY,
                                      "attractorFlow.rotationSpeedY", modSources, "%.1f °/s");
            ModulatableSliderAngleDeg("Spin Z##attr", &e->attractorFlow.rotationSpeedZ,
                                      "attractorFlow.rotationSpeedZ", modSources, "%.1f °/s");
            if (e->attractorFlow.attractorType == ATTRACTOR_LORENZ) {
                ImGui::SliderFloat("Sigma", &e->attractorFlow.sigma, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Rho", &e->attractorFlow.rho, 10.0f, 50.0f, "%.1f");
                ImGui::SliderFloat("Beta", &e->attractorFlow.beta, 0.5f, 5.0f, "%.2f");
            } else if (e->attractorFlow.attractorType == ATTRACTOR_ROSSLER) {
                ImGui::SliderFloat("C", &e->attractorFlow.rosslerC, 4.0f, 7.0f, "%.2f");
            } else if (e->attractorFlow.attractorType == ATTRACTOR_THOMAS) {
                ImGui::SliderFloat("B", &e->attractorFlow.thomasB, 0.17f, 0.22f, "%.4f");
            }
            ImGui::SliderFloat("Deposit##attr", &e->attractorFlow.depositAmount, 0.01f, 0.2f, "%.3f");
            ImGui::SliderFloat("Decay##attr", &e->attractorFlow.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##attr", &e->attractorFlow.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost##attr", &e->attractorFlow.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->attractorFlow.blendMode;
            if (ImGui::Combo("Blend Mode##attr", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->attractorFlow.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->attractorFlow.color);
            ImGui::Checkbox("Debug##attr", &e->attractorFlow.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Boids", Theme::GetSectionGlow(simIdx++), &sectionBoids)) {
        ImGui::Checkbox("Enabled##boids", &e->boids.enabled);
        if (e->boids.enabled) {
            int boundsMode = (int)e->boids.boundsMode;
            if (ImGui::Combo("Bounds##boids", &boundsMode, BOIDS_BOUNDS_MODES, 2)) {
                e->boids.boundsMode = (BoidsBoundsMode)boundsMode;
            }
            ImGui::SliderInt("Agents##boids", &e->boids.agentCount, 1000, 125000);
            ImGui::SliderFloat("Perception##boids", &e->boids.perceptionRadius, 10.0f, 100.0f, "%.0f px");
            ImGui::SliderFloat("Separation##boids", &e->boids.separationRadius, 5.0f, 50.0f, "%.0f px");
            ModulatableSlider("Cohesion##boids", &e->boids.cohesionWeight,
                              "boids.cohesionWeight", "%.2f", modSources);
            ModulatableSlider("Separation Wt##boids", &e->boids.separationWeight,
                              "boids.separationWeight", "%.2f", modSources);
            ModulatableSlider("Alignment##boids", &e->boids.alignmentWeight,
                              "boids.alignmentWeight", "%.2f", modSources);
            ImGui::SliderFloat("Hue Affinity##boids", &e->boids.hueAffinity, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Accum Repulsion##boids", &e->boids.accumRepulsion, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Max Speed##boids", &e->boids.maxSpeed, 1.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("Min Speed##boids", &e->boids.minSpeed, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Deposit##boids", &e->boids.depositAmount, 0.01f, 2.0f, "%.3f");
            ImGui::SliderFloat("Decay##boids", &e->boids.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##boids", &e->boids.diffusionScale, 0, 4);
            ImGui::SliderFloat("Boost##boids", &e->boids.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->boids.blendMode;
            if (ImGui::Combo("Blend Mode##boids", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->boids.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->boids.color);
            ImGui::Checkbox("Debug##boids", &e->boids.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Curl Advection", Theme::GetSectionGlow(simIdx++), &sectionCurlAdvection)) {
        ImGui::Checkbox("Enabled##curlAdv", &e->curlAdvection.enabled);
        if (e->curlAdvection.enabled) {
            ImGui::SliderInt("Steps##curlAdv", &e->curlAdvection.steps, 10, 80);
            ModulatableSlider("Advection Curl##curlAdv", &e->curlAdvection.advectionCurl,
                              "curlAdvection.advectionCurl", "%.2f", modSources);
            ModulatableSlider("Curl Scale##curlAdv", &e->curlAdvection.curlScale,
                              "curlAdvection.curlScale", "%.2f", modSources);
            ImGui::SliderFloat("Laplacian##curlAdv", &e->curlAdvection.laplacianScale, 0.0f, 0.2f, "%.3f");
            ImGui::SliderFloat("Pressure##curlAdv", &e->curlAdvection.pressureScale, -4.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Div Scale##curlAdv", &e->curlAdvection.divergenceScale, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Div Update##curlAdv", &e->curlAdvection.divergenceUpdate, -0.1f, 0.1f, "%.3f");
            ImGui::SliderFloat("Div Smooth##curlAdv", &e->curlAdvection.divergenceSmoothing, 0.0f, 1.0f, "%.2f");
            ModulatableSlider("Self Amp##curlAdv", &e->curlAdvection.selfAmp,
                              "curlAdvection.selfAmp", "%.2f", modSources);
            ImGui::SliderFloat("Update Smooth##curlAdv", &e->curlAdvection.updateSmoothing, 0.1f, 0.9f, "%.2f");
            ModulatableSlider("Injection##curlAdv", &e->curlAdvection.injectionIntensity,
                              "curlAdvection.injectionIntensity", "%.2f", modSources);
            ImGui::SliderFloat("Inj Threshold##curlAdv", &e->curlAdvection.injectionThreshold, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Boost##curlAdv", &e->curlAdvection.boostIntensity, 0.0f, 5.0f);
            int blendModeInt = (int)e->curlAdvection.blendMode;
            if (ImGui::Combo("Blend Mode##curlAdv", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->curlAdvection.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->curlAdvection.color);
            ImGui::Checkbox("Debug##curlAdv", &e->curlAdvection.debugOverlay);
        }
        DrawSectionEnd();
    }

    ImGui::Spacing();

    if (DrawSectionBegin("Cymatics", Theme::GetSectionGlow(simIdx++), &sectionCymatics)) {
        ImGui::Checkbox("Enabled##cym", &e->cymatics.enabled);
        if (e->cymatics.enabled) {
            ModulatableSlider("Wave Scale##cym", &e->cymatics.waveScale,
                              "cymatics.waveScale", "%.1f", modSources);
            ModulatableSlider("Falloff##cym", &e->cymatics.falloff,
                              "cymatics.falloff", "%.2f", modSources);
            ModulatableSlider("Gain##cym", &e->cymatics.visualGain,
                              "cymatics.visualGain", "%.2f", modSources);
            ImGui::SliderInt("Contours##cym", &e->cymatics.contourCount, 0, 10);
            ImGui::SliderFloat("Decay##cym", &e->cymatics.decayHalfLife, 0.1f, 5.0f, "%.2f s");
            ImGui::SliderInt("Diffusion##cym", &e->cymatics.diffusionScale, 0, 4);
            ImGui::SliderInt("Sources##cym", &e->cymatics.sourceCount, 1, 8);
            ModulatableSlider("Base Radius##cym", &e->cymatics.baseRadius,
                              "cymatics.baseRadius", "%.2f", modSources);
            ModulatableSliderAngleDeg("Pattern Angle##cym", &e->cymatics.patternAngle,
                                      "cymatics.patternAngle", modSources);
            ImGui::SliderFloat("Source Amplitude##cym", &e->cymatics.sourceAmplitude, 0.0f, 0.5f, "%.2f");
            ImGui::SliderFloat("Source Freq X##cym", &e->cymatics.sourceFreqX, 0.01f, 0.2f, "%.3f Hz");
            ImGui::SliderFloat("Source Freq Y##cym", &e->cymatics.sourceFreqY, 0.01f, 0.2f, "%.3f Hz");
            ModulatableSlider("Boost##cym", &e->cymatics.boostIntensity,
                              "cymatics.boostIntensity", "%.2f", modSources);
            int blendModeInt = (int)e->cymatics.blendMode;
            if (ImGui::Combo("Blend Mode##cym", &blendModeInt, BLEND_MODES, BLEND_MODE_COUNT)) {
                e->cymatics.blendMode = (EffectBlendMode)blendModeInt;
            }
            ImGuiDrawColorMode(&e->cymatics.color);
            ImGui::Checkbox("Debug##cym", &e->cymatics.debugOverlay);
        }
        DrawSectionEnd();
    }

    // -------------------------------------------------------------------------
    // TRANSFORMS GROUP
    // -------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Spacing();
    DrawGroupHeader("TRANSFORMS", Theme::ACCENT_CYAN_U32);

    // Pipeline list - shows only enabled effects
    if (ImGui::BeginListBox("##PipelineList", ImVec2(-FLT_MIN, 120))) {
        const float listWidth = ImGui::GetContentRegionAvail().x;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        int visibleRow = 0;

        for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
            const TransformEffectType type = e->transformOrder[i];
            if (!IsTransformEnabled(e, type)) { continue; }

            const char* name = TransformEffectName(type);
            const TransformCategory cat = GetTransformCategory(type);
            const bool isSelected = (selectedTransformEffect == i);

            ImGui::PushID(i);

            // Get row bounds for background drawing
            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
            const ImVec2 rowMax = ImVec2(rowMin.x + listWidth, rowMin.y + rowHeight);

            // Alternating row background (subtle)
            if (visibleRow % 2 == 1) {
                drawList->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 8));
            }

            // Full-width selectable (provides highlight and drag source)
            if (ImGui::Selectable("", isSelected, ImGuiSelectableFlags_AllowOverlap,
                                  ImVec2(listWidth, 0))) {
                selectedTransformEffect = i;
            }

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("TRANSFORM_ORDER", &i, sizeof(int));
                ImGui::Text("%s", name);
                ImGui::EndDragDropSource();
            }

            // Drop target with cyan highlight
            if (ImGui::BeginDragDropTarget()) {
                // Draw cyan highlight on drop target
                drawList->AddRectFilled(rowMin, rowMax, SetColorAlpha(Theme::ACCENT_CYAN_U32, 60));
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TRANSFORM_ORDER")) {
                    const int srcIdx = *(const int*)payload->Data;
                    if (srcIdx != i) {
                        // Move: remove from srcIdx, insert at i
                        const TransformEffectType moving = e->transformOrder[srcIdx];
                        if (srcIdx < i) {
                            // Shift left: items between src and dst move down
                            for (int j = srcIdx; j < i; j++) {
                                e->transformOrder[j] = e->transformOrder[j + 1];
                            }
                        } else {
                            // Shift right: items between dst and src move up
                            for (int j = srcIdx; j > i; j--) {
                                e->transformOrder[j] = e->transformOrder[j - 1];
                            }
                        }
                        e->transformOrder[i] = moving;
                        selectedTransformEffect = i;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Overlay content on same line
            ImGui::SameLine(4);

            // Drag handle (dimmed)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Theme::TEXT_SECONDARY_U32));
            ImGui::Text("::");
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Effect name
            ImGui::Text("%s", name);

            // Category badge (colored, right-aligned) - uses same color cycle as section headers
            ImGui::SameLine(listWidth - 35);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Theme::GetSectionAccent(cat.sectionIndex)));
            ImGui::Text("%s", cat.badge);
            ImGui::PopStyleColor();

            ImGui::PopID();
            visibleRow++;
        }

        if (visibleRow == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_SECONDARY);
            ImGui::Text("No effects enabled");
            ImGui::PopStyleColor();
        }

        ImGui::EndListBox();
    }

    // Transform subcategories (extracted to imgui_effects_transforms.cpp)
    ImGui::Spacing();
    DrawSymmetryCategory(e, modSources);
    ImGui::Spacing();
    DrawWarpCategory(e, modSources);
    ImGui::Spacing();
    DrawCellularCategory(e, modSources);
    ImGui::Spacing();
    DrawMotionCategory(e, modSources);
    ImGui::Spacing();
    DrawStyleCategory(e, modSources);
    ImGui::Spacing();
    DrawColorCategory(e, modSources);

    ImGui::End();
}
