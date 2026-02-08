#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "simulation/bounds_mode.h"
#include "simulation/physarum.h"
#include "ui/imgui_effects_artistic.h"
#include "ui/imgui_effects_generators.h"
#include "ui/imgui_effects_graphic.h"
#include "ui/imgui_effects_optical.h"
#include "ui/imgui_effects_retro.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

// Persistent section open states (simulations only - transforms in
// imgui_effects_transforms.cpp)
static bool sectionPhysarum = false;
static bool sectionCurlFlow = false;
static bool sectionCurlAdvection = false;
static bool sectionAttractorFlow = false;
static bool sectionBoids = false;
static bool sectionCymatics = false;
static bool sectionParticleLife = false;
static bool sectionFlowField = false;

// Selection tracking for effect order list
static int selectedTransformEffect = -1;

// Category badge and section index for pipeline list (indices match
// Draw*Category section colors)
struct TransformCategory {
  const char *badge;
  int sectionIndex;
};

static TransformCategory GetTransformCategory(TransformEffectType type) {
  switch (type) {
  // Symmetry - section 0
  case TRANSFORM_KALEIDOSCOPE:
  case TRANSFORM_KIFS:
  case TRANSFORM_POINCARE_DISK:
  case TRANSFORM_MANDELBOX:
  case TRANSFORM_TRIANGLE_FOLD:
  case TRANSFORM_MOIRE_INTERFERENCE:
  case TRANSFORM_RADIAL_IFS:
    return {"SYM", 0};
  // Warp - section 1
  case TRANSFORM_SINE_WARP:
  case TRANSFORM_TEXTURE_WARP:
  case TRANSFORM_GRADIENT_FLOW:
  case TRANSFORM_WAVE_RIPPLE:
  case TRANSFORM_MOBIUS:
  case TRANSFORM_CHLADNI_WARP:
  case TRANSFORM_DOMAIN_WARP:
  case TRANSFORM_SURFACE_WARP:
  case TRANSFORM_INTERFERENCE_WARP:
  case TRANSFORM_CORRIDOR_WARP:
  case TRANSFORM_FFT_RADIAL_WARP:
  case TRANSFORM_RADIAL_PULSE:
  case TRANSFORM_CIRCUIT_BOARD:
    return {"WARP", 1};
  // Cellular - section 2
  case TRANSFORM_VORONOI:
  case TRANSFORM_LATTICE_FOLD:
  case TRANSFORM_PHYLLOTAXIS:
  case TRANSFORM_MULTI_SCALE_GRID:
  case TRANSFORM_DOT_MATRIX:
    return {"CELL", 2};
  // Motion - section 3
  case TRANSFORM_INFINITE_ZOOM:
  case TRANSFORM_RADIAL_STREAK:
  case TRANSFORM_DROSTE_ZOOM:
  case TRANSFORM_DENSITY_WAVE_SPIRAL:
  case TRANSFORM_RELATIVISTIC_DOPPLER:
  case TRANSFORM_SHAKE:
    return {"MOT", 3};
  // Artistic - section 4
  case TRANSFORM_OIL_PAINT:
  case TRANSFORM_WATERCOLOR:
  case TRANSFORM_IMPRESSIONIST:
  case TRANSFORM_INK_WASH:
  case TRANSFORM_PENCIL_SKETCH:
  case TRANSFORM_CROSS_HATCHING:
    return {"ART", 4};
  // Graphic - section 5
  case TRANSFORM_TOON:
  case TRANSFORM_NEON_GLOW:
  case TRANSFORM_KUWAHARA:
  case TRANSFORM_HALFTONE:
  case TRANSFORM_DISCO_BALL:
  case TRANSFORM_LEGO_BRICKS:
    return {"GFX", 5};
  // Retro - section 6
  case TRANSFORM_PIXELATION:
  case TRANSFORM_GLITCH:
  case TRANSFORM_CRT:
  case TRANSFORM_ASCII_ART:
  case TRANSFORM_MATRIX_RAIN:
  case TRANSFORM_SYNTHWAVE:
    return {"RET", 6};
  // Optical - section 7
  case TRANSFORM_BLOOM:
  case TRANSFORM_BOKEH:
  case TRANSFORM_HEIGHTFIELD_RELIEF:
  case TRANSFORM_ANAMORPHIC_STREAK:
    return {"OPT", 7};
  // Color - section 8
  case TRANSFORM_COLOR_GRADE:
  case TRANSFORM_FALSE_COLOR:
  case TRANSFORM_PALETTE_QUANTIZATION:
    return {"COL", 8};
  // Simulation - section 9
  case TRANSFORM_PHYSARUM_BOOST:
  case TRANSFORM_CURL_FLOW_BOOST:
  case TRANSFORM_CURL_ADVECTION_BOOST:
  case TRANSFORM_ATTRACTOR_FLOW_BOOST:
  case TRANSFORM_BOIDS_BOOST:
  case TRANSFORM_CYMATICS_BOOST:
  case TRANSFORM_PARTICLE_LIFE_BOOST:
    return {"SIM", 9};
  // Generators - section 10
  case TRANSFORM_CONSTELLATION_BLEND:
  case TRANSFORM_PLASMA_BLEND:
  case TRANSFORM_INTERFERENCE_BLEND:
  case TRANSFORM_SCAN_BARS_BLEND:
  case TRANSFORM_PITCH_SPIRAL_BLEND:
  case TRANSFORM_MOIRE_GENERATOR_BLEND:
  case TRANSFORM_SPECTRAL_ARCS_BLEND:
  case TRANSFORM_MUONS_BLEND:
  case TRANSFORM_FILAMENTS_BLEND:
  case TRANSFORM_SLASHES_BLEND:
  case TRANSFORM_GLYPH_FIELD_BLEND:
  case TRANSFORM_SPARK_WEB_BLEND:
  case TRANSFORM_SOLID_COLOR:
    return {"GEN", 10};
  default:
    return {"???", 0};
  }
}

// Bounds mode options for simulations
static const char *PHYSARUM_BOUNDS_MODES[] = {
    "Toroidal",   "Reflect", "Redirect",      "Scatter",    "Random",
    "Fixed Home", "Orbit",   "Species Orbit", "Multi-Home", "Antipodal"};
static const int PHYSARUM_BOUNDS_MODE_COUNT = 10;
static const char *BOIDS_BOUNDS_MODES[] = {"Toroidal", "Soft Repulsion"};

// Walk mode options for physarum
static const char *PHYSARUM_WALK_MODES[] = {"Normal", "Levy",        "Adaptive",
                                            "Cauchy", "Exponential", "Gaussian",
                                            "Sprint", "Gradient"};
static const int PHYSARUM_WALK_MODE_COUNT = 8;

// NOLINTNEXTLINE(readability-function-size) - immediate-mode UI requires
// sequential widget calls
void ImGuiDrawEffectsPanel(EffectConfig *e, const ModSources *modSources) {
  if (!ImGui::Begin("Effects")) {
    ImGui::End();
    return;
  }

  int groupIdx = 0;

  // -------------------------------------------------------------------------
  // FEEDBACK GROUP
  // -------------------------------------------------------------------------
  DrawGroupHeader("FEEDBACK", Theme::GetSectionAccent(groupIdx++));

  ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px",
                    modSources);
  ModulatableSliderLog("Motion", &e->motionScale, "effects.motionScale", "%.3f",
                       modSources);
  ImGui::SliderFloat("Half-life", &e->halfLife, 0.1f, 2.0f, "%.2f s");
  ImGui::SliderFloat("Desat", &e->feedbackDesaturate, 0.0f, 0.2f);

  ImGui::Spacing();

  if (DrawSectionBegin("Flow Field", Theme::GetSectionGlow(0),
                       &sectionFlowField)) {
    ImGui::SeparatorText("Base");
    ModulatableSlider("Zoom##base", &e->flowField.zoomBase,
                      "flowField.zoomBase", "%.4f", modSources);
    ModulatableSliderSpeedDeg("Spin##base", &e->flowField.rotationSpeed,
                              "flowField.rotationSpeed", modSources);
    ModulatableSlider("DX##base", &e->flowField.dxBase, "flowField.dxBase",
                      "%.4f", modSources);
    ModulatableSlider("DY##base", &e->flowField.dyBase, "flowField.dyBase",
                      "%.4f", modSources);

    ImGui::SeparatorText("Radial");
    ModulatableSlider("Zoom##radial", &e->flowField.zoomRadial,
                      "flowField.zoomRadial", "%.4f", modSources);
    ModulatableSliderSpeedDeg("Spin##radial", &e->flowField.rotationSpeedRadial,
                              "flowField.rotationSpeedRadial", modSources);
    ModulatableSlider("DX##radial", &e->flowField.dxRadial,
                      "flowField.dxRadial", "%.4f", modSources);
    ModulatableSlider("DY##radial", &e->flowField.dyRadial,
                      "flowField.dyRadial", "%.4f", modSources);

    ImGui::SeparatorText("Angular");
    ModulatableSlider("Zoom##angular", &e->flowField.zoomAngular,
                      "flowField.zoomAngular", "%.4f", modSources);
    ImGui::SliderInt("Zoom Freq", &e->flowField.zoomAngularFreq, 1, 8);
    ModulatableSliderSpeedDeg("Spin##angular", &e->flowField.rotAngular,
                              "flowField.rotAngular", modSources);
    ImGui::SliderInt("Spin Freq", &e->flowField.rotAngularFreq, 1, 8);
    ModulatableSlider("DX##angular", &e->flowField.dxAngular,
                      "flowField.dxAngular", "%.4f", modSources);
    ImGui::SliderInt("DX Freq", &e->flowField.dxAngularFreq, 1, 8);
    ModulatableSlider("DY##angular", &e->flowField.dyAngular,
                      "flowField.dyAngular", "%.4f", modSources);
    ImGui::SliderInt("DY Freq", &e->flowField.dyAngularFreq, 1, 8);

    ImGui::SeparatorText("Center");
    ModulatableSlider("CX", &e->flowField.cx, "flowField.cx", "%.2f",
                      modSources);
    ModulatableSlider("CY", &e->flowField.cy, "flowField.cy", "%.2f",
                      modSources);

    ImGui::SeparatorText("Stretch");
    ModulatableSlider("SX", &e->flowField.sx, "flowField.sx", "%.3f",
                      modSources);
    ModulatableSlider("SY", &e->flowField.sy, "flowField.sy", "%.3f",
                      modSources);

    ImGui::SeparatorText("Warp");
    ModulatableSlider("Warp", &e->proceduralWarp.warp, "proceduralWarp.warp",
                      "%.2f", modSources);
    ModulatableSlider("Warp Speed", &e->proceduralWarp.warpSpeed,
                      "proceduralWarp.warpSpeed", "%.2f", modSources);
    ModulatableSlider("Warp Scale", &e->proceduralWarp.warpScale,
                      "proceduralWarp.warpScale", "%.1f", modSources);

    ImGui::SeparatorText("Gradient Flow");
    ModulatableSlider("Strength", &e->feedbackFlow.strength,
                      "feedbackFlow.strength", "%.1f px", modSources);
    ModulatableSliderAngleDeg("Flow Angle", &e->feedbackFlow.flowAngle,
                              "feedbackFlow.flowAngle", modSources);
    ModulatableSlider("Scale", &e->feedbackFlow.scale, "feedbackFlow.scale",
                      "%.1f", modSources);
    ModulatableSlider("Threshold", &e->feedbackFlow.threshold,
                      "feedbackFlow.threshold", "%.3f", modSources);
    DrawSectionEnd();
  }

  // -------------------------------------------------------------------------
  // OUTPUT GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("OUTPUT", Theme::GetSectionAccent(groupIdx++));

  ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset",
                    "%.0f px", modSources);
  ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f");
  ImGui::SliderFloat("Clarity", &e->clarity, 0.0f, 2.0f, "%.2f");

  // -------------------------------------------------------------------------
  // SIMULATIONS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("SIMULATIONS", Theme::GetSectionAccent(groupIdx++));

  int simIdx = 0;
  if (DrawSectionBegin("Physarum", Theme::GetSectionGlow(simIdx++),
                       &sectionPhysarum)) {
    ImGui::Checkbox("Enabled##phys", &e->physarum.enabled);
    if (e->physarum.enabled) {
      ImGui::SliderInt("Agents", &e->physarum.agentCount, 10000, 1000000);

      ImGui::SeparatorText("Bounds");
      int boundsMode = (int)e->physarum.boundsMode;
      if (ImGui::Combo("Bounds Mode##phys", &boundsMode, PHYSARUM_BOUNDS_MODES,
                       PHYSARUM_BOUNDS_MODE_COUNT)) {
        e->physarum.boundsMode = (PhysarumBoundsMode)boundsMode;
      }
      if (boundsMode == PHYSARUM_BOUNDS_REDIRECT ||
          boundsMode == PHYSARUM_BOUNDS_MULTI_HOME) {
        ImGui::Checkbox("Respawn", &e->physarum.respawnMode);
      }
      if (boundsMode == PHYSARUM_BOUNDS_MULTI_HOME) {
        ImGui::SliderInt("Attractors", &e->physarum.attractorCount, 2, 8);
        DrawLissajousControls(&e->physarum.lissajous, "phys_liss",
                              "physarum.lissajous", modSources, 0.2f);
        ModulatableSlider("Base Radius##phys", &e->physarum.attractorBaseRadius,
                          "physarum.attractorBaseRadius", "%.2f", modSources);
      }
      if (boundsMode == PHYSARUM_BOUNDS_SPECIES_ORBIT) {
        ModulatableSlider("Orbit Offset", &e->physarum.orbitOffset,
                          "physarum.orbitOffset", "%.2f", modSources);
      }

      ImGui::SeparatorText("Sensing");
      ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                        "physarum.sensorDistance", "%.1f px", modSources);
      ModulatableSlider("Sensor Variance", &e->physarum.sensorDistanceVariance,
                        "physarum.sensorDistanceVariance", "%.1f px",
                        modSources);
      ModulatableSliderAngleDeg("Sensor Angle", &e->physarum.sensorAngle,
                                "physarum.sensorAngle", modSources);
      ModulatableSliderAngleDeg("Turn Angle", &e->physarum.turningAngle,
                                "physarum.turningAngle", modSources);
      ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f,
                         1.0f);

      ImGui::SeparatorText("Movement");
      ModulatableSlider("Step Size", &e->physarum.stepSize, "physarum.stepSize",
                        "%.1f px", modSources);
      int walkModeInt = (int)e->physarum.walkMode;
      if (ImGui::Combo("Walk Mode##phys", &walkModeInt, PHYSARUM_WALK_MODES,
                       PHYSARUM_WALK_MODE_COUNT)) {
        e->physarum.walkMode = (PhysarumWalkMode)walkModeInt;
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_LEVY) {
        ModulatableSlider("Levy Alpha", &e->physarum.levyAlpha,
                          "physarum.levyAlpha", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_ADAPTIVE) {
        ModulatableSlider("Density Response", &e->physarum.densityResponse,
                          "physarum.densityResponse", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_CAUCHY) {
        ModulatableSlider("Cauchy Scale", &e->physarum.cauchyScale,
                          "physarum.cauchyScale", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_EXPONENTIAL) {
        ModulatableSlider("Exp Scale", &e->physarum.expScale,
                          "physarum.expScale", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_GAUSSIAN) {
        ModulatableSlider("Variance", &e->physarum.gaussianVariance,
                          "physarum.gaussianVariance", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_SPRINT) {
        ModulatableSlider("Sprint Factor", &e->physarum.sprintFactor,
                          "physarum.sprintFactor", "%.2f", modSources);
      }
      if (e->physarum.walkMode == PHYSARUM_WALK_GRADIENT) {
        ModulatableSlider("Gradient Boost", &e->physarum.gradientBoost,
                          "physarum.gradientBoost", "%.2f", modSources);
      }
      ModulatableSlider("Gravity", &e->physarum.gravityStrength,
                        "physarum.gravityStrength", "%.2f", modSources);
      ImGui::Checkbox("Vector Steering", &e->physarum.vectorSteering);
      ModulatableSlider("Sampling Exp", &e->physarum.samplingExponent,
                        "physarum.samplingExponent", "%.1f", modSources);

      ImGui::SeparatorText("Species");
      ModulatableSlider("Repulsion", &e->physarum.repulsionStrength,
                        "physarum.repulsionStrength", "%.2f", modSources);

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
      ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f,
                         "%.2f s");
      ImGui::SliderInt("Diffusion", &e->physarum.diffusionScale, 0, 4);

      ImGui::SeparatorText("Output");
      ImGui::SliderFloat("Boost", &e->physarum.boostIntensity, 0.0f, 5.0f);
      int blendModeInt = (int)e->physarum.blendMode;
      if (ImGui::Combo("Blend Mode", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->physarum.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->physarum.color);
      ImGui::Checkbox("Debug", &e->physarum.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Curl Flow", Theme::GetSectionGlow(simIdx++),
                       &sectionCurlFlow)) {
    ImGui::Checkbox("Enabled##curl", &e->curlFlow.enabled);
    if (e->curlFlow.enabled) {
      ImGui::SliderInt("Agents##curl", &e->curlFlow.agentCount, 1000, 1000000);

      ImGui::SeparatorText("Field");
      ImGui::SliderFloat("Frequency", &e->curlFlow.noiseFrequency, 0.001f, 0.1f,
                         "%.4f");
      ImGui::SliderFloat("Evolution", &e->curlFlow.noiseEvolution, 0.0f, 2.0f,
                         "%.2f");
      ImGui::SliderFloat("Momentum", &e->curlFlow.momentum, 0.0f, 0.99f,
                         "%.2f");

      ImGui::SeparatorText("Sensing");
      ImGui::SliderFloat("Density Influence", &e->curlFlow.trailInfluence, 0.0f,
                         1.0f, "%.2f");
      ImGui::SliderFloat("Sense Blend##curl", &e->curlFlow.accumSenseBlend,
                         0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Gradient Radius", &e->curlFlow.gradientRadius, 1.0f,
                         32.0f, "%.0f px");

      ImGui::SeparatorText("Movement");
      ImGui::SliderFloat("Step Size##curl", &e->curlFlow.stepSize, 0.5f, 5.0f,
                         "%.1f px");
      ImGui::SliderFloat("Respawn##curl", &e->curlFlow.respawnProbability, 0.0f,
                         0.1f, "%.3f");

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Deposit##curl", &e->curlFlow.depositAmount, 0.01f,
                         0.2f, "%.3f");
      ImGui::SliderFloat("Decay##curl", &e->curlFlow.decayHalfLife, 0.1f, 5.0f,
                         "%.2f s");
      ImGui::SliderInt("Diffusion##curl", &e->curlFlow.diffusionScale, 0, 4);

      ImGui::SeparatorText("Output");
      ImGui::SliderFloat("Boost##curl", &e->curlFlow.boostIntensity, 0.0f,
                         5.0f);
      int blendModeInt = (int)e->curlFlow.blendMode;
      if (ImGui::Combo("Blend Mode##curl", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->curlFlow.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->curlFlow.color);
      ImGui::Checkbox("Debug##curl", &e->curlFlow.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Attractor Flow", Theme::GetSectionGlow(simIdx++),
                       &sectionAttractorFlow)) {
    ImGui::Checkbox("Enabled##attr", &e->attractorFlow.enabled);
    if (e->attractorFlow.enabled) {
      ImGui::SliderInt("Agents##attr", &e->attractorFlow.agentCount, 10000,
                       500000);

      ImGui::SeparatorText("Attractor");
      const char *attractorTypes[] = {"Lorenz", "Rossler", "Aizawa", "Thomas"};
      int attractorType = (int)e->attractorFlow.attractorType;
      if (ImGui::Combo("Type##attr", &attractorType, attractorTypes, 4)) {
        e->attractorFlow.attractorType = (AttractorType)attractorType;
      }
      ImGui::SliderFloat("Time Scale", &e->attractorFlow.timeScale, 0.001f,
                         0.1f, "%.3f");
      ImGui::SliderFloat("Scale##attr", &e->attractorFlow.attractorScale,
                         0.005f, 0.1f, "%.3f");
      if (e->attractorFlow.attractorType == ATTRACTOR_LORENZ) {
        ImGui::SliderFloat("Sigma", &e->attractorFlow.sigma, 1.0f, 20.0f,
                           "%.1f");
        ImGui::SliderFloat("Rho", &e->attractorFlow.rho, 10.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Beta", &e->attractorFlow.beta, 0.5f, 5.0f, "%.2f");
      } else if (e->attractorFlow.attractorType == ATTRACTOR_ROSSLER) {
        ImGui::SliderFloat("C", &e->attractorFlow.rosslerC, 4.0f, 7.0f, "%.2f");
      } else if (e->attractorFlow.attractorType == ATTRACTOR_THOMAS) {
        ImGui::SliderFloat("B", &e->attractorFlow.thomasB, 0.17f, 0.22f,
                           "%.4f");
      }

      ImGui::SeparatorText("Projection");
      ImGui::SliderFloat("X##attr", &e->attractorFlow.x, 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Y##attr", &e->attractorFlow.y, 0.0f, 1.0f, "%.2f");
      ModulatableSliderAngleDeg("Angle X##attr",
                                &e->attractorFlow.rotationAngleX,
                                "attractorFlow.rotationAngleX", modSources);
      ModulatableSliderAngleDeg("Angle Y##attr",
                                &e->attractorFlow.rotationAngleY,
                                "attractorFlow.rotationAngleY", modSources);
      ModulatableSliderAngleDeg("Angle Z##attr",
                                &e->attractorFlow.rotationAngleZ,
                                "attractorFlow.rotationAngleZ", modSources);
      ModulatableSliderSpeedDeg("Spin X##attr",
                                &e->attractorFlow.rotationSpeedX,
                                "attractorFlow.rotationSpeedX", modSources);
      ModulatableSliderSpeedDeg("Spin Y##attr",
                                &e->attractorFlow.rotationSpeedY,
                                "attractorFlow.rotationSpeedY", modSources);
      ModulatableSliderSpeedDeg("Spin Z##attr",
                                &e->attractorFlow.rotationSpeedZ,
                                "attractorFlow.rotationSpeedZ", modSources);

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Deposit##attr", &e->attractorFlow.depositAmount,
                         0.01f, 0.2f, "%.3f");
      ImGui::SliderFloat("Decay##attr", &e->attractorFlow.decayHalfLife, 0.1f,
                         5.0f, "%.2f s");
      ImGui::SliderInt("Diffusion##attr", &e->attractorFlow.diffusionScale, 0,
                       4);

      ImGui::SeparatorText("Output");
      ImGui::SliderFloat("Boost##attr", &e->attractorFlow.boostIntensity, 0.0f,
                         5.0f);
      int blendModeInt = (int)e->attractorFlow.blendMode;
      if (ImGui::Combo("Blend Mode##attr", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->attractorFlow.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->attractorFlow.color);
      ImGui::Checkbox("Debug##attr", &e->attractorFlow.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Boids", Theme::GetSectionGlow(simIdx++),
                       &sectionBoids)) {
    ImGui::Checkbox("Enabled##boids", &e->boids.enabled);
    if (e->boids.enabled) {
      ImGui::SliderInt("Agents##boids", &e->boids.agentCount, 1000, 125000);

      ImGui::SeparatorText("Bounds");
      int boundsMode = (int)e->boids.boundsMode;
      if (ImGui::Combo("Bounds Mode##boids", &boundsMode, BOIDS_BOUNDS_MODES,
                       2)) {
        e->boids.boundsMode = (BoidsBoundsMode)boundsMode;
      }

      ImGui::SeparatorText("Flocking");
      ImGui::SliderFloat("Perception##boids", &e->boids.perceptionRadius, 10.0f,
                         100.0f, "%.0f px");
      ImGui::SliderFloat("Separation Radius##boids", &e->boids.separationRadius,
                         5.0f, 50.0f, "%.0f px");
      ModulatableSlider("Cohesion##boids", &e->boids.cohesionWeight,
                        "boids.cohesionWeight", "%.2f", modSources);
      ModulatableSlider("Separation Wt##boids", &e->boids.separationWeight,
                        "boids.separationWeight", "%.2f", modSources);
      ModulatableSlider("Alignment##boids", &e->boids.alignmentWeight,
                        "boids.alignmentWeight", "%.2f", modSources);
      ImGui::SliderFloat("Accum Repulsion##boids", &e->boids.accumRepulsion,
                         0.0f, 2.0f, "%.2f");

      ImGui::SeparatorText("Species");
      ImGui::SliderFloat("Hue Affinity##boids", &e->boids.hueAffinity, 0.0f,
                         2.0f, "%.2f");

      ImGui::SeparatorText("Movement");
      ImGui::SliderFloat("Max Speed##boids", &e->boids.maxSpeed, 1.0f, 10.0f,
                         "%.1f");
      ImGui::SliderFloat("Min Speed##boids", &e->boids.minSpeed, 0.0f, 2.0f,
                         "%.2f");

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Deposit##boids", &e->boids.depositAmount, 0.01f, 2.0f,
                         "%.3f");
      ImGui::SliderFloat("Decay##boids", &e->boids.decayHalfLife, 0.1f, 5.0f,
                         "%.2f s");
      ImGui::SliderInt("Diffusion##boids", &e->boids.diffusionScale, 0, 4);

      ImGui::SeparatorText("Output");
      ImGui::SliderFloat("Boost##boids", &e->boids.boostIntensity, 0.0f, 5.0f);
      int blendModeInt = (int)e->boids.blendMode;
      if (ImGui::Combo("Blend Mode##boids", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->boids.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->boids.color);
      ImGui::Checkbox("Debug##boids", &e->boids.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Curl Advection", Theme::GetSectionGlow(simIdx++),
                       &sectionCurlAdvection)) {
    ImGui::Checkbox("Enabled##curlAdv", &e->curlAdvection.enabled);
    if (e->curlAdvection.enabled) {
      ImGui::SeparatorText("Field");
      ImGui::SliderInt("Steps##curlAdv", &e->curlAdvection.steps, 10, 80);
      ModulatableSlider("Advection Curl##curlAdv",
                        &e->curlAdvection.advectionCurl,
                        "curlAdvection.advectionCurl", "%.2f", modSources);
      ModulatableSlider("Curl Scale##curlAdv", &e->curlAdvection.curlScale,
                        "curlAdvection.curlScale", "%.2f", modSources);
      ModulatableSlider("Self Amp##curlAdv", &e->curlAdvection.selfAmp,
                        "curlAdvection.selfAmp", "%.2f", modSources);

      ImGui::SeparatorText("Pressure");
      ModulatableSlider("Laplacian##curlAdv", &e->curlAdvection.laplacianScale,
                        "curlAdvection.laplacianScale", "%.3f", modSources);
      ModulatableSlider("Pressure##curlAdv", &e->curlAdvection.pressureScale,
                        "curlAdvection.pressureScale", "%.2f", modSources);
      ModulatableSlider("Div Scale##curlAdv", &e->curlAdvection.divergenceScale,
                        "curlAdvection.divergenceScale", "%.2f", modSources);
      ModulatableSlider("Div Update##curlAdv",
                        &e->curlAdvection.divergenceUpdate,
                        "curlAdvection.divergenceUpdate", "%.3f", modSources);
      ModulatableSlider(
          "Div Smooth##curlAdv", &e->curlAdvection.divergenceSmoothing,
          "curlAdvection.divergenceSmoothing", "%.2f", modSources);
      ModulatableSlider("Update Smooth##curlAdv",
                        &e->curlAdvection.updateSmoothing,
                        "curlAdvection.updateSmoothing", "%.2f", modSources);

      ImGui::SeparatorText("Injection");
      ModulatableSlider("Injection##curlAdv",
                        &e->curlAdvection.injectionIntensity,
                        "curlAdvection.injectionIntensity", "%.2f", modSources);
      ModulatableSlider("Inj Threshold##curlAdv",
                        &e->curlAdvection.injectionThreshold,
                        "curlAdvection.injectionThreshold", "%.2f", modSources);

      ImGui::SeparatorText("Trail");
      ModulatableSlider("Decay##curlAdv", &e->curlAdvection.decayHalfLife,
                        "curlAdvection.decayHalfLife", "%.2f s", modSources);
      ImGui::SliderInt("Diffusion##curlAdv", &e->curlAdvection.diffusionScale,
                       0, 4);

      ImGui::SeparatorText("Output");
      ModulatableSlider("Boost##curlAdv", &e->curlAdvection.boostIntensity,
                        "curlAdvection.boostIntensity", "%.2f", modSources);
      int blendModeInt = (int)e->curlAdvection.blendMode;
      if (ImGui::Combo("Blend Mode##curlAdv", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->curlAdvection.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->curlAdvection.color);
      ImGui::Checkbox("Debug##curlAdv", &e->curlAdvection.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Cymatics", Theme::GetSectionGlow(simIdx++),
                       &sectionCymatics)) {
    ImGui::Checkbox("Enabled##cym", &e->cymatics.enabled);
    if (e->cymatics.enabled) {
      ImGui::SeparatorText("Wave");
      ModulatableSlider("Wave Scale##cym", &e->cymatics.waveScale,
                        "cymatics.waveScale", "%.1f", modSources);
      ModulatableSlider("Falloff##cym", &e->cymatics.falloff,
                        "cymatics.falloff", "%.2f", modSources);
      ModulatableSlider("Gain##cym", &e->cymatics.visualGain,
                        "cymatics.visualGain", "%.2f", modSources);
      ImGui::SliderInt("Contours##cym", &e->cymatics.contourCount, 0, 10);

      ImGui::SeparatorText("Boundaries");
      ImGui::Checkbox("Boundaries##cym", &e->cymatics.boundaries);
      if (e->cymatics.boundaries) {
        ModulatableSlider("Reflection Gain##cym", &e->cymatics.reflectionGain,
                          "cymatics.reflectionGain", "%.2f", modSources);
      }

      ImGui::SeparatorText("Sources");
      ImGui::SliderInt("Source Count##cym", &e->cymatics.sourceCount, 1, 8);
      ModulatableSlider("Base Radius##cym", &e->cymatics.baseRadius,
                        "cymatics.baseRadius", "%.2f", modSources);
      DrawLissajousControls(&e->cymatics.lissajous, "cym_liss",
                            "cymatics.lissajous", modSources, 0.2f);

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Decay##cym", &e->cymatics.decayHalfLife, 0.1f, 5.0f,
                         "%.2f s");
      ImGui::SliderInt("Diffusion##cym", &e->cymatics.diffusionScale, 0, 4);

      ImGui::SeparatorText("Output");
      ModulatableSlider("Boost##cym", &e->cymatics.boostIntensity,
                        "cymatics.boostIntensity", "%.2f", modSources);
      int blendModeInt = (int)e->cymatics.blendMode;
      if (ImGui::Combo("Blend Mode##cym", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->cymatics.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->cymatics.color);
      ImGui::Checkbox("Debug##cym", &e->cymatics.debugOverlay);
    }
    DrawSectionEnd();
  }

  ImGui::Spacing();

  if (DrawSectionBegin("Particle Life", Theme::GetSectionGlow(simIdx++),
                       &sectionParticleLife)) {
    ImGui::Checkbox("Enabled##plife", &e->particleLife.enabled);
    if (e->particleLife.enabled) {
      ImGui::SliderInt("Agents##plife", &e->particleLife.agentCount, 1000,
                       100000);

      ImGui::SeparatorText("Species");
      int speciesCount = e->particleLife.speciesCount;
      if (ImGui::SliderInt("Species##plife", &speciesCount, 2, 16)) {
        e->particleLife.speciesCount = speciesCount;
      }
      ImGui::SliderInt("Seed##plife", &e->particleLife.attractionSeed, 0,
                       99999);
      if (ImGui::Button("Randomize##plife")) {
        e->particleLife.attractionSeed = GetRandomValue(0, 99999);
      }
      ImGui::Checkbox("Symmetric##plife", &e->particleLife.symmetricForces);
      ModulatableSlider("Evo Speed##plife", &e->particleLife.evolutionSpeed,
                        "particleLife.evolutionSpeed", "%.2f", modSources);

      ImGui::SeparatorText("Physics");
      ModulatableSlider("Radius##plife", &e->particleLife.rMax,
                        "particleLife.rMax", "%.2f", modSources);
      ModulatableSlider("Force##plife", &e->particleLife.forceFactor,
                        "particleLife.forceFactor", "%.1f", modSources);
      ModulatableSlider("Momentum##plife", &e->particleLife.momentum,
                        "particleLife.momentum", "%.2f", modSources);
      ModulatableSlider("Beta##plife", &e->particleLife.beta,
                        "particleLife.beta", "%.2f", modSources);
      ImGui::SliderFloat("Bounds##plife", &e->particleLife.boundsRadius, 0.5f,
                         2.0f, "%.2f");
      ImGui::SliderFloat("Boundary Stiffness##plife",
                         &e->particleLife.boundaryStiffness, 0.1f, 5.0f,
                         "%.2f");

      ImGui::SeparatorText("3D View");
      ImGui::SliderFloat("X##plife", &e->particleLife.x, 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Y##plife", &e->particleLife.y, 0.0f, 1.0f, "%.2f");
      ImGui::SliderFloat("Scale##plife", &e->particleLife.projectionScale, 0.1f,
                         1.0f, "%.2f");
      ModulatableSliderAngleDeg("Angle X##plife",
                                &e->particleLife.rotationAngleX,
                                "particleLife.rotationAngleX", modSources);
      ModulatableSliderAngleDeg("Angle Y##plife",
                                &e->particleLife.rotationAngleY,
                                "particleLife.rotationAngleY", modSources);
      ModulatableSliderAngleDeg("Angle Z##plife",
                                &e->particleLife.rotationAngleZ,
                                "particleLife.rotationAngleZ", modSources);
      ModulatableSliderSpeedDeg("Spin X##plife",
                                &e->particleLife.rotationSpeedX,
                                "particleLife.rotationSpeedX", modSources);
      ModulatableSliderSpeedDeg("Spin Y##plife",
                                &e->particleLife.rotationSpeedY,
                                "particleLife.rotationSpeedY", modSources);
      ModulatableSliderSpeedDeg("Spin Z##plife",
                                &e->particleLife.rotationSpeedZ,
                                "particleLife.rotationSpeedZ", modSources);

      ImGui::SeparatorText("Trail");
      ImGui::SliderFloat("Deposit##plife", &e->particleLife.depositAmount,
                         0.01f, 0.5f, "%.3f");
      ImGui::SliderFloat("Decay##plife", &e->particleLife.decayHalfLife, 0.1f,
                         5.0f, "%.2f s");
      ImGui::SliderInt("Diffusion##plife", &e->particleLife.diffusionScale, 0,
                       4);

      ImGui::SeparatorText("Output");
      ImGui::SliderFloat("Boost##plife", &e->particleLife.boostIntensity, 0.0f,
                         5.0f);
      int blendModeInt = (int)e->particleLife.blendMode;
      if (ImGui::Combo("Blend Mode##plife", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        e->particleLife.blendMode = (EffectBlendMode)blendModeInt;
      }
      ImGuiDrawColorMode(&e->particleLife.color);
      ImGui::Checkbox("Debug##plife", &e->particleLife.debugOverlay);
    }
    DrawSectionEnd();
  }

  // -------------------------------------------------------------------------
  // GENERATORS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("GENERATORS", Theme::GetSectionAccent(groupIdx++));
  int genIdx = 0;
  DrawGeneratorsCategory(e, modSources, genIdx);

  // -------------------------------------------------------------------------
  // TRANSFORMS GROUP
  // -------------------------------------------------------------------------
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("TRANSFORMS", Theme::GetSectionAccent(groupIdx++));

  // Pipeline list - shows only enabled effects
  if (ImGui::BeginListBox("##PipelineList", ImVec2(-FLT_MIN, 120))) {
    const float listWidth = ImGui::GetContentRegionAvail().x;
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    int visibleRow = 0;

    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
      const TransformEffectType type = e->transformOrder[i];
      if (!IsTransformEnabled(e, type)) {
        continue;
      }

      const char *name = TransformEffectName(type);
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
        drawList->AddRectFilled(rowMin, rowMax,
                                SetColorAlpha(Theme::ACCENT_CYAN_U32, 60));
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("TRANSFORM_ORDER")) {
          const int srcIdx = *(const int *)payload->Data;
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
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(
                                               Theme::TEXT_SECONDARY_U32));
      ImGui::Text("::");
      ImGui::PopStyleColor();
      ImGui::SameLine();

      // Effect name
      ImGui::Text("%s", name);

      // Category badge (colored, right-aligned) - uses same color cycle as
      // section headers
      ImGui::SameLine(listWidth - 35);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::ColorConvertU32ToFloat4(
                                Theme::GetSectionAccent(cat.sectionIndex)));
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
  DrawArtisticCategory(e, modSources);
  ImGui::Spacing();
  DrawGraphicCategory(e, modSources);
  ImGui::Spacing();
  DrawRetroCategory(e, modSources);
  ImGui::Spacing();
  DrawOpticalCategory(e, modSources);
  ImGui::Spacing();
  DrawColorCategory(e, modSources);

  ImGui::End();
}
