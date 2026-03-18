#include "shader_setup.h"
#include "blend_compositor.h"
#include "config/effect_descriptor.h"
#include "post_effect.h"
#include <math.h>

TransformEffectEntry GetTransformEffect(PostEffect *pe,
                                        TransformEffectType type) {
  const EffectDescriptor &d = EFFECT_DESCRIPTORS[type];
  bool *enabled = reinterpret_cast<bool *>(
      reinterpret_cast<char *>(&pe->effects) + d.enabledOffset);
  return {d.getShader(pe), d.setup, enabled};
}

GeneratorPassInfo GetGeneratorScratchPass(PostEffect *pe,
                                          TransformEffectType type) {
  const EffectDescriptor &d = EFFECT_DESCRIPTORS[type];
  return {*d.getScratchShader(pe), d.scratchSetup};
}

void SetupFeedback(PostEffect *pe) {
  const float ms = pe->effects.motionScale;
  const FlowFieldConfig *ff = &pe->effects.flowField;

  SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                 &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);

  // Identity-centered values: scale deviation from 1.0
  float zoomEff = 1.0f + (ff->zoomBase - 1.0f) * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomBaseLoc, &zoomEff,
                 SHADER_UNIFORM_FLOAT);

  // Radial/angular zoom offsets: direct multiplication (additive modifiers)
  float zoomRadialEff = ff->zoomRadial * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomRadialLoc, &zoomRadialEff,
                 SHADER_UNIFORM_FLOAT);

  // Speed values: direct multiplication
  float rotBase = ff->rotationSpeed * pe->currentDeltaTime * ms;
  float rotRadial = ff->rotationSpeedRadial * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc, &rotBase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc, &rotRadial,
                 SHADER_UNIFORM_FLOAT);

  // Translation: direct multiplication
  float dxBaseEff = ff->dxBase * ms;
  float dxRadialEff = ff->dxRadial * ms;
  float dyBaseEff = ff->dyBase * ms;
  float dyRadialEff = ff->dyRadial * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDxBaseLoc, &dxBaseEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDxRadialLoc, &dxRadialEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyBaseLoc, &dyBaseEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyRadialLoc, &dyRadialEff,
                 SHADER_UNIFORM_FLOAT);

  // Feedback flow strength: direct multiplication
  float flowStrengthEff = pe->effects.feedbackFlow.strength * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackFlowStrengthLoc,
                 &flowStrengthEff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackFlowAngleLoc,
                 &pe->effects.feedbackFlow.flowAngle, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackFlowScaleLoc,
                 &pe->effects.feedbackFlow.scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackFlowThresholdLoc,
                 &pe->effects.feedbackFlow.threshold, SHADER_UNIFORM_FLOAT);

  // Center pivot (not motion-related, pass through)
  SetShaderValue(pe->feedbackShader, pe->feedbackCxLoc, &ff->cx,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackCyLoc, &ff->cy,
                 SHADER_UNIFORM_FLOAT);

  // Directional stretch: identity-centered
  float sxEff = 1.0f + (ff->sx - 1.0f) * ms;
  float syEff = 1.0f + (ff->sy - 1.0f) * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackSxLoc, &sxEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackSyLoc, &syEff,
                 SHADER_UNIFORM_FLOAT);

  // Angular modulation: treat as speeds (need deltaTime for frame-rate
  // independence)
  float zoomAngularEff = ff->zoomAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularLoc,
                 &zoomAngularEff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularFreqLoc,
                 &ff->zoomAngularFreq, SHADER_UNIFORM_INT);
  float rotAngularEff = ff->rotAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularLoc, &rotAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularFreqLoc,
                 &ff->rotAngularFreq, SHADER_UNIFORM_INT);
  float dxAngularEff = ff->dxAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularLoc, &dxAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularFreqLoc,
                 &ff->dxAngularFreq, SHADER_UNIFORM_INT);
  float dyAngularEff = ff->dyAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularLoc, &dyAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularFreqLoc,
                 &ff->dyAngularFreq, SHADER_UNIFORM_INT);

  // Procedural warp: scale displacement intensity
  float warpEff = pe->effects.proceduralWarp.warp * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpLoc, &warpEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpTimeLoc, &pe->warpTime,
                 SHADER_UNIFORM_FLOAT);
  float warpScaleInverse = 1.0f / pe->effects.proceduralWarp.warpScale;
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpScaleInverseLoc,
                 &warpScaleInverse, SHADER_UNIFORM_FLOAT);
}

void SetupBlurH(PostEffect *pe) {
  SetShaderValue(pe->blurHShader, pe->blurHScaleLoc, &pe->currentBlurScale,
                 SHADER_UNIFORM_FLOAT);
}

void SetupBlurV(PostEffect *pe) {
  SetShaderValue(pe->blurVShader, pe->blurVScaleLoc, &pe->currentBlurScale,
                 SHADER_UNIFORM_FLOAT);
  // Decay compensation: increase halfLife proportionally to motion slowdown
  const float safeMotionScale = fmaxf(pe->effects.motionScale, 0.01f);
  float effectiveHalfLife = pe->effects.halfLife / safeMotionScale;
  SetShaderValue(pe->blurVShader, pe->halfLifeLoc, &effectiveHalfLife,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->blurVShader, pe->deltaTimeLoc, &pe->currentDeltaTime,
                 SHADER_UNIFORM_FLOAT);
}

void SetupGamma(PostEffect *pe) {
  SetShaderValue(pe->gammaShader, pe->gammaGammaLoc, &pe->effects.gamma,
                 SHADER_UNIFORM_FLOAT);
}

void SetupClarity(PostEffect *pe) {
  SetShaderValue(pe->clarityShader, pe->clarityAmountLoc, &pe->effects.clarity,
                 SHADER_UNIFORM_FLOAT);
}

void SetupAccumComposite(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->accumTexture.texture,
                       pe->effects.accumBlendIntensity,
                       pe->effects.accumBlendMode);
}

// Manual registration: uses blend compositor shader, no init/uninit/resize
static Shader *GetShader_accumComposite(PostEffect *pe) {
  return &pe->blendCompositor->shader;
}

// clang-format off
static bool reg_accumComposite = EffectDescriptorRegister(
    TRANSFORM_ACCUM_COMPOSITE,
    EffectDescriptor{TRANSFORM_ACCUM_COMPOSITE, "Accum Composite", "TRL", -1,
     offsetof(EffectConfig, accumCompositeEnabled), nullptr,
     (uint8_t)EFFECT_FLAG_NONE,
     NULL, NULL, NULL, NULL,
     GetShader_accumComposite, SetupAccumComposite,
     nullptr, nullptr, nullptr,
     nullptr, nullptr});
// clang-format on

void ApplyHalfResEffect(PostEffect *pe, const RenderTexture2D *source,
                        const int *writeIdx, Shader shader,
                        RenderPipelineShaderSetupFn setup) {
  const int halfW = pe->screenWidth / 2;
  const int halfH = pe->screenHeight / 2;
  const Rectangle srcRect = {0, 0, (float)source->texture.width,
                             (float)-source->texture.height};
  const Rectangle halfRect = {0, 0, (float)halfW, (float)halfH};
  const Rectangle fullRect = {0, 0, (float)pe->screenWidth,
                              (float)pe->screenHeight};

  BeginTextureMode(pe->halfResA);
  DrawTexturePro(source->texture, srcRect, halfRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();

  const int resLoc = GetShaderLocation(shader, "resolution");
  if (resLoc >= 0) {
    const float halfRes[2] = {(float)halfW, (float)halfH};
    SetShaderValue(shader, resLoc, halfRes, SHADER_UNIFORM_VEC2);
  }

  if (setup != NULL) {
    setup(pe);
  }
  BeginTextureMode(pe->halfResB);
  BeginShaderMode(shader);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Subsequent effects may share this shader
  if (resLoc >= 0) {
    const float fullRes[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
    SetShaderValue(shader, resLoc, fullRes, SHADER_UNIFORM_VEC2);
  }

  BeginTextureMode(pe->pingPong[*writeIdx]);
  DrawTexturePro(pe->halfResB.texture, {0, 0, (float)halfW, (float)-halfH},
                 fullRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();
}
