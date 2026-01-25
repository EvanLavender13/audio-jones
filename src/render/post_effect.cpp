#include "post_effect.h"
#include "blend_compositor.h"
#include "color_lut.h"
#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/curl_advection.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/cymatics.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "rlgl.h"
#include <stdlib.h>

static const char* LOG_PREFIX = "POST_EFFECT";

static const int WAVEFORM_TEXTURE_SIZE = 2048;

static void InitFFTTexture(Texture2D* tex)
{
    tex->id = rlLoadTexture(NULL, FFT_BIN_COUNT, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
    tex->width = FFT_BIN_COUNT;
    tex->height = 1;
    tex->mipmaps = 1;
    tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

    SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static void InitWaveformTexture(Texture2D* tex)
{
    tex->id = rlLoadTexture(NULL, WAVEFORM_TEXTURE_SIZE, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
    tex->width = WAVEFORM_TEXTURE_SIZE;
    tex->height = 1;
    tex->mipmaps = 1;
    tex->format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

    SetTextureFilter(*tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(*tex, TEXTURE_WRAP_CLAMP);
}

static void InitBloomMips(PostEffect* pe, int width, int height)
{
    int w = width / 2;
    int h = height / 2;
    for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
        RenderUtilsInitTextureHDR(&pe->bloomMips[i], w, h, LOG_PREFIX);
        w /= 2;
        h /= 2;
        if (w < 1) { w = 1; }
        if (h < 1) { h = 1; }
    }
}

static void UnloadBloomMips(PostEffect* pe)
{
    for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
        UnloadRenderTexture(pe->bloomMips[i]);
    }
}

static bool LoadPostEffectShaders(PostEffect* pe)
{
    pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
    pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
    pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
    pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
    pe->kaleidoShader = LoadShader(0, "shaders/kaleidoscope.fs");
    pe->voronoiShader = LoadShader(0, "shaders/voronoi.fs");
    pe->fxaaShader = LoadShader(0, "shaders/fxaa.fs");
    pe->clarityShader = LoadShader(0, "shaders/clarity.fs");
    pe->gammaShader = LoadShader(0, "shaders/gamma.fs");
    pe->shapeTextureShader = LoadShader(0, "shaders/shape_texture.fs");
    pe->infiniteZoomShader = LoadShader(0, "shaders/infinite_zoom.fs");
    pe->sineWarpShader = LoadShader(0, "shaders/sine_warp.fs");
    pe->radialStreakShader = LoadShader(0, "shaders/radial_streak.fs");
    pe->textureWarpShader = LoadShader(0, "shaders/texture_warp.fs");
    pe->waveRippleShader = LoadShader(0, "shaders/wave_ripple.fs");
    pe->mobiusShader = LoadShader(0, "shaders/mobius.fs");
    pe->pixelationShader = LoadShader(0, "shaders/pixelation.fs");
    pe->glitchShader = LoadShader(0, "shaders/glitch.fs");
    pe->poincareDiskShader = LoadShader(0, "shaders/poincare_disk.fs");
    pe->toonShader = LoadShader(0, "shaders/toon.fs");
    pe->heightfieldReliefShader = LoadShader(0, "shaders/heightfield_relief.fs");
    pe->gradientFlowShader = LoadShader(0, "shaders/gradient_flow.fs");
    pe->drosteZoomShader = LoadShader(0, "shaders/droste_zoom.fs");
    pe->kifsShader = LoadShader(0, "shaders/kifs.fs");
    pe->latticeFoldShader = LoadShader(0, "shaders/lattice_fold.fs");
    pe->colorGradeShader = LoadShader(0, "shaders/color_grade.fs");
    pe->asciiArtShader = LoadShader(0, "shaders/ascii_art.fs");
    pe->oilPaintShader = LoadShader(0, "shaders/oil_paint.fs");
    pe->oilPaintStrokeShader = LoadShader(0, "shaders/oil_paint_stroke.fs");
    pe->watercolorShader = LoadShader(0, "shaders/watercolor.fs");
    pe->neonGlowShader = LoadShader(0, "shaders/neon_glow.fs");
    pe->radialPulseShader = LoadShader(0, "shaders/radial_pulse.fs");
    pe->falseColorShader = LoadShader(0, "shaders/false_color.fs");
    pe->halftoneShader = LoadShader(0, "shaders/halftone.fs");
    pe->chladniWarpShader = LoadShader(0, "shaders/chladni_warp.fs");
    pe->crossHatchingShader = LoadShader(0, "shaders/cross_hatching.fs");
    pe->paletteQuantizationShader = LoadShader(0, "shaders/palette_quantization.fs");
    pe->bokehShader = LoadShader(0, "shaders/bokeh.fs");
    pe->bloomPrefilterShader = LoadShader(0, "shaders/bloom_prefilter.fs");
    pe->bloomDownsampleShader = LoadShader(0, "shaders/bloom_downsample.fs");
    pe->bloomUpsampleShader = LoadShader(0, "shaders/bloom_upsample.fs");
    pe->bloomCompositeShader = LoadShader(0, "shaders/bloom_composite.fs");
    pe->mandelboxShader = LoadShader(0, "shaders/mandelbox.fs");
    pe->triangleFoldShader = LoadShader(0, "shaders/triangle_fold.fs");
    pe->domainWarpShader = LoadShader(0, "shaders/domain_warp.fs");
    pe->phyllotaxisShader = LoadShader(0, "shaders/phyllotaxis.fs");
    pe->phyllotaxisWarpShader = LoadShader(0, "shaders/phyllotaxis_warp.fs");
    pe->densityWaveSpiralShader = LoadShader(0, "shaders/density_wave_spiral.fs");
    pe->moireInterferenceShader = LoadShader(0, "shaders/moire_interference.fs");
    pe->pencilSketchShader = LoadShader(0, "shaders/pencil_sketch.fs");
    pe->matrixRainShader = LoadShader(0, "shaders/matrix_rain.fs");
    pe->impressionistShader = LoadShader(0, "shaders/impressionist.fs");
    pe->kuwaharaShader = LoadShader(0, "shaders/kuwahara.fs");
    pe->inkWashShader = LoadShader(0, "shaders/ink_wash.fs");

    return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
           pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
           pe->kaleidoShader.id != 0 && pe->voronoiShader.id != 0 &&
           pe->fxaaShader.id != 0 &&
           pe->clarityShader.id != 0 && pe->gammaShader.id != 0 &&
           pe->shapeTextureShader.id != 0 && pe->infiniteZoomShader.id != 0 &&
           pe->sineWarpShader.id != 0 &&
           pe->radialStreakShader.id != 0 &&
           pe->textureWarpShader.id != 0 &&
           pe->waveRippleShader.id != 0 &&
           pe->mobiusShader.id != 0 &&
           pe->pixelationShader.id != 0 &&
           pe->glitchShader.id != 0 &&
           pe->poincareDiskShader.id != 0 &&
           pe->toonShader.id != 0 &&
           pe->heightfieldReliefShader.id != 0 &&
           pe->gradientFlowShader.id != 0 &&
           pe->drosteZoomShader.id != 0 &&
           pe->kifsShader.id != 0 &&
           pe->latticeFoldShader.id != 0 &&
           pe->colorGradeShader.id != 0 &&
           pe->asciiArtShader.id != 0 &&
           pe->oilPaintShader.id != 0 &&
           pe->oilPaintStrokeShader.id != 0 &&
           pe->watercolorShader.id != 0 &&
           pe->neonGlowShader.id != 0 &&
           pe->radialPulseShader.id != 0 &&
           pe->falseColorShader.id != 0 &&
           pe->halftoneShader.id != 0 &&
           pe->chladniWarpShader.id != 0 &&
           pe->crossHatchingShader.id != 0 &&
           pe->paletteQuantizationShader.id != 0 &&
           pe->bokehShader.id != 0 &&
           pe->bloomPrefilterShader.id != 0 &&
           pe->bloomDownsampleShader.id != 0 &&
           pe->bloomUpsampleShader.id != 0 &&
           pe->bloomCompositeShader.id != 0 &&
           pe->mandelboxShader.id != 0 &&
           pe->triangleFoldShader.id != 0 &&
           pe->domainWarpShader.id != 0 &&
           pe->phyllotaxisShader.id != 0 &&
           pe->phyllotaxisWarpShader.id != 0 &&
           pe->densityWaveSpiralShader.id != 0 &&
           pe->moireInterferenceShader.id != 0 &&
           pe->pencilSketchShader.id != 0 &&
           pe->matrixRainShader.id != 0 &&
           pe->impressionistShader.id != 0 &&
           pe->kuwaharaShader.id != 0 &&
           pe->inkWashShader.id != 0;
}

// NOLINTNEXTLINE(readability-function-size) - caches all shader uniform locations
static void GetShaderUniformLocations(PostEffect* pe)
{
    pe->blurHResolutionLoc = GetShaderLocation(pe->blurHShader, "resolution");
    pe->blurVResolutionLoc = GetShaderLocation(pe->blurVShader, "resolution");
    pe->blurHScaleLoc = GetShaderLocation(pe->blurHShader, "blurScale");
    pe->blurVScaleLoc = GetShaderLocation(pe->blurVShader, "blurScale");
    pe->halfLifeLoc = GetShaderLocation(pe->blurVShader, "halfLife");
    pe->deltaTimeLoc = GetShaderLocation(pe->blurVShader, "deltaTime");
    pe->chromaticResolutionLoc = GetShaderLocation(pe->chromaticShader, "resolution");
    pe->chromaticOffsetLoc = GetShaderLocation(pe->chromaticShader, "chromaticOffset");
    pe->kaleidoSegmentsLoc = GetShaderLocation(pe->kaleidoShader, "segments");
    pe->kaleidoRotationLoc = GetShaderLocation(pe->kaleidoShader, "rotation");
    pe->kaleidoTwistLoc = GetShaderLocation(pe->kaleidoShader, "twistAngle");
    pe->kaleidoSmoothingLoc = GetShaderLocation(pe->kaleidoShader, "smoothing");
    pe->kifsRotationLoc = GetShaderLocation(pe->kifsShader, "rotation");
    pe->kifsTwistLoc = GetShaderLocation(pe->kifsShader, "twistAngle");
    pe->kifsIterationsLoc = GetShaderLocation(pe->kifsShader, "iterations");
    pe->kifsScaleLoc = GetShaderLocation(pe->kifsShader, "scale");
    pe->kifsOffsetLoc = GetShaderLocation(pe->kifsShader, "kifsOffset");
    pe->kifsOctantFoldLoc = GetShaderLocation(pe->kifsShader, "octantFold");
    pe->kifsPolarFoldLoc = GetShaderLocation(pe->kifsShader, "polarFold");
    pe->kifsPolarFoldSegmentsLoc = GetShaderLocation(pe->kifsShader, "polarFoldSegments");
    pe->kifsPolarFoldSmoothingLoc = GetShaderLocation(pe->kifsShader, "polarFoldSmoothing");
    pe->latticeFoldCellTypeLoc = GetShaderLocation(pe->latticeFoldShader, "cellType");
    pe->latticeFoldCellScaleLoc = GetShaderLocation(pe->latticeFoldShader, "cellScale");
    pe->latticeFoldRotationLoc = GetShaderLocation(pe->latticeFoldShader, "rotation");
    pe->latticeFoldTimeLoc = GetShaderLocation(pe->latticeFoldShader, "time");
    pe->latticeFoldSmoothingLoc = GetShaderLocation(pe->latticeFoldShader, "smoothing");
    pe->voronoiResolutionLoc = GetShaderLocation(pe->voronoiShader, "resolution");
    pe->voronoiScaleLoc = GetShaderLocation(pe->voronoiShader, "scale");
    pe->voronoiTimeLoc = GetShaderLocation(pe->voronoiShader, "time");
    pe->voronoiEdgeFalloffLoc = GetShaderLocation(pe->voronoiShader, "edgeFalloff");
    pe->voronoiIsoFrequencyLoc = GetShaderLocation(pe->voronoiShader, "isoFrequency");
    pe->voronoiSmoothModeLoc = GetShaderLocation(pe->voronoiShader, "smoothMode");
    pe->voronoiUvDistortIntensityLoc = GetShaderLocation(pe->voronoiShader, "uvDistortIntensity");
    pe->voronoiEdgeIsoIntensityLoc = GetShaderLocation(pe->voronoiShader, "edgeIsoIntensity");
    pe->voronoiCenterIsoIntensityLoc = GetShaderLocation(pe->voronoiShader, "centerIsoIntensity");
    pe->voronoiFlatFillIntensityLoc = GetShaderLocation(pe->voronoiShader, "flatFillIntensity");
    pe->voronoiOrganicFlowIntensityLoc = GetShaderLocation(pe->voronoiShader, "organicFlowIntensity");
    pe->voronoiEdgeGlowIntensityLoc = GetShaderLocation(pe->voronoiShader, "edgeGlowIntensity");
    pe->voronoiDeterminantIntensityLoc = GetShaderLocation(pe->voronoiShader, "determinantIntensity");
    pe->voronoiRatioIntensityLoc = GetShaderLocation(pe->voronoiShader, "ratioIntensity");
    pe->voronoiEdgeDetectIntensityLoc = GetShaderLocation(pe->voronoiShader, "edgeDetectIntensity");
    pe->feedbackResolutionLoc = GetShaderLocation(pe->feedbackShader, "resolution");
    pe->feedbackDesaturateLoc = GetShaderLocation(pe->feedbackShader, "desaturate");
    pe->feedbackZoomBaseLoc = GetShaderLocation(pe->feedbackShader, "zoomBase");
    pe->feedbackZoomRadialLoc = GetShaderLocation(pe->feedbackShader, "zoomRadial");
    pe->feedbackRotBaseLoc = GetShaderLocation(pe->feedbackShader, "rotBase");
    pe->feedbackRotRadialLoc = GetShaderLocation(pe->feedbackShader, "rotRadial");
    pe->feedbackDxBaseLoc = GetShaderLocation(pe->feedbackShader, "dxBase");
    pe->feedbackDxRadialLoc = GetShaderLocation(pe->feedbackShader, "dxRadial");
    pe->feedbackDyBaseLoc = GetShaderLocation(pe->feedbackShader, "dyBase");
    pe->feedbackDyRadialLoc = GetShaderLocation(pe->feedbackShader, "dyRadial");
    pe->feedbackFlowStrengthLoc = GetShaderLocation(pe->feedbackShader, "feedbackFlowStrength");
    pe->feedbackFlowAngleLoc = GetShaderLocation(pe->feedbackShader, "feedbackFlowAngle");
    pe->feedbackFlowScaleLoc = GetShaderLocation(pe->feedbackShader, "feedbackFlowScale");
    pe->feedbackFlowThresholdLoc = GetShaderLocation(pe->feedbackShader, "feedbackFlowThreshold");
    pe->feedbackCxLoc = GetShaderLocation(pe->feedbackShader, "cx");
    pe->feedbackCyLoc = GetShaderLocation(pe->feedbackShader, "cy");
    pe->feedbackSxLoc = GetShaderLocation(pe->feedbackShader, "sx");
    pe->feedbackSyLoc = GetShaderLocation(pe->feedbackShader, "sy");
    pe->feedbackZoomAngularLoc = GetShaderLocation(pe->feedbackShader, "zoomAngular");
    pe->feedbackZoomAngularFreqLoc = GetShaderLocation(pe->feedbackShader, "zoomAngularFreq");
    pe->feedbackRotAngularLoc = GetShaderLocation(pe->feedbackShader, "rotAngular");
    pe->feedbackRotAngularFreqLoc = GetShaderLocation(pe->feedbackShader, "rotAngularFreq");
    pe->feedbackDxAngularLoc = GetShaderLocation(pe->feedbackShader, "dxAngular");
    pe->feedbackDxAngularFreqLoc = GetShaderLocation(pe->feedbackShader, "dxAngularFreq");
    pe->feedbackDyAngularLoc = GetShaderLocation(pe->feedbackShader, "dyAngular");
    pe->feedbackDyAngularFreqLoc = GetShaderLocation(pe->feedbackShader, "dyAngularFreq");
    pe->feedbackWarpLoc = GetShaderLocation(pe->feedbackShader, "warp");
    pe->feedbackWarpTimeLoc = GetShaderLocation(pe->feedbackShader, "warpTime");
    pe->feedbackWarpScaleInverseLoc = GetShaderLocation(pe->feedbackShader, "warpScaleInverse");
    pe->fxaaResolutionLoc = GetShaderLocation(pe->fxaaShader, "resolution");
    pe->clarityResolutionLoc = GetShaderLocation(pe->clarityShader, "resolution");
    pe->clarityAmountLoc = GetShaderLocation(pe->clarityShader, "clarity");
    pe->gammaGammaLoc = GetShaderLocation(pe->gammaShader, "gamma");
    pe->shapeTexZoomLoc = GetShaderLocation(pe->shapeTextureShader, "texZoom");
    pe->shapeTexAngleLoc = GetShaderLocation(pe->shapeTextureShader, "texAngle");
    pe->shapeTexBrightnessLoc = GetShaderLocation(pe->shapeTextureShader, "texBrightness");
    pe->infiniteZoomTimeLoc = GetShaderLocation(pe->infiniteZoomShader, "time");
    pe->infiniteZoomZoomDepthLoc = GetShaderLocation(pe->infiniteZoomShader, "zoomDepth");
    pe->infiniteZoomLayersLoc = GetShaderLocation(pe->infiniteZoomShader, "layers");
    pe->infiniteZoomSpiralAngleLoc = GetShaderLocation(pe->infiniteZoomShader, "spiralAngle");
    pe->infiniteZoomSpiralTwistLoc = GetShaderLocation(pe->infiniteZoomShader, "spiralTwist");
    pe->sineWarpTimeLoc = GetShaderLocation(pe->sineWarpShader, "time");
    pe->sineWarpRotationLoc = GetShaderLocation(pe->sineWarpShader, "rotation");
    pe->sineWarpOctavesLoc = GetShaderLocation(pe->sineWarpShader, "octaves");
    pe->sineWarpStrengthLoc = GetShaderLocation(pe->sineWarpShader, "strength");
    pe->sineWarpOctaveRotationLoc = GetShaderLocation(pe->sineWarpShader, "octaveRotation");
    pe->radialStreakSamplesLoc = GetShaderLocation(pe->radialStreakShader, "samples");
    pe->radialStreakStreakLengthLoc = GetShaderLocation(pe->radialStreakShader, "streakLength");
    pe->textureWarpStrengthLoc = GetShaderLocation(pe->textureWarpShader, "strength");
    pe->textureWarpIterationsLoc = GetShaderLocation(pe->textureWarpShader, "iterations");
    pe->textureWarpChannelModeLoc = GetShaderLocation(pe->textureWarpShader, "channelMode");
    pe->textureWarpRidgeAngleLoc = GetShaderLocation(pe->textureWarpShader, "ridgeAngle");
    pe->textureWarpAnisotropyLoc = GetShaderLocation(pe->textureWarpShader, "anisotropy");
    pe->textureWarpNoiseAmountLoc = GetShaderLocation(pe->textureWarpShader, "noiseAmount");
    pe->textureWarpNoiseScaleLoc = GetShaderLocation(pe->textureWarpShader, "noiseScale");
    pe->waveRippleTimeLoc = GetShaderLocation(pe->waveRippleShader, "time");
    pe->waveRippleOctavesLoc = GetShaderLocation(pe->waveRippleShader, "octaves");
    pe->waveRippleStrengthLoc = GetShaderLocation(pe->waveRippleShader, "strength");
    pe->waveRippleFrequencyLoc = GetShaderLocation(pe->waveRippleShader, "frequency");
    pe->waveRippleSteepnessLoc = GetShaderLocation(pe->waveRippleShader, "steepness");
    pe->waveRippleDecayLoc = GetShaderLocation(pe->waveRippleShader, "decay");
    pe->waveRippleCenterHoleLoc = GetShaderLocation(pe->waveRippleShader, "centerHole");
    pe->waveRippleOriginLoc = GetShaderLocation(pe->waveRippleShader, "origin");
    pe->waveRippleShadeEnabledLoc = GetShaderLocation(pe->waveRippleShader, "shadeEnabled");
    pe->waveRippleShadeIntensityLoc = GetShaderLocation(pe->waveRippleShader, "shadeIntensity");
    pe->mobiusTimeLoc = GetShaderLocation(pe->mobiusShader, "time");
    pe->mobiusPoint1Loc = GetShaderLocation(pe->mobiusShader, "point1");
    pe->mobiusPoint2Loc = GetShaderLocation(pe->mobiusShader, "point2");
    pe->mobiusSpiralTightnessLoc = GetShaderLocation(pe->mobiusShader, "spiralTightness");
    pe->mobiusZoomFactorLoc = GetShaderLocation(pe->mobiusShader, "zoomFactor");
    pe->pixelationResolutionLoc = GetShaderLocation(pe->pixelationShader, "resolution");
    pe->pixelationCellCountLoc = GetShaderLocation(pe->pixelationShader, "cellCount");
    pe->pixelationDitherScaleLoc = GetShaderLocation(pe->pixelationShader, "ditherScale");
    pe->pixelationPosterizeLevelsLoc = GetShaderLocation(pe->pixelationShader, "posterizeLevels");
    pe->glitchResolutionLoc = GetShaderLocation(pe->glitchShader, "resolution");
    pe->glitchTimeLoc = GetShaderLocation(pe->glitchShader, "time");
    pe->glitchFrameLoc = GetShaderLocation(pe->glitchShader, "frame");
    pe->glitchCrtEnabledLoc = GetShaderLocation(pe->glitchShader, "crtEnabled");
    pe->glitchCurvatureLoc = GetShaderLocation(pe->glitchShader, "curvature");
    pe->glitchVignetteEnabledLoc = GetShaderLocation(pe->glitchShader, "vignetteEnabled");
    pe->glitchAnalogIntensityLoc = GetShaderLocation(pe->glitchShader, "analogIntensity");
    pe->glitchAberrationLoc = GetShaderLocation(pe->glitchShader, "aberration");
    pe->glitchBlockThresholdLoc = GetShaderLocation(pe->glitchShader, "blockThreshold");
    pe->glitchBlockOffsetLoc = GetShaderLocation(pe->glitchShader, "blockOffset");
    pe->glitchVhsEnabledLoc = GetShaderLocation(pe->glitchShader, "vhsEnabled");
    pe->glitchTrackingBarIntensityLoc = GetShaderLocation(pe->glitchShader, "trackingBarIntensity");
    pe->glitchScanlineNoiseIntensityLoc = GetShaderLocation(pe->glitchShader, "scanlineNoiseIntensity");
    pe->glitchColorDriftIntensityLoc = GetShaderLocation(pe->glitchShader, "colorDriftIntensity");
    pe->glitchScanlineAmountLoc = GetShaderLocation(pe->glitchShader, "scanlineAmount");
    pe->glitchNoiseAmountLoc = GetShaderLocation(pe->glitchShader, "noiseAmount");
    pe->poincareDiskTilePLoc = GetShaderLocation(pe->poincareDiskShader, "tileP");
    pe->poincareDiskTileQLoc = GetShaderLocation(pe->poincareDiskShader, "tileQ");
    pe->poincareDiskTileRLoc = GetShaderLocation(pe->poincareDiskShader, "tileR");
    pe->poincareDiskTranslationLoc = GetShaderLocation(pe->poincareDiskShader, "translation");
    pe->poincareDiskRotationLoc = GetShaderLocation(pe->poincareDiskShader, "rotation");
    pe->poincareDiskDiskScaleLoc = GetShaderLocation(pe->poincareDiskShader, "diskScale");
    pe->toonResolutionLoc = GetShaderLocation(pe->toonShader, "resolution");
    pe->toonLevelsLoc = GetShaderLocation(pe->toonShader, "levels");
    pe->toonEdgeThresholdLoc = GetShaderLocation(pe->toonShader, "edgeThreshold");
    pe->toonEdgeSoftnessLoc = GetShaderLocation(pe->toonShader, "edgeSoftness");
    pe->toonThicknessVariationLoc = GetShaderLocation(pe->toonShader, "thicknessVariation");
    pe->toonNoiseScaleLoc = GetShaderLocation(pe->toonShader, "noiseScale");
    pe->heightfieldReliefResolutionLoc = GetShaderLocation(pe->heightfieldReliefShader, "resolution");
    pe->heightfieldReliefIntensityLoc = GetShaderLocation(pe->heightfieldReliefShader, "intensity");
    pe->heightfieldReliefReliefScaleLoc = GetShaderLocation(pe->heightfieldReliefShader, "reliefScale");
    pe->heightfieldReliefLightAngleLoc = GetShaderLocation(pe->heightfieldReliefShader, "lightAngle");
    pe->heightfieldReliefLightHeightLoc = GetShaderLocation(pe->heightfieldReliefShader, "lightHeight");
    pe->heightfieldReliefShininessLoc = GetShaderLocation(pe->heightfieldReliefShader, "shininess");
    pe->gradientFlowResolutionLoc = GetShaderLocation(pe->gradientFlowShader, "resolution");
    pe->gradientFlowStrengthLoc = GetShaderLocation(pe->gradientFlowShader, "strength");
    pe->gradientFlowIterationsLoc = GetShaderLocation(pe->gradientFlowShader, "iterations");
    pe->gradientFlowEdgeWeightLoc = GetShaderLocation(pe->gradientFlowShader, "edgeWeight");
    pe->gradientFlowRandomDirectionLoc = GetShaderLocation(pe->gradientFlowShader, "randomDirection");
    pe->drosteZoomTimeLoc = GetShaderLocation(pe->drosteZoomShader, "time");
    pe->drosteZoomScaleLoc = GetShaderLocation(pe->drosteZoomShader, "scale");
    pe->drosteZoomSpiralAngleLoc = GetShaderLocation(pe->drosteZoomShader, "spiralAngle");
    pe->drosteZoomShearCoeffLoc = GetShaderLocation(pe->drosteZoomShader, "shearCoeff");
    pe->drosteZoomInnerRadiusLoc = GetShaderLocation(pe->drosteZoomShader, "innerRadius");
    pe->drosteZoomBranchesLoc = GetShaderLocation(pe->drosteZoomShader, "branches");
    pe->colorGradeHueShiftLoc = GetShaderLocation(pe->colorGradeShader, "hueShift");
    pe->colorGradeSaturationLoc = GetShaderLocation(pe->colorGradeShader, "saturation");
    pe->colorGradeBrightnessLoc = GetShaderLocation(pe->colorGradeShader, "brightness");
    pe->colorGradeContrastLoc = GetShaderLocation(pe->colorGradeShader, "contrast");
    pe->colorGradeTemperatureLoc = GetShaderLocation(pe->colorGradeShader, "temperature");
    pe->colorGradeShadowsOffsetLoc = GetShaderLocation(pe->colorGradeShader, "shadowsOffset");
    pe->colorGradeMidtonesOffsetLoc = GetShaderLocation(pe->colorGradeShader, "midtonesOffset");
    pe->colorGradeHighlightsOffsetLoc = GetShaderLocation(pe->colorGradeShader, "highlightsOffset");
    pe->asciiArtResolutionLoc = GetShaderLocation(pe->asciiArtShader, "resolution");
    pe->asciiArtCellPixelsLoc = GetShaderLocation(pe->asciiArtShader, "cellPixels");
    pe->asciiArtColorModeLoc = GetShaderLocation(pe->asciiArtShader, "colorMode");
    pe->asciiArtForegroundLoc = GetShaderLocation(pe->asciiArtShader, "foreground");
    pe->asciiArtBackgroundLoc = GetShaderLocation(pe->asciiArtShader, "background");
    pe->asciiArtInvertLoc = GetShaderLocation(pe->asciiArtShader, "invert");
    pe->oilPaintStrokeResolutionLoc = GetShaderLocation(pe->oilPaintStrokeShader, "resolution");
    pe->oilPaintBrushSizeLoc = GetShaderLocation(pe->oilPaintStrokeShader, "brushSize");
    pe->oilPaintStrokeBendLoc = GetShaderLocation(pe->oilPaintStrokeShader, "strokeBend");
    pe->oilPaintLayersLoc = GetShaderLocation(pe->oilPaintStrokeShader, "layers");
    pe->oilPaintNoiseTexLoc = GetShaderLocation(pe->oilPaintStrokeShader, "texture1");
    pe->oilPaintResolutionLoc = GetShaderLocation(pe->oilPaintShader, "resolution");
    pe->oilPaintSpecularLoc = GetShaderLocation(pe->oilPaintShader, "specular");
    pe->watercolorResolutionLoc = GetShaderLocation(pe->watercolorShader, "resolution");
    pe->watercolorSamplesLoc = GetShaderLocation(pe->watercolorShader, "samples");
    pe->watercolorStrokeStepLoc = GetShaderLocation(pe->watercolorShader, "strokeStep");
    pe->watercolorWashStrengthLoc = GetShaderLocation(pe->watercolorShader, "washStrength");
    pe->watercolorPaperScaleLoc = GetShaderLocation(pe->watercolorShader, "paperScale");
    pe->watercolorPaperStrengthLoc = GetShaderLocation(pe->watercolorShader, "paperStrength");
    pe->neonGlowResolutionLoc = GetShaderLocation(pe->neonGlowShader, "resolution");
    pe->neonGlowGlowColorLoc = GetShaderLocation(pe->neonGlowShader, "glowColor");
    pe->neonGlowEdgeThresholdLoc = GetShaderLocation(pe->neonGlowShader, "edgeThreshold");
    pe->neonGlowEdgePowerLoc = GetShaderLocation(pe->neonGlowShader, "edgePower");
    pe->neonGlowGlowIntensityLoc = GetShaderLocation(pe->neonGlowShader, "glowIntensity");
    pe->neonGlowGlowRadiusLoc = GetShaderLocation(pe->neonGlowShader, "glowRadius");
    pe->neonGlowGlowSamplesLoc = GetShaderLocation(pe->neonGlowShader, "glowSamples");
    pe->neonGlowOriginalVisibilityLoc = GetShaderLocation(pe->neonGlowShader, "originalVisibility");
    pe->radialPulseRadialFreqLoc = GetShaderLocation(pe->radialPulseShader, "radialFreq");
    pe->radialPulseRadialAmpLoc = GetShaderLocation(pe->radialPulseShader, "radialAmp");
    pe->radialPulseSegmentsLoc = GetShaderLocation(pe->radialPulseShader, "segments");
    pe->radialPulseAngularAmpLoc = GetShaderLocation(pe->radialPulseShader, "angularAmp");
    pe->radialPulsePetalAmpLoc = GetShaderLocation(pe->radialPulseShader, "petalAmp");
    pe->radialPulsePhaseLoc = GetShaderLocation(pe->radialPulseShader, "phase");
    pe->radialPulseSpiralTwistLoc = GetShaderLocation(pe->radialPulseShader, "spiralTwist");
    pe->falseColorIntensityLoc = GetShaderLocation(pe->falseColorShader, "intensity");
    pe->falseColorGradientLUTLoc = GetShaderLocation(pe->falseColorShader, "texture1");
    pe->halftoneResolutionLoc = GetShaderLocation(pe->halftoneShader, "resolution");
    pe->halftoneDotScaleLoc = GetShaderLocation(pe->halftoneShader, "dotScale");
    pe->halftoneDotSizeLoc = GetShaderLocation(pe->halftoneShader, "dotSize");
    pe->halftoneRotationLoc = GetShaderLocation(pe->halftoneShader, "rotation");
    pe->halftoneThresholdLoc = GetShaderLocation(pe->halftoneShader, "threshold");
    pe->halftoneSoftnessLoc = GetShaderLocation(pe->halftoneShader, "softness");
    pe->chladniWarpNLoc = GetShaderLocation(pe->chladniWarpShader, "n");
    pe->chladniWarpMLoc = GetShaderLocation(pe->chladniWarpShader, "m");
    pe->chladniWarpPlateSizeLoc = GetShaderLocation(pe->chladniWarpShader, "plateSize");
    pe->chladniWarpStrengthLoc = GetShaderLocation(pe->chladniWarpShader, "strength");
    pe->chladniWarpModeLoc = GetShaderLocation(pe->chladniWarpShader, "warpMode");
    pe->chladniWarpAnimPhaseLoc = GetShaderLocation(pe->chladniWarpShader, "animPhase");
    pe->chladniWarpAnimRangeLoc = GetShaderLocation(pe->chladniWarpShader, "animRange");
    pe->chladniWarpPreFoldLoc = GetShaderLocation(pe->chladniWarpShader, "preFold");
    pe->crossHatchingResolutionLoc = GetShaderLocation(pe->crossHatchingShader, "resolution");
    pe->crossHatchingTimeLoc = GetShaderLocation(pe->crossHatchingShader, "time");
    pe->crossHatchingWidthLoc = GetShaderLocation(pe->crossHatchingShader, "width");
    pe->crossHatchingThresholdLoc = GetShaderLocation(pe->crossHatchingShader, "threshold");
    pe->crossHatchingNoiseLoc = GetShaderLocation(pe->crossHatchingShader, "noise");
    pe->crossHatchingOutlineLoc = GetShaderLocation(pe->crossHatchingShader, "outline");
    pe->paletteQuantizationColorLevelsLoc = GetShaderLocation(pe->paletteQuantizationShader, "colorLevels");
    pe->paletteQuantizationDitherStrengthLoc = GetShaderLocation(pe->paletteQuantizationShader, "ditherStrength");
    pe->paletteQuantizationBayerSizeLoc = GetShaderLocation(pe->paletteQuantizationShader, "bayerSize");
    pe->bokehResolutionLoc = GetShaderLocation(pe->bokehShader, "resolution");
    pe->bokehRadiusLoc = GetShaderLocation(pe->bokehShader, "radius");
    pe->bokehIterationsLoc = GetShaderLocation(pe->bokehShader, "iterations");
    pe->bokehBrightnessPowerLoc = GetShaderLocation(pe->bokehShader, "brightnessPower");
    pe->bloomThresholdLoc = GetShaderLocation(pe->bloomPrefilterShader, "threshold");
    pe->bloomKneeLoc = GetShaderLocation(pe->bloomPrefilterShader, "knee");
    pe->bloomDownsampleHalfpixelLoc = GetShaderLocation(pe->bloomDownsampleShader, "halfpixel");
    pe->bloomUpsampleHalfpixelLoc = GetShaderLocation(pe->bloomUpsampleShader, "halfpixel");
    pe->bloomIntensityLoc = GetShaderLocation(pe->bloomCompositeShader, "intensity");
    pe->bloomBloomTexLoc = GetShaderLocation(pe->bloomCompositeShader, "bloomTexture");
    pe->mandelboxIterationsLoc = GetShaderLocation(pe->mandelboxShader, "iterations");
    pe->mandelboxBoxLimitLoc = GetShaderLocation(pe->mandelboxShader, "boxLimit");
    pe->mandelboxSphereMinLoc = GetShaderLocation(pe->mandelboxShader, "sphereMin");
    pe->mandelboxSphereMaxLoc = GetShaderLocation(pe->mandelboxShader, "sphereMax");
    pe->mandelboxScaleLoc = GetShaderLocation(pe->mandelboxShader, "scale");
    pe->mandelboxOffsetLoc = GetShaderLocation(pe->mandelboxShader, "mandelboxOffset");
    pe->mandelboxRotationLoc = GetShaderLocation(pe->mandelboxShader, "rotation");
    pe->mandelboxTwistAngleLoc = GetShaderLocation(pe->mandelboxShader, "twistAngle");
    pe->mandelboxBoxIntensityLoc = GetShaderLocation(pe->mandelboxShader, "boxIntensity");
    pe->mandelboxSphereIntensityLoc = GetShaderLocation(pe->mandelboxShader, "sphereIntensity");
    pe->mandelboxPolarFoldLoc = GetShaderLocation(pe->mandelboxShader, "polarFold");
    pe->mandelboxPolarFoldSegmentsLoc = GetShaderLocation(pe->mandelboxShader, "polarFoldSegments");
    pe->triangleFoldIterationsLoc = GetShaderLocation(pe->triangleFoldShader, "iterations");
    pe->triangleFoldScaleLoc = GetShaderLocation(pe->triangleFoldShader, "scale");
    pe->triangleFoldOffsetLoc = GetShaderLocation(pe->triangleFoldShader, "triangleOffset");
    pe->triangleFoldRotationLoc = GetShaderLocation(pe->triangleFoldShader, "rotation");
    pe->triangleFoldTwistAngleLoc = GetShaderLocation(pe->triangleFoldShader, "twistAngle");
    pe->domainWarpWarpStrengthLoc = GetShaderLocation(pe->domainWarpShader, "warpStrength");
    pe->domainWarpWarpScaleLoc = GetShaderLocation(pe->domainWarpShader, "warpScale");
    pe->domainWarpWarpIterationsLoc = GetShaderLocation(pe->domainWarpShader, "warpIterations");
    pe->domainWarpFalloffLoc = GetShaderLocation(pe->domainWarpShader, "falloff");
    pe->domainWarpTimeOffsetLoc = GetShaderLocation(pe->domainWarpShader, "timeOffset");
    pe->phyllotaxisResolutionLoc = GetShaderLocation(pe->phyllotaxisShader, "resolution");
    pe->phyllotaxisScaleLoc = GetShaderLocation(pe->phyllotaxisShader, "scale");
    pe->phyllotaxisDivergenceAngleLoc = GetShaderLocation(pe->phyllotaxisShader, "divergenceAngle");
    pe->phyllotaxisPhaseTimeLoc = GetShaderLocation(pe->phyllotaxisShader, "phaseTime");
    pe->phyllotaxisCellRadiusLoc = GetShaderLocation(pe->phyllotaxisShader, "cellRadius");
    pe->phyllotaxisIsoFrequencyLoc = GetShaderLocation(pe->phyllotaxisShader, "isoFrequency");
    pe->phyllotaxisUvDistortIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "uvDistortIntensity");
    pe->phyllotaxisOrganicFlowIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "organicFlowIntensity");
    pe->phyllotaxisEdgeIsoIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "edgeIsoIntensity");
    pe->phyllotaxisCenterIsoIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "centerIsoIntensity");
    pe->phyllotaxisFlatFillIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "flatFillIntensity");
    pe->phyllotaxisEdgeGlowIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "edgeGlowIntensity");
    pe->phyllotaxisRatioIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "ratioIntensity");
    pe->phyllotaxisDeterminantIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "determinantIntensity");
    pe->phyllotaxisEdgeDetectIntensityLoc = GetShaderLocation(pe->phyllotaxisShader, "edgeDetectIntensity");
    pe->phyllotaxisWarpScaleLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "scale");
    pe->phyllotaxisWarpDivergenceAngleLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "divergenceAngle");
    pe->phyllotaxisWarpWarpStrengthLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "warpStrength");
    pe->phyllotaxisWarpWarpFalloffLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "warpFalloff");
    pe->phyllotaxisWarpTangentIntensityLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "tangentIntensity");
    pe->phyllotaxisWarpRadialIntensityLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "radialIntensity");
    pe->phyllotaxisWarpSpinOffsetLoc = GetShaderLocation(pe->phyllotaxisWarpShader, "spinOffset");
    pe->densityWaveSpiralCenterLoc = GetShaderLocation(pe->densityWaveSpiralShader, "center");
    pe->densityWaveSpiralAspectLoc = GetShaderLocation(pe->densityWaveSpiralShader, "aspect");
    pe->densityWaveSpiralTightnessLoc = GetShaderLocation(pe->densityWaveSpiralShader, "tightness");
    pe->densityWaveSpiralRotationAccumLoc = GetShaderLocation(pe->densityWaveSpiralShader, "rotationAccum");
    pe->densityWaveSpiralGlobalRotationAccumLoc = GetShaderLocation(pe->densityWaveSpiralShader, "globalRotationAccum");
    pe->densityWaveSpiralThicknessLoc = GetShaderLocation(pe->densityWaveSpiralShader, "thickness");
    pe->densityWaveSpiralRingCountLoc = GetShaderLocation(pe->densityWaveSpiralShader, "ringCount");
    pe->densityWaveSpiralFalloffLoc = GetShaderLocation(pe->densityWaveSpiralShader, "falloff");
    pe->moireInterferenceRotationAngleLoc = GetShaderLocation(pe->moireInterferenceShader, "rotationAngle");
    pe->moireInterferenceScaleDiffLoc = GetShaderLocation(pe->moireInterferenceShader, "scaleDiff");
    pe->moireInterferenceLayersLoc = GetShaderLocation(pe->moireInterferenceShader, "layers");
    pe->moireInterferenceBlendModeLoc = GetShaderLocation(pe->moireInterferenceShader, "blendMode");
    pe->moireInterferenceCenterXLoc = GetShaderLocation(pe->moireInterferenceShader, "centerX");
    pe->moireInterferenceCenterYLoc = GetShaderLocation(pe->moireInterferenceShader, "centerY");
    pe->moireInterferenceRotationAccumLoc = GetShaderLocation(pe->moireInterferenceShader, "rotationAccum");
    pe->pencilSketchResolutionLoc = GetShaderLocation(pe->pencilSketchShader, "resolution");
    pe->pencilSketchAngleCountLoc = GetShaderLocation(pe->pencilSketchShader, "angleCount");
    pe->pencilSketchSampleCountLoc = GetShaderLocation(pe->pencilSketchShader, "sampleCount");
    pe->pencilSketchStrokeFalloffLoc = GetShaderLocation(pe->pencilSketchShader, "strokeFalloff");
    pe->pencilSketchGradientEpsLoc = GetShaderLocation(pe->pencilSketchShader, "gradientEps");
    pe->pencilSketchPaperStrengthLoc = GetShaderLocation(pe->pencilSketchShader, "paperStrength");
    pe->pencilSketchVignetteStrengthLoc = GetShaderLocation(pe->pencilSketchShader, "vignetteStrength");
    pe->pencilSketchWobbleTimeLoc = GetShaderLocation(pe->pencilSketchShader, "wobbleTime");
    pe->pencilSketchWobbleAmountLoc = GetShaderLocation(pe->pencilSketchShader, "wobbleAmount");
    pe->matrixRainResolutionLoc = GetShaderLocation(pe->matrixRainShader, "resolution");
    pe->matrixRainCellSizeLoc = GetShaderLocation(pe->matrixRainShader, "cellSize");
    pe->matrixRainTrailLengthLoc = GetShaderLocation(pe->matrixRainShader, "trailLength");
    pe->matrixRainFallerCountLoc = GetShaderLocation(pe->matrixRainShader, "fallerCount");
    pe->matrixRainOverlayIntensityLoc = GetShaderLocation(pe->matrixRainShader, "overlayIntensity");
    pe->matrixRainRefreshRateLoc = GetShaderLocation(pe->matrixRainShader, "refreshRate");
    pe->matrixRainLeadBrightnessLoc = GetShaderLocation(pe->matrixRainShader, "leadBrightness");
    pe->matrixRainTimeLoc = GetShaderLocation(pe->matrixRainShader, "time");
    pe->matrixRainSampleModeLoc = GetShaderLocation(pe->matrixRainShader, "sampleMode");
    pe->impressionistResolutionLoc = GetShaderLocation(pe->impressionistShader, "resolution");
    pe->impressionistSplatCountLoc = GetShaderLocation(pe->impressionistShader, "splatCount");
    pe->impressionistSplatSizeMinLoc = GetShaderLocation(pe->impressionistShader, "splatSizeMin");
    pe->impressionistSplatSizeMaxLoc = GetShaderLocation(pe->impressionistShader, "splatSizeMax");
    pe->impressionistStrokeFreqLoc = GetShaderLocation(pe->impressionistShader, "strokeFreq");
    pe->impressionistStrokeOpacityLoc = GetShaderLocation(pe->impressionistShader, "strokeOpacity");
    pe->impressionistOutlineStrengthLoc = GetShaderLocation(pe->impressionistShader, "outlineStrength");
    pe->impressionistEdgeStrengthLoc = GetShaderLocation(pe->impressionistShader, "edgeStrength");
    pe->impressionistEdgeMaxDarkenLoc = GetShaderLocation(pe->impressionistShader, "edgeMaxDarken");
    pe->impressionistGrainScaleLoc = GetShaderLocation(pe->impressionistShader, "grainScale");
    pe->impressionistGrainAmountLoc = GetShaderLocation(pe->impressionistShader, "grainAmount");
    pe->impressionistExposureLoc = GetShaderLocation(pe->impressionistShader, "exposure");
    pe->kuwaharaResolutionLoc = GetShaderLocation(pe->kuwaharaShader, "resolution");
    pe->kuwaharaRadiusLoc = GetShaderLocation(pe->kuwaharaShader, "radius");
    pe->inkWashResolutionLoc = GetShaderLocation(pe->inkWashShader, "resolution");
    pe->inkWashStrengthLoc = GetShaderLocation(pe->inkWashShader, "strength");
    pe->inkWashGranulationLoc = GetShaderLocation(pe->inkWashShader, "granulation");
    pe->inkWashBleedStrengthLoc = GetShaderLocation(pe->inkWashShader, "bleedStrength");
    pe->inkWashBleedRadiusLoc = GetShaderLocation(pe->inkWashShader, "bleedRadius");
}

static void SetResolutionUniforms(PostEffect* pe, int width, int height)
{
    float resolution[2] = { (float)width, (float)height };
    SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->voronoiShader, pe->voronoiResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->feedbackShader, pe->feedbackResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->fxaaShader, pe->fxaaResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->clarityShader, pe->clarityResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->pixelationShader, pe->pixelationResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->glitchShader, pe->glitchResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->toonShader, pe->toonResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->asciiArtShader, pe->asciiArtResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->oilPaintShader, pe->oilPaintResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->watercolorShader, pe->watercolorResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->halftoneShader, pe->halftoneResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->bokehShader, pe->bokehResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->pencilSketchShader, pe->pencilSketchResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->matrixRainShader, pe->matrixRainResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->impressionistShader, pe->impressionistResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->kuwaharaShader, pe->kuwaharaResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->inkWashShader, pe->inkWashResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

PostEffect* PostEffectInit(int screenWidth, int screenHeight)
{
    PostEffect* pe = (PostEffect*)calloc(1, sizeof(PostEffect));
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
    pe->voronoiTime = 0.0f;
    pe->infiniteZoomTime = 0.0f;
    pe->sineWarpTime = 0.0f;
    pe->waveRippleTime = 0.0f;
    pe->mobiusTime = 0.0f;
    pe->glitchTime = 0.0f;
    pe->glitchFrame = 0;
    pe->poincareTime = 0.0f;
    pe->currentPoincareTranslation[0] = 0.0f;
    pe->currentPoincareTranslation[1] = 0.0f;
    pe->currentPoincareRotation = 0.0f;
    pe->drosteZoomTime = 0.0f;
    pe->radialPulseTime = 0.0f;
    pe->warpTime = 0.0f;
    pe->chladniWarpPhase = 0.0f;
    pe->phyllotaxisWarpSpinOffset = 0.0f;

    SetResolutionUniforms(pe, screenWidth, screenHeight);

    RenderUtilsInitTextureHDR(&pe->accumTexture, screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[0], screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->pingPong[1], screenWidth, screenHeight, LOG_PREFIX);
    RenderUtilsInitTextureHDR(&pe->outputTexture, screenWidth, screenHeight, LOG_PREFIX);

    if (pe->accumTexture.id == 0 || pe->pingPong[0].id == 0 || pe->pingPong[1].id == 0 ||
        pe->outputTexture.id == 0) {
        TraceLog(LOG_ERROR, "POST_EFFECT: Failed to create render textures");
        UnloadShader(pe->feedbackShader);
        UnloadShader(pe->blurHShader);
        UnloadShader(pe->blurVShader);
        UnloadShader(pe->chromaticShader);
        UnloadShader(pe->kaleidoShader);
        UnloadShader(pe->voronoiShader);
        UnloadShader(pe->fxaaShader);
        free(pe);
        return NULL;
    }

    pe->physarum = PhysarumInit(screenWidth, screenHeight, NULL);
    pe->curlFlow = CurlFlowInit(screenWidth, screenHeight, NULL);
    pe->curlAdvection = CurlAdvectionInit(screenWidth, screenHeight, NULL);
    pe->attractorFlow = AttractorFlowInit(screenWidth, screenHeight, NULL);
    pe->boids = BoidsInit(screenWidth, screenHeight, NULL);
    pe->cymatics = CymaticsInit(screenWidth, screenHeight, NULL);
    pe->blendCompositor = BlendCompositorInit();
    pe->falseColorLUT = ColorLUTInit(&pe->effects.falseColor.gradient);

    InitFFTTexture(&pe->fftTexture);
    pe->fftMaxMagnitude = 1.0f;
    TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)", pe->fftTexture.width, pe->fftTexture.height);

    InitWaveformTexture(&pe->waveformTexture);
    TraceLog(LOG_INFO, "POST_EFFECT: Waveform texture created (%dx%d)", pe->waveformTexture.width, pe->waveformTexture.height);

    InitBloomMips(pe, screenWidth, screenHeight);
    TraceLog(LOG_INFO, "POST_EFFECT: Bloom mips allocated (%dx%d to %dx%d)",
             pe->bloomMips[0].texture.width, pe->bloomMips[0].texture.height,
             pe->bloomMips[4].texture.width, pe->bloomMips[4].texture.height);

    // Generate 256x256 RGBA noise for oil paint brush randomization
    Image noiseImg = GenImageColor(256, 256, BLANK);
    Color* pixels = (Color*)noiseImg.data;
    for (int i = 0; i < 256 * 256; i++) {
        pixels[i] = Color{ (unsigned char)(rand() % 256), (unsigned char)(rand() % 256),
                           (unsigned char)(rand() % 256), (unsigned char)(rand() % 256) };
    }
    pe->oilPaintNoiseTex = LoadTextureFromImage(noiseImg);
    UnloadImage(noiseImg);
    SetTextureFilter(pe->oilPaintNoiseTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(pe->oilPaintNoiseTex, TEXTURE_WRAP_REPEAT);

    RenderUtilsInitTextureHDR(&pe->oilPaintIntermediate, screenWidth, screenHeight, LOG_PREFIX);

    return pe;
}

void PostEffectUninit(PostEffect* pe)
{
    if (pe == NULL) {
        return;
    }

    PhysarumUninit(pe->physarum);
    CurlFlowUninit(pe->curlFlow);
    CurlAdvectionUninit(pe->curlAdvection);
    AttractorFlowUninit(pe->attractorFlow);
    BoidsUninit(pe->boids);
    CymaticsUninit(pe->cymatics);
    BlendCompositorUninit(pe->blendCompositor);
    ColorLUTUninit(pe->falseColorLUT);
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
    UnloadShader(pe->kaleidoShader);
    UnloadShader(pe->voronoiShader);
    UnloadShader(pe->fxaaShader);
    UnloadShader(pe->clarityShader);
    UnloadShader(pe->gammaShader);
    UnloadShader(pe->shapeTextureShader);
    UnloadShader(pe->infiniteZoomShader);
    UnloadShader(pe->sineWarpShader);
    UnloadShader(pe->radialStreakShader);
    UnloadShader(pe->textureWarpShader);
    UnloadShader(pe->waveRippleShader);
    UnloadShader(pe->mobiusShader);
    UnloadShader(pe->pixelationShader);
    UnloadShader(pe->glitchShader);
    UnloadShader(pe->poincareDiskShader);
    UnloadShader(pe->toonShader);
    UnloadShader(pe->heightfieldReliefShader);
    UnloadShader(pe->gradientFlowShader);
    UnloadShader(pe->drosteZoomShader);
    UnloadShader(pe->kifsShader);
    UnloadShader(pe->latticeFoldShader);
    UnloadShader(pe->colorGradeShader);
    UnloadShader(pe->asciiArtShader);
    UnloadShader(pe->oilPaintShader);
    UnloadShader(pe->oilPaintStrokeShader);
    UnloadTexture(pe->oilPaintNoiseTex);
    UnloadRenderTexture(pe->oilPaintIntermediate);
    UnloadShader(pe->watercolorShader);
    UnloadShader(pe->neonGlowShader);
    UnloadShader(pe->radialPulseShader);
    UnloadShader(pe->falseColorShader);
    UnloadShader(pe->halftoneShader);
    UnloadShader(pe->chladniWarpShader);
    UnloadShader(pe->crossHatchingShader);
    UnloadShader(pe->paletteQuantizationShader);
    UnloadShader(pe->bokehShader);
    UnloadShader(pe->bloomPrefilterShader);
    UnloadShader(pe->bloomDownsampleShader);
    UnloadShader(pe->bloomUpsampleShader);
    UnloadShader(pe->bloomCompositeShader);
    UnloadShader(pe->mandelboxShader);
    UnloadShader(pe->triangleFoldShader);
    UnloadShader(pe->domainWarpShader);
    UnloadShader(pe->phyllotaxisShader);
    UnloadShader(pe->phyllotaxisWarpShader);
    UnloadShader(pe->densityWaveSpiralShader);
    UnloadShader(pe->moireInterferenceShader);
    UnloadShader(pe->pencilSketchShader);
    UnloadShader(pe->matrixRainShader);
    UnloadShader(pe->impressionistShader);
    UnloadShader(pe->kuwaharaShader);
    UnloadShader(pe->inkWashShader);
    UnloadBloomMips(pe);
    free(pe);
}

void PostEffectResize(PostEffect* pe, int width, int height)
{
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

    UnloadRenderTexture(pe->oilPaintIntermediate);
    RenderUtilsInitTextureHDR(&pe->oilPaintIntermediate, width, height, LOG_PREFIX);

    PhysarumResize(pe->physarum, width, height);
    CurlFlowResize(pe->curlFlow, width, height);
    CurlAdvectionResize(pe->curlAdvection, width, height);
    AttractorFlowResize(pe->attractorFlow, width, height);
    BoidsResize(pe->boids, width, height);
    CymaticsResize(pe->cymatics, width, height);
}

void PostEffectClearFeedback(PostEffect* pe)
{
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

    // Reset only enabled simulations to avoid expensive GPU uploads for disabled effects
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
    if (pe->boids != NULL && pe->effects.boids.enabled) {
        BoidsReset(pe->boids);
    }
    if (pe->cymatics != NULL && pe->effects.cymatics.enabled) {
        CymaticsReset(pe->cymatics);
    }

    TraceLog(LOG_INFO, "%s: Cleared feedback buffers and reset simulations", LOG_PREFIX);
}

void PostEffectBeginDrawStage(PostEffect* pe)
{
    BeginTextureMode(pe->accumTexture);
}

void PostEffectEndDrawStage(void)
{
    EndTextureMode();
}
