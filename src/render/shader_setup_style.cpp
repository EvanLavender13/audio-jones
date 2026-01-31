#include "shader_setup_style.h"
#include "post_effect.h"

void SetupPixelation(PostEffect *pe) {
  const PixelationConfig *p = &pe->effects.pixelation;
  SetShaderValue(pe->pixelationShader, pe->pixelationCellCountLoc,
                 &p->cellCount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pixelationShader, pe->pixelationDitherScaleLoc,
                 &p->ditherScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->pixelationShader, pe->pixelationPosterizeLevelsLoc,
                 &p->posterizeLevels, SHADER_UNIFORM_INT);
}

void SetupGlitch(PostEffect *pe) {
  const GlitchConfig *g = &pe->effects.glitch;

  // Update time (CPU accumulated for smooth animation)
  pe->glitchTime += pe->currentDeltaTime;
  pe->glitchFrame++;

  SetShaderValue(pe->glitchShader, pe->glitchTimeLoc, &pe->glitchTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchFrameLoc, &pe->glitchFrame,
                 SHADER_UNIFORM_INT);

  // CRT mode
  int crtEnabled = g->crtEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchCrtEnabledLoc, &crtEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchCurvatureLoc, &g->curvature,
                 SHADER_UNIFORM_FLOAT);
  int vignetteEnabled = g->vignetteEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchVignetteEnabledLoc,
                 &vignetteEnabled, SHADER_UNIFORM_INT);

  // Analog mode (enabled when analogIntensity > 0)
  SetShaderValue(pe->glitchShader, pe->glitchAnalogIntensityLoc,
                 &g->analogIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchAberrationLoc, &g->aberration,
                 SHADER_UNIFORM_FLOAT);

  // Digital mode (enabled when blockThreshold > 0)
  SetShaderValue(pe->glitchShader, pe->glitchBlockThresholdLoc,
                 &g->blockThreshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockOffsetLoc, &g->blockOffset,
                 SHADER_UNIFORM_FLOAT);

  // VHS mode
  int vhsEnabled = g->vhsEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchVhsEnabledLoc, &vhsEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchTrackingBarIntensityLoc,
                 &g->trackingBarIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchScanlineNoiseIntensityLoc,
                 &g->scanlineNoiseIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchColorDriftIntensityLoc,
                 &g->colorDriftIntensity, SHADER_UNIFORM_FLOAT);

  // Overlay
  SetShaderValue(pe->glitchShader, pe->glitchScanlineAmountLoc,
                 &g->scanlineAmount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchNoiseAmountLoc, &g->noiseAmount,
                 SHADER_UNIFORM_FLOAT);

  // Datamosh
  int datamoshEnabled = g->datamoshEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshEnabledLoc,
                 &datamoshEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshIntensityLoc,
                 &g->datamoshIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshMinLoc, &g->datamoshMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshMaxLoc, &g->datamoshMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshSpeedLoc,
                 &g->datamoshSpeed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDatamoshBandsLoc,
                 &g->datamoshBands, SHADER_UNIFORM_FLOAT);

  // Row Slice
  int rowSliceEnabled = g->rowSliceEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchRowSliceEnabledLoc,
                 &rowSliceEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchRowSliceIntensityLoc,
                 &g->rowSliceIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchRowSliceBurstFreqLoc,
                 &g->rowSliceBurstFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchRowSliceBurstPowerLoc,
                 &g->rowSliceBurstPower, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchRowSliceColumnsLoc,
                 &g->rowSliceColumns, SHADER_UNIFORM_FLOAT);

  // Column Slice
  int colSliceEnabled = g->colSliceEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchColSliceEnabledLoc,
                 &colSliceEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchColSliceIntensityLoc,
                 &g->colSliceIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchColSliceBurstFreqLoc,
                 &g->colSliceBurstFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchColSliceBurstPowerLoc,
                 &g->colSliceBurstPower, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchColSliceRowsLoc, &g->colSliceRows,
                 SHADER_UNIFORM_FLOAT);

  // Diagonal Bands
  int diagonalBandsEnabled = g->diagonalBandsEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchDiagonalBandsEnabledLoc,
                 &diagonalBandsEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchDiagonalBandCountLoc,
                 &g->diagonalBandCount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDiagonalBandDisplaceLoc,
                 &g->diagonalBandDisplace, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchDiagonalBandSpeedLoc,
                 &g->diagonalBandSpeed, SHADER_UNIFORM_FLOAT);

  // Block Mask
  int blockMaskEnabled = g->blockMaskEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchBlockMaskEnabledLoc,
                 &blockMaskEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMaskIntensityLoc,
                 &g->blockMaskIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMaskMinSizeLoc,
                 &g->blockMaskMinSize, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMaskMaxSizeLoc,
                 &g->blockMaskMaxSize, SHADER_UNIFORM_INT);
  float blockMaskTint[3] = {g->blockMaskTintR, g->blockMaskTintG,
                            g->blockMaskTintB};
  SetShaderValue(pe->glitchShader, pe->glitchBlockMaskTintLoc, blockMaskTint,
                 SHADER_UNIFORM_VEC3);

  // Temporal Jitter
  int temporalJitterEnabled = g->temporalJitterEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterEnabledLoc,
                 &temporalJitterEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterAmountLoc,
                 &g->temporalJitterAmount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterGateLoc,
                 &g->temporalJitterGate, SHADER_UNIFORM_FLOAT);
}

void SetupToon(PostEffect *pe) {
  const ToonConfig *t = &pe->effects.toon;
  SetShaderValue(pe->toonShader, pe->toonLevelsLoc, &t->levels,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->toonShader, pe->toonEdgeThresholdLoc, &t->edgeThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonEdgeSoftnessLoc, &t->edgeSoftness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonThicknessVariationLoc,
                 &t->thicknessVariation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->toonShader, pe->toonNoiseScaleLoc, &t->noiseScale,
                 SHADER_UNIFORM_FLOAT);
}

void SetupHeightfieldRelief(PostEffect *pe) {
  const HeightfieldReliefConfig *h = &pe->effects.heightfieldRelief;
  SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefIntensityLoc,
                 &h->intensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefReliefScaleLoc, &h->reliefScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefLightAngleLoc, &h->lightAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefLightHeightLoc, &h->lightHeight,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefShininessLoc,
                 &h->shininess, SHADER_UNIFORM_FLOAT);
}

void SetupAsciiArt(PostEffect *pe) {
  const AsciiArtConfig *aa = &pe->effects.asciiArt;
  int cellPixels = (int)aa->cellSize;
  SetShaderValue(pe->asciiArtShader, pe->asciiArtCellPixelsLoc, &cellPixels,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->asciiArtShader, pe->asciiArtColorModeLoc, &aa->colorMode,
                 SHADER_UNIFORM_INT);
  float foreground[3] = {aa->foregroundR, aa->foregroundG, aa->foregroundB};
  SetShaderValue(pe->asciiArtShader, pe->asciiArtForegroundLoc, foreground,
                 SHADER_UNIFORM_VEC3);
  float background[3] = {aa->backgroundR, aa->backgroundG, aa->backgroundB};
  SetShaderValue(pe->asciiArtShader, pe->asciiArtBackgroundLoc, background,
                 SHADER_UNIFORM_VEC3);
  int invert = aa->invert ? 1 : 0;
  SetShaderValue(pe->asciiArtShader, pe->asciiArtInvertLoc, &invert,
                 SHADER_UNIFORM_INT);
}

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

void SetupNeonGlow(PostEffect *pe) {
  const NeonGlowConfig *ng = &pe->effects.neonGlow;
  float glowColor[3] = {ng->glowR, ng->glowG, ng->glowB};
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowColorLoc, glowColor,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgeThresholdLoc,
                 &ng->edgeThreshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgePowerLoc, &ng->edgePower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowIntensityLoc,
                 &ng->glowIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowRadiusLoc, &ng->glowRadius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowSamplesLoc,
                 &ng->glowSamples, SHADER_UNIFORM_INT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowOriginalVisibilityLoc,
                 &ng->originalVisibility, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowColorModeLoc, &ng->colorMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowSaturationBoostLoc,
                 &ng->saturationBoost, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowBrightnessBoostLoc,
                 &ng->brightnessBoost, SHADER_UNIFORM_FLOAT);
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

void SetupBokeh(PostEffect *pe) {
  const BokehConfig *b = &pe->effects.bokeh;
  SetShaderValue(pe->bokehShader, pe->bokehRadiusLoc, &b->radius,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->bokehShader, pe->bokehIterationsLoc, &b->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->bokehShader, pe->bokehBrightnessPowerLoc,
                 &b->brightnessPower, SHADER_UNIFORM_FLOAT);
}

void SetupBloom(PostEffect *pe) {
  const BloomConfig *b = &pe->effects.bloom;
  SetShaderValue(pe->bloomCompositeShader, pe->bloomIntensityLoc, &b->intensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(pe->bloomCompositeShader, pe->bloomBloomTexLoc,
                        pe->bloomMips[0].texture);
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

void SetupMatrixRain(PostEffect *pe) {
  const MatrixRainConfig *cfg = &pe->effects.matrixRain;

  // CPU time accumulation — avoids position jumps when rainSpeed changes
  pe->matrixRainTime += pe->currentDeltaTime * cfg->rainSpeed;

  SetShaderValue(pe->matrixRainShader, pe->matrixRainCellSizeLoc,
                 &cfg->cellSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainTrailLengthLoc,
                 &cfg->trailLength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainFallerCountLoc,
                 &cfg->fallerCount, SHADER_UNIFORM_INT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainOverlayIntensityLoc,
                 &cfg->overlayIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainRefreshRateLoc,
                 &cfg->refreshRate, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainLeadBrightnessLoc,
                 &cfg->leadBrightness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainTimeLoc,
                 &pe->matrixRainTime, SHADER_UNIFORM_FLOAT);
  const int sampleModeInt = cfg->sampleMode ? 1 : 0;
  SetShaderValue(pe->matrixRainShader, pe->matrixRainSampleModeLoc,
                 &sampleModeInt, SHADER_UNIFORM_INT);
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

void SetupKuwahara(PostEffect *pe) {
  const KuwaharaConfig *k = &pe->effects.kuwahara;
  int radius = (int)k->radius;
  SetShaderValue(pe->kuwaharaShader, pe->kuwaharaRadiusLoc, &radius,
                 SHADER_UNIFORM_INT);
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

void SetupLegoBricks(PostEffect *pe) {
  const LegoBricksConfig *cfg = &pe->effects.legoBricks;
  SetShaderValue(pe->legoBricksShader, pe->legoBricksBrickScaleLoc,
                 &cfg->brickScale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksStudHeightLoc,
                 &cfg->studHeight, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksEdgeShadowLoc,
                 &cfg->edgeShadow, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksColorThresholdLoc,
                 &cfg->colorThreshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksMaxBrickSizeLoc,
                 &cfg->maxBrickSize, SHADER_UNIFORM_INT);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksLightAngleLoc,
                 &cfg->lightAngle, SHADER_UNIFORM_FLOAT);
}
