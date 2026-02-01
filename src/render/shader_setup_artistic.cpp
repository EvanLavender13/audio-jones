#include "shader_setup_artistic.h"
#include "post_effect.h"

void SetupOilPaint(PostEffect *pe) {
  const OilPaintConfig *op = &pe->effects.oilPaint;
  SetShaderValue(pe->oilPaintShader, pe->oilPaintSpecularLoc, &op->specular,
                 SHADER_UNIFORM_FLOAT);
}

void SetupWatercolor(PostEffect *pe) {
  const WatercolorConfig *wc = &pe->effects.watercolor;
  SetShaderValue(pe->watercolorShader, pe->watercolorSamplesLoc, &wc->samples,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->watercolorShader, pe->watercolorStrokeStepLoc,
                 &wc->strokeStep, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorWashStrengthLoc,
                 &wc->washStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorPaperScaleLoc,
                 &wc->paperScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorPaperStrengthLoc,
                 &wc->paperStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorEdgePoolLoc, &wc->edgePool,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorFlowCenterLoc,
                 &wc->flowCenter, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->watercolorShader, pe->watercolorFlowWidthLoc,
                 &wc->flowWidth, SHADER_UNIFORM_FLOAT);
}

void SetupImpressionist(PostEffect *pe) {
  const ImpressionistConfig *cfg = &pe->effects.impressionist;
  SetShaderValue(pe->impressionistShader, pe->impressionistSplatCountLoc,
                 &cfg->splatCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->impressionistShader, pe->impressionistSplatSizeMinLoc,
                 &cfg->splatSizeMin, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistSplatSizeMaxLoc,
                 &cfg->splatSizeMax, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistStrokeFreqLoc,
                 &cfg->strokeFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistStrokeOpacityLoc,
                 &cfg->strokeOpacity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistOutlineStrengthLoc,
                 &cfg->outlineStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistEdgeStrengthLoc,
                 &cfg->edgeStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistEdgeMaxDarkenLoc,
                 &cfg->edgeMaxDarken, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistGrainScaleLoc,
                 &cfg->grainScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistGrainAmountLoc,
                 &cfg->grainAmount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->impressionistShader, pe->impressionistExposureLoc,
                 &cfg->exposure, SHADER_UNIFORM_FLOAT);
}

void SetupInkWash(PostEffect *pe) {
  const InkWashConfig *iw = &pe->effects.inkWash;
  SetShaderValue(pe->inkWashShader, pe->inkWashStrengthLoc, &iw->strength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->inkWashShader, pe->inkWashGranulationLoc, &iw->granulation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->inkWashShader, pe->inkWashBleedStrengthLoc,
                 &iw->bleedStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->inkWashShader, pe->inkWashBleedRadiusLoc, &iw->bleedRadius,
                 SHADER_UNIFORM_FLOAT);
  int softness = (int)iw->softness;
  SetShaderValue(pe->inkWashShader, pe->inkWashSoftnessLoc, &softness,
                 SHADER_UNIFORM_INT);
}

void SetupPencilSketch(PostEffect *pe) {
  const PencilSketchConfig *ps = &pe->effects.pencilSketch;

  // CPU wobble time accumulation for smooth animation without jumps
  pe->pencilSketchWobbleTime += pe->currentDeltaTime * ps->wobbleSpeed;

  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchAngleCountLoc,
                 &ps->angleCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchSampleCountLoc,
                 &ps->sampleCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchStrokeFalloffLoc,
                 &ps->strokeFalloff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchGradientEpsLoc,
                 &ps->gradientEps, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchPaperStrengthLoc,
                 &ps->paperStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchVignetteStrengthLoc,
                 &ps->vignetteStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchWobbleTimeLoc,
                 &pe->pencilSketchWobbleTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchWobbleAmountLoc,
                 &ps->wobbleAmount, SHADER_UNIFORM_FLOAT);
}

void SetupCrossHatching(PostEffect *pe) {
  const CrossHatchingConfig *ch = &pe->effects.crossHatching;

  // CPU time accumulation for temporal stutter
  pe->crossHatchingTime += pe->currentDeltaTime;

  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingTimeLoc,
                 &pe->crossHatchingTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingWidthLoc, &ch->width,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingThresholdLoc,
                 &ch->threshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingNoiseLoc, &ch->noise,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingOutlineLoc,
                 &ch->outline, SHADER_UNIFORM_FLOAT);
}
