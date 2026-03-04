// SpinCage effect module implementation
// Rotating platonic solid wireframes with per-edge FFT glow —
// CPU-side 3D rotation and perspective projection, shader-side
// distance-to-segment rendering

#include "spin_cage.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
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
// Platonic solid vertex/edge data — all vertices normalized to unit sphere
// ---------------------------------------------------------------------------

struct ShapeDescriptor {
  int vertexCount;
  int edgeCount;
  const float (*vertices)[3];
  const int (*edges)[2];
};

// --- Tetrahedron (4V, 6E) ---
static const float TETRA_VERTICES[][3] = {
    {0.0f, 0.0f, 1.0f},
    {0.942809f, 0.0f, -0.333333f},        // sqrt(8/9) = 0.942809
    {-0.471405f, 0.816497f, -0.333333f},  // -sqrt(2/9), sqrt(2/3)
    {-0.471405f, -0.816497f, -0.333333f}, // -sqrt(2/9), -sqrt(2/3)
};
static const int TETRA_EDGES[][2] = {
    {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3},
};

// --- Cube (8V, 12E) ---
// a = 1/sqrt(3) = 0.57735
// 0=(-,-,-) 1=(+,-,-) 2=(-,+,-) 3=(+,+,-) 4=(-,-,+) 5=(+,-,+) 6=(-,+,+)
// 7=(+,+,+)
static const float CUBE_VERTICES[][3] = {
    {-0.57735f, -0.57735f, -0.57735f}, {0.57735f, -0.57735f, -0.57735f},
    {-0.57735f, 0.57735f, -0.57735f},  {0.57735f, 0.57735f, -0.57735f},
    {-0.57735f, -0.57735f, 0.57735f},  {0.57735f, -0.57735f, 0.57735f},
    {-0.57735f, 0.57735f, 0.57735f},   {0.57735f, 0.57735f, 0.57735f},
};
static const int CUBE_EDGES[][2] = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0}, // front face
    {4, 5}, {5, 7}, {7, 6}, {6, 4}, // back face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}, // connecting
};

// --- Octahedron (6V, 12E) ---
static const float OCTA_VERTICES[][3] = {
    {1.0f, 0.0f, 0.0f},  {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f, -1.0f},
};
// Each vertex connects to 4 neighbors (all except its opposite)
static const int OCTA_EDGES[][2] = {
    {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 2}, {1, 3},
    {1, 4}, {1, 5}, {2, 4}, {2, 5}, {3, 4}, {3, 5},
};

// --- Dodecahedron (20V, 30E) ---
// Golden ratio phi = (1+sqrt(5))/2 = 1.618034
// Raw vertices: 8 at (+-1,+-1,+-1), 4 at (0,+-1/phi,+-phi),
// 4 at (+-1/phi,+-phi,0), 4 at (+-phi,0,+-1/phi)
// All normalized to unit sphere
static const float DODECA_VERTICES[][3] = {
    // 8 cube vertices (+-1,+-1,+-1) normalized
    {-0.57735f, -0.57735f, -0.57735f}, // 0
    {-0.57735f, -0.57735f, 0.57735f},  // 1
    {-0.57735f, 0.57735f, -0.57735f},  // 2
    {-0.57735f, 0.57735f, 0.57735f},   // 3
    {0.57735f, -0.57735f, -0.57735f},  // 4
    {0.57735f, -0.57735f, 0.57735f},   // 5
    {0.57735f, 0.57735f, -0.57735f},   // 6
    {0.57735f, 0.57735f, 0.57735f},    // 7
    // 4 at (0, +-1/phi, +-phi) normalized: 1/phi=0.618034, phi=1.618034
    // norm = sqrt(0 + 0.381966 + 2.618034) = sqrt(3) = 1.732051
    {0.0f, -0.356822f, -0.934172f}, // 8:  (0, -1/phi, -phi)/norm
    {0.0f, -0.356822f, 0.934172f},  // 9:  (0, -1/phi, +phi)/norm
    {0.0f, 0.356822f, -0.934172f},  // 10: (0, +1/phi, -phi)/norm
    {0.0f, 0.356822f, 0.934172f},   // 11: (0, +1/phi, +phi)/norm
    // 4 at (+-1/phi, +-phi, 0) normalized
    {-0.356822f, -0.934172f, 0.0f}, // 12
    {-0.356822f, 0.934172f, 0.0f},  // 13
    {0.356822f, -0.934172f, 0.0f},  // 14
    {0.356822f, 0.934172f, 0.0f},   // 15
    // 4 at (+-phi, 0, +-1/phi) normalized
    {-0.934172f, 0.0f, -0.356822f}, // 16
    {-0.934172f, 0.0f, 0.356822f},  // 17
    {0.934172f, 0.0f, -0.356822f},  // 18
    {0.934172f, 0.0f, 0.356822f},   // 19
};
// Dodecahedron edges — each vertex has degree 3
static const int DODECA_EDGES[][2] = {
    {0, 8},  {0, 12}, {0, 16},  {1, 9},   {1, 12},  {1, 17},  {2, 10}, {2, 13},
    {2, 16}, {3, 11}, {3, 13},  {3, 17},  {4, 8},   {4, 14},  {4, 18}, {5, 9},
    {5, 14}, {5, 19}, {6, 10},  {6, 15},  {6, 18},  {7, 11},  {7, 15}, {7, 19},
    {8, 10}, {9, 11}, {12, 14}, {13, 15}, {16, 17}, {18, 19},
};

// --- Icosahedron (12V, 30E) ---
// Cyclic permutations of (0, +-1, +-phi) normalized to unit sphere
// norm = sqrt(0 + 1 + phi^2) = sqrt(1 + 2.618034) = sqrt(3.618034) = 1.902113
// 1/norm = 0.525731, phi/norm = 0.850651
static const float ICOSA_VERTICES[][3] = {
    {0.0f, -0.525731f, -0.850651f}, // 0
    {0.0f, -0.525731f, 0.850651f},  // 1
    {0.0f, 0.525731f, -0.850651f},  // 2
    {0.0f, 0.525731f, 0.850651f},   // 3
    {-0.525731f, -0.850651f, 0.0f}, // 4
    {-0.525731f, 0.850651f, 0.0f},  // 5
    {0.525731f, -0.850651f, 0.0f},  // 6
    {0.525731f, 0.850651f, 0.0f},   // 7
    {-0.850651f, 0.0f, -0.525731f}, // 8
    {-0.850651f, 0.0f, 0.525731f},  // 9
    {0.850651f, 0.0f, -0.525731f},  // 10
    {0.850651f, 0.0f, 0.525731f},   // 11
};
// Each vertex degree 5
static const int ICOSA_EDGES[][2] = {
    {0, 2},  {0, 4},  {0, 6}, {0, 8}, {0, 10}, {1, 3},   {1, 4},  {1, 6},
    {1, 9},  {1, 11}, {2, 5}, {2, 7}, {2, 8},  {2, 10},  {3, 5},  {3, 7},
    {3, 9},  {3, 11}, {4, 8}, {4, 9}, {5, 8},  {5, 9},   {6, 10}, {6, 11},
    {7, 10}, {7, 11}, {4, 6}, {5, 7}, {8, 9},  {10, 11},
};

static const ShapeDescriptor SHAPES[] = {
    {4, 6, TETRA_VERTICES, TETRA_EDGES},
    {8, 12, CUBE_VERTICES, CUBE_EDGES},
    {6, 12, OCTA_VERTICES, OCTA_EDGES},
    {20, 30, DODECA_VERTICES, DODECA_EDGES},
    {12, 30, ICOSA_VERTICES, ICOSA_EDGES},
};

static const int SHAPE_COUNT = 5;
static const int MAX_EDGES = 30; // Dodecahedron and Icosahedron

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
  float cx = cosf(ax), sx = sinf(ax);
  float cy = cosf(ay), sy = sinf(ay);
  float cz = cosf(az), sz = sinf(az);

  // R = Rx(ax) * Ry(ay) * Rz(az)
  float r00 = cy * cz, r01 = cy * sz, r02 = -sy;
  float r10 = sx * sy * cz - cx * sz;
  float r11 = sx * sy * sz + cx * cz, r12 = sx * cy;
  float r20 = cx * sy * cz + sx * sz;
  float r21 = cx * sy * sz - sx * cz, r22 = cx * cy;

  for (int v = 0; v < shape->vertexCount; v++) {
    float vx = shape->vertices[v][0];
    float vy = shape->vertices[v][1];
    float vz = shape->vertices[v][2];

    float rz = r20 * vx + r21 * vy + r22 * vz;
    rotatedZ[v] = rz;
    float invDepth = 1.0f / (rz + perspective);
    projectedX[v] = (r00 * vx + r01 * vy + r02 * vz) * invDepth * scale;
    projectedY[v] = (r10 * vx + r11 * vy + r12 * vz) * invDepth * scale;
  }
}

// Upload all shader uniforms for the current frame
static void UploadUniforms(SpinCageEffect *e, const SpinCageConfig *cfg,
                           const ShapeDescriptor *shape, const float *edges,
                           const float *edgeT, Texture2D fftTexture) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
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
                         float deltaTime, Texture2D fftTexture) {
  int shapeIdx = cfg->shape;
  if (shapeIdx < 0)
    shapeIdx = 0;
  if (shapeIdx >= SHAPE_COUNT)
    shapeIdx = SHAPE_COUNT - 1;
  const ShapeDescriptor *shape = &SHAPES[shapeIdx];

  e->angleX += cfg->speedX * cfg->speedMult * deltaTime;
  e->angleY += cfg->speedY * cfg->speedMult * deltaTime;
  e->angleZ += cfg->speedZ * cfg->speedMult * deltaTime;

  float projectedX[20], projectedY[20], rotatedZ[20];
  TransformVertices(shape, e->angleX, e->angleY, e->angleZ, cfg->perspective,
                    cfg->scale, projectedX, projectedY, rotatedZ);

  // Pack edges and compute edgeT
  float edges[MAX_EDGES * 4];
  float edgeT[MAX_EDGES];
  for (int ei = 0; ei < shape->edgeCount; ei++) {
    int i = shape->edges[ei][0];
    int j = shape->edges[ei][1];
    edges[ei * 4 + 0] = projectedX[i];
    edges[ei * 4 + 1] = projectedY[i];
    edges[ei * 4 + 2] = projectedX[j];
    edges[ei * 4 + 3] = projectedY[j];

    if (cfg->colorMode == 1) {
      float avgZ = (rotatedZ[i] + rotatedZ[j]) * 0.5f;
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

SpinCageConfig SpinCageConfigDefault(void) { return SpinCageConfig{}; }

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

  // Shape
  ImGui::SeparatorText("Shape");
  ImGui::Combo("Shape##spinCage", &cfg->shape,
               "Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0");
  ImGui::Combo("Color Mode##spinCage", &cfg->colorMode, "Edge Index\0Depth\0");

  // Rotation
  ImGui::SeparatorText("Rotation");
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
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(spinCage)
REGISTER_GENERATOR(TRANSFORM_SPIN_CAGE_BLEND, SpinCage, spinCage,
                   "Spin Cage", SetupSpinCageBlend, SetupSpinCage, 10,
                   DrawSpinCageParams, DrawOutput_spinCage)
// clang-format on
