#include "trail_map.h"
#include "external/glad.h"
#include "rlgl.h"
#include "shader_utils.h"
#include <math.h>
#include <stdlib.h>

static const char *TRAIL_SHADER_PATH = "shaders/trail_diffusion.glsl";

static bool CreateRenderTexture(RenderTexture2D *rt, int width, int height) {
  rt->id = rlLoadFramebuffer();
  if (rt->id == 0) {
    return false;
  }

  rlEnableFramebuffer(rt->id);
  rt->texture.id = rlLoadTexture(NULL, width, height,
                                 RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
  rt->texture.width = width;
  rt->texture.height = height;
  rt->texture.mipmaps = 1;
  rt->texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
  rlFramebufferAttach(rt->id, rt->texture.id, RL_ATTACHMENT_COLOR_CHANNEL0,
                      RL_ATTACHMENT_TEXTURE2D, 0);

  if (!rlFramebufferComplete(rt->id)) {
    rlUnloadFramebuffer(rt->id);
    rlUnloadTexture(rt->texture.id);
    rt->id = 0;
    rt->texture.id = 0;
    return false;
  }

  rlDisableFramebuffer();
  rt->depth.id = 0;

  BeginTextureMode(*rt);
  ClearBackground(BLACK);
  EndTextureMode();

  return true;
}

static void ClearRenderTexture(RenderTexture2D *rt) {
  BeginTextureMode(*rt);
  ClearBackground(BLACK);
  EndTextureMode();
}

static GLuint LoadTrailProgram(TrailMap *tm) {
  char *shaderSource = SimLoadShaderSource(TRAIL_SHADER_PATH);
  if (shaderSource == NULL) {
    return 0;
  }

  const unsigned int shaderId =
      rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
  UnloadFileText(shaderSource);

  if (shaderId == 0) {
    TraceLog(LOG_ERROR, "TRAILMAP: Failed to compile trail shader");
    return 0;
  }

  const GLuint program = rlLoadComputeShaderProgram(shaderId);
  if (program == 0) {
    TraceLog(LOG_ERROR, "TRAILMAP: Failed to load trail shader program");
    return 0;
  }

  tm->resolutionLoc = rlGetLocationUniform(program, "resolution");
  tm->diffusionScaleLoc = rlGetLocationUniform(program, "diffusionScale");
  tm->decayFactorLoc = rlGetLocationUniform(program, "decayFactor");
  tm->applyDecayLoc = rlGetLocationUniform(program, "applyDecay");
  tm->directionLoc = rlGetLocationUniform(program, "direction");

  return program;
}

TrailMap *TrailMapInit(int width, int height) {
  TrailMap *tm = (TrailMap *)calloc(1, sizeof(TrailMap));
  if (tm == NULL) {
    return NULL;
  }

  tm->width = width;
  tm->height = height;

  if (!CreateRenderTexture(&tm->primary, width, height)) {
    TraceLog(LOG_ERROR, "TRAILMAP: Failed to create primary texture");
    goto cleanup;
  }

  if (!CreateRenderTexture(&tm->temp, width, height)) {
    TraceLog(LOG_ERROR, "TRAILMAP: Failed to create temp texture");
    goto cleanup;
  }

  tm->program = LoadTrailProgram(tm);
  if (tm->program == 0) {
    goto cleanup;
  }

  TraceLog(LOG_INFO, "TRAILMAP: Initialized at %dx%d", width, height);
  return tm;

cleanup:
  TrailMapUninit(tm);
  return NULL;
}

void TrailMapUninit(TrailMap *tm) {
  if (tm == NULL) {
    return;
  }

  if (tm->program != 0) {
    rlUnloadShaderProgram(tm->program);
  }
  UnloadRenderTexture(tm->temp);
  UnloadRenderTexture(tm->primary);
  free(tm);
}

void TrailMapResize(TrailMap *tm, int width, int height) {
  if (tm == NULL || (width == tm->width && height == tm->height)) {
    return;
  }

  RenderTexture2D newPrimary = {0};
  RenderTexture2D newTemp = {0};

  if (!CreateRenderTexture(&newPrimary, width, height)) {
    TraceLog(LOG_ERROR,
             "TRAILMAP: Failed to recreate primary texture after resize");
    return;
  }

  if (!CreateRenderTexture(&newTemp, width, height)) {
    TraceLog(LOG_ERROR,
             "TRAILMAP: Failed to recreate temp texture after resize");
    UnloadRenderTexture(newPrimary);
    return;
  }

  UnloadRenderTexture(tm->primary);
  UnloadRenderTexture(tm->temp);
  tm->primary = newPrimary;
  tm->temp = newTemp;
  tm->width = width;
  tm->height = height;
}

void TrailMapClear(TrailMap *tm) {
  if (tm == NULL) {
    return;
  }

  ClearRenderTexture(&tm->primary);
  ClearRenderTexture(&tm->temp);
}

void TrailMapProcess(TrailMap *tm, float deltaTime, float decayHalfLife,
                     int diffusionScale) {
  if (tm == NULL || tm->program == 0) {
    return;
  }

  const float safeHalfLife = fmaxf(decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);

  rlEnableShader(tm->program);

  float resolution[2] = {(float)tm->width, (float)tm->height};
  rlSetUniform(tm->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
  rlSetUniform(tm->diffusionScaleLoc, &diffusionScale, RL_SHADER_UNIFORM_INT,
               1);
  rlSetUniform(tm->decayFactorLoc, &decayFactor, RL_SHADER_UNIFORM_FLOAT, 1);

  const int workGroupsX = (tm->width + 15) / 16;
  const int workGroupsY = (tm->height + 15) / 16;

  // Horizontal pass: primary -> temp
  int direction = 0;
  int applyDecay = 0;
  rlSetUniform(tm->directionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
  rlSetUniform(tm->applyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
  rlBindImageTexture(tm->primary.texture.id, 1,
                     RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
  rlBindImageTexture(tm->temp.texture.id, 2,
                     RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
  rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY,
                          1);

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  // Vertical pass with decay: temp -> primary
  direction = 1;
  applyDecay = 1;
  rlSetUniform(tm->directionLoc, &direction, RL_SHADER_UNIFORM_INT, 1);
  rlSetUniform(tm->applyDecayLoc, &applyDecay, RL_SHADER_UNIFORM_INT, 1);
  rlBindImageTexture(tm->temp.texture.id, 1,
                     RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, true);
  rlBindImageTexture(tm->primary.texture.id, 2,
                     RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);
  rlComputeShaderDispatch((unsigned int)workGroupsX, (unsigned int)workGroupsY,
                          1);

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                  GL_TEXTURE_FETCH_BARRIER_BIT);

  rlDisableShader();
}

bool TrailMapBeginDraw(TrailMap *tm) {
  if (tm == NULL) {
    return false;
  }
  BeginTextureMode(tm->primary);
  return true;
}

void TrailMapEndDraw(TrailMap *tm) {
  if (tm == NULL) {
    return;
  }
  EndTextureMode();
}

Texture2D TrailMapGetTexture(const TrailMap *tm) {
  if (tm == NULL) {
    return Texture2D{};
  }
  return tm->primary.texture;
}
