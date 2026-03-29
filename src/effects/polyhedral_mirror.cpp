// Polyhedral Mirror effect module implementation
// Raymarched interior reflections inside platonic solid polyhedra -- CPU
// computes face normals from dual solid, edge endpoints, and orbital camera;
// shader ray-marches reflections with gradient-mapped color

#include "polyhedral_mirror.h"
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
// Dual solid mapping and inradius ratios
// ---------------------------------------------------------------------------

// Face normals of a platonic solid are the vertex positions of its dual
static const int DUAL_IDX[] = {0, 2, 1, 4, 3};

// Inradius of each solid when circumradius = 1 (vertices on unit sphere).
// The face planes sit at this distance from the origin.
// Tetra: 1/3, Cube/Octa: 1/sqrt(3), Dodeca/Icosa: phi^2/sqrt(3)
static const float INRADIUS[] = {0.33333f, 0.57735f, 0.57735f, 0.79465f,
                                 0.79465f};

// ---------------------------------------------------------------------------
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

bool PolyhedralMirrorEffectInit(PolyhedralMirrorEffect *e,
                                const PolyhedralMirrorConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/polyhedral_mirror.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->faceNormalsLoc = GetShaderLocation(e->shader, "faceNormals");
  e->faceCountLoc = GetShaderLocation(e->shader, "faceCount");
  e->planeOffsetLoc = GetShaderLocation(e->shader, "planeOffset");
  e->edgeALoc = GetShaderLocation(e->shader, "edgeA");
  e->edgeBLoc = GetShaderLocation(e->shader, "edgeB");
  e->edgeCountLoc = GetShaderLocation(e->shader, "edgeCount");
  e->angleXZLoc = GetShaderLocation(e->shader, "angleXZ");
  e->angleYZLoc = GetShaderLocation(e->shader, "angleYZ");
  e->cameraDistanceLoc = GetShaderLocation(e->shader, "cameraDistance");
  e->maxBouncesLoc = GetShaderLocation(e->shader, "maxBounces");
  e->reflectivityLoc = GetShaderLocation(e->shader, "reflectivity");
  e->edgeRadiusLoc = GetShaderLocation(e->shader, "edgeRadius");
  e->edgeGlowLoc = GetShaderLocation(e->shader, "edgeGlow");
  e->maxIterationsLoc = GetShaderLocation(e->shader, "maxIterations");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->angleXZAccum = 0.0f;
  e->angleYZAccum = 0.0f;

  return true;
}

static void UploadUniforms(PolyhedralMirrorEffect *e,
                           const PolyhedralMirrorConfig *cfg,
                           const float *faceNormalData, int faceCount,
                           float planeOffset, const float *edgeAData,
                           const float *edgeBData, int edgeCount,
                           int maxBouncesInt, const Texture2D &fftTexture) {
  const float resolution[2] = {static_cast<float>(GetScreenWidth()),
                               static_cast<float>(GetScreenHeight())};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueV(e->shader, e->faceNormalsLoc, faceNormalData,
                  SHADER_UNIFORM_VEC3, faceCount);
  SetShaderValue(e->shader, e->faceCountLoc, &faceCount, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->planeOffsetLoc, &planeOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueV(e->shader, e->edgeALoc, edgeAData, SHADER_UNIFORM_VEC3,
                  edgeCount);
  SetShaderValueV(e->shader, e->edgeBLoc, edgeBData, SHADER_UNIFORM_VEC3,
                  edgeCount);
  SetShaderValue(e->shader, e->edgeCountLoc, &edgeCount, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->angleXZLoc, &e->angleXZAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->angleYZLoc, &e->angleYZAccum,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraDistanceLoc, &cfg->cameraDistance,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxBouncesLoc, &maxBouncesInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->reflectivityLoc, &cfg->reflectivity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeRadiusLoc, &cfg->edgeRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->edgeGlowLoc, &cfg->edgeGlow,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxIterationsLoc, &cfg->maxIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  const float sampleRate = static_cast<float>(AUDIO_SAMPLE_RATE);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
}

static void PrepareEdgeData(const ShapeDescriptor *shape, float edgeScale,
                            float *edgeAData, float *edgeBData) {
  for (int i = 0; i < shape->edgeCount; i++) {
    const int a = shape->edges[i][0];
    const int b = shape->edges[i][1];
    edgeAData[i * 3 + 0] = shape->vertices[a][0] * edgeScale;
    edgeAData[i * 3 + 1] = shape->vertices[a][1] * edgeScale;
    edgeAData[i * 3 + 2] = shape->vertices[a][2] * edgeScale;
    edgeBData[i * 3 + 0] = shape->vertices[b][0] * edgeScale;
    edgeBData[i * 3 + 1] = shape->vertices[b][1] * edgeScale;
    edgeBData[i * 3 + 2] = shape->vertices[b][2] * edgeScale;
  }
}

void PolyhedralMirrorEffectSetup(PolyhedralMirrorEffect *e,
                                 const PolyhedralMirrorConfig *cfg,
                                 float deltaTime, const Texture2D &fftTexture) {
  int shapeIdx = cfg->shape;
  if (shapeIdx < 0) {
    shapeIdx = 0;
  }
  if (shapeIdx >= SHAPE_COUNT) {
    shapeIdx = SHAPE_COUNT - 1;
  }

  // Face normals from dual solid
  const int dualIdx = DUAL_IDX[shapeIdx];
  const ShapeDescriptor *dualShape = &SHAPES[dualIdx];

  const int faceCount = dualShape->vertexCount;
  float faceNormalData[20 * 3] = {};
  for (int i = 0; i < faceCount; i++) {
    faceNormalData[i * 3 + 0] = dualShape->vertices[i][0];
    faceNormalData[i * 3 + 1] = dualShape->vertices[i][1];
    faceNormalData[i * 3 + 2] = dualShape->vertices[i][2];
  }

  // Normalize so face planes sit at distance 1.0 from origin
  const float inradius = INRADIUS[shapeIdx];
  const float planeOffset = 1.0f;

  // Scale vertices from unit sphere to match normalized face plane distance
  const float edgeScale = 1.0f / inradius;
  const ShapeDescriptor *shape = &SHAPES[shapeIdx];
  float edgeAData[MAX_EDGES * 3];
  float edgeBData[MAX_EDGES * 3];
  PrepareEdgeData(shape, edgeScale, edgeAData, edgeBData);

  // Phase accumulation
  e->angleXZAccum += cfg->orbitSpeedXZ * deltaTime;
  e->angleYZAccum += cfg->orbitSpeedYZ * deltaTime;

  // Convert maxBounces float to int for shader
  const int maxBouncesInt = static_cast<int>(cfg->maxBounces);

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  UploadUniforms(e, cfg, faceNormalData, faceCount, planeOffset, edgeAData,
                 edgeBData, shape->edgeCount, maxBouncesInt, fftTexture);
}

void PolyhedralMirrorEffectUninit(PolyhedralMirrorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void PolyhedralMirrorRegisterParams(PolyhedralMirrorConfig *cfg) {
  ModEngineRegisterParam("polyhedralMirror.orbitSpeedXZ", &cfg->orbitSpeedXZ,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("polyhedralMirror.orbitSpeedYZ", &cfg->orbitSpeedYZ,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("polyhedralMirror.cameraDistance",
                         &cfg->cameraDistance, 0.1f, 0.8f);
  ModEngineRegisterParam("polyhedralMirror.maxBounces", &cfg->maxBounces, 1.0f,
                         8.0f);
  ModEngineRegisterParam("polyhedralMirror.reflectivity", &cfg->reflectivity,
                         0.1f, 1.0f);
  ModEngineRegisterParam("polyhedralMirror.edgeRadius", &cfg->edgeRadius, 0.01f,
                         0.15f);
  ModEngineRegisterParam("polyhedralMirror.edgeGlow", &cfg->edgeGlow, 0.0f,
                         2.0f);
  ModEngineRegisterParam("polyhedralMirror.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("polyhedralMirror.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("polyhedralMirror.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("polyhedralMirror.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("polyhedralMirror.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("polyhedralMirror.blendIntensity",
                         &cfg->blendIntensity, 0.0f, 5.0f);
}

void SetupPolyhedralMirror(PostEffect *pe) {
  PolyhedralMirrorEffectSetup(&pe->polyhedralMirror,
                              &pe->effects.polyhedralMirror,
                              pe->currentDeltaTime, pe->fftTexture);
}

void SetupPolyhedralMirrorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.polyhedralMirror.blendIntensity,
                       pe->effects.polyhedralMirror.blendMode);
}

// === UI ===

static void DrawPolyhedralMirrorParams(EffectConfig *e,
                                       const ModSources *modSources,
                                       ImU32 categoryGlow) {
  (void)categoryGlow;
  PolyhedralMirrorConfig *cfg = &e->polyhedralMirror;

  // Shape
  ImGui::SeparatorText("Shape");
  ImGui::Combo("Shape##polyhedralMirror", &cfg->shape,
               "Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0");

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSliderSpeedDeg("Orbit XZ##polyhedralMirror", &cfg->orbitSpeedXZ,
                            "polyhedralMirror.orbitSpeedXZ", modSources);
  ModulatableSliderSpeedDeg("Orbit YZ##polyhedralMirror", &cfg->orbitSpeedYZ,
                            "polyhedralMirror.orbitSpeedYZ", modSources);
  ModulatableSlider("Camera Dist##polyhedralMirror", &cfg->cameraDistance,
                    "polyhedralMirror.cameraDistance", "%.2f", modSources);

  // Reflection
  ImGui::SeparatorText("Reflection");
  ModulatableSliderInt("Max Bounces##polyhedralMirror", &cfg->maxBounces,
                       "polyhedralMirror.maxBounces", modSources);
  ModulatableSlider("Reflectivity##polyhedralMirror", &cfg->reflectivity,
                    "polyhedralMirror.reflectivity", "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##polyhedralMirror", &cfg->baseFreq,
                    "polyhedralMirror.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##polyhedralMirror", &cfg->maxFreq,
                    "polyhedralMirror.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##polyhedralMirror", &cfg->gain,
                    "polyhedralMirror.gain", "%.1f", modSources);
  ModulatableSlider("Contrast##polyhedralMirror", &cfg->curve,
                    "polyhedralMirror.curve", "%.2f", modSources);
  ModulatableSlider("Base Bright##polyhedralMirror", &cfg->baseBright,
                    "polyhedralMirror.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSliderLog("Edge Radius##polyhedralMirror", &cfg->edgeRadius,
                       "polyhedralMirror.edgeRadius", "%.3f", modSources);
  ModulatableSlider("Edge Glow##polyhedralMirror", &cfg->edgeGlow,
                    "polyhedralMirror.edgeGlow", "%.2f", modSources);
  ImGui::SliderInt("Max Iterations##polyhedralMirror", &cfg->maxIterations, 32,
                   128);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(polyhedralMirror)
REGISTER_GENERATOR(TRANSFORM_POLYHEDRAL_MIRROR_BLEND, PolyhedralMirror,
                   polyhedralMirror, "Polyhedral Mirror",
                   SetupPolyhedralMirrorBlend, SetupPolyhedralMirror, 10,
                   DrawPolyhedralMirrorParams, DrawOutput_polyhedralMirror)
// clang-format on
