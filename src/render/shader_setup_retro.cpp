#include "shader_setup_retro.h"
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

  // Block Multiply
  int blockMultiplyEnabled = g->blockMultiplyEnabled ? 1 : 0;
  SetShaderValue(pe->glitchShader, pe->glitchBlockMultiplyEnabledLoc,
                 &blockMultiplyEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMultiplySizeLoc,
                 &g->blockMultiplySize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMultiplyControlLoc,
                 &g->blockMultiplyControl, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMultiplyIterationsLoc,
                 &g->blockMultiplyIterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->glitchShader, pe->glitchBlockMultiplyIntensityLoc,
                 &g->blockMultiplyIntensity, SHADER_UNIFORM_FLOAT);
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

void SetupSynthwave(PostEffect *pe) {
  const SynthwaveConfig *sw = &pe->effects.synthwave;

  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonYLoc, &sw->horizonY,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveColorMixLoc, &sw->colorMix,
                 SHADER_UNIFORM_FLOAT);

  float palettePhase[3] = {sw->palettePhaseR, sw->palettePhaseG,
                           sw->palettePhaseB};
  SetShaderValue(pe->synthwaveShader, pe->synthwavePalettePhaseLoc,
                 palettePhase, SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridSpacingLoc,
                 &sw->gridSpacing, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridThicknessLoc,
                 &sw->gridThickness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridOpacityLoc,
                 &sw->gridOpacity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridGlowLoc, &sw->gridGlow,
                 SHADER_UNIFORM_FLOAT);

  float gridColor[3] = {sw->gridR, sw->gridG, sw->gridB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridColorLoc, gridColor,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeCountLoc,
                 &sw->stripeCount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeSoftnessLoc,
                 &sw->stripeSoftness, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeIntensityLoc,
                 &sw->stripeIntensity, SHADER_UNIFORM_FLOAT);

  float sunColor[3] = {sw->sunR, sw->sunG, sw->sunB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveSunColorLoc, sunColor,
                 SHADER_UNIFORM_VEC3);

  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonIntensityLoc,
                 &sw->horizonIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonFalloffLoc,
                 &sw->horizonFalloff, SHADER_UNIFORM_FLOAT);

  float horizonColor[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
  SetShaderValue(pe->synthwaveShader, pe->synthwaveHorizonColorLoc,
                 horizonColor, SHADER_UNIFORM_VEC3);

  // Animation (times accumulated with speed in render_pipeline.cpp)
  SetShaderValue(pe->synthwaveShader, pe->synthwaveGridTimeLoc,
                 &pe->synthwaveGridTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveStripeTimeLoc,
                 &pe->synthwaveStripeTime, SHADER_UNIFORM_FLOAT);
}
