#include "physarum.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "external/glad.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_config.h"
#include "render/post_effect.h"
#include "rlgl.h"
#include "shader_utils.h"
#include "trail_map.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stdlib.h>

static const char *COMPUTE_SHADER_PATH = "shaders/physarum_agents.glsl";

static void InitializeAgents(PhysarumAgent *agents, int count, int width,
                             int height, const ColorConfig *color) {
  for (int i = 0; i < count; i++) {
    agents[i].x = (float)(GetRandomValue(0, width - 1));
    agents[i].y = (float)(GetRandomValue(0, height - 1));
    agents[i].heading = (float)GetRandomValue(0, 628) / 100.0f;
    agents[i].hue = ColorConfigAgentHue(color, i, count);
  }
}

bool PhysarumSupported(void) {
  const int version = rlGetVersion();
  return version == RL_OPENGL_43;
}

// Load and link the agent compute shader, returning the program ID (0 on
// failure)
static GLuint LoadComputeProgram(Physarum *p) {
  char *shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
  if (shaderSource == NULL) {
    return 0;
  }

  const unsigned int shaderId =
      rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
  UnloadFileText(shaderSource);

  if (shaderId == 0) {
    TraceLog(LOG_ERROR, "PHYSARUM: Failed to compile compute shader");
    return 0;
  }

  const GLuint program = rlLoadComputeShaderProgram(shaderId);
  if (program == 0) {
    TraceLog(LOG_ERROR, "PHYSARUM: Failed to load compute shader program");
    return 0;
  }

  p->resolutionLoc = rlGetLocationUniform(program, "resolution");
  p->sensorDistanceLoc = rlGetLocationUniform(program, "sensorDistance");
  p->sensorDistanceVarianceLoc =
      rlGetLocationUniform(program, "sensorDistanceVariance");
  p->sensorAngleLoc = rlGetLocationUniform(program, "sensorAngle");
  p->turningAngleLoc = rlGetLocationUniform(program, "turningAngle");
  p->stepSizeLoc = rlGetLocationUniform(program, "stepSize");
  p->levyAlphaLoc = rlGetLocationUniform(program, "levyAlpha");
  p->depositAmountLoc = rlGetLocationUniform(program, "depositAmount");
  p->timeLoc = rlGetLocationUniform(program, "time");
  p->saturationLoc = rlGetLocationUniform(program, "saturation");
  p->valueLoc = rlGetLocationUniform(program, "value");
  p->accumSenseBlendLoc = rlGetLocationUniform(program, "accumSenseBlend");
  p->repulsionStrengthLoc = rlGetLocationUniform(program, "repulsionStrength");
  p->samplingExponentLoc = rlGetLocationUniform(program, "samplingExponent");
  p->vectorSteeringLoc = rlGetLocationUniform(program, "vectorSteering");
  p->boundsModeLoc = rlGetLocationUniform(program, "boundsMode");
  p->attractorCountLoc = rlGetLocationUniform(program, "attractorCount");
  p->respawnModeLoc = rlGetLocationUniform(program, "respawnMode");
  p->gravityStrengthLoc = rlGetLocationUniform(program, "gravityStrength");
  p->orbitOffsetLoc = rlGetLocationUniform(program, "orbitOffset");
  p->attractorsLoc = rlGetLocationUniform(program, "attractors");
  p->walkModeLoc = rlGetLocationUniform(program, "walkMode");
  p->densityResponseLoc = rlGetLocationUniform(program, "densityResponse");
  p->cauchyScaleLoc = rlGetLocationUniform(program, "cauchyScale");
  p->expScaleLoc = rlGetLocationUniform(program, "expScale");
  p->gaussianVarianceLoc = rlGetLocationUniform(program, "gaussianVariance");
  p->sprintFactorLoc = rlGetLocationUniform(program, "sprintFactor");
  p->gradientBoostLoc = rlGetLocationUniform(program, "gradientBoost");

  return program;
}

// Create and upload the agent SSBO, returning the buffer ID (0 on failure)
static GLuint CreateAgentBuffer(int agentCount, int width, int height,
                                const ColorConfig *color) {
  PhysarumAgent *agents =
      (PhysarumAgent *)malloc(agentCount * sizeof(PhysarumAgent));
  if (agents == NULL) {
    return 0;
  }

  InitializeAgents(agents, agentCount, width, height, color);
  const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(PhysarumAgent),
                                           agents, RL_DYNAMIC_COPY);
  free(agents);

  if (buffer == 0) {
    TraceLog(LOG_ERROR, "PHYSARUM: Failed to create agent SSBO");
  }
  return buffer;
}

Physarum *PhysarumInit(int width, int height, const PhysarumConfig *config) {
  if (!PhysarumSupported()) {
    TraceLog(LOG_WARNING,
             "PHYSARUM: Compute shaders not supported (requires OpenGL 4.3)");
    return NULL;
  }

  Physarum *p = (Physarum *)calloc(1, sizeof(Physarum));
  if (p == NULL) {
    return NULL;
  }

  p->width = width;
  p->height = height;
  p->config = (config != NULL) ? *config : PhysarumConfig{};
  p->agentCount = p->config.agentCount;
  if (p->agentCount < 1) {
    p->agentCount = 1;
  }
  p->time = 0.0f;
  p->supported = true;

  p->computeProgram = LoadComputeProgram(p);
  if (p->computeProgram == 0) {
    goto cleanup;
  }

  p->trailMap = TrailMapInit(width, height);
  if (p->trailMap == NULL) {
    TraceLog(LOG_ERROR, "PHYSARUM: Failed to create trail map");
    goto cleanup;
  }

  p->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
  if (p->debugShader.id == 0) {
    TraceLog(LOG_WARNING,
             "PHYSARUM: Failed to load debug shader, using default");
  }

  p->agentBuffer =
      CreateAgentBuffer(p->agentCount, width, height, &p->config.color);
  if (p->agentBuffer == 0) {
    goto cleanup;
  }

  TraceLog(LOG_INFO, "PHYSARUM: Initialized with %d agents at %dx%d",
           p->agentCount, width, height);
  return p;

cleanup:
  PhysarumUninit(p);
  return NULL;
}

void PhysarumUninit(Physarum *p) {
  if (p == NULL) {
    return;
  }

  rlUnloadShaderBuffer(p->agentBuffer);
  TrailMapUninit(p->trailMap);
  if (p->debugShader.id != 0) {
    UnloadShader(p->debugShader);
  }
  rlUnloadShaderProgram(p->computeProgram);
  free(p);
}

void PhysarumRegisterParams(PhysarumConfig *cfg) {
  ModEngineRegisterParam("physarum.sensorDistance", &cfg->sensorDistance, 1.0f,
                         100.0f);
  ModEngineRegisterParam("physarum.sensorDistanceVariance",
                         &cfg->sensorDistanceVariance, 0.0f, 20.0f);
  ModEngineRegisterParam("physarum.sensorAngle", &cfg->sensorAngle, 0.0f,
                         6.28f);
  ModEngineRegisterParam("physarum.turningAngle", &cfg->turningAngle, 0.0f,
                         6.28f);
  ModEngineRegisterParam("physarum.stepSize", &cfg->stepSize, 0.1f, 100.0f);
  ModEngineRegisterParam("physarum.levyAlpha", &cfg->levyAlpha, 0.1f, 3.0f);
  ModEngineRegisterParam("physarum.densityResponse", &cfg->densityResponse,
                         0.1f, 5.0f);
  ModEngineRegisterParam("physarum.cauchyScale", &cfg->cauchyScale, 0.1f, 2.0f);
  ModEngineRegisterParam("physarum.expScale", &cfg->expScale, 0.1f, 3.0f);
  ModEngineRegisterParam("physarum.gaussianVariance", &cfg->gaussianVariance,
                         0.0f, 1.0f);
  ModEngineRegisterParam("physarum.sprintFactor", &cfg->sprintFactor, 0.0f,
                         5.0f);
  ModEngineRegisterParam("physarum.gradientBoost", &cfg->gradientBoost, 0.0f,
                         10.0f);
  ModEngineRegisterParam("physarum.repulsionStrength", &cfg->repulsionStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("physarum.samplingExponent", &cfg->samplingExponent,
                         0.0f, 10.0f);
  ModEngineRegisterParam("physarum.gravityStrength", &cfg->gravityStrength,
                         0.0f, 1.0f);
  ModEngineRegisterParam("physarum.orbitOffset", &cfg->orbitOffset, 0.0f, 1.0f);
  ModEngineRegisterParam("physarum.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("physarum.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 10.0f);
  ModEngineRegisterParam("physarum.attractorBaseRadius",
                         &cfg->attractorBaseRadius, 0.1f, 0.5f);
}

void PhysarumUpdate(Physarum *p, float deltaTime, Texture2D accumTexture,
                    Texture2D fftTexture) {
  if (p == NULL || !p->supported || !p->config.enabled) {
    return;
  }

  p->time += deltaTime;

  // Compute attractor positions using shared Lissajous config
  float attractors[16]; // 8 attractors * 2 components
  const int count = p->config.attractorCount;
  DualLissajousUpdateCircular(&p->config.lissajous, deltaTime,
                              p->config.attractorBaseRadius, 0.5f, 0.5f, count,
                              attractors);

  rlEnableShader(p->computeProgram);

  float resolution[2] = {(float)p->width, (float)p->height};
  rlSetUniform(p->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
  rlSetUniform(p->sensorDistanceLoc, &p->config.sensorDistance,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->sensorDistanceVarianceLoc, &p->config.sensorDistanceVariance,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->sensorAngleLoc, &p->config.sensorAngle,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->turningAngleLoc, &p->config.turningAngle,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->stepSizeLoc, &p->config.stepSize, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->levyAlphaLoc, &p->config.levyAlpha, RL_SHADER_UNIFORM_FLOAT,
               1);
  rlSetUniform(p->depositAmountLoc, &p->config.depositAmount,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->timeLoc, &p->time, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->accumSenseBlendLoc, &p->config.accumSenseBlend,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->repulsionStrengthLoc, &p->config.repulsionStrength,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->samplingExponentLoc, &p->config.samplingExponent,
               RL_SHADER_UNIFORM_FLOAT, 1);
  float vectorSteeringVal = p->config.vectorSteering ? 1.0f : 0.0f;
  rlSetUniform(p->vectorSteeringLoc, &vectorSteeringVal,
               RL_SHADER_UNIFORM_FLOAT, 1);
  int boundsMode = (int)p->config.boundsMode;
  rlSetUniform(p->boundsModeLoc, &boundsMode, RL_SHADER_UNIFORM_INT, 1);
  int attractorCount = p->config.attractorCount;
  rlSetUniform(p->attractorCountLoc, &attractorCount, RL_SHADER_UNIFORM_INT, 1);
  float respawnModeVal = p->config.respawnMode ? 1.0f : 0.0f;
  rlSetUniform(p->respawnModeLoc, &respawnModeVal, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->gravityStrengthLoc, &p->config.gravityStrength,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->orbitOffsetLoc, &p->config.orbitOffset,
               RL_SHADER_UNIFORM_FLOAT, 1);
  glUniform2fv(p->attractorsLoc, 8, attractors);
  int walkMode = (int)p->config.walkMode;
  rlSetUniform(p->walkModeLoc, &walkMode, RL_SHADER_UNIFORM_INT, 1);
  rlSetUniform(p->densityResponseLoc, &p->config.densityResponse,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->cauchyScaleLoc, &p->config.cauchyScale,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->expScaleLoc, &p->config.expScale, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->gaussianVarianceLoc, &p->config.gaussianVariance,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->sprintFactorLoc, &p->config.sprintFactor,
               RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->gradientBoostLoc, &p->config.gradientBoost,
               RL_SHADER_UNIFORM_FLOAT, 1);

  float saturation;
  float value;
  ColorConfigGetSV(&p->config.color, &saturation, &value);
  rlSetUniform(p->saturationLoc, &saturation, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(p->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

  rlBindShaderBuffer(p->agentBuffer, 0);
  rlBindImageTexture(TrailMapGetTexture(p->trailMap).id, 1,
                     RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, accumTexture.id);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, fftTexture.id);

  const int workGroupSize = 1024;
  const int numGroups = (p->agentCount + workGroupSize - 1) / workGroupSize;
  rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

  // Ensure compute writes are visible to both image operations and texture
  // fetches
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                  GL_TEXTURE_FETCH_BARRIER_BIT);

  rlDisableShader();
}

void PhysarumProcessTrails(Physarum *p, float deltaTime) {
  if (p == NULL || !p->supported || !p->config.enabled) {
    return;
  }

  TrailMapProcess(p->trailMap, deltaTime, p->config.decayHalfLife,
                  p->config.diffusionScale);
}

void PhysarumDrawDebug(Physarum *p) {
  if (p == NULL || !p->supported || !p->config.enabled) {
    return;
  }

  const Texture2D trailTex = TrailMapGetTexture(p->trailMap);
  if (p->debugShader.id != 0) {
    BeginShaderMode(p->debugShader);
  }
  DrawTextureRec(trailTex, {0, 0, (float)p->width, (float)-p->height}, {0, 0},
                 WHITE);
  if (p->debugShader.id != 0) {
    EndShaderMode();
  }
}

void PhysarumResize(Physarum *p, int width, int height) {
  if (p == NULL || (width == p->width && height == p->height)) {
    return;
  }

  p->width = width;
  p->height = height;

  TrailMapResize(p->trailMap, width, height);

  PhysarumReset(p);
}

void PhysarumReset(Physarum *p) {
  if (p == NULL) {
    return;
  }

  TrailMapClear(p->trailMap);

  PhysarumAgent *agents =
      (PhysarumAgent *)malloc(p->agentCount * sizeof(PhysarumAgent));
  if (agents == NULL) {
    return;
  }

  InitializeAgents(agents, p->agentCount, p->width, p->height,
                   &p->config.color);
  rlUpdateShaderBuffer(p->agentBuffer, agents,
                       p->agentCount * sizeof(PhysarumAgent), 0);
  free(agents);
}

void PhysarumApplyConfig(Physarum *p, const PhysarumConfig *newConfig) {
  if (p == NULL || newConfig == NULL) {
    return;
  }

  int newAgentCount = newConfig->agentCount;
  if (newAgentCount < 1) {
    newAgentCount = 1;
  }

  const bool needsBufferRealloc = (newAgentCount != p->agentCount);
  const bool needsHueReinit =
      !ColorConfigEquals(&p->config.color, &newConfig->color);

  p->config = *newConfig;

  if (needsBufferRealloc) {
    rlUnloadShaderBuffer(p->agentBuffer);
    p->agentCount = newAgentCount;

    PhysarumAgent *agents =
        (PhysarumAgent *)malloc(p->agentCount * sizeof(PhysarumAgent));
    if (agents == NULL) {
      p->agentBuffer = 0;
      return;
    }

    InitializeAgents(agents, p->agentCount, p->width, p->height,
                     &p->config.color);
    p->agentBuffer = rlLoadShaderBuffer(p->agentCount * sizeof(PhysarumAgent),
                                        agents, RL_DYNAMIC_COPY);
    free(agents);

    TrailMapClear(p->trailMap);

    TraceLog(LOG_INFO, "PHYSARUM: Reallocated buffer for %d agents",
             p->agentCount);
  } else if (needsHueReinit) {
    PhysarumReset(p);
  }
}

bool PhysarumBeginTrailMapDraw(Physarum *p) {
  if (p == NULL || !p->supported || !p->config.enabled) {
    return false;
  }
  return TrailMapBeginDraw(p->trailMap);
}

void PhysarumEndTrailMapDraw(Physarum *p) {
  if (p == NULL || !p->supported || !p->config.enabled) {
    return;
  }
  TrailMapEndDraw(p->trailMap);
}

// Bounds mode options for physarum
static const char *PHYSARUM_BOUNDS_MODES[] = {
    "Toroidal",   "Reflect", "Redirect",      "Scatter",    "Random",
    "Fixed Home", "Orbit",   "Species Orbit", "Multi-Home", "Antipodal"};
static const int PHYSARUM_BOUNDS_MODE_COUNT = 10;

// Walk mode options for physarum
static const char *PHYSARUM_WALK_MODES[] = {"Normal", "Levy",        "Adaptive",
                                            "Cauchy", "Exponential", "Gaussian",
                                            "Sprint", "Gradient"};
static const int PHYSARUM_WALK_MODE_COUNT = 8;

static void DrawPhysarumParams(EffectConfig *e, const ModSources *ms, ImU32) {
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
                          "physarum.lissajous", ms, 0.2f);
    ModulatableSlider("Base Radius##phys", &e->physarum.attractorBaseRadius,
                      "physarum.attractorBaseRadius", "%.2f", ms);
  }
  if (boundsMode == PHYSARUM_BOUNDS_SPECIES_ORBIT) {
    ModulatableSlider("Orbit Offset", &e->physarum.orbitOffset,
                      "physarum.orbitOffset", "%.2f", ms);
  }

  ImGui::SeparatorText("Sensing");
  ModulatableSlider("Sensor Dist", &e->physarum.sensorDistance,
                    "physarum.sensorDistance", "%.1f px", ms);
  ModulatableSlider("Sensor Variance", &e->physarum.sensorDistanceVariance,
                    "physarum.sensorDistanceVariance", "%.1f px", ms);
  ModulatableSliderAngleDeg("Sensor Angle", &e->physarum.sensorAngle,
                            "physarum.sensorAngle", ms);
  ModulatableSliderAngleDeg("Turn Angle", &e->physarum.turningAngle,
                            "physarum.turningAngle", ms);
  ImGui::SliderFloat("Sense Blend", &e->physarum.accumSenseBlend, 0.0f, 1.0f);

  ImGui::SeparatorText("Movement");
  ModulatableSlider("Step Size", &e->physarum.stepSize, "physarum.stepSize",
                    "%.1f px", ms);
  int walkModeInt = (int)e->physarum.walkMode;
  if (ImGui::Combo("Walk Mode##phys", &walkModeInt, PHYSARUM_WALK_MODES,
                   PHYSARUM_WALK_MODE_COUNT)) {
    e->physarum.walkMode = (PhysarumWalkMode)walkModeInt;
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_LEVY) {
    ModulatableSlider("Levy Alpha", &e->physarum.levyAlpha,
                      "physarum.levyAlpha", "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_ADAPTIVE) {
    ModulatableSlider("Density Response", &e->physarum.densityResponse,
                      "physarum.densityResponse", "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_CAUCHY) {
    ModulatableSlider("Cauchy Scale", &e->physarum.cauchyScale,
                      "physarum.cauchyScale", "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_EXPONENTIAL) {
    ModulatableSlider("Exp Scale", &e->physarum.expScale, "physarum.expScale",
                      "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_GAUSSIAN) {
    ModulatableSlider("Variance", &e->physarum.gaussianVariance,
                      "physarum.gaussianVariance", "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_SPRINT) {
    ModulatableSlider("Sprint Factor", &e->physarum.sprintFactor,
                      "physarum.sprintFactor", "%.2f", ms);
  }
  if (e->physarum.walkMode == PHYSARUM_WALK_GRADIENT) {
    ModulatableSlider("Gradient Boost", &e->physarum.gradientBoost,
                      "physarum.gradientBoost", "%.2f", ms);
  }
  ModulatableSlider("Gravity", &e->physarum.gravityStrength,
                    "physarum.gravityStrength", "%.2f", ms);
  ImGui::Checkbox("Vector Steering", &e->physarum.vectorSteering);
  ModulatableSlider("Sampling Exp", &e->physarum.samplingExponent,
                    "physarum.samplingExponent", "%.1f", ms);

  ImGui::SeparatorText("Species");
  ModulatableSlider("Repulsion", &e->physarum.repulsionStrength,
                    "physarum.repulsionStrength", "%.2f", ms);

  ImGui::SeparatorText("Trail");
  ImGui::SliderFloat("Deposit", &e->physarum.depositAmount, 0.01f, 5.0f);
  ImGui::SliderFloat("Decay", &e->physarum.decayHalfLife, 0.1f, 5.0f, "%.2f s");
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

void SetupPhysarumTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->physarum->trailMap),
      pe->effects.physarum.boostIntensity, pe->effects.physarum.blendMode);
}

// clang-format off
REGISTER_SIM_BOOST(TRANSFORM_PHYSARUM, physarum, "Physarum",
                   SetupPhysarumTrailBoost, PhysarumRegisterParams, DrawPhysarumParams)
// clang-format on
