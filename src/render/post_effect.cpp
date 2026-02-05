#include "post_effect.h"
#include "analysis/fft.h"
#include "blend_compositor.h"
#include "color_lut.h"
#include "effects/chladni_warp.h"
#include "effects/circuit_board.h"
#include "effects/corridor_warp.h"
#include "effects/density_wave_spiral.h"
#include "effects/domain_warp.h"
#include "effects/droste_zoom.h"
#include "effects/fft_radial_warp.h"
#include "effects/gradient_flow.h"
#include "effects/infinite_zoom.h"
#include "effects/interference_warp.h"
#include "effects/lattice_fold.h"
#include "effects/mobius.h"
#include "effects/phyllotaxis.h"
#include "effects/radial_pulse.h"
#include "effects/radial_streak.h"
#include "effects/relativistic_doppler.h"
#include "effects/shake.h"
#include "effects/sine_warp.h"
#include "effects/surface_warp.h"
#include "effects/texture_warp.h"
#include "effects/voronoi.h"
#include "effects/wave_ripple.h"
#include "render_utils.h"
#include "rlgl.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"
#include <stdlib.h>

static const char *LOG_PREFIX = "POST_EFFECT";

static const int WAVEFORM_TEXTURE_SIZE = 2048;

static void InitFFTTexture(Texture2D *tex) {
  tex->id =
      rlLoadTexture(NULL, FFT_BIN_COUNT, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
  tex->width = FFT_BIN_COUNT;
  tex->height = 1;
  tex->mipmaps = 1;
  tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

  SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static void InitWaveformTexture(Texture2D *tex) {
  tex->id = rlLoadTexture(NULL, WAVEFORM_TEXTURE_SIZE, 1,
                          RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
  tex->width = WAVEFORM_TEXTURE_SIZE;
  tex->height = 1;
  tex->mipmaps = 1;
  tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

  SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static void InitBloomMips(PostEffect *pe, int width, int height) {
  int w = width / 2;
  int h = height / 2;
  for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
    RenderUtilsInitTextureHDR(&pe->bloomMips[i], w, h, LOG_PREFIX);
    w /= 2;
    h /= 2;
    if (w < 1) {
      w = 1;
    }
    if (h < 1) {
      h = 1;
    }
  }
}

static void UnloadBloomMips(PostEffect *pe) {
  for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
    UnloadRenderTexture(pe->bloomMips[i]);
  }
}

static bool LoadPostEffectShaders(PostEffect *pe) {
  pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
  pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
  pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
  pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
  pe->fxaaShader = LoadShader(0, "shaders/fxaa.fs");
  pe->clarityShader = LoadShader(0, "shaders/clarity.fs");
  pe->gammaShader = LoadShader(0, "shaders/gamma.fs");
  pe->shapeTextureShader = LoadShader(0, "shaders/shape_texture.fs");
  pe->pixelationShader = LoadShader(0, "shaders/pixelation.fs");
  pe->glitchShader = LoadShader(0, "shaders/glitch.fs");
  pe->toonShader = LoadShader(0, "shaders/toon.fs");
  pe->heightfieldReliefShader = LoadShader(0, "shaders/heightfield_relief.fs");
  pe->colorGradeShader = LoadShader(0, "shaders/color_grade.fs");
  pe->asciiArtShader = LoadShader(0, "shaders/ascii_art.fs");
  pe->oilPaintShader = LoadShader(0, "shaders/oil_paint.fs");
  pe->oilPaintStrokeShader = LoadShader(0, "shaders/oil_paint_stroke.fs");
  pe->watercolorShader = LoadShader(0, "shaders/watercolor.fs");
  pe->neonGlowShader = LoadShader(0, "shaders/neon_glow.fs");
  pe->falseColorShader = LoadShader(0, "shaders/false_color.fs");
  pe->halftoneShader = LoadShader(0, "shaders/halftone.fs");
  pe->crossHatchingShader = LoadShader(0, "shaders/cross_hatching.fs");
  pe->paletteQuantizationShader =
      LoadShader(0, "shaders/palette_quantization.fs");
  pe->bokehShader = LoadShader(0, "shaders/bokeh.fs");
  pe->bloomPrefilterShader = LoadShader(0, "shaders/bloom_prefilter.fs");
  pe->bloomDownsampleShader = LoadShader(0, "shaders/bloom_downsample.fs");
  pe->bloomUpsampleShader = LoadShader(0, "shaders/bloom_upsample.fs");
  pe->bloomCompositeShader = LoadShader(0, "shaders/bloom_composite.fs");
  pe->anamorphicStreakPrefilterShader =
      LoadShader(0, "shaders/anamorphic_streak_prefilter.fs");
  pe->anamorphicStreakBlurShader =
      LoadShader(0, "shaders/anamorphic_streak_blur.fs");
  pe->anamorphicStreakCompositeShader =
      LoadShader(0, "shaders/anamorphic_streak_composite.fs");
  pe->pencilSketchShader = LoadShader(0, "shaders/pencil_sketch.fs");
  pe->matrixRainShader = LoadShader(0, "shaders/matrix_rain.fs");
  pe->impressionistShader = LoadShader(0, "shaders/impressionist.fs");
  pe->kuwaharaShader = LoadShader(0, "shaders/kuwahara.fs");
  pe->inkWashShader = LoadShader(0, "shaders/ink_wash.fs");
  pe->discoBallShader = LoadShader(0, "shaders/disco_ball.fs");
  pe->legoBricksShader = LoadShader(0, "shaders/lego_bricks.fs");
  pe->synthwaveShader = LoadShader(0, "shaders/synthwave.fs");
  pe->constellationShader = LoadShader(0, "shaders/constellation.fs");
  pe->plasmaShader = LoadShader(0, "shaders/plasma.fs");
  pe->interferenceShader = LoadShader(0, "shaders/interference.fs");

  return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
         pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
         pe->fxaaShader.id != 0 && pe->clarityShader.id != 0 &&
         pe->gammaShader.id != 0 && pe->shapeTextureShader.id != 0 &&
         pe->pixelationShader.id != 0 && pe->glitchShader.id != 0 &&
         pe->toonShader.id != 0 && pe->heightfieldReliefShader.id != 0 &&
         pe->colorGradeShader.id != 0 && pe->asciiArtShader.id != 0 &&
         pe->oilPaintShader.id != 0 && pe->oilPaintStrokeShader.id != 0 &&
         pe->watercolorShader.id != 0 && pe->neonGlowShader.id != 0 &&
         pe->falseColorShader.id != 0 && pe->halftoneShader.id != 0 &&
         pe->crossHatchingShader.id != 0 &&
         pe->paletteQuantizationShader.id != 0 && pe->bokehShader.id != 0 &&
         pe->bloomPrefilterShader.id != 0 &&
         pe->bloomDownsampleShader.id != 0 && pe->bloomUpsampleShader.id != 0 &&
         pe->bloomCompositeShader.id != 0 &&
         pe->anamorphicStreakPrefilterShader.id != 0 &&
         pe->anamorphicStreakBlurShader.id != 0 &&
         pe->anamorphicStreakCompositeShader.id != 0 &&
         pe->pencilSketchShader.id != 0 && pe->matrixRainShader.id != 0 &&
         pe->impressionistShader.id != 0 && pe->kuwaharaShader.id != 0 &&
         pe->inkWashShader.id != 0 && pe->discoBallShader.id != 0 &&
         pe->legoBricksShader.id != 0 && pe->synthwaveShader.id != 0 &&
         pe->constellationShader.id != 0 && pe->plasmaShader.id != 0 &&
         pe->interferenceShader.id != 0;
}

// NOLINTNEXTLINE(readability-function-size) - caches all shader uniform
// locations
static void GetShaderUniformLocations(PostEffect *pe) {
  pe->blurHResolutionLoc = GetShaderLocation(pe->blurHShader, "resolution");
  pe->blurVResolutionLoc = GetShaderLocation(pe->blurVShader, "resolution");
  pe->blurHScaleLoc = GetShaderLocation(pe->blurHShader, "blurScale");
  pe->blurVScaleLoc = GetShaderLocation(pe->blurVShader, "blurScale");
  pe->halfLifeLoc = GetShaderLocation(pe->blurVShader, "halfLife");
  pe->deltaTimeLoc = GetShaderLocation(pe->blurVShader, "deltaTime");
  pe->chromaticResolutionLoc =
      GetShaderLocation(pe->chromaticShader, "resolution");
  pe->chromaticOffsetLoc =
      GetShaderLocation(pe->chromaticShader, "chromaticOffset");
  pe->feedbackResolutionLoc =
      GetShaderLocation(pe->feedbackShader, "resolution");
  pe->feedbackDesaturateLoc =
      GetShaderLocation(pe->feedbackShader, "desaturate");
  pe->feedbackZoomBaseLoc = GetShaderLocation(pe->feedbackShader, "zoomBase");
  pe->feedbackZoomRadialLoc =
      GetShaderLocation(pe->feedbackShader, "zoomRadial");
  pe->feedbackRotBaseLoc = GetShaderLocation(pe->feedbackShader, "rotBase");
  pe->feedbackRotRadialLoc = GetShaderLocation(pe->feedbackShader, "rotRadial");
  pe->feedbackDxBaseLoc = GetShaderLocation(pe->feedbackShader, "dxBase");
  pe->feedbackDxRadialLoc = GetShaderLocation(pe->feedbackShader, "dxRadial");
  pe->feedbackDyBaseLoc = GetShaderLocation(pe->feedbackShader, "dyBase");
  pe->feedbackDyRadialLoc = GetShaderLocation(pe->feedbackShader, "dyRadial");
  pe->feedbackFlowStrengthLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowStrength");
  pe->feedbackFlowAngleLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowAngle");
  pe->feedbackFlowScaleLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowScale");
  pe->feedbackFlowThresholdLoc =
      GetShaderLocation(pe->feedbackShader, "feedbackFlowThreshold");
  pe->feedbackCxLoc = GetShaderLocation(pe->feedbackShader, "cx");
  pe->feedbackCyLoc = GetShaderLocation(pe->feedbackShader, "cy");
  pe->feedbackSxLoc = GetShaderLocation(pe->feedbackShader, "sx");
  pe->feedbackSyLoc = GetShaderLocation(pe->feedbackShader, "sy");
  pe->feedbackZoomAngularLoc =
      GetShaderLocation(pe->feedbackShader, "zoomAngular");
  pe->feedbackZoomAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "zoomAngularFreq");
  pe->feedbackRotAngularLoc =
      GetShaderLocation(pe->feedbackShader, "rotAngular");
  pe->feedbackRotAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "rotAngularFreq");
  pe->feedbackDxAngularLoc = GetShaderLocation(pe->feedbackShader, "dxAngular");
  pe->feedbackDxAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "dxAngularFreq");
  pe->feedbackDyAngularLoc = GetShaderLocation(pe->feedbackShader, "dyAngular");
  pe->feedbackDyAngularFreqLoc =
      GetShaderLocation(pe->feedbackShader, "dyAngularFreq");
  pe->feedbackWarpLoc = GetShaderLocation(pe->feedbackShader, "warp");
  pe->feedbackWarpTimeLoc = GetShaderLocation(pe->feedbackShader, "warpTime");
  pe->feedbackWarpScaleInverseLoc =
      GetShaderLocation(pe->feedbackShader, "warpScaleInverse");
  pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
  pe->clarityResolutionLoc = GetShaderLocation(pe->clarityShader, "resolution");
  pe->clarityAmountLoc = GetShaderLocation(pe->clarityShader, "clarity");
  pe->gammaGammaLoc = GetShaderLocation(pe->gammaShader, "gamma");
  pe->shapeTexZoomLoc = GetShaderLocation(pe->shapeTextureShader, "texZoom");
  pe->shapeTexAngleLoc = GetShaderLocation(pe->shapeTextureShader, "texAngle");
  pe->shapeTexBrightnessLoc =
      GetShaderLocation(pe->shapeTextureShader, "texBrightness");
  pe->pixelationResolutionLoc =
      GetShaderLocation(pe->pixelationShader, "resolution");
  pe->pixelationCellCountLoc =
      GetShaderLocation(pe->pixelationShader, "cellCount");
  pe->pixelationDitherScaleLoc =
      GetShaderLocation(pe->pixelationShader, "ditherScale");
  pe->pixelationPosterizeLevelsLoc =
      GetShaderLocation(pe->pixelationShader, "posterizeLevels");
  pe->glitchResolutionLoc = GetShaderLocation(pe->glitchShader, "resolution");
  pe->glitchTimeLoc = GetShaderLocation(pe->glitchShader, "time");
  pe->glitchFrameLoc = GetShaderLocation(pe->glitchShader, "frame");
  pe->glitchCrtEnabledLoc = GetShaderLocation(pe->glitchShader, "crtEnabled");
  pe->glitchCurvatureLoc = GetShaderLocation(pe->glitchShader, "curvature");
  pe->glitchVignetteEnabledLoc =
      GetShaderLocation(pe->glitchShader, "vignetteEnabled");
  pe->glitchAnalogIntensityLoc =
      GetShaderLocation(pe->glitchShader, "analogIntensity");
  pe->glitchAberrationLoc = GetShaderLocation(pe->glitchShader, "aberration");
  pe->glitchBlockThresholdLoc =
      GetShaderLocation(pe->glitchShader, "blockThreshold");
  pe->glitchBlockOffsetLoc = GetShaderLocation(pe->glitchShader, "blockOffset");
  pe->glitchVhsEnabledLoc = GetShaderLocation(pe->glitchShader, "vhsEnabled");
  pe->glitchTrackingBarIntensityLoc =
      GetShaderLocation(pe->glitchShader, "trackingBarIntensity");
  pe->glitchScanlineNoiseIntensityLoc =
      GetShaderLocation(pe->glitchShader, "scanlineNoiseIntensity");
  pe->glitchColorDriftIntensityLoc =
      GetShaderLocation(pe->glitchShader, "colorDriftIntensity");
  pe->glitchScanlineAmountLoc =
      GetShaderLocation(pe->glitchShader, "scanlineAmount");
  pe->glitchNoiseAmountLoc = GetShaderLocation(pe->glitchShader, "noiseAmount");
  pe->glitchDatamoshEnabledLoc =
      GetShaderLocation(pe->glitchShader, "datamoshEnabled");
  pe->glitchDatamoshIntensityLoc =
      GetShaderLocation(pe->glitchShader, "datamoshIntensity");
  pe->glitchDatamoshMinLoc = GetShaderLocation(pe->glitchShader, "datamoshMin");
  pe->glitchDatamoshMaxLoc = GetShaderLocation(pe->glitchShader, "datamoshMax");
  pe->glitchDatamoshSpeedLoc =
      GetShaderLocation(pe->glitchShader, "datamoshSpeed");
  pe->glitchDatamoshBandsLoc =
      GetShaderLocation(pe->glitchShader, "datamoshBands");
  pe->glitchRowSliceEnabledLoc =
      GetShaderLocation(pe->glitchShader, "rowSliceEnabled");
  pe->glitchRowSliceIntensityLoc =
      GetShaderLocation(pe->glitchShader, "rowSliceIntensity");
  pe->glitchRowSliceBurstFreqLoc =
      GetShaderLocation(pe->glitchShader, "rowSliceBurstFreq");
  pe->glitchRowSliceBurstPowerLoc =
      GetShaderLocation(pe->glitchShader, "rowSliceBurstPower");
  pe->glitchRowSliceColumnsLoc =
      GetShaderLocation(pe->glitchShader, "rowSliceColumns");
  pe->glitchColSliceEnabledLoc =
      GetShaderLocation(pe->glitchShader, "colSliceEnabled");
  pe->glitchColSliceIntensityLoc =
      GetShaderLocation(pe->glitchShader, "colSliceIntensity");
  pe->glitchColSliceBurstFreqLoc =
      GetShaderLocation(pe->glitchShader, "colSliceBurstFreq");
  pe->glitchColSliceBurstPowerLoc =
      GetShaderLocation(pe->glitchShader, "colSliceBurstPower");
  pe->glitchColSliceRowsLoc =
      GetShaderLocation(pe->glitchShader, "colSliceRows");
  pe->glitchDiagonalBandsEnabledLoc =
      GetShaderLocation(pe->glitchShader, "diagonalBandsEnabled");
  pe->glitchDiagonalBandCountLoc =
      GetShaderLocation(pe->glitchShader, "diagonalBandCount");
  pe->glitchDiagonalBandDisplaceLoc =
      GetShaderLocation(pe->glitchShader, "diagonalBandDisplace");
  pe->glitchDiagonalBandSpeedLoc =
      GetShaderLocation(pe->glitchShader, "diagonalBandSpeed");
  pe->glitchBlockMaskEnabledLoc =
      GetShaderLocation(pe->glitchShader, "blockMaskEnabled");
  pe->glitchBlockMaskIntensityLoc =
      GetShaderLocation(pe->glitchShader, "blockMaskIntensity");
  pe->glitchBlockMaskMinSizeLoc =
      GetShaderLocation(pe->glitchShader, "blockMaskMinSize");
  pe->glitchBlockMaskMaxSizeLoc =
      GetShaderLocation(pe->glitchShader, "blockMaskMaxSize");
  pe->glitchBlockMaskTintLoc =
      GetShaderLocation(pe->glitchShader, "blockMaskTint");
  pe->glitchTemporalJitterEnabledLoc =
      GetShaderLocation(pe->glitchShader, "temporalJitterEnabled");
  pe->glitchTemporalJitterAmountLoc =
      GetShaderLocation(pe->glitchShader, "temporalJitterAmount");
  pe->glitchTemporalJitterGateLoc =
      GetShaderLocation(pe->glitchShader, "temporalJitterGate");
  pe->glitchBlockMultiplyEnabledLoc =
      GetShaderLocation(pe->glitchShader, "blockMultiplyEnabled");
  pe->glitchBlockMultiplySizeLoc =
      GetShaderLocation(pe->glitchShader, "blockMultiplySize");
  pe->glitchBlockMultiplyControlLoc =
      GetShaderLocation(pe->glitchShader, "blockMultiplyControl");
  pe->glitchBlockMultiplyIterationsLoc =
      GetShaderLocation(pe->glitchShader, "blockMultiplyIterations");
  pe->glitchBlockMultiplyIntensityLoc =
      GetShaderLocation(pe->glitchShader, "blockMultiplyIntensity");
  pe->toonResolutionLoc = GetShaderLocation(pe->toonShader, "resolution");
  pe->toonLevelsLoc = GetShaderLocation(pe->toonShader, "levels");
  pe->toonEdgeThresholdLoc = GetShaderLocation(pe->toonShader, "edgeThreshold");
  pe->toonEdgeSoftnessLoc = GetShaderLocation(pe->toonShader, "edgeSoftness");
  pe->toonThicknessVariationLoc =
      GetShaderLocation(pe->toonShader, "thicknessVariation");
  pe->toonNoiseScaleLoc = GetShaderLocation(pe->toonShader, "noiseScale");
  pe->heightfieldReliefResolutionLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "resolution");
  pe->heightfieldReliefIntensityLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "intensity");
  pe->heightfieldReliefReliefScaleLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "reliefScale");
  pe->heightfieldReliefLightAngleLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "lightAngle");
  pe->heightfieldReliefLightHeightLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "lightHeight");
  pe->heightfieldReliefShininessLoc =
      GetShaderLocation(pe->heightfieldReliefShader, "shininess");
  pe->colorGradeHueShiftLoc =
      GetShaderLocation(pe->colorGradeShader, "hueShift");
  pe->colorGradeSaturationLoc =
      GetShaderLocation(pe->colorGradeShader, "saturation");
  pe->colorGradeBrightnessLoc =
      GetShaderLocation(pe->colorGradeShader, "brightness");
  pe->colorGradeContrastLoc =
      GetShaderLocation(pe->colorGradeShader, "contrast");
  pe->colorGradeTemperatureLoc =
      GetShaderLocation(pe->colorGradeShader, "temperature");
  pe->colorGradeShadowsOffsetLoc =
      GetShaderLocation(pe->colorGradeShader, "shadowsOffset");
  pe->colorGradeMidtonesOffsetLoc =
      GetShaderLocation(pe->colorGradeShader, "midtonesOffset");
  pe->colorGradeHighlightsOffsetLoc =
      GetShaderLocation(pe->colorGradeShader, "highlightsOffset");
  pe->asciiArtResolutionLoc =
      GetShaderLocation(pe->asciiArtShader, "resolution");
  pe->asciiArtCellPixelsLoc =
      GetShaderLocation(pe->asciiArtShader, "cellPixels");
  pe->asciiArtColorModeLoc = GetShaderLocation(pe->asciiArtShader, "colorMode");
  pe->asciiArtForegroundLoc =
      GetShaderLocation(pe->asciiArtShader, "foreground");
  pe->asciiArtBackgroundLoc =
      GetShaderLocation(pe->asciiArtShader, "background");
  pe->asciiArtInvertLoc = GetShaderLocation(pe->asciiArtShader, "invert");
  pe->oilPaintStrokeResolutionLoc =
      GetShaderLocation(pe->oilPaintStrokeShader, "resolution");
  pe->oilPaintBrushSizeLoc =
      GetShaderLocation(pe->oilPaintStrokeShader, "brushSize");
  pe->oilPaintStrokeBendLoc =
      GetShaderLocation(pe->oilPaintStrokeShader, "strokeBend");
  pe->oilPaintLayersLoc = GetShaderLocation(pe->oilPaintStrokeShader, "layers");
  pe->oilPaintNoiseTexLoc =
      GetShaderLocation(pe->oilPaintStrokeShader, "texture1");
  pe->oilPaintResolutionLoc =
      GetShaderLocation(pe->oilPaintShader, "resolution");
  pe->oilPaintSpecularLoc = GetShaderLocation(pe->oilPaintShader, "specular");
  pe->watercolorResolutionLoc =
      GetShaderLocation(pe->watercolorShader, "resolution");
  pe->watercolorSamplesLoc = GetShaderLocation(pe->watercolorShader, "samples");
  pe->watercolorStrokeStepLoc =
      GetShaderLocation(pe->watercolorShader, "strokeStep");
  pe->watercolorWashStrengthLoc =
      GetShaderLocation(pe->watercolorShader, "washStrength");
  pe->watercolorPaperScaleLoc =
      GetShaderLocation(pe->watercolorShader, "paperScale");
  pe->watercolorPaperStrengthLoc =
      GetShaderLocation(pe->watercolorShader, "paperStrength");
  pe->watercolorEdgePoolLoc =
      GetShaderLocation(pe->watercolorShader, "edgePool");
  pe->watercolorFlowCenterLoc =
      GetShaderLocation(pe->watercolorShader, "flowCenter");
  pe->watercolorFlowWidthLoc =
      GetShaderLocation(pe->watercolorShader, "flowWidth");
  pe->neonGlowResolutionLoc =
      GetShaderLocation(pe->neonGlowShader, "resolution");
  pe->neonGlowGlowColorLoc = GetShaderLocation(pe->neonGlowShader, "glowColor");
  pe->neonGlowEdgeThresholdLoc =
      GetShaderLocation(pe->neonGlowShader, "edgeThreshold");
  pe->neonGlowEdgePowerLoc = GetShaderLocation(pe->neonGlowShader, "edgePower");
  pe->neonGlowGlowIntensityLoc =
      GetShaderLocation(pe->neonGlowShader, "glowIntensity");
  pe->neonGlowGlowRadiusLoc =
      GetShaderLocation(pe->neonGlowShader, "glowRadius");
  pe->neonGlowGlowSamplesLoc =
      GetShaderLocation(pe->neonGlowShader, "glowSamples");
  pe->neonGlowOriginalVisibilityLoc =
      GetShaderLocation(pe->neonGlowShader, "originalVisibility");
  pe->neonGlowColorModeLoc = GetShaderLocation(pe->neonGlowShader, "colorMode");
  pe->neonGlowSaturationBoostLoc =
      GetShaderLocation(pe->neonGlowShader, "saturationBoost");
  pe->neonGlowBrightnessBoostLoc =
      GetShaderLocation(pe->neonGlowShader, "brightnessBoost");
  pe->falseColorIntensityLoc =
      GetShaderLocation(pe->falseColorShader, "intensity");
  pe->falseColorGradientLUTLoc =
      GetShaderLocation(pe->falseColorShader, "texture1");
  pe->halftoneResolutionLoc =
      GetShaderLocation(pe->halftoneShader, "resolution");
  pe->halftoneDotScaleLoc = GetShaderLocation(pe->halftoneShader, "dotScale");
  pe->halftoneDotSizeLoc = GetShaderLocation(pe->halftoneShader, "dotSize");
  pe->halftoneRotationLoc = GetShaderLocation(pe->halftoneShader, "rotation");
  pe->crossHatchingResolutionLoc =
      GetShaderLocation(pe->crossHatchingShader, "resolution");
  pe->crossHatchingTimeLoc = GetShaderLocation(pe->crossHatchingShader, "time");
  pe->crossHatchingWidthLoc =
      GetShaderLocation(pe->crossHatchingShader, "width");
  pe->crossHatchingThresholdLoc =
      GetShaderLocation(pe->crossHatchingShader, "threshold");
  pe->crossHatchingNoiseLoc =
      GetShaderLocation(pe->crossHatchingShader, "noise");
  pe->crossHatchingOutlineLoc =
      GetShaderLocation(pe->crossHatchingShader, "outline");
  pe->paletteQuantizationColorLevelsLoc =
      GetShaderLocation(pe->paletteQuantizationShader, "colorLevels");
  pe->paletteQuantizationDitherStrengthLoc =
      GetShaderLocation(pe->paletteQuantizationShader, "ditherStrength");
  pe->paletteQuantizationBayerSizeLoc =
      GetShaderLocation(pe->paletteQuantizationShader, "bayerSize");
  pe->bokehResolutionLoc = GetShaderLocation(pe->bokehShader, "resolution");
  pe->bokehRadiusLoc = GetShaderLocation(pe->bokehShader, "radius");
  pe->bokehIterationsLoc = GetShaderLocation(pe->bokehShader, "iterations");
  pe->bokehBrightnessPowerLoc =
      GetShaderLocation(pe->bokehShader, "brightnessPower");
  pe->bloomThresholdLoc =
      GetShaderLocation(pe->bloomPrefilterShader, "threshold");
  pe->bloomKneeLoc = GetShaderLocation(pe->bloomPrefilterShader, "knee");
  pe->bloomDownsampleHalfpixelLoc =
      GetShaderLocation(pe->bloomDownsampleShader, "halfpixel");
  pe->bloomUpsampleHalfpixelLoc =
      GetShaderLocation(pe->bloomUpsampleShader, "halfpixel");
  pe->bloomIntensityLoc =
      GetShaderLocation(pe->bloomCompositeShader, "intensity");
  pe->bloomBloomTexLoc =
      GetShaderLocation(pe->bloomCompositeShader, "bloomTexture");
  pe->anamorphicStreakThresholdLoc =
      GetShaderLocation(pe->anamorphicStreakPrefilterShader, "threshold");
  pe->anamorphicStreakKneeLoc =
      GetShaderLocation(pe->anamorphicStreakPrefilterShader, "knee");
  pe->anamorphicStreakResolutionLoc =
      GetShaderLocation(pe->anamorphicStreakBlurShader, "resolution");
  pe->anamorphicStreakOffsetLoc =
      GetShaderLocation(pe->anamorphicStreakBlurShader, "offset");
  pe->anamorphicStreakSharpnessLoc =
      GetShaderLocation(pe->anamorphicStreakBlurShader, "sharpness");
  pe->anamorphicStreakIntensityLoc =
      GetShaderLocation(pe->anamorphicStreakCompositeShader, "intensity");
  pe->anamorphicStreakStreakTexLoc =
      GetShaderLocation(pe->anamorphicStreakCompositeShader, "streakTexture");
  pe->pencilSketchResolutionLoc =
      GetShaderLocation(pe->pencilSketchShader, "resolution");
  pe->pencilSketchAngleCountLoc =
      GetShaderLocation(pe->pencilSketchShader, "angleCount");
  pe->pencilSketchSampleCountLoc =
      GetShaderLocation(pe->pencilSketchShader, "sampleCount");
  pe->pencilSketchStrokeFalloffLoc =
      GetShaderLocation(pe->pencilSketchShader, "strokeFalloff");
  pe->pencilSketchGradientEpsLoc =
      GetShaderLocation(pe->pencilSketchShader, "gradientEps");
  pe->pencilSketchPaperStrengthLoc =
      GetShaderLocation(pe->pencilSketchShader, "paperStrength");
  pe->pencilSketchVignetteStrengthLoc =
      GetShaderLocation(pe->pencilSketchShader, "vignetteStrength");
  pe->pencilSketchWobbleTimeLoc =
      GetShaderLocation(pe->pencilSketchShader, "wobbleTime");
  pe->pencilSketchWobbleAmountLoc =
      GetShaderLocation(pe->pencilSketchShader, "wobbleAmount");
  pe->matrixRainResolutionLoc =
      GetShaderLocation(pe->matrixRainShader, "resolution");
  pe->matrixRainCellSizeLoc =
      GetShaderLocation(pe->matrixRainShader, "cellSize");
  pe->matrixRainTrailLengthLoc =
      GetShaderLocation(pe->matrixRainShader, "trailLength");
  pe->matrixRainFallerCountLoc =
      GetShaderLocation(pe->matrixRainShader, "fallerCount");
  pe->matrixRainOverlayIntensityLoc =
      GetShaderLocation(pe->matrixRainShader, "overlayIntensity");
  pe->matrixRainRefreshRateLoc =
      GetShaderLocation(pe->matrixRainShader, "refreshRate");
  pe->matrixRainLeadBrightnessLoc =
      GetShaderLocation(pe->matrixRainShader, "leadBrightness");
  pe->matrixRainTimeLoc = GetShaderLocation(pe->matrixRainShader, "time");
  pe->matrixRainSampleModeLoc =
      GetShaderLocation(pe->matrixRainShader, "sampleMode");
  pe->impressionistResolutionLoc =
      GetShaderLocation(pe->impressionistShader, "resolution");
  pe->impressionistSplatCountLoc =
      GetShaderLocation(pe->impressionistShader, "splatCount");
  pe->impressionistSplatSizeMinLoc =
      GetShaderLocation(pe->impressionistShader, "splatSizeMin");
  pe->impressionistSplatSizeMaxLoc =
      GetShaderLocation(pe->impressionistShader, "splatSizeMax");
  pe->impressionistStrokeFreqLoc =
      GetShaderLocation(pe->impressionistShader, "strokeFreq");
  pe->impressionistStrokeOpacityLoc =
      GetShaderLocation(pe->impressionistShader, "strokeOpacity");
  pe->impressionistOutlineStrengthLoc =
      GetShaderLocation(pe->impressionistShader, "outlineStrength");
  pe->impressionistEdgeStrengthLoc =
      GetShaderLocation(pe->impressionistShader, "edgeStrength");
  pe->impressionistEdgeMaxDarkenLoc =
      GetShaderLocation(pe->impressionistShader, "edgeMaxDarken");
  pe->impressionistGrainScaleLoc =
      GetShaderLocation(pe->impressionistShader, "grainScale");
  pe->impressionistGrainAmountLoc =
      GetShaderLocation(pe->impressionistShader, "grainAmount");
  pe->impressionistExposureLoc =
      GetShaderLocation(pe->impressionistShader, "exposure");
  pe->kuwaharaResolutionLoc =
      GetShaderLocation(pe->kuwaharaShader, "resolution");
  pe->kuwaharaRadiusLoc = GetShaderLocation(pe->kuwaharaShader, "radius");
  pe->inkWashResolutionLoc = GetShaderLocation(pe->inkWashShader, "resolution");
  pe->inkWashStrengthLoc = GetShaderLocation(pe->inkWashShader, "strength");
  pe->inkWashGranulationLoc =
      GetShaderLocation(pe->inkWashShader, "granulation");
  pe->inkWashBleedStrengthLoc =
      GetShaderLocation(pe->inkWashShader, "bleedStrength");
  pe->inkWashBleedRadiusLoc =
      GetShaderLocation(pe->inkWashShader, "bleedRadius");
  pe->inkWashSoftnessLoc = GetShaderLocation(pe->inkWashShader, "softness");
  pe->discoBallResolutionLoc =
      GetShaderLocation(pe->discoBallShader, "resolution");
  pe->discoBallSphereRadiusLoc =
      GetShaderLocation(pe->discoBallShader, "sphereRadius");
  pe->discoBallTileSizeLoc = GetShaderLocation(pe->discoBallShader, "tileSize");
  pe->discoBallSphereAngleLoc =
      GetShaderLocation(pe->discoBallShader, "sphereAngle");
  pe->discoBallBumpHeightLoc =
      GetShaderLocation(pe->discoBallShader, "bumpHeight");
  pe->discoBallReflectIntensityLoc =
      GetShaderLocation(pe->discoBallShader, "reflectIntensity");
  pe->discoBallSpotIntensityLoc =
      GetShaderLocation(pe->discoBallShader, "spotIntensity");
  pe->discoBallSpotFalloffLoc =
      GetShaderLocation(pe->discoBallShader, "spotFalloff");
  pe->discoBallBrightnessThresholdLoc =
      GetShaderLocation(pe->discoBallShader, "brightnessThreshold");
  pe->legoBricksResolutionLoc =
      GetShaderLocation(pe->legoBricksShader, "resolution");
  pe->legoBricksBrickScaleLoc =
      GetShaderLocation(pe->legoBricksShader, "brickScale");
  pe->legoBricksStudHeightLoc =
      GetShaderLocation(pe->legoBricksShader, "studHeight");
  pe->legoBricksEdgeShadowLoc =
      GetShaderLocation(pe->legoBricksShader, "edgeShadow");
  pe->legoBricksColorThresholdLoc =
      GetShaderLocation(pe->legoBricksShader, "colorThreshold");
  pe->legoBricksMaxBrickSizeLoc =
      GetShaderLocation(pe->legoBricksShader, "maxBrickSize");
  pe->legoBricksLightAngleLoc =
      GetShaderLocation(pe->legoBricksShader, "lightAngle");
  pe->synthwaveResolutionLoc =
      GetShaderLocation(pe->synthwaveShader, "resolution");
  pe->synthwaveHorizonYLoc = GetShaderLocation(pe->synthwaveShader, "horizonY");
  pe->synthwaveColorMixLoc = GetShaderLocation(pe->synthwaveShader, "colorMix");
  pe->synthwavePalettePhaseLoc =
      GetShaderLocation(pe->synthwaveShader, "palettePhase");
  pe->synthwaveGridSpacingLoc =
      GetShaderLocation(pe->synthwaveShader, "gridSpacing");
  pe->synthwaveGridThicknessLoc =
      GetShaderLocation(pe->synthwaveShader, "gridThickness");
  pe->synthwaveGridOpacityLoc =
      GetShaderLocation(pe->synthwaveShader, "gridOpacity");
  pe->synthwaveGridGlowLoc = GetShaderLocation(pe->synthwaveShader, "gridGlow");
  pe->synthwaveGridColorLoc =
      GetShaderLocation(pe->synthwaveShader, "gridColor");
  pe->synthwaveStripeCountLoc =
      GetShaderLocation(pe->synthwaveShader, "stripeCount");
  pe->synthwaveStripeSoftnessLoc =
      GetShaderLocation(pe->synthwaveShader, "stripeSoftness");
  pe->synthwaveStripeIntensityLoc =
      GetShaderLocation(pe->synthwaveShader, "stripeIntensity");
  pe->synthwaveSunColorLoc = GetShaderLocation(pe->synthwaveShader, "sunColor");
  pe->synthwaveHorizonIntensityLoc =
      GetShaderLocation(pe->synthwaveShader, "horizonIntensity");
  pe->synthwaveHorizonFalloffLoc =
      GetShaderLocation(pe->synthwaveShader, "horizonFalloff");
  pe->synthwaveHorizonColorLoc =
      GetShaderLocation(pe->synthwaveShader, "horizonColor");
  pe->synthwaveGridTimeLoc = GetShaderLocation(pe->synthwaveShader, "gridTime");
  pe->synthwaveStripeTimeLoc =
      GetShaderLocation(pe->synthwaveShader, "stripeTime");
  pe->constellationResolutionLoc =
      GetShaderLocation(pe->constellationShader, "resolution");
  pe->constellationAnimPhaseLoc =
      GetShaderLocation(pe->constellationShader, "animPhase");
  pe->constellationRadialPhaseLoc =
      GetShaderLocation(pe->constellationShader, "radialPhase");
  pe->constellationGridScaleLoc =
      GetShaderLocation(pe->constellationShader, "gridScale");
  pe->constellationWanderAmpLoc =
      GetShaderLocation(pe->constellationShader, "wanderAmp");
  pe->constellationRadialFreqLoc =
      GetShaderLocation(pe->constellationShader, "radialFreq");
  pe->constellationRadialAmpLoc =
      GetShaderLocation(pe->constellationShader, "radialAmp");
  pe->constellationPointSizeLoc =
      GetShaderLocation(pe->constellationShader, "pointSize");
  pe->constellationPointBrightnessLoc =
      GetShaderLocation(pe->constellationShader, "pointBrightness");
  pe->constellationLineThicknessLoc =
      GetShaderLocation(pe->constellationShader, "lineThickness");
  pe->constellationMaxLineLenLoc =
      GetShaderLocation(pe->constellationShader, "maxLineLen");
  pe->constellationLineOpacityLoc =
      GetShaderLocation(pe->constellationShader, "lineOpacity");
  pe->constellationInterpolateLineColorLoc =
      GetShaderLocation(pe->constellationShader, "interpolateLineColor");
  pe->constellationPointLUTLoc =
      GetShaderLocation(pe->constellationShader, "pointLUT");
  pe->constellationLineLUTLoc =
      GetShaderLocation(pe->constellationShader, "lineLUT");
  pe->plasmaResolutionLoc = GetShaderLocation(pe->plasmaShader, "resolution");
  pe->plasmaAnimPhaseLoc = GetShaderLocation(pe->plasmaShader, "animPhase");
  pe->plasmaDriftPhaseLoc = GetShaderLocation(pe->plasmaShader, "driftPhase");
  pe->plasmaFlickerTimeLoc = GetShaderLocation(pe->plasmaShader, "flickerTime");
  pe->plasmaBoltCountLoc = GetShaderLocation(pe->plasmaShader, "boltCount");
  pe->plasmaLayerCountLoc = GetShaderLocation(pe->plasmaShader, "layerCount");
  pe->plasmaOctavesLoc = GetShaderLocation(pe->plasmaShader, "octaves");
  pe->plasmaFalloffTypeLoc = GetShaderLocation(pe->plasmaShader, "falloffType");
  pe->plasmaDriftAmountLoc = GetShaderLocation(pe->plasmaShader, "driftAmount");
  pe->plasmaDisplacementLoc =
      GetShaderLocation(pe->plasmaShader, "displacement");
  pe->plasmaGlowRadiusLoc = GetShaderLocation(pe->plasmaShader, "glowRadius");
  pe->plasmaCoreBrightnessLoc =
      GetShaderLocation(pe->plasmaShader, "coreBrightness");
  pe->plasmaFlickerAmountLoc =
      GetShaderLocation(pe->plasmaShader, "flickerAmount");
  pe->plasmaGradientLUTLoc = GetShaderLocation(pe->plasmaShader, "gradientLUT");
  pe->interferenceResolutionLoc =
      GetShaderLocation(pe->interferenceShader, "resolution");
  pe->interferenceTimeLoc = GetShaderLocation(pe->interferenceShader, "time");
  pe->interferenceSourcesLoc =
      GetShaderLocation(pe->interferenceShader, "sources");
  pe->interferencePhasesLoc =
      GetShaderLocation(pe->interferenceShader, "phases");
  pe->interferenceWaveFreqLoc =
      GetShaderLocation(pe->interferenceShader, "waveFreq");
  pe->interferenceFalloffTypeLoc =
      GetShaderLocation(pe->interferenceShader, "falloffType");
  pe->interferenceFalloffStrengthLoc =
      GetShaderLocation(pe->interferenceShader, "falloffStrength");
  pe->interferenceBoundariesLoc =
      GetShaderLocation(pe->interferenceShader, "boundaries");
  pe->interferenceReflectionGainLoc =
      GetShaderLocation(pe->interferenceShader, "reflectionGain");
  pe->interferenceVisualModeLoc =
      GetShaderLocation(pe->interferenceShader, "visualMode");
  pe->interferenceContourCountLoc =
      GetShaderLocation(pe->interferenceShader, "contourCount");
  pe->interferenceVisualGainLoc =
      GetShaderLocation(pe->interferenceShader, "visualGain");
  pe->interferenceChromaSpreadLoc =
      GetShaderLocation(pe->interferenceShader, "chromaSpread");
  pe->interferenceColorModeLoc =
      GetShaderLocation(pe->interferenceShader, "colorMode");
  pe->interferenceColorLUTLoc =
      GetShaderLocation(pe->interferenceShader, "colorLUT");
  pe->interferenceSourceCountLoc =
      GetShaderLocation(pe->interferenceShader, "sourceCount");
}

static void SetResolutionUniforms(PostEffect *pe, int width, int height) {
  float resolution[2] = {(float)width, (float)height};
  SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->voronoi.shader, pe->voronoi.resolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->feedbackShader, pe->feedbackResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->fxaaShader, pe->fxaaResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->clarityShader, pe->clarityResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->pixelationShader, pe->pixelationResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->glitchShader, pe->glitchResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->toonShader, pe->toonResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->heightfieldReliefShader,
                 pe->heightfieldReliefResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->asciiArtShader, pe->asciiArtResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->oilPaintShader, pe->oilPaintResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->watercolorShader, pe->watercolorResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->neonGlowShader, pe->neonGlowResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->halftoneShader, pe->halftoneResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->crossHatchingShader, pe->crossHatchingResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->bokehShader, pe->bokehResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->phyllotaxis.shader, pe->phyllotaxis.resolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->pencilSketchShader, pe->pencilSketchResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->matrixRainShader, pe->matrixRainResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->impressionistShader, pe->impressionistResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->kuwaharaShader, pe->kuwaharaResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->inkWashShader, pe->inkWashResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->discoBallShader, pe->discoBallResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->legoBricksShader, pe->legoBricksResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->synthwaveShader, pe->synthwaveResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->constellationShader, pe->constellationResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->plasmaShader, pe->plasmaResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->interferenceShader, pe->interferenceResolutionLoc,
                 resolution, SHADER_UNIFORM_VEC2);
}

PostEffect *PostEffectInit(int screenWidth, int screenHeight) {
  PostEffect *pe = (PostEffect *)calloc(1, sizeof(PostEffect));
  if (pe == NULL) {
    return NULL;
  }

  pe->screenWidth = screenWidth;
  pe->screenHeight = screenHeight;
  pe->effects = EffectConfig{};

  if (!LoadPostEffectShaders(pe)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to load shaders");
    free(pe);
    return NULL;
  }

  GetShaderUniformLocations(pe);
  pe->synthwaveGridTime = 0.0f;
  pe->synthwaveStripeTime = 0.0f;
  pe->glitchTime = 0.0f;
  pe->glitchFrame = 0;
  pe->warpTime = 0.0f;

  SetResolutionUniforms(pe, screenWidth, screenHeight);

  RenderUtilsInitTextureHDR(&pe->accumTexture, screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[0], screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[1], screenWidth, screenHeight,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->outputTexture, screenWidth, screenHeight,
                            LOG_PREFIX);

  if (pe->accumTexture.id == 0 || pe->pingPong[0].id == 0 ||
      pe->pingPong[1].id == 0 || pe->outputTexture.id == 0) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
    UnloadShader(pe->feedbackShader);
    UnloadShader(pe->blurHShader);
    UnloadShader(pe->blurVShader);
    UnloadShader(pe->chromaticShader);
    UnloadShader(pe->fxaaShader);
    free(pe);
    return NULL;
  }

  pe->physarum = PhysarumInit(screenWidth, screenHeight, NULL);
  pe->curlFlow = CurlFlowInit(screenWidth, screenHeight, NULL);
  pe->curlAdvection = CurlAdvectionInit(screenWidth, screenHeight, NULL);
  pe->attractorFlow = AttractorFlowInit(screenWidth, screenHeight, NULL);
  pe->particleLife = ParticleLifeInit(screenWidth, screenHeight, NULL);
  pe->boids = BoidsInit(screenWidth, screenHeight, NULL);
  pe->cymatics = CymaticsInit(screenWidth, screenHeight, NULL);
  pe->blendCompositor = BlendCompositorInit();
  if (!VoronoiEffectInit(&pe->voronoi)) {
    free(pe);
    return NULL;
  }
  if (!LatticeFoldEffectInit(&pe->latticeFold)) {
    free(pe);
    return NULL;
  }
  if (!PhyllotaxisEffectInit(&pe->phyllotaxis)) {
    free(pe);
    return NULL;
  }
  if (!SineWarpEffectInit(&pe->sineWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize sine warp effect");
    free(pe);
    return NULL;
  }
  if (!TextureWarpEffectInit(&pe->textureWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init TextureWarp");
    free(pe);
    return NULL;
  }
  if (!WaveRippleEffectInit(&pe->waveRipple)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init WaveRipple");
    free(pe);
    return NULL;
  }
  if (!MobiusEffectInit(&pe->mobius)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init Mobius");
    free(pe);
    return NULL;
  }
  if (!GradientFlowEffectInit(&pe->gradientFlow)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init GradientFlow");
    free(pe);
    return NULL;
  }
  if (!ChladniWarpEffectInit(&pe->chladniWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init ChladniWarp");
    free(pe);
    return NULL;
  }
  if (!DomainWarpEffectInit(&pe->domainWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init DomainWarp");
    free(pe);
    return NULL;
  }
  if (!SurfaceWarpEffectInit(&pe->surfaceWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init SurfaceWarp");
    free(pe);
    return NULL;
  }
  if (!InterferenceWarpEffectInit(&pe->interferenceWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init InterferenceWarp");
    free(pe);
    return NULL;
  }
  if (!CorridorWarpEffectInit(&pe->corridorWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init CorridorWarp");
    free(pe);
    return NULL;
  }
  if (!FftRadialWarpEffectInit(&pe->fftRadialWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init FftRadialWarp");
    free(pe);
    return NULL;
  }
  if (!CircuitBoardEffectInit(&pe->circuitBoard)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init CircuitBoard");
    free(pe);
    return NULL;
  }
  if (!RadialPulseEffectInit(&pe->radialPulse)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init RadialPulse");
    free(pe);
    return NULL;
  }
  if (!KaleidoscopeEffectInit(&pe->kaleidoscope)) {
    free(pe);
    return NULL;
  }
  if (!KifsEffectInit(&pe->kifs)) {
    free(pe);
    return NULL;
  }
  if (!PoincareDiskEffectInit(&pe->poincareDisk)) {
    free(pe);
    return NULL;
  }
  if (!MandelboxEffectInit(&pe->mandelbox)) {
    free(pe);
    return NULL;
  }
  if (!TriangleFoldEffectInit(&pe->triangleFold)) {
    free(pe);
    return NULL;
  }
  if (!MoireInterferenceEffectInit(&pe->moireInterference)) {
    free(pe);
    return NULL;
  }
  if (!RadialIfsEffectInit(&pe->radialIfs)) {
    free(pe);
    return NULL;
  }
  if (!InfiniteZoomEffectInit(&pe->infiniteZoom)) {
    free(pe);
    return NULL;
  }
  if (!RadialStreakEffectInit(&pe->radialStreak)) {
    free(pe);
    return NULL;
  }
  if (!DrosteZoomEffectInit(&pe->drosteZoom)) {
    free(pe);
    return NULL;
  }
  if (!DensityWaveSpiralEffectInit(&pe->densityWaveSpiral)) {
    free(pe);
    return NULL;
  }
  if (!ShakeEffectInit(&pe->shake)) {
    free(pe);
    return NULL;
  }
  if (!RelativisticDopplerEffectInit(&pe->relativisticDoppler)) {
    free(pe);
    return NULL;
  }
  pe->falseColorLUT = ColorLUTInit(&pe->effects.falseColor.gradient);
  pe->constellationPointLUT =
      ColorLUTInit(&pe->effects.constellation.pointGradient);
  pe->constellationLineLUT =
      ColorLUTInit(&pe->effects.constellation.lineGradient);
  pe->constellationAnimPhase = 0.0f;
  pe->constellationRadialPhase = 0.0f;
  pe->plasmaGradientLUT = ColorLUTInit(&pe->effects.plasma.gradient);
  pe->plasmaAnimPhase = 0.0f;
  pe->plasmaDriftPhase = 0.0f;
  pe->plasmaFlickerTime = 0.0f;
  pe->interferenceColorLUT = ColorLUTInit(&pe->effects.interference.color);
  pe->interferenceTime = 0.0f;

  InitFFTTexture(&pe->fftTexture);
  pe->fftMaxMagnitude = 1.0f;
  TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)",
           pe->fftTexture.width, pe->fftTexture.height);

  InitWaveformTexture(&pe->waveformTexture);
  TraceLog(LOG_INFO, "POST_EFFECT: Waveform texture created (%dx%d)",
           pe->waveformTexture.width, pe->waveformTexture.height);

  InitBloomMips(pe, screenWidth, screenHeight);
  TraceLog(LOG_INFO, "POST_EFFECT: Bloom mips allocated (%dx%d to %dx%d)",
           pe->bloomMips[0].texture.width, pe->bloomMips[0].texture.height,
           pe->bloomMips[4].texture.width, pe->bloomMips[4].texture.height);

  RenderUtilsInitTextureHDR(&pe->halfResA, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  TraceLog(LOG_INFO, "POST_EFFECT: Half-res textures allocated (%dx%d)",
           pe->halfResA.texture.width, pe->halfResA.texture.height);

  // Generate 256x256 RGBA noise for oil paint brush randomization
  const Image noiseImg = GenImageColor(256, 256, BLANK);
  Color *pixels = (Color *)noiseImg.data;
  for (int i = 0; i < 256 * 256; i++) {
    pixels[i] =
        Color{(unsigned char)(rand() % 256), (unsigned char)(rand() % 256),
              (unsigned char)(rand() % 256), (unsigned char)(rand() % 256)};
  }
  pe->oilPaintNoiseTex = LoadTextureFromImage(noiseImg);
  UnloadImage(noiseImg);
  SetTextureFilter(pe->oilPaintNoiseTex, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(pe->oilPaintNoiseTex, TEXTURE_WRAP_REPEAT);

  RenderUtilsInitTextureHDR(&pe->oilPaintIntermediate, screenWidth,
                            screenHeight, LOG_PREFIX);

  return pe;
}

void PostEffectUninit(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  PhysarumUninit(pe->physarum);
  CurlFlowUninit(pe->curlFlow);
  CurlAdvectionUninit(pe->curlAdvection);
  AttractorFlowUninit(pe->attractorFlow);
  ParticleLifeUninit(pe->particleLife);
  BoidsUninit(pe->boids);
  CymaticsUninit(pe->cymatics);
  BlendCompositorUninit(pe->blendCompositor);
  ColorLUTUninit(pe->falseColorLUT);
  ColorLUTUninit(pe->constellationPointLUT);
  ColorLUTUninit(pe->constellationLineLUT);
  UnloadTexture(pe->fftTexture);
  UnloadTexture(pe->waveformTexture);
  UnloadRenderTexture(pe->accumTexture);
  UnloadRenderTexture(pe->pingPong[0]);
  UnloadRenderTexture(pe->pingPong[1]);
  UnloadRenderTexture(pe->outputTexture);
  UnloadShader(pe->feedbackShader);
  UnloadShader(pe->blurHShader);
  UnloadShader(pe->blurVShader);
  UnloadShader(pe->chromaticShader);
  VoronoiEffectUninit(&pe->voronoi);
  UnloadShader(pe->fxaaShader);
  UnloadShader(pe->clarityShader);
  UnloadShader(pe->gammaShader);
  UnloadShader(pe->shapeTextureShader);
  InfiniteZoomEffectUninit(&pe->infiniteZoom);
  SineWarpEffectUninit(&pe->sineWarp);
  TextureWarpEffectUninit(&pe->textureWarp);
  WaveRippleEffectUninit(&pe->waveRipple);
  MobiusEffectUninit(&pe->mobius);
  GradientFlowEffectUninit(&pe->gradientFlow);
  ChladniWarpEffectUninit(&pe->chladniWarp);
  DomainWarpEffectUninit(&pe->domainWarp);
  SurfaceWarpEffectUninit(&pe->surfaceWarp);
  InterferenceWarpEffectUninit(&pe->interferenceWarp);
  CorridorWarpEffectUninit(&pe->corridorWarp);
  FftRadialWarpEffectUninit(&pe->fftRadialWarp);
  CircuitBoardEffectUninit(&pe->circuitBoard);
  RadialPulseEffectUninit(&pe->radialPulse);
  KaleidoscopeEffectUninit(&pe->kaleidoscope);
  KifsEffectUninit(&pe->kifs);
  PoincareDiskEffectUninit(&pe->poincareDisk);
  MandelboxEffectUninit(&pe->mandelbox);
  TriangleFoldEffectUninit(&pe->triangleFold);
  MoireInterferenceEffectUninit(&pe->moireInterference);
  RadialIfsEffectUninit(&pe->radialIfs);
  RadialStreakEffectUninit(&pe->radialStreak);
  UnloadShader(pe->pixelationShader);
  UnloadShader(pe->glitchShader);
  UnloadShader(pe->toonShader);
  UnloadShader(pe->heightfieldReliefShader);
  DrosteZoomEffectUninit(&pe->drosteZoom);
  LatticeFoldEffectUninit(&pe->latticeFold);
  UnloadShader(pe->colorGradeShader);
  UnloadShader(pe->asciiArtShader);
  UnloadShader(pe->oilPaintShader);
  UnloadShader(pe->oilPaintStrokeShader);
  UnloadTexture(pe->oilPaintNoiseTex);
  UnloadRenderTexture(pe->oilPaintIntermediate);
  UnloadShader(pe->watercolorShader);
  UnloadShader(pe->neonGlowShader);
  UnloadShader(pe->falseColorShader);
  UnloadShader(pe->halftoneShader);
  UnloadShader(pe->crossHatchingShader);
  UnloadShader(pe->paletteQuantizationShader);
  UnloadShader(pe->bokehShader);
  UnloadShader(pe->bloomPrefilterShader);
  UnloadShader(pe->bloomDownsampleShader);
  UnloadShader(pe->bloomUpsampleShader);
  UnloadShader(pe->bloomCompositeShader);
  UnloadShader(pe->anamorphicStreakPrefilterShader);
  UnloadShader(pe->anamorphicStreakBlurShader);
  UnloadShader(pe->anamorphicStreakCompositeShader);
  PhyllotaxisEffectUninit(&pe->phyllotaxis);
  DensityWaveSpiralEffectUninit(&pe->densityWaveSpiral);
  UnloadShader(pe->pencilSketchShader);
  UnloadShader(pe->matrixRainShader);
  UnloadShader(pe->impressionistShader);
  UnloadShader(pe->kuwaharaShader);
  UnloadShader(pe->inkWashShader);
  UnloadShader(pe->discoBallShader);
  ShakeEffectUninit(&pe->shake);
  UnloadShader(pe->legoBricksShader);
  UnloadShader(pe->synthwaveShader);
  RelativisticDopplerEffectUninit(&pe->relativisticDoppler);
  UnloadShader(pe->constellationShader);
  UnloadShader(pe->plasmaShader);
  ColorLUTUninit(pe->plasmaGradientLUT);
  UnloadShader(pe->interferenceShader);
  ColorLUTUninit(pe->interferenceColorLUT);
  UnloadBloomMips(pe);
  UnloadRenderTexture(pe->halfResA);
  UnloadRenderTexture(pe->halfResB);
  free(pe);
}

void PostEffectResize(PostEffect *pe, int width, int height) {
  if (pe == NULL || (width == pe->screenWidth && height == pe->screenHeight)) {
    return;
  }

  pe->screenWidth = width;
  pe->screenHeight = height;

  UnloadRenderTexture(pe->accumTexture);
  UnloadRenderTexture(pe->pingPong[0]);
  UnloadRenderTexture(pe->pingPong[1]);
  UnloadRenderTexture(pe->outputTexture);
  RenderUtilsInitTextureHDR(&pe->accumTexture, width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[0], width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->pingPong[1], width, height, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->outputTexture, width, height, LOG_PREFIX);

  SetResolutionUniforms(pe, width, height);

  UnloadBloomMips(pe);
  InitBloomMips(pe, width, height);

  UnloadRenderTexture(pe->halfResA);
  UnloadRenderTexture(pe->halfResB);
  RenderUtilsInitTextureHDR(&pe->halfResA, width / 2, height / 2, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, width / 2, height / 2, LOG_PREFIX);

  UnloadRenderTexture(pe->oilPaintIntermediate);
  RenderUtilsInitTextureHDR(&pe->oilPaintIntermediate, width, height,
                            LOG_PREFIX);

  PhysarumResize(pe->physarum, width, height);
  CurlFlowResize(pe->curlFlow, width, height);
  CurlAdvectionResize(pe->curlAdvection, width, height);
  AttractorFlowResize(pe->attractorFlow, width, height);
  ParticleLifeResize(pe->particleLife, width, height);
  BoidsResize(pe->boids, width, height);
  CymaticsResize(pe->cymatics, width, height);
}

void PostEffectClearFeedback(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  // Clear accumulation and ping-pong buffers to black
  BeginTextureMode(pe->accumTexture);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(pe->pingPong[0]);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(pe->pingPong[1]);
  ClearBackground(BLACK);
  EndTextureMode();

  // Reset only enabled simulations to avoid expensive GPU uploads for disabled
  // effects
  if (pe->effects.physarum.enabled) {
    PhysarumReset(pe->physarum);
  }
  if (pe->effects.curlFlow.enabled) {
    CurlFlowReset(pe->curlFlow);
  }
  if (pe->effects.curlAdvection.enabled) {
    CurlAdvectionReset(pe->curlAdvection);
  }
  if (pe->effects.attractorFlow.enabled) {
    AttractorFlowReset(pe->attractorFlow);
  }
  if (pe->particleLife != NULL && pe->effects.particleLife.enabled) {
    ParticleLifeReset(pe->particleLife);
  }
  if (pe->boids != NULL && pe->effects.boids.enabled) {
    BoidsReset(pe->boids);
  }
  if (pe->cymatics != NULL && pe->effects.cymatics.enabled) {
    CymaticsReset(pe->cymatics);
  }

  TraceLog(LOG_INFO, "%s: Cleared feedback buffers and reset simulations",
           LOG_PREFIX);
}

void PostEffectBeginDrawStage(PostEffect *pe) {
  BeginTextureMode(pe->accumTexture);
}

void PostEffectEndDrawStage(void) { EndTextureMode(); }
