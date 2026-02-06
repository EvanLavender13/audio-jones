// Oil paint effect module implementation

#include "oil_paint.h"
#include "automation/modulation_engine.h"
#include "render/render_utils.h"
#include <stdlib.h>

bool OilPaintEffectInit(OilPaintEffect *e, int width, int height) {
  e->strokeShader = LoadShader(NULL, "shaders/oil_paint_stroke.fs");
  if (e->strokeShader.id == 0) {
    return false;
  }

  e->compositeShader = LoadShader(NULL, "shaders/oil_paint.fs");
  if (e->compositeShader.id == 0) {
    UnloadShader(e->strokeShader);
    return false;
  }

  // Stroke shader uniform locations
  e->strokeResolutionLoc = GetShaderLocation(e->strokeShader, "resolution");
  e->brushSizeLoc = GetShaderLocation(e->strokeShader, "brushSize");
  e->strokeBendLoc = GetShaderLocation(e->strokeShader, "strokeBend");
  e->layersLoc = GetShaderLocation(e->strokeShader, "layers");
  e->noiseTexLoc = GetShaderLocation(e->strokeShader, "texture1");

  // Composite shader uniform locations
  e->compositeResolutionLoc =
      GetShaderLocation(e->compositeShader, "resolution");
  e->specularLoc = GetShaderLocation(e->compositeShader, "specular");

  // Generate 256x256 RGBA noise for brush stroke randomization
  const Image noiseImg = GenImageColor(256, 256, BLANK);
  Color *pixels = (Color *)noiseImg.data;
  // NOLINTBEGIN(concurrency-mt-unsafe) - single-threaded init
  for (int i = 0; i < 256 * 256; i++) {
    pixels[i] =
        Color{(unsigned char)(rand() % 256), (unsigned char)(rand() % 256),
              (unsigned char)(rand() % 256), (unsigned char)(rand() % 256)};
  }
  // NOLINTEND(concurrency-mt-unsafe)
  e->noiseTex = LoadTextureFromImage(noiseImg);
  UnloadImage(noiseImg);
  SetTextureFilter(e->noiseTex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(e->noiseTex, TEXTURE_WRAP_REPEAT);

  RenderUtilsInitTextureHDR(&e->intermediate, width, height, "OIL_PAINT");

  return true;
}

void OilPaintEffectSetup(OilPaintEffect *e, const OilPaintConfig *cfg) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->compositeShader, e->compositeResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->compositeShader, e->specularLoc, &cfg->specular,
                 SHADER_UNIFORM_FLOAT);
}

void OilPaintEffectUninit(OilPaintEffect *e) {
  UnloadShader(e->strokeShader);
  UnloadShader(e->compositeShader);
  UnloadTexture(e->noiseTex);
  UnloadRenderTexture(e->intermediate);
}

void OilPaintEffectResize(OilPaintEffect *e, int width, int height) {
  UnloadRenderTexture(e->intermediate);
  RenderUtilsInitTextureHDR(&e->intermediate, width, height, "OIL_PAINT");
}

OilPaintConfig OilPaintConfigDefault(void) { return OilPaintConfig{}; }

void OilPaintRegisterParams(OilPaintConfig *cfg) {
  ModEngineRegisterParam("oilPaint.brushSize", &cfg->brushSize, 0.5f, 3.0f);
  ModEngineRegisterParam("oilPaint.strokeBend", &cfg->strokeBend, -2.0f, 2.0f);
  ModEngineRegisterParam("oilPaint.specular", &cfg->specular, 0.0f, 1.0f);
}
