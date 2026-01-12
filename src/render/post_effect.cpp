#include "post_effect.h"
#include "blend_compositor.h"
#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "render_utils.h"
#include "analysis/fft.h"
#include "rlgl.h"
#include <stdlib.h>

static const char* LOG_PREFIX = "POST_EFFECT";

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
    pe->watercolorShader = LoadShader(0, "shaders/watercolor.fs");
    pe->neonGlowShader = LoadShader(0, "shaders/neon_glow.fs");
    pe->radialPulseShader = LoadShader(0, "shaders/radial_pulse.fs");

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
           pe->watercolorShader.id != 0 &&
           pe->neonGlowShader.id != 0 &&
           pe->radialPulseShader.id != 0;
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
    pe->kaleidoTimeLoc = GetShaderLocation(pe->kaleidoShader, "time");
    pe->kaleidoTwistLoc = GetShaderLocation(pe->kaleidoShader, "twistAngle");
    pe->kaleidoFocalLoc = GetShaderLocation(pe->kaleidoShader, "focalOffset");
    pe->kaleidoWarpStrengthLoc = GetShaderLocation(pe->kaleidoShader, "warpStrength");
    pe->kaleidoWarpSpeedLoc = GetShaderLocation(pe->kaleidoShader, "warpSpeed");
    pe->kaleidoNoiseScaleLoc = GetShaderLocation(pe->kaleidoShader, "noiseScale");
    pe->kaleidoSmoothingLoc = GetShaderLocation(pe->kaleidoShader, "smoothing");
    pe->kifsRotationLoc = GetShaderLocation(pe->kifsShader, "rotation");
    pe->kifsTwistLoc = GetShaderLocation(pe->kifsShader, "twistAngle");
    pe->kifsIterationsLoc = GetShaderLocation(pe->kifsShader, "iterations");
    pe->kifsScaleLoc = GetShaderLocation(pe->kifsShader, "scale");
    pe->kifsOffsetLoc = GetShaderLocation(pe->kifsShader, "kifsOffset");
    pe->kifsOctantFoldLoc = GetShaderLocation(pe->kifsShader, "octantFold");
    pe->kifsPolarFoldLoc = GetShaderLocation(pe->kifsShader, "polarFold");
    pe->kifsPolarFoldSegmentsLoc = GetShaderLocation(pe->kifsShader, "polarFoldSegments");
    pe->latticeFoldCellTypeLoc = GetShaderLocation(pe->latticeFoldShader, "cellType");
    pe->latticeFoldCellScaleLoc = GetShaderLocation(pe->latticeFoldShader, "cellScale");
    pe->latticeFoldRotationLoc = GetShaderLocation(pe->latticeFoldShader, "rotation");
    pe->latticeFoldTimeLoc = GetShaderLocation(pe->latticeFoldShader, "time");
    pe->voronoiResolutionLoc = GetShaderLocation(pe->voronoiShader, "resolution");
    pe->voronoiScaleLoc = GetShaderLocation(pe->voronoiShader, "scale");
    pe->voronoiTimeLoc = GetShaderLocation(pe->voronoiShader, "time");
    pe->voronoiEdgeFalloffLoc = GetShaderLocation(pe->voronoiShader, "edgeFalloff");
    pe->voronoiIsoFrequencyLoc = GetShaderLocation(pe->voronoiShader, "isoFrequency");
    pe->voronoiUvDistortIntensityLoc = GetShaderLocation(pe->voronoiShader, "uvDistortIntensity");
    pe->voronoiEdgeIsoIntensityLoc = GetShaderLocation(pe->voronoiShader, "edgeIsoIntensity");
    pe->voronoiCenterIsoIntensityLoc = GetShaderLocation(pe->voronoiShader, "centerIsoIntensity");
    pe->voronoiFlatFillIntensityLoc = GetShaderLocation(pe->voronoiShader, "flatFillIntensity");
    pe->voronoiEdgeDarkenIntensityLoc = GetShaderLocation(pe->voronoiShader, "edgeDarkenIntensity");
    pe->voronoiAngleShadeIntensityLoc = GetShaderLocation(pe->voronoiShader, "angleShadeIntensity");
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
    pe->sineWarpOctavesLoc = GetShaderLocation(pe->sineWarpShader, "octaves");
    pe->sineWarpStrengthLoc = GetShaderLocation(pe->sineWarpShader, "strength");
    pe->sineWarpOctaveRotationLoc = GetShaderLocation(pe->sineWarpShader, "octaveRotation");
    pe->sineWarpUvScaleLoc = GetShaderLocation(pe->sineWarpShader, "uvScale");
    pe->radialStreakSamplesLoc = GetShaderLocation(pe->radialStreakShader, "samples");
    pe->radialStreakStreakLengthLoc = GetShaderLocation(pe->radialStreakShader, "streakLength");
    pe->textureWarpStrengthLoc = GetShaderLocation(pe->textureWarpShader, "strength");
    pe->textureWarpIterationsLoc = GetShaderLocation(pe->textureWarpShader, "iterations");
    pe->textureWarpChannelModeLoc = GetShaderLocation(pe->textureWarpShader, "channelMode");
    pe->waveRippleTimeLoc = GetShaderLocation(pe->waveRippleShader, "time");
    pe->waveRippleOctavesLoc = GetShaderLocation(pe->waveRippleShader, "octaves");
    pe->waveRippleStrengthLoc = GetShaderLocation(pe->waveRippleShader, "strength");
    pe->waveRippleFrequencyLoc = GetShaderLocation(pe->waveRippleShader, "frequency");
    pe->waveRippleSteepnessLoc = GetShaderLocation(pe->waveRippleShader, "steepness");
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
    pe->gradientFlowFlowAngleLoc = GetShaderLocation(pe->gradientFlowShader, "flowAngle");
    pe->gradientFlowEdgeWeightLoc = GetShaderLocation(pe->gradientFlowShader, "edgeWeight");
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
    pe->oilPaintResolutionLoc = GetShaderLocation(pe->oilPaintShader, "resolution");
    pe->oilPaintRadiusLoc = GetShaderLocation(pe->oilPaintShader, "radius");
    pe->watercolorResolutionLoc = GetShaderLocation(pe->watercolorShader, "resolution");
    pe->watercolorEdgeDarkeningLoc = GetShaderLocation(pe->watercolorShader, "edgeDarkening");
    pe->watercolorGranulationStrengthLoc = GetShaderLocation(pe->watercolorShader, "granulationStrength");
    pe->watercolorPaperScaleLoc = GetShaderLocation(pe->watercolorShader, "paperScale");
    pe->watercolorSoftnessLoc = GetShaderLocation(pe->watercolorShader, "softness");
    pe->watercolorBleedStrengthLoc = GetShaderLocation(pe->watercolorShader, "bleedStrength");
    pe->watercolorBleedRadiusLoc = GetShaderLocation(pe->watercolorShader, "bleedRadius");
    pe->watercolorColorLevelsLoc = GetShaderLocation(pe->watercolorShader, "colorLevels");
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
    pe->radialPulsePhaseLoc = GetShaderLocation(pe->radialPulseShader, "phase");
    pe->radialPulseSpiralTwistLoc = GetShaderLocation(pe->radialPulseShader, "spiralTwist");
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
    SetShaderValue(pe->oilPaintShader, pe->oilPaintResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->watercolorShader, pe->watercolorResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
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
    pe->attractorFlow = AttractorFlowInit(screenWidth, screenHeight, NULL);
    pe->boids = BoidsInit(screenWidth, screenHeight, NULL);
    pe->blendCompositor = BlendCompositorInit();

    InitFFTTexture(&pe->fftTexture);
    pe->fftMaxMagnitude = 1.0f;
    TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)", pe->fftTexture.width, pe->fftTexture.height);

    return pe;
}

void PostEffectUninit(PostEffect* pe)
{
    if (pe == NULL) {
        return;
    }

    PhysarumUninit(pe->physarum);
    CurlFlowUninit(pe->curlFlow);
    AttractorFlowUninit(pe->attractorFlow);
    BoidsUninit(pe->boids);
    BlendCompositorUninit(pe->blendCompositor);
    UnloadTexture(pe->fftTexture);
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
    UnloadShader(pe->watercolorShader);
    UnloadShader(pe->neonGlowShader);
    UnloadShader(pe->radialPulseShader);
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

    PhysarumResize(pe->physarum, width, height);
    CurlFlowResize(pe->curlFlow, width, height);
    AttractorFlowResize(pe->attractorFlow, width, height);
    BoidsResize(pe->boids, width, height);
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
    if (pe->effects.attractorFlow.enabled) {
        AttractorFlowReset(pe->attractorFlow);
    }
    if (pe->boids != NULL && pe->effects.boids.enabled) {
        BoidsReset(pe->boids);
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
