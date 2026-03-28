#include "maze_worms.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "external/glad.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_config.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "rlgl.h"
#include "shader_utils.h"
#include "trail_map.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stdlib.h>

static const char *COMPUTE_SHADER_PATH = "shaders/maze_worm_agents.glsl";
static const int MARGIN = 10;

static void InitializeAgents(MazeWormAgent *agents, int count, int width,
                             int height, const ColorConfig *color) {
  for (int i = 0; i < count; i++) {
    agents[i].x = (float)(GetRandomValue(MARGIN, width - 1 - MARGIN));
    agents[i].y = (float)(GetRandomValue(MARGIN, height - 1 - MARGIN));
    agents[i].angle = (float)GetRandomValue(0, 628) / 100.0f;
    agents[i].age = 1.0f;
    agents[i].alive = 1.0f;
    agents[i].hue = ColorConfigAgentHue(color, i, count);
    agents[i].respawnTimer = 0.0f;
    agents[i]._pad = 0.0f;
  }
}

bool MazeWormsSupported(void) {
  const int version = rlGetVersion();
  return version == RL_OPENGL_43;
}

static GLuint LoadComputeProgram(MazeWorms *mw) {
  char *shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
  if (shaderSource == NULL) {
    return 0;
  }

  const unsigned int shaderId =
      rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
  UnloadFileText(shaderSource);

  if (shaderId == 0) {
    TraceLog(LOG_ERROR, "MAZE_WORMS: Failed to compile compute shader");
    return 0;
  }

  const GLuint program = rlLoadComputeShaderProgram(shaderId);
  if (program == 0) {
    TraceLog(LOG_ERROR, "MAZE_WORMS: Failed to load compute shader program");
    return 0;
  }

  mw->resolutionLoc = rlGetLocationUniform(program, "resolution");
  mw->turningModeLoc = rlGetLocationUniform(program, "turningMode");
  mw->curvatureLoc = rlGetLocationUniform(program, "curvature");
  mw->turnAngleLoc = rlGetLocationUniform(program, "turnAngle");
  mw->trailWidthLoc = rlGetLocationUniform(program, "trailWidth");
  mw->collisionGapLoc = rlGetLocationUniform(program, "collisionGap");
  mw->timeLoc = rlGetLocationUniform(program, "time");
  mw->gradientLUTLoc = rlGetLocationUniform(program, "gradientLUT");
  mw->respawnCooldownLoc = rlGetLocationUniform(program, "respawnCooldown");
  mw->stepDeltaTimeLoc = rlGetLocationUniform(program, "stepDeltaTime");

  return program;
}

static GLuint CreateAgentBuffer(int agentCount, int width, int height,
                                const ColorConfig *color) {
  MazeWormAgent *agents =
      static_cast<MazeWormAgent *>(malloc(agentCount * sizeof(MazeWormAgent)));
  if (agents == NULL) {
    return 0;
  }

  InitializeAgents(agents, agentCount, width, height, color);
  const GLuint buffer = rlLoadShaderBuffer(agentCount * sizeof(MazeWormAgent),
                                           agents, RL_DYNAMIC_COPY);
  free(agents);

  if (buffer == 0) {
    TraceLog(LOG_ERROR, "MAZE_WORMS: Failed to create agent SSBO");
  }
  return buffer;
}

MazeWorms *MazeWormsInit(int width, int height, const MazeWormsConfig *config) {
  if (!MazeWormsSupported()) {
    TraceLog(LOG_WARNING,
             "MAZE_WORMS: Compute shaders not supported (requires OpenGL 4.3)");
    return NULL;
  }

  MazeWorms *mw = static_cast<MazeWorms *>(calloc(1, sizeof(MazeWorms)));
  if (mw == NULL) {
    return NULL;
  }

  mw->width = width;
  mw->height = height;
  mw->config = (config != NULL) ? *config : MazeWormsConfig{};
  mw->agentCount = mw->config.wormCount;
  if (mw->agentCount < 4) {
    mw->agentCount = 4;
  }
  mw->time = 0.0f;
  mw->supported = true;

  mw->computeProgram = LoadComputeProgram(mw);
  if (mw->computeProgram == 0) {
    goto cleanup;
  }

  mw->trailMap = TrailMapInit(width, height);
  if (mw->trailMap == NULL) {
    TraceLog(LOG_ERROR, "MAZE_WORMS: Failed to create trail map");
    goto cleanup;
  }
  TrailMapClear(mw->trailMap);

  mw->colorLUT = ColorLUTInit(&mw->config.color);
  if (mw->colorLUT == NULL) {
    TraceLog(LOG_ERROR, "MAZE_WORMS: Failed to create color LUT");
    goto cleanup;
  }

  mw->agentBuffer =
      CreateAgentBuffer(mw->agentCount, width, height, &mw->config.color);
  if (mw->agentBuffer == 0) {
    goto cleanup;
  }

  TraceLog(LOG_INFO, "MAZE_WORMS: Initialized with %d agents at %dx%d",
           mw->agentCount, width, height);
  return mw;

cleanup:
  MazeWormsUninit(mw);
  return NULL;
}

void MazeWormsUninit(MazeWorms *mw) {
  if (mw == NULL) {
    return;
  }

  rlUnloadShaderBuffer(mw->agentBuffer);
  TrailMapUninit(mw->trailMap);
  ColorLUTUninit(mw->colorLUT);
  rlUnloadShaderProgram(mw->computeProgram);
  free(mw);
}

void MazeWormsUpdate(MazeWorms *mw, float deltaTime) {
  if (mw == NULL || !mw->supported || !mw->config.enabled ||
      mw->agentBuffer == 0) {
    return;
  }

  mw->time += deltaTime;

  const float resolution[2] = {(float)mw->width, (float)mw->height};
  const int turningMode = (int)mw->config.turningMode;

  for (int step = 0; step < mw->config.stepsPerFrame; step++) {
    rlEnableShader(mw->computeProgram);

    rlSetUniform(mw->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(mw->turningModeLoc, &turningMode, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(mw->curvatureLoc, &mw->config.curvature,
                 RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(mw->turnAngleLoc, &mw->config.turnAngle,
                 RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(mw->trailWidthLoc, &mw->config.trailWidth,
                 RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(mw->collisionGapLoc, &mw->config.collisionGap,
                 RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(mw->timeLoc, &mw->time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(mw->respawnCooldownLoc, &mw->config.respawnCooldown,
                 RL_SHADER_UNIFORM_FLOAT, 1);
    const float stepDt = deltaTime / (float)mw->config.stepsPerFrame;
    rlSetUniform(mw->stepDeltaTimeLoc, &stepDt, RL_SHADER_UNIFORM_FLOAT, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ColorLUTGetTexture(mw->colorLUT).id);
    glUniform1i(mw->gradientLUTLoc, 0);

    rlBindShaderBuffer(mw->agentBuffer, 0);
    rlBindImageTexture(TrailMapGetTexture(mw->trailMap).id, 1,
                       RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    const int workGroupSize = 1024;
    const int numGroups = (mw->agentCount + workGroupSize - 1) / workGroupSize;
    rlComputeShaderDispatch((unsigned int)numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_SHADER_STORAGE_BARRIER_BIT);

    rlDisableShader();
  }
}

void MazeWormsProcessTrails(MazeWorms *mw, float deltaTime) {
  if (mw == NULL || !mw->supported || !mw->config.enabled) {
    return;
  }

  TrailMapProcess(mw->trailMap, deltaTime, mw->config.decayHalfLife,
                  mw->config.diffusionScale);
}

void MazeWormsResize(MazeWorms *mw, int width, int height) {
  if (mw == NULL || (width == mw->width && height == mw->height)) {
    return;
  }

  mw->width = width;
  mw->height = height;

  TrailMapResize(mw->trailMap, width, height);
  MazeWormsReset(mw);
}

void MazeWormsReset(MazeWorms *mw) {
  if (mw == NULL) {
    return;
  }

  TrailMapClear(mw->trailMap);

  MazeWormAgent *agents = static_cast<MazeWormAgent *>(
      malloc(mw->agentCount * sizeof(MazeWormAgent)));
  if (agents == NULL) {
    return;
  }

  InitializeAgents(agents, mw->agentCount, mw->width, mw->height,
                   &mw->config.color);
  rlUpdateShaderBuffer(mw->agentBuffer, agents,
                       mw->agentCount * sizeof(MazeWormAgent), 0);
  free(agents);
}

void MazeWormsRegisterParams(MazeWormsConfig *cfg) {
  ModEngineRegisterParam("mazeWorms.curvature", &cfg->curvature, 0.01f, 10.0f);
  ModEngineRegisterParam("mazeWorms.turnAngle", &cfg->turnAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("mazeWorms.trailWidth", &cfg->trailWidth, 0.5f, 5.0f);
  ModEngineRegisterParam("mazeWorms.decayHalfLife", &cfg->decayHalfLife, 0.5f,
                         30.0f);
  ModEngineRegisterParam("mazeWorms.respawnCooldown", &cfg->respawnCooldown,
                         0.0f, 5.0f);
  ModEngineRegisterParam("mazeWorms.collisionGap", &cfg->collisionGap, 0.0f,
                         5.0f);
  ModEngineRegisterParam("mazeWorms.boostIntensity", &cfg->boostIntensity, 0.0f,
                         2.0f);
}

void MazeWormsApplyConfig(MazeWorms *mw, const MazeWormsConfig *newConfig) {
  if (mw == NULL || newConfig == NULL) {
    return;
  }

  int newWormCount = newConfig->wormCount;
  if (newWormCount < 4) {
    newWormCount = 4;
  }

  const bool needsBufferRealloc = (newWormCount != mw->agentCount);
  const bool needsHueReinit =
      !ColorConfigEquals(&mw->config.color, &newConfig->color);

  ColorLUTUpdate(mw->colorLUT, &newConfig->color);
  mw->config = *newConfig;

  if (needsBufferRealloc) {
    rlUnloadShaderBuffer(mw->agentBuffer);
    mw->agentCount = newWormCount;

    MazeWormAgent *agents = static_cast<MazeWormAgent *>(
        malloc(mw->agentCount * sizeof(MazeWormAgent)));
    if (agents == NULL) {
      mw->agentBuffer = 0;
      return;
    }

    InitializeAgents(agents, mw->agentCount, mw->width, mw->height,
                     &mw->config.color);
    mw->agentBuffer = rlLoadShaderBuffer(mw->agentCount * sizeof(MazeWormAgent),
                                         agents, RL_DYNAMIC_COPY);
    free(agents);

    TrailMapClear(mw->trailMap);

    TraceLog(LOG_INFO, "MAZE_WORMS: Reallocated buffer for %d agents",
             mw->agentCount);
  } else if (needsHueReinit) {
    MazeWormsReset(mw);
  }
}

// === UI ===

static const char *TURNING_MODE_NAMES[] = {"Spiral", "Wall Follow", "Wall Hug",
                                           "Mixed"};
static const int TURNING_MODE_COUNT = 4;

static void DrawMazeWormsParams(EffectConfig *e, const ModSources *ms, ImU32) {
  ImGui::SeparatorText("Simulation");
  ImGui::SliderInt("Worm Count", &e->mazeWorms.wormCount, 4, 200);
  int turningMode = (int)e->mazeWorms.turningMode;
  if (ImGui::Combo("Turning Mode", &turningMode, TURNING_MODE_NAMES,
                   TURNING_MODE_COUNT)) {
    e->mazeWorms.turningMode = (MazeWormTurningMode)turningMode;
  }
  ModulatableSlider("Curvature", &e->mazeWorms.curvature, "mazeWorms.curvature",
                    "%.2f", ms);
  if (e->mazeWorms.turningMode == MAZE_WORM_TURN_WALL_FOLLOW ||
      e->mazeWorms.turningMode == MAZE_WORM_TURN_WALL_HUG) {
    ModulatableSliderAngleDeg("Turn Angle", &e->mazeWorms.turnAngle,
                              "mazeWorms.turnAngle", ms);
  }
  ModulatableSlider("Trail Width", &e->mazeWorms.trailWidth,
                    "mazeWorms.trailWidth", "%.1f", ms);
  ModulatableSlider("Collision Gap", &e->mazeWorms.collisionGap,
                    "mazeWorms.collisionGap", "%.1f", ms);
  ImGui::SliderInt("Steps/Frame", &e->mazeWorms.stepsPerFrame, 1, 8);

  ImGui::SeparatorText("Trail");
  ModulatableSlider("Decay Half-Life", &e->mazeWorms.decayHalfLife,
                    "mazeWorms.decayHalfLife", "%.1f s", ms);
  ModulatableSlider("Respawn Cooldown", &e->mazeWorms.respawnCooldown,
                    "mazeWorms.respawnCooldown", "%.2f s", ms);
  ImGui::SliderInt("Diffusion", &e->mazeWorms.diffusionScale, 0, 5);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Boost Intensity", &e->mazeWorms.boostIntensity,
                    "mazeWorms.boostIntensity", "%.2f", ms);
  int blendModeInt = (int)e->mazeWorms.blendMode;
  if (ImGui::Combo("Blend Mode##mw", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    e->mazeWorms.blendMode = (EffectBlendMode)blendModeInt;
  }
  ImGuiDrawColorMode(&e->mazeWorms.color);
}

void SetupMazeWormsTrailBoost(PostEffect *pe) {
  if (pe->mazeWorms == NULL) {
    return;
  }
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->mazeWorms->trailMap),
      pe->effects.mazeWorms.boostIntensity, pe->effects.mazeWorms.blendMode);
}

// clang-format off
REGISTER_SIM_BOOST(TRANSFORM_MAZE_WORMS, mazeWorms, "Maze Worms",
                   SetupMazeWormsTrailBoost, MazeWormsRegisterParams, DrawMazeWormsParams)
// clang-format on
