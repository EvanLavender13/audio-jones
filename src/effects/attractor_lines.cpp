// Attractor lines effect module implementation
// Traces 3D strange attractor trajectories as glowing lines with trail
// persistence via ping-pong render textures

#include "attractor_lines.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/attractor_types.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "external/glad.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static void CacheLocations(AttractorLinesEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->attractorTypeLoc = GetShaderLocation(e->shader, "attractorType");
  e->sigmaLoc = GetShaderLocation(e->shader, "sigma");
  e->rhoLoc = GetShaderLocation(e->shader, "rho");
  e->betaLoc = GetShaderLocation(e->shader, "beta");
  e->rosslerCLoc = GetShaderLocation(e->shader, "rosslerC");
  e->thomasBLoc = GetShaderLocation(e->shader, "thomasB");
  e->dadrasALoc = GetShaderLocation(e->shader, "dadrasA");
  e->dadrasBLoc = GetShaderLocation(e->shader, "dadrasB");
  e->dadrasCLoc = GetShaderLocation(e->shader, "dadrasC");
  e->dadrasDLoc = GetShaderLocation(e->shader, "dadrasD");
  e->dadrasELoc = GetShaderLocation(e->shader, "dadrasE");
  e->chuaAlphaLoc = GetShaderLocation(e->shader, "chuaAlpha");
  e->chuaGammaLoc = GetShaderLocation(e->shader, "chuaGamma");
  e->chuaM0Loc = GetShaderLocation(e->shader, "chuaM0");
  e->chuaM1Loc = GetShaderLocation(e->shader, "chuaM1");
  e->stepsLoc = GetShaderLocation(e->shader, "steps");
  e->speedLoc = GetShaderLocation(e->shader, "speed");

  e->viewScaleLoc = GetShaderLocation(e->shader, "viewScale");
  e->intensityLoc = GetShaderLocation(e->shader, "intensity");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->focusLoc = GetShaderLocation(e->shader, "focus");
  e->maxSpeedLoc = GetShaderLocation(e->shader, "maxSpeed");
  e->xLoc = GetShaderLocation(e->shader, "x");
  e->yLoc = GetShaderLocation(e->shader, "y");
  e->rotationMatrixLoc = GetShaderLocation(e->shader, "rotationMatrix");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->numParticlesLoc = GetShaderLocation(e->shader, "numParticles");
}

static void InitPingPong(AttractorLinesEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "ATTRACTOR_LINES");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "ATTRACTOR_LINES");
}

static void UnloadPingPong(const AttractorLinesEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool AttractorLinesEffectInit(AttractorLinesEffect *e,
                              const AttractorLinesConfig *cfg, int width,
                              int height) {
  e->shader = LoadShader(NULL, "shaders/attractor_lines.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  e->readIdx = 0;
  e->lastType = cfg->attractorType;
  e->rotationAccumX = 0.0f;
  e->rotationAccumY = 0.0f;
  e->rotationAccumZ = 0.0f;

  // Both textures start cleared to black by RenderUtilsInitTextureHDR

  return true;
}

static void BuildRotationMatrix(float rotX, float rotY, float rotZ,
                                float *out) {
  const float cx = cosf(rotX);
  const float sx = sinf(rotX);
  const float cy = cosf(rotY);
  const float sy = sinf(rotY);
  const float cz = cosf(rotZ);
  const float sz = sinf(rotZ);

  // Rz * Ry * Rx, column-major for OpenGL
  out[0] = cy * cz;
  out[1] = cy * sz;
  out[2] = -sy;
  out[3] = sx * sy * cz - cx * sz;
  out[4] = sx * sy * sz + cx * cz;
  out[5] = sx * cy;
  out[6] = cx * sy * cz + sx * sz;
  out[7] = cx * sy * sz - sx * cz;
  out[8] = cx * cy;
}

static void BindScalarUniforms(const AttractorLinesEffect *e,
                               const AttractorLinesConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight) {
  const float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  int attractorType = (int)cfg->attractorType;
  SetShaderValue(e->shader, e->attractorTypeLoc, &attractorType,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->sigmaLoc, &cfg->sigma, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rhoLoc, &cfg->rho, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->betaLoc, &cfg->beta, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rosslerCLoc, &cfg->rosslerC,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->thomasBLoc, &cfg->thomasB, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dadrasALoc, &cfg->dadrasA, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dadrasBLoc, &cfg->dadrasB, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dadrasCLoc, &cfg->dadrasC, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dadrasDLoc, &cfg->dadrasD, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dadrasELoc, &cfg->dadrasE, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chuaAlphaLoc, &cfg->chuaAlpha,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chuaGammaLoc, &cfg->chuaGamma,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chuaM0Loc, &cfg->chuaM0, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->chuaM1Loc, &cfg->chuaM1, SHADER_UNIFORM_FLOAT);

  int steps = cfg->steps;
  SetShaderValue(e->shader, e->stepsLoc, &steps, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->speedLoc, &cfg->speed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->viewScaleLoc, &cfg->viewScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->intensityLoc, &cfg->intensity,
                 SHADER_UNIFORM_FLOAT);
  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->focusLoc, &cfg->focus, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxSpeedLoc, &cfg->maxSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->xLoc, &cfg->x, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->yLoc, &cfg->y, SHADER_UNIFORM_FLOAT);

  int numParticles = cfg->numParticles;
  SetShaderValue(e->shader, e->numParticlesLoc, &numParticles,
                 SHADER_UNIFORM_INT);
}

void AttractorLinesEffectSetup(AttractorLinesEffect *e,
                               const AttractorLinesConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight) {
  // Reset trails when attractor type changes
  if (cfg->attractorType != e->lastType) {
    RenderUtilsClearTexture(&e->pingPong[0]);
    RenderUtilsClearTexture(&e->pingPong[1]);
    e->readIdx = 0;
    e->lastType = cfg->attractorType;
  }

  e->rotationAccumX += cfg->rotationSpeedX * deltaTime;
  e->rotationAccumY += cfg->rotationSpeedY * deltaTime;
  e->rotationAccumZ += cfg->rotationSpeedZ * deltaTime;

  float rotationMatrix[9];
  BuildRotationMatrix(cfg->rotationAngleX + e->rotationAccumX,
                      cfg->rotationAngleY + e->rotationAccumY,
                      cfg->rotationAngleZ + e->rotationAccumZ, rotationMatrix);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  BindScalarUniforms(e, cfg, deltaTime, screenWidth, screenHeight);
  glUniformMatrix3fv(e->rotationMatrixLoc, 1, GL_FALSE, rotationMatrix);
}

void AttractorLinesEffectRender(AttractorLinesEffect *e,
                                const AttractorLinesConfig *cfg,
                                float deltaTime, int screenWidth,
                                int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  // Texture bindings use raylib's activeTextureId[] which resets on every batch
  // flush. They MUST be set after BeginTextureMode/BeginShaderMode (both
  // flush).
  SetShaderValueTexture(e->shader, e->previousFrameLoc,
                        e->pingPong[e->readIdx].texture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void AttractorLinesEffectResize(AttractorLinesEffect *e, int width,
                                int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void AttractorLinesEffectUninit(AttractorLinesEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

void AttractorLinesRegisterParams(AttractorLinesConfig *cfg) {
  ModEngineRegisterParam("attractorLines.speed", &cfg->speed, 0.05f, 1.0f);

  ModEngineRegisterParam("attractorLines.viewScale", &cfg->viewScale, 0.005f,
                         0.1f);
  ModEngineRegisterParam("attractorLines.intensity", &cfg->intensity, 0.01f,
                         1.0f);
  ModEngineRegisterParam("attractorLines.decayHalfLife", &cfg->decayHalfLife,
                         0.1f, 10.0f);
  ModEngineRegisterParam("attractorLines.focus", &cfg->focus, 0.5f, 5.0f);
  ModEngineRegisterParam("attractorLines.maxSpeed", &cfg->maxSpeed, 5.0f,
                         200.0f);
  ModEngineRegisterParam("attractorLines.x", &cfg->x, 0.0f, 1.0f);
  ModEngineRegisterParam("attractorLines.y", &cfg->y, 0.0f, 1.0f);
  ModEngineRegisterParam("attractorLines.rotationAngleX", &cfg->rotationAngleX,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("attractorLines.rotationAngleY", &cfg->rotationAngleY,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("attractorLines.rotationAngleZ", &cfg->rotationAngleZ,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("attractorLines.rotationSpeedX", &cfg->rotationSpeedX,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("attractorLines.rotationSpeedY", &cfg->rotationSpeedY,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("attractorLines.rotationSpeedZ", &cfg->rotationSpeedZ,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("attractorLines.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

AttractorLinesEffect *GetAttractorLinesEffect(PostEffect *pe) {
  return (AttractorLinesEffect *)
      pe->effectStates[TRANSFORM_ATTRACTOR_LINES_BLEND];
}

void SetupAttractorLines(PostEffect *pe) {
  AttractorLinesEffectSetup(GetAttractorLinesEffect(pe),
                            &pe->effects.attractorLines, pe->currentDeltaTime,
                            pe->screenWidth, pe->screenHeight);
}

void SetupAttractorLinesBlend(PostEffect *pe) {
  AttractorLinesEffect *e = GetAttractorLinesEffect(pe);
  BlendCompositorApply(pe->blendCompositor, e->pingPong[e->readIdx].texture,
                       pe->effects.attractorLines.blendIntensity,
                       pe->effects.attractorLines.blendMode);
}

void RenderAttractorLines(PostEffect *pe) {
  AttractorLinesEffectRender(GetAttractorLinesEffect(pe),
                             &pe->effects.attractorLines, pe->currentDeltaTime,
                             pe->screenWidth, pe->screenHeight);
}

// === UI ===

static void DrawAttractorSystemParamsUI(AttractorLinesConfig *c) {
  if (c->attractorType == ATTRACTOR_LORENZ) {
    ImGui::SliderFloat("Sigma##attractorLines", &c->sigma, 1.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Rho##attractorLines", &c->rho, 10.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Beta##attractorLines", &c->beta, 0.5f, 5.0f, "%.2f");
  } else if (c->attractorType == ATTRACTOR_ROSSLER) {
    ImGui::SliderFloat("Rossler C##attractorLines", &c->rosslerC, 2.0f, 12.0f,
                       "%.2f");
  } else if (c->attractorType == ATTRACTOR_THOMAS) {
    ImGui::SliderFloat("Thomas B##attractorLines", &c->thomasB, 0.1f, 0.3f,
                       "%.3f");
  } else if (c->attractorType == ATTRACTOR_DADRAS) {
    ImGui::SliderFloat("Dadras A##attractorLines", &c->dadrasA, 1.0f, 5.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras B##attractorLines", &c->dadrasB, 1.0f, 5.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras C##attractorLines", &c->dadrasC, 0.5f, 3.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras D##attractorLines", &c->dadrasD, 0.5f, 4.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras E##attractorLines", &c->dadrasE, 4.0f, 15.0f,
                       "%.1f");
  } else if (c->attractorType == ATTRACTOR_CHUA) {
    ImGui::SliderFloat("Alpha##chua", &c->chuaAlpha, 5.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Gamma##chua", &c->chuaGamma, 10.0f, 40.0f, "%.2f");
    ImGui::SliderFloat("M0##chua", &c->chuaM0, -3.0f, 0.0f, "%.1f");
    ImGui::SliderFloat("M1##chua", &c->chuaM1, -1.0f, 1.0f, "%.2f");
  }
}

static void DrawAttractorLinesParams(EffectConfig *e,
                                     const ModSources *modSources,
                                     ImU32 categoryGlow) {
  (void)categoryGlow;
  AttractorLinesConfig *c = &e->attractorLines;

  const char *attractorNames[] = {"Lorenz", "Rossler", "Aizawa",
                                  "Thomas", "Dadras",  "Chua"};
  int attractorType = (int)c->attractorType;
  if (ImGui::Combo("Attractor Type##attractorLines", &attractorType,
                   attractorNames, ATTRACTOR_COUNT)) {
    c->attractorType = (AttractorType)attractorType;
  }

  DrawAttractorSystemParamsUI(c);

  ImGui::SeparatorText("Tracing");
  ImGui::SliderInt("Particles##attractorLines", &c->numParticles, 1, 16);
  ImGui::SliderInt("Steps##attractorLines", &c->steps, 4, 48);
  ModulatableSlider("Speed##attractorLines", &c->speed, "attractorLines.speed",
                    "%.2f", modSources);
  ModulatableSlider("View Scale##attractorLines", &c->viewScale,
                    "attractorLines.viewScale", "%.3f", modSources);

  ImGui::SeparatorText("Glow");
  ModulatableSlider("Intensity##attractorLines", &c->intensity,
                    "attractorLines.intensity", "%.2f", modSources);
  ModulatableSlider("Decay Half-Life##attractorLines", &c->decayHalfLife,
                    "attractorLines.decayHalfLife", "%.1f", modSources);
  ModulatableSlider("Focus##attractorLines", &c->focus, "attractorLines.focus",
                    "%.1f", modSources);
  ModulatableSlider("Max Speed##attractorLines", &c->maxSpeed,
                    "attractorLines.maxSpeed", "%.0f", modSources);

  ImGui::SeparatorText("Transform");
  ModulatableSlider("X Position##attractorLines", &c->x, "attractorLines.x",
                    "%.2f", modSources);
  ModulatableSlider("Y Position##attractorLines", &c->y, "attractorLines.y",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Angle X##attractorLines", &c->rotationAngleX,
                            "attractorLines.rotationAngleX", modSources);
  ModulatableSliderAngleDeg("Angle Y##attractorLines", &c->rotationAngleY,
                            "attractorLines.rotationAngleY", modSources);
  ModulatableSliderAngleDeg("Angle Z##attractorLines", &c->rotationAngleZ,
                            "attractorLines.rotationAngleZ", modSources);
  ModulatableSliderSpeedDeg("Spin X##attractorLines", &c->rotationSpeedX,
                            "attractorLines.rotationSpeedX", modSources);
  ModulatableSliderSpeedDeg("Spin Y##attractorLines", &c->rotationSpeedY,
                            "attractorLines.rotationSpeedY", modSources);
  ModulatableSliderSpeedDeg("Spin Z##attractorLines", &c->rotationSpeedZ,
                            "attractorLines.rotationSpeedZ", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(attractorLines)
REGISTER_GENERATOR_FULL(TRANSFORM_ATTRACTOR_LINES_BLEND, AttractorLines,
                        attractorLines, "Attractor Lines",
                        SetupAttractorLinesBlend, SetupAttractorLines,
                        RenderAttractorLines, 11, DrawAttractorLinesParams,
                        DrawOutput_attractorLines)
// clang-format on
