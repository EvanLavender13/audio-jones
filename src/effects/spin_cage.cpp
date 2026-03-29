// SpinCage effect module implementation
// Rotating platonic solid wireframes with per-edge FFT glow -
// CPU-side 3D rotation and perspective projection, shader-side
// distance-to-segment rendering

#include "spin_cage.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "config/platonic_solids.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

bool SpinCageEffectInit(SpinCageEffect *e, const SpinCageConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/spin_cage.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->edgesLoc = GetShaderLocation(e->shader, "edges");
  e->edgeTLoc = GetShaderLocation(e->shader, "edgeT");
  e->edgeCountLoc = GetShaderLocation(e->shader, "edgeCount");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->angleX = 0.0f;
  e->angleY = 0.0f;
  e->angleZ = 0.0f;

  return true;
}

// Rotate vertices by Euler angles and perspective-project to 2D
static void TransformVertices(const ShapeDescriptor *shape, float ax, float ay,
                              float az, float perspective, float scale,
                              float *projectedX, float *projectedY,
                              float *rotatedZ) {
  const float cx = cosf(ax);
  const float sx = sinf(ax);
  const float cy = cosf(ay);
  const float sy = sinf(ay);
  const float cz = cosf(az);
  const float sz = sinf(az);

  // R = Rx(ax) * Ry(ay) * Rz(az)
  const float r00 = cy * cz;
  const float r01 = cy * sz;
  const float r02 = -sy;
  const float r10 = sx * sy * cz - cx * sz;
  const float r11 = sx * sy * sz + cx * cz;
  const float r12 = sx * cy;
  const float r20 = cx * sy * cz + sx * sz;
  const float r21 = cx * sy * sz - sx * cz;
  const float r22 = cx * cy;

  for (int v = 0; v < shape->vertexCount; v++) {
    const float vx = shape->vertices[v][0];
    const float vy = shape->vertices[v][1];
    const float vz = shape->vertices[v][2];

    const float rz = r20 * vx + r21 * vy + r22 * vz;
    rotatedZ[v] = rz;
    const float invDepth = 1.0f / (rz + perspective);
    projectedX[v] = (r00 * vx + r01 * vy + r02 * vz) * invDepth * scale;
    projectedY[v] = (r10 * vx + r11 * vy + r12 * vz) * invDepth * scale;
  }
}

// Upload all shader uniforms for the current frame
static void UploadUniforms(const SpinCageEffect *e, const SpinCageConfig *cfg,
                           const ShapeDescriptor *shape, const float *edges,
                           const float *edgeT, const Texture2D &fftTexture) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValueV(e->shader, e->edgesLoc, edges, SHADER_UNIFORM_VEC4,
                  shape->edgeCount);
  SetShaderValueV(e->shader, e->edgeTLoc, edgeT, SHADER_UNIFORM_FLOAT,
                  shape->edgeCount);
  SetShaderValue(e->shader, e->edgeCountLoc, &shape->edgeCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void SpinCageEffectSetup(SpinCageEffect *e, const SpinCageConfig *cfg,
                         float deltaTime, const Texture2D &fftTexture) {
  int shapeIdx = cfg->shape;
  if (shapeIdx < 0) {
    shapeIdx = 0;
  }
  if (shapeIdx >= SHAPE_COUNT) {
    shapeIdx = SHAPE_COUNT - 1;
  }
  const ShapeDescriptor *shape = &SHAPES[shapeIdx];

  e->angleX += cfg->speedX * cfg->speedMult * deltaTime;
  e->angleY += cfg->speedY * cfg->speedMult * deltaTime;
  e->angleZ += cfg->speedZ * cfg->speedMult * deltaTime;

  float projectedX[20];
  float projectedY[20];
  float rotatedZ[20];
  TransformVertices(shape, e->angleX, e->angleY, e->angleZ, cfg->perspective,
                    cfg->scale, projectedX, projectedY, rotatedZ);

  // Pack edges and compute edgeT
  float edges[MAX_EDGES * 4] = {};
  float edgeT[MAX_EDGES] = {};
  for (int ei = 0; ei < shape->edgeCount; ei++) {
    const int i = shape->edges[ei][0];
    const int j = shape->edges[ei][1];
    edges[ei * 4 + 0] = projectedX[i];
    edges[ei * 4 + 1] = projectedY[i];
    edges[ei * 4 + 2] = projectedX[j];
    edges[ei * 4 + 3] = projectedY[j];

    if (cfg->colorMode == 1) {
      const float avgZ = (rotatedZ[i] + rotatedZ[j]) * 0.5f;
      edgeT[ei] = (avgZ + 1.0f) * 0.5f;
    } else {
      edgeT[ei] = (shape->edgeCount > 1)
                      ? (float)ei / (float)(shape->edgeCount - 1)
                      : 0.5f;
    }
  }

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  UploadUniforms(e, cfg, shape, edges, edgeT, fftTexture);
}

void SpinCageEffectUninit(SpinCageEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void SpinCageRegisterParams(SpinCageConfig *cfg) {
  ModEngineRegisterParam("spinCage.speedX", &cfg->speedX, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spinCage.speedY", &cfg->speedY, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spinCage.speedZ", &cfg->speedZ, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("spinCage.speedMult", &cfg->speedMult, 0.1f, 100.0f);
  ModEngineRegisterParam("spinCage.perspective", &cfg->perspective, 1.0f,
                         20.0f);
  ModEngineRegisterParam("spinCage.scale", &cfg->scale, 0.1f, 5.0f);
  ModEngineRegisterParam("spinCage.lineWidth", &cfg->lineWidth, 0.5f, 10.0f);
  ModEngineRegisterParam("spinCage.glowIntensity", &cfg->glowIntensity, 0.1f,
                         5.0f);
  ModEngineRegisterParam("spinCage.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("spinCage.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("spinCage.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("spinCage.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("spinCage.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("spinCage.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupSpinCage(PostEffect *pe) {
  SpinCageEffectSetup(&pe->spinCage, &pe->effects.spinCage,
                      pe->currentDeltaTime, pe->fftTexture);
}

void SetupSpinCageBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.spinCage.blendIntensity,
                       pe->effects.spinCage.blendMode);
}

// === UI ===

static void DrawSpinCageParams(EffectConfig *e, const ModSources *modSources,
                               ImU32 categoryGlow) {
  (void)categoryGlow;
  SpinCageConfig *cfg = &e->spinCage;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##spinCage", &cfg->baseFreq,
                    "spinCage.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##spinCage", &cfg->maxFreq,
                    "spinCage.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##spinCage", &cfg->gain, "spinCage.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##spinCage", &cfg->curve, "spinCage.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##spinCage", &cfg->baseBright,
                    "spinCage.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::Combo("Shape##spinCage", &cfg->shape,
               "Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0");
  ImGui::Combo("Color Mode##spinCage", &cfg->colorMode, "Edge Index\0Depth\0");

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Speed X##spinCage", &cfg->speedX,
                            "spinCage.speedX", modSources);
  ModulatableSliderSpeedDeg("Speed Y##spinCage", &cfg->speedY,
                            "spinCage.speedY", modSources);
  ModulatableSliderSpeedDeg("Speed Z##spinCage", &cfg->speedZ,
                            "spinCage.speedZ", modSources);
  ModulatableSlider("Speed Mult##spinCage", &cfg->speedMult,
                    "spinCage.speedMult", "%.1f", modSources);

  // Projection
  ImGui::SeparatorText("Projection");
  ModulatableSlider("Perspective##spinCage", &cfg->perspective,
                    "spinCage.perspective", "%.1f", modSources);
  ModulatableSlider("Scale##spinCage", &cfg->scale, "spinCage.scale", "%.2f",
                    modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Line Width##spinCage", &cfg->lineWidth,
                    "spinCage.lineWidth", "%.1f", modSources);
  ModulatableSlider("Glow Intensity##spinCage", &cfg->glowIntensity,
                    "spinCage.glowIntensity", "%.1f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spinCage)
REGISTER_GENERATOR(TRANSFORM_SPIN_CAGE_BLEND, SpinCage, spinCage,
                   "Spin Cage", SetupSpinCageBlend, SetupSpinCage, 10,
                   DrawSpinCageParams, DrawOutput_spinCage)
// clang-format on
