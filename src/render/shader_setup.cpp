#include "shader_setup.h"
#include "post_effect.h"
#include <math.h>
#include "blend_compositor.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"

TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type)
{
    switch (type) {
        case TRANSFORM_SINE_WARP:
            return { &pe->sineWarpShader, SetupSineWarp, &pe->effects.sineWarp.enabled };
        case TRANSFORM_KALEIDOSCOPE:
            return { &pe->kaleidoShader, SetupKaleido, &pe->effects.kaleidoscope.enabled };
        case TRANSFORM_INFINITE_ZOOM:
            return { &pe->infiniteZoomShader, SetupInfiniteZoom, &pe->effects.infiniteZoom.enabled };
        case TRANSFORM_RADIAL_STREAK:
            return { &pe->radialStreakShader, SetupRadialStreak, &pe->effects.radialStreak.enabled };
        case TRANSFORM_TEXTURE_WARP:
            return { &pe->textureWarpShader, SetupTextureWarp, &pe->effects.textureWarp.enabled };
        case TRANSFORM_VORONOI:
            return { &pe->voronoiShader, SetupVoronoi, &pe->effects.voronoi.enabled };
        case TRANSFORM_WAVE_RIPPLE:
            return { &pe->waveRippleShader, SetupWaveRipple, &pe->effects.waveRipple.enabled };
        case TRANSFORM_MOBIUS:
            return { &pe->mobiusShader, SetupMobius, &pe->effects.mobius.enabled };
        case TRANSFORM_PIXELATION:
            return { &pe->pixelationShader, SetupPixelation, &pe->effects.pixelation.enabled };
        case TRANSFORM_GLITCH:
            return { &pe->glitchShader, SetupGlitch, &pe->effects.glitch.enabled };
        case TRANSFORM_POINCARE_DISK:
            return { &pe->poincareDiskShader, SetupPoincareDisk, &pe->effects.poincareDisk.enabled };
        case TRANSFORM_TOON:
            return { &pe->toonShader, SetupToon, &pe->effects.toon.enabled };
        case TRANSFORM_HEIGHTFIELD_RELIEF:
            return { &pe->heightfieldReliefShader, SetupHeightfieldRelief, &pe->effects.heightfieldRelief.enabled };
        case TRANSFORM_GRADIENT_FLOW:
            return { &pe->gradientFlowShader, SetupGradientFlow, &pe->effects.gradientFlow.enabled };
        default:
            return { NULL, NULL, NULL };
    }
}

void SetupVoronoi(PostEffect* pe)
{
    const VoronoiConfig* v = &pe->effects.voronoi;
    SetShaderValue(pe->voronoiShader, pe->voronoiScaleLoc,
                   &v->scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiTimeLoc,
                   &pe->voronoiTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeFalloffLoc,
                   &v->edgeFalloff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiIsoFrequencyLoc,
                   &v->isoFrequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiUvDistortIntensityLoc,
                   &v->uvDistortIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeIsoIntensityLoc,
                   &v->edgeIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiCenterIsoIntensityLoc,
                   &v->centerIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiFlatFillIntensityLoc,
                   &v->flatFillIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeDarkenIntensityLoc,
                   &v->edgeDarkenIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiAngleShadeIntensityLoc,
                   &v->angleShadeIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiDeterminantIntensityLoc,
                   &v->determinantIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiRatioIntensityLoc,
                   &v->ratioIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeDetectIntensityLoc,
                   &v->edgeDetectIntensity, SHADER_UNIFORM_FLOAT);
}

void SetupFeedback(PostEffect* pe)
{
    SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                   &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomBaseLoc,
                   &pe->effects.flowField.zoomBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomRadialLoc,
                   &pe->effects.flowField.zoomRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc,
                   &pe->effects.flowField.rotationSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc,
                   &pe->effects.flowField.rotationSpeedRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxBaseLoc,
                   &pe->effects.flowField.dxBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxRadialLoc,
                   &pe->effects.flowField.dxRadial, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyBaseLoc,
                   &pe->effects.flowField.dyBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyRadialLoc,
                   &pe->effects.flowField.dyRadial, SHADER_UNIFORM_FLOAT);
}

void SetupBlurH(PostEffect* pe)
{
    SetShaderValue(pe->blurHShader, pe->blurHScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_FLOAT);
}

void SetupBlurV(PostEffect* pe)
{
    SetShaderValue(pe->blurVShader, pe->blurVScaleLoc,
                   &pe->currentBlurScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->blurVShader, pe->halfLifeLoc,
                   &pe->effects.halfLife, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->blurVShader, pe->deltaTimeLoc,
                   &pe->currentDeltaTime, SHADER_UNIFORM_FLOAT);
}

void SetupTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->physarum->trailMap),
                         pe->effects.physarum.boostIntensity,
                         pe->effects.physarum.blendMode);
}

void SetupCurlFlowTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->curlFlow->trailMap),
                         pe->effects.curlFlow.boostIntensity,
                         pe->effects.curlFlow.blendMode);
}

void SetupAttractorFlowTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->attractorFlow->trailMap),
                         pe->effects.attractorFlow.boostIntensity,
                         pe->effects.attractorFlow.blendMode);
}

void SetupKaleido(PostEffect* pe)
{
    const KaleidoscopeConfig* k = &pe->effects.kaleidoscope;

    // Technique intensities
    SetShaderValue(pe->kaleidoShader, pe->kaleidoPolarIntensityLoc,
                   &k->polarIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsIntensityLoc,
                   &k->kifsIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoIterMirrorIntensityLoc,
                   &k->iterMirrorIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoHexFoldIntensityLoc,
                   &k->hexFoldIntensity, SHADER_UNIFORM_FLOAT);

    // Shared params
    SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                   &k->segments, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                   &pe->currentKaleidoRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTimeLoc,
                   &pe->currentKaleidoTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTwistLoc,
                   &k->twistAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoFocalLoc,
                   pe->currentKaleidoFocal, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoWarpStrengthLoc,
                   &k->warpStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoWarpSpeedLoc,
                   &k->warpSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoNoiseScaleLoc,
                   &k->noiseScale, SHADER_UNIFORM_FLOAT);

    // KIFS params
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsIterationsLoc,
                   &k->kifsIterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsScaleLoc,
                   &k->kifsScale, SHADER_UNIFORM_FLOAT);
    const float kifsOffset[2] = { k->kifsOffsetX, k->kifsOffsetY };
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsOffsetLoc,
                   kifsOffset, SHADER_UNIFORM_VEC2);

    // Hex Fold params
    SetShaderValue(pe->kaleidoShader, pe->kaleidoHexScaleLoc,
                   &k->hexScale, SHADER_UNIFORM_FLOAT);
}

void SetupSineWarp(PostEffect* pe)
{
    const SineWarpConfig* sw = &pe->effects.sineWarp;
    SetShaderValue(pe->sineWarpShader, pe->sineWarpTimeLoc, &pe->sineWarpTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpOctavesLoc, &sw->octaves, SHADER_UNIFORM_INT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpStrengthLoc, &sw->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpOctaveRotationLoc, &sw->octaveRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpUvScaleLoc, &sw->uvScale, SHADER_UNIFORM_FLOAT);
}

void SetupInfiniteZoom(PostEffect* pe)
{
    const InfiniteZoomConfig* iz = &pe->effects.infiniteZoom;
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomTimeLoc,
                   &pe->infiniteZoomTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomZoomDepthLoc,
                   &iz->zoomDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomLayersLoc,
                   &iz->layers, SHADER_UNIFORM_INT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpiralAngleLoc,
                   &iz->spiralAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomSpiralTwistLoc,
                   &iz->spiralTwist, SHADER_UNIFORM_FLOAT);
    float drosteShearSnapped = roundf(iz->drosteShear);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomDrosteShearLoc,
                   &drosteShearSnapped, SHADER_UNIFORM_FLOAT);
}

void SetupRadialStreak(PostEffect* pe)
{
    const RadialStreakConfig* rs = &pe->effects.radialStreak;
    SetShaderValue(pe->radialStreakShader, pe->radialStreakSamplesLoc,
                   &rs->samples, SHADER_UNIFORM_INT);
    SetShaderValue(pe->radialStreakShader, pe->radialStreakStreakLengthLoc,
                   &rs->streakLength, SHADER_UNIFORM_FLOAT);
}

void SetupTextureWarp(PostEffect* pe)
{
    const TextureWarpConfig* tw = &pe->effects.textureWarp;
    SetShaderValue(pe->textureWarpShader, pe->textureWarpStrengthLoc,
                   &tw->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->textureWarpShader, pe->textureWarpIterationsLoc,
                   &tw->iterations, SHADER_UNIFORM_INT);
    int channelMode = (int)tw->channelMode;
    SetShaderValue(pe->textureWarpShader, pe->textureWarpChannelModeLoc,
                   &channelMode, SHADER_UNIFORM_INT);
}

void SetupWaveRipple(PostEffect* pe)
{
    const WaveRippleConfig* wr = &pe->effects.waveRipple;
    SetShaderValue(pe->waveRippleShader, pe->waveRippleTimeLoc,
                   &pe->waveRippleTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleOctavesLoc,
                   &wr->octaves, SHADER_UNIFORM_INT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleStrengthLoc,
                   &wr->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleFrequencyLoc,
                   &wr->frequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleSteepnessLoc,
                   &wr->steepness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleOriginLoc,
                   pe->currentWaveRippleOrigin, SHADER_UNIFORM_VEC2);
    int shadeEnabled = wr->shadeEnabled ? 1 : 0;
    SetShaderValue(pe->waveRippleShader, pe->waveRippleShadeEnabledLoc,
                   &shadeEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleShadeIntensityLoc,
                   &wr->shadeIntensity, SHADER_UNIFORM_FLOAT);
}

void SetupMobius(PostEffect* pe)
{
    const MobiusConfig* m = &pe->effects.mobius;
    SetShaderValue(pe->mobiusShader, pe->mobiusTimeLoc,
                   &pe->mobiusTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mobiusShader, pe->mobiusPoint1Loc,
                   pe->currentMobiusPoint1, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->mobiusShader, pe->mobiusPoint2Loc,
                   pe->currentMobiusPoint2, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->mobiusShader, pe->mobiusSpiralTightnessLoc,
                   &m->spiralTightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mobiusShader, pe->mobiusZoomFactorLoc,
                   &m->zoomFactor, SHADER_UNIFORM_FLOAT);
}

void SetupPixelation(PostEffect* pe)
{
    const PixelationConfig* p = &pe->effects.pixelation;
    SetShaderValue(pe->pixelationShader, pe->pixelationCellCountLoc,
                   &p->cellCount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->pixelationShader, pe->pixelationDitherScaleLoc,
                   &p->ditherScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->pixelationShader, pe->pixelationPosterizeLevelsLoc,
                   &p->posterizeLevels, SHADER_UNIFORM_INT);
}

void SetupGlitch(PostEffect* pe)
{
    const GlitchConfig* g = &pe->effects.glitch;

    // Update time (CPU accumulated for smooth animation)
    pe->glitchTime += pe->currentDeltaTime;
    pe->glitchFrame++;

    SetShaderValue(pe->glitchShader, pe->glitchTimeLoc,
                   &pe->glitchTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchFrameLoc,
                   &pe->glitchFrame, SHADER_UNIFORM_INT);

    // CRT mode
    int crtEnabled = g->crtEnabled ? 1 : 0;
    SetShaderValue(pe->glitchShader, pe->glitchCrtEnabledLoc,
                   &crtEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(pe->glitchShader, pe->glitchCurvatureLoc,
                   &g->curvature, SHADER_UNIFORM_FLOAT);
    int vignetteEnabled = g->vignetteEnabled ? 1 : 0;
    SetShaderValue(pe->glitchShader, pe->glitchVignetteEnabledLoc,
                   &vignetteEnabled, SHADER_UNIFORM_INT);

    // Analog mode (enabled when analogIntensity > 0)
    SetShaderValue(pe->glitchShader, pe->glitchAnalogIntensityLoc,
                   &g->analogIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchAberrationLoc,
                   &g->aberration, SHADER_UNIFORM_FLOAT);

    // Digital mode (enabled when blockThreshold > 0)
    SetShaderValue(pe->glitchShader, pe->glitchBlockThresholdLoc,
                   &g->blockThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchBlockOffsetLoc,
                   &g->blockOffset, SHADER_UNIFORM_FLOAT);

    // VHS mode
    int vhsEnabled = g->vhsEnabled ? 1 : 0;
    SetShaderValue(pe->glitchShader, pe->glitchVhsEnabledLoc,
                   &vhsEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(pe->glitchShader, pe->glitchTrackingBarIntensityLoc,
                   &g->trackingBarIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchScanlineNoiseIntensityLoc,
                   &g->scanlineNoiseIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchColorDriftIntensityLoc,
                   &g->colorDriftIntensity, SHADER_UNIFORM_FLOAT);

    // Overlay
    SetShaderValue(pe->glitchShader, pe->glitchScanlineAmountLoc,
                   &g->scanlineAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchNoiseAmountLoc,
                   &g->noiseAmount, SHADER_UNIFORM_FLOAT);
}

void SetupChromatic(PostEffect* pe)
{
    SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                   &pe->effects.chromaticOffset, SHADER_UNIFORM_FLOAT);
}

void SetupGamma(PostEffect* pe)
{
    SetShaderValue(pe->gammaShader, pe->gammaGammaLoc,
                   &pe->effects.gamma, SHADER_UNIFORM_FLOAT);
}

void SetupClarity(PostEffect* pe)
{
    SetShaderValue(pe->clarityShader, pe->clarityAmountLoc,
                   &pe->effects.clarity, SHADER_UNIFORM_FLOAT);
}

void SetupPoincareDisk(PostEffect* pe)
{
    const PoincareDiskConfig* pd = &pe->effects.poincareDisk;
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTilePLoc,
                   &pd->tileP, SHADER_UNIFORM_INT);
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTileQLoc,
                   &pd->tileQ, SHADER_UNIFORM_INT);
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTileRLoc,
                   &pd->tileR, SHADER_UNIFORM_INT);
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTranslationLoc,
                   pe->currentPoincareTranslation, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskRotationLoc,
                   &pe->currentPoincareRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->poincareDiskShader, pe->poincareDiskDiskScaleLoc,
                   &pd->diskScale, SHADER_UNIFORM_FLOAT);
}

void SetupToon(PostEffect* pe)
{
    const ToonConfig* t = &pe->effects.toon;
    SetShaderValue(pe->toonShader, pe->toonLevelsLoc,
                   &t->levels, SHADER_UNIFORM_INT);
    SetShaderValue(pe->toonShader, pe->toonEdgeThresholdLoc,
                   &t->edgeThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->toonShader, pe->toonEdgeSoftnessLoc,
                   &t->edgeSoftness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->toonShader, pe->toonThicknessVariationLoc,
                   &t->thicknessVariation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->toonShader, pe->toonNoiseScaleLoc,
                   &t->noiseScale, SHADER_UNIFORM_FLOAT);
}

void SetupHeightfieldRelief(PostEffect* pe)
{
    const HeightfieldReliefConfig* h = &pe->effects.heightfieldRelief;
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefIntensityLoc,
                   &h->intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefReliefScaleLoc,
                   &h->reliefScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefLightAngleLoc,
                   &h->lightAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefLightHeightLoc,
                   &h->lightHeight, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->heightfieldReliefShader, pe->heightfieldReliefShininessLoc,
                   &h->shininess, SHADER_UNIFORM_FLOAT);
}

void SetupGradientFlow(PostEffect* pe)
{
    const GradientFlowConfig* gf = &pe->effects.gradientFlow;
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowStrengthLoc,
                   &gf->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowIterationsLoc,
                   &gf->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowFlowAngleLoc,
                   &gf->flowAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowEdgeWeightLoc,
                   &gf->edgeWeight, SHADER_UNIFORM_FLOAT);
}
