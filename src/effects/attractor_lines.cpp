// Attractor lines effect module implementation
// Traces 3D strange attractor trajectories as glowing lines with trail
// persistence via ping-pong render textures

#include "attractor_lines.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "external/glad.h"
#include "render/color_lut.h"
#include "render/render_utils.h"
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
}

static void InitPingPong(AttractorLinesEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "ATTRACTOR_LINES");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "ATTRACTOR_LINES");
}

static void UnloadPingPong(AttractorLinesEffect *e) {
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
  const float cx = cosf(rotX), sx = sinf(rotX);
  const float cy = cosf(rotY), sy = sinf(rotY);
  const float cz = cosf(rotZ), sz = sinf(rotZ);

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

static void BindScalarUniforms(AttractorLinesEffect *e,
                               const AttractorLinesConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight) {
  float resolution[2] = {(float)screenWidth, (float)screenHeight};
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

  SetShaderValue(e->shader, e->stepsLoc, &cfg->steps, SHADER_UNIFORM_FLOAT);
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

  int writeIdx = 1 - e->readIdx;
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

AttractorLinesConfig AttractorLinesConfigDefault(void) {
  return AttractorLinesConfig{};
}

void AttractorLinesRegisterParams(AttractorLinesConfig *cfg) {
  ModEngineRegisterParam("attractorLines.sigma", &cfg->sigma, 1.0f, 30.0f);
  ModEngineRegisterParam("attractorLines.rho", &cfg->rho, 10.0f, 50.0f);
  ModEngineRegisterParam("attractorLines.beta", &cfg->beta, 0.5f, 5.0f);
  ModEngineRegisterParam("attractorLines.rosslerC", &cfg->rosslerC, 2.0f,
                         12.0f);
  ModEngineRegisterParam("attractorLines.thomasB", &cfg->thomasB, 0.1f, 0.3f);
  ModEngineRegisterParam("attractorLines.dadrasA", &cfg->dadrasA, 1.0f, 5.0f);
  ModEngineRegisterParam("attractorLines.dadrasB", &cfg->dadrasB, 1.0f, 5.0f);
  ModEngineRegisterParam("attractorLines.dadrasC", &cfg->dadrasC, 0.5f, 3.0f);
  ModEngineRegisterParam("attractorLines.dadrasD", &cfg->dadrasD, 0.5f, 4.0f);
  ModEngineRegisterParam("attractorLines.dadrasE", &cfg->dadrasE, 4.0f, 15.0f);
  ModEngineRegisterParam("attractorLines.steps", &cfg->steps, 32.0f, 256.0f);
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
