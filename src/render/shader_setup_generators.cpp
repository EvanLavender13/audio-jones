#include "shader_setup_generators.h"
#include "blend_compositor.h"
#include "post_effect.h"
#include "render_utils.h"
#include "shader_setup.h"

void SetupConstellation(PostEffect *pe) {
  ConstellationEffectSetup(&pe->constellation, &pe->effects.constellation,
                           pe->currentDeltaTime);
}

void SetupPlasma(PostEffect *pe) {
  PlasmaEffectSetup(&pe->plasma, &pe->effects.plasma, pe->currentDeltaTime);
}

void SetupInterference(PostEffect *pe) {
  InterferenceEffectSetup(&pe->interference, &pe->effects.interference,
                          pe->currentDeltaTime);
}

void SetupSolidColor(PostEffect *pe) {
  SolidColorEffectSetup(&pe->solidColor, &pe->effects.solidColor);
}

void SetupConstellationBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.constellation.blendIntensity,
                       pe->effects.constellation.blendMode);
}

void SetupPlasmaBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.plasma.blendIntensity,
                       pe->effects.plasma.blendMode);
}

void SetupInterferenceBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.interference.blendIntensity,
                       pe->effects.interference.blendMode);
}

void SetupSolidColorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.solidColor.blendIntensity,
                       pe->effects.solidColor.blendMode);
}

void RenderGeneratorToScratch(PostEffect *pe, TransformEffectType type,
                              RenderTexture2D *src) {
  Shader shader = {0};
  RenderPipelineShaderSetupFn setup = NULL;

  switch (type) {
  case TRANSFORM_CONSTELLATION_BLEND:
    shader = pe->constellation.shader;
    setup = SetupConstellation;
    break;
  case TRANSFORM_PLASMA_BLEND:
    shader = pe->plasma.shader;
    setup = SetupPlasma;
    break;
  case TRANSFORM_INTERFERENCE_BLEND:
    shader = pe->interference.shader;
    setup = SetupInterference;
    break;
  case TRANSFORM_SOLID_COLOR:
    shader = pe->solidColor.shader;
    setup = SetupSolidColor;
    break;
  default:
    return;
  }

  // Render generator output to scratch texture
  BeginTextureMode(pe->generatorScratch);
  if (shader.id != 0) {
    BeginShaderMode(shader);
    if (setup != NULL) {
      setup(pe);
    }
  }
  RenderUtilsDrawFullscreenQuad(src->texture, pe->screenWidth,
                                pe->screenHeight);
  if (shader.id != 0) {
    EndShaderMode();
  }
  EndTextureMode();
}
