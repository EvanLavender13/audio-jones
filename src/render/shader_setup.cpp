#include "shader_setup.h"
#include "post_effect.h"
#include <math.h>
#include "blend_compositor.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/curl_advection.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/cymatics.h"

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
        case TRANSFORM_DROSTE_ZOOM:
            return { &pe->drosteZoomShader, SetupDrosteZoom, &pe->effects.drosteZoom.enabled };
        case TRANSFORM_KIFS:
            return { &pe->kifsShader, SetupKifs, &pe->effects.kifs.enabled };
        case TRANSFORM_LATTICE_FOLD:
            return { &pe->latticeFoldShader, SetupLatticeFold, &pe->effects.latticeFold.enabled };
        case TRANSFORM_COLOR_GRADE:
            return { &pe->colorGradeShader, SetupColorGrade, &pe->effects.colorGrade.enabled };
        case TRANSFORM_ASCII_ART:
            return { &pe->asciiArtShader, SetupAsciiArt, &pe->effects.asciiArt.enabled };
        case TRANSFORM_OIL_PAINT:
            return { &pe->oilPaintShader, SetupOilPaint, &pe->effects.oilPaint.enabled };
        case TRANSFORM_WATERCOLOR:
            return { &pe->watercolorShader, SetupWatercolor, &pe->effects.watercolor.enabled };
        case TRANSFORM_NEON_GLOW:
            return { &pe->neonGlowShader, SetupNeonGlow, &pe->effects.neonGlow.enabled };
        case TRANSFORM_RADIAL_PULSE:
            return { &pe->radialPulseShader, SetupRadialPulse, &pe->effects.radialPulse.enabled };
        case TRANSFORM_DUOTONE:
            return { &pe->duotoneShader, SetupDuotone, &pe->effects.duotone.enabled };
        case TRANSFORM_HALFTONE:
            return { &pe->halftoneShader, SetupHalftone, &pe->effects.halftone.enabled };
        case TRANSFORM_CHLADNI_WARP:
            return { &pe->chladniWarpShader, SetupChladniWarp, &pe->effects.chladniWarp.enabled };
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
    int smoothModeInt = v->smoothMode ? 1 : 0;
    SetShaderValue(pe->voronoiShader, pe->voronoiSmoothModeLoc,
                   &smoothModeInt, SHADER_UNIFORM_INT);
    SetShaderValue(pe->voronoiShader, pe->voronoiUvDistortIntensityLoc,
                   &v->uvDistortIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeIsoIntensityLoc,
                   &v->edgeIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiCenterIsoIntensityLoc,
                   &v->centerIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiFlatFillIntensityLoc,
                   &v->flatFillIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiOrganicFlowIntensityLoc,
                   &v->organicFlowIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeGlowIntensityLoc,
                   &v->edgeGlowIntensity, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->feedbackShader, pe->feedbackFlowStrengthLoc,
                   &pe->effects.feedbackFlow.strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackFlowAngleLoc,
                   &pe->effects.feedbackFlow.flowAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackFlowScaleLoc,
                   &pe->effects.feedbackFlow.scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackFlowThresholdLoc,
                   &pe->effects.feedbackFlow.threshold, SHADER_UNIFORM_FLOAT);
    // Center pivot
    SetShaderValue(pe->feedbackShader, pe->feedbackCxLoc,
                   &pe->effects.flowField.cx, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackCyLoc,
                   &pe->effects.flowField.cy, SHADER_UNIFORM_FLOAT);
    // Directional stretch
    SetShaderValue(pe->feedbackShader, pe->feedbackSxLoc,
                   &pe->effects.flowField.sx, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackSyLoc,
                   &pe->effects.flowField.sy, SHADER_UNIFORM_FLOAT);
    // Angular modulation
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularLoc,
                   &pe->effects.flowField.zoomAngular, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularFreqLoc,
                   &pe->effects.flowField.zoomAngularFreq, SHADER_UNIFORM_INT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularLoc,
                   &pe->effects.flowField.rotAngular, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularFreqLoc,
                   &pe->effects.flowField.rotAngularFreq, SHADER_UNIFORM_INT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularLoc,
                   &pe->effects.flowField.dxAngular, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularFreqLoc,
                   &pe->effects.flowField.dxAngularFreq, SHADER_UNIFORM_INT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularLoc,
                   &pe->effects.flowField.dyAngular, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularFreqLoc,
                   &pe->effects.flowField.dyAngularFreq, SHADER_UNIFORM_INT);
    // Procedural warp
    SetShaderValue(pe->feedbackShader, pe->feedbackWarpLoc,
                   &pe->effects.proceduralWarp.warp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackWarpTimeLoc,
                   &pe->warpTime, SHADER_UNIFORM_FLOAT);
    float warpScaleInverse = 1.0f / pe->effects.proceduralWarp.warpScale;
    SetShaderValue(pe->feedbackShader, pe->feedbackWarpScaleInverseLoc,
                   &warpScaleInverse, SHADER_UNIFORM_FLOAT);
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

void SetupCurlAdvectionTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->curlAdvection->trailMap),
                         pe->effects.curlAdvection.boostIntensity,
                         pe->effects.curlAdvection.blendMode);
}

void SetupAttractorFlowTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->attractorFlow->trailMap),
                         pe->effects.attractorFlow.boostIntensity,
                         pe->effects.attractorFlow.blendMode);
}

void SetupBoidsTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->boids->trailMap),
                         pe->effects.boids.boostIntensity,
                         pe->effects.boids.blendMode);
}

void SetupCymaticsTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->cymatics->trailMap),
                         pe->effects.cymatics.boostIntensity,
                         pe->effects.cymatics.blendMode);
}

void SetupKaleido(PostEffect* pe)
{
    const KaleidoscopeConfig* k = &pe->effects.kaleidoscope;

    SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                   &k->segments, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                   &pe->currentKaleidoRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTimeLoc,
                   &pe->transformTime, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->kaleidoShader, pe->kaleidoSmoothingLoc,
                   &k->smoothing, SHADER_UNIFORM_FLOAT);
}

void SetupKifs(PostEffect* pe)
{
    const KifsConfig* k = &pe->effects.kifs;

    SetShaderValue(pe->kifsShader, pe->kifsRotationLoc,
                   &pe->currentKifsRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kifsShader, pe->kifsTwistLoc,
                   &k->twistAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kifsShader, pe->kifsIterationsLoc,
                   &k->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kifsShader, pe->kifsScaleLoc,
                   &k->scale, SHADER_UNIFORM_FLOAT);
    const float kifsOffset[2] = { k->offsetX, k->offsetY };
    SetShaderValue(pe->kifsShader, pe->kifsOffsetLoc,
                   kifsOffset, SHADER_UNIFORM_VEC2);
    const int octantFold = k->octantFold ? 1 : 0;
    SetShaderValue(pe->kifsShader, pe->kifsOctantFoldLoc,
                   &octantFold, SHADER_UNIFORM_INT);
    const int polarFold = k->polarFold ? 1 : 0;
    SetShaderValue(pe->kifsShader, pe->kifsPolarFoldLoc,
                   &polarFold, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kifsShader, pe->kifsPolarFoldSegmentsLoc,
                   &k->polarFoldSegments, SHADER_UNIFORM_INT);
}

void SetupLatticeFold(PostEffect* pe)
{
    const LatticeFoldConfig* l = &pe->effects.latticeFold;

    SetShaderValue(pe->latticeFoldShader, pe->latticeFoldCellTypeLoc,
                   &l->cellType, SHADER_UNIFORM_INT);
    SetShaderValue(pe->latticeFoldShader, pe->latticeFoldCellScaleLoc,
                   &l->cellScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->latticeFoldShader, pe->latticeFoldRotationLoc,
                   &pe->currentLatticeFoldRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->latticeFoldShader, pe->latticeFoldTimeLoc,
                   &pe->transformTime, SHADER_UNIFORM_FLOAT);
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

void SetupDrosteZoom(PostEffect* pe)
{
    const DrosteZoomConfig* dz = &pe->effects.drosteZoom;
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomTimeLoc,
                   &pe->drosteZoomTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomScaleLoc,
                   &dz->scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomSpiralAngleLoc,
                   &dz->spiralAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomShearCoeffLoc,
                   &dz->shearCoeff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomInnerRadiusLoc,
                   &dz->innerRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->drosteZoomShader, pe->drosteZoomBranchesLoc,
                   &dz->branches, SHADER_UNIFORM_INT);
}

void SetupColorGrade(PostEffect* pe)
{
    const ColorGradeConfig* cg = &pe->effects.colorGrade;
    SetShaderValue(pe->colorGradeShader, pe->colorGradeHueShiftLoc,
                   &cg->hueShift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeSaturationLoc,
                   &cg->saturation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeBrightnessLoc,
                   &cg->brightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeContrastLoc,
                   &cg->contrast, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeTemperatureLoc,
                   &cg->temperature, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeShadowsOffsetLoc,
                   &cg->shadowsOffset, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeMidtonesOffsetLoc,
                   &cg->midtonesOffset, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->colorGradeShader, pe->colorGradeHighlightsOffsetLoc,
                   &cg->highlightsOffset, SHADER_UNIFORM_FLOAT);
}

void SetupAsciiArt(PostEffect* pe)
{
    const AsciiArtConfig* aa = &pe->effects.asciiArt;
    int cellPixels = (int)aa->cellSize;
    SetShaderValue(pe->asciiArtShader, pe->asciiArtCellPixelsLoc,
                   &cellPixels, SHADER_UNIFORM_INT);
    SetShaderValue(pe->asciiArtShader, pe->asciiArtColorModeLoc,
                   &aa->colorMode, SHADER_UNIFORM_INT);
    float foreground[3] = { aa->foregroundR, aa->foregroundG, aa->foregroundB };
    SetShaderValue(pe->asciiArtShader, pe->asciiArtForegroundLoc,
                   foreground, SHADER_UNIFORM_VEC3);
    float background[3] = { aa->backgroundR, aa->backgroundG, aa->backgroundB };
    SetShaderValue(pe->asciiArtShader, pe->asciiArtBackgroundLoc,
                   background, SHADER_UNIFORM_VEC3);
    int invert = aa->invert ? 1 : 0;
    SetShaderValue(pe->asciiArtShader, pe->asciiArtInvertLoc,
                   &invert, SHADER_UNIFORM_INT);
}

void SetupOilPaint(PostEffect* pe)
{
    const OilPaintConfig* op = &pe->effects.oilPaint;
    int radius = (int)op->radius;
    SetShaderValue(pe->oilPaintShader, pe->oilPaintRadiusLoc,
                   &radius, SHADER_UNIFORM_INT);
}

void SetupWatercolor(PostEffect* pe)
{
    const WatercolorConfig* wc = &pe->effects.watercolor;
    SetShaderValue(pe->watercolorShader, pe->watercolorEdgeDarkeningLoc,
                   &wc->edgeDarkening, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorGranulationStrengthLoc,
                   &wc->granulationStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorPaperScaleLoc,
                   &wc->paperScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorSoftnessLoc,
                   &wc->softness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorBleedStrengthLoc,
                   &wc->bleedStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorBleedRadiusLoc,
                   &wc->bleedRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorColorLevelsLoc,
                   &wc->colorLevels, SHADER_UNIFORM_INT);
}

void SetupNeonGlow(PostEffect* pe)
{
    const NeonGlowConfig* ng = &pe->effects.neonGlow;
    float glowColor[3] = { ng->glowR, ng->glowG, ng->glowB };
    SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowColorLoc,
                   glowColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgeThresholdLoc,
                   &ng->edgeThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowEdgePowerLoc,
                   &ng->edgePower, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowIntensityLoc,
                   &ng->glowIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowRadiusLoc,
                   &ng->glowRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowGlowSamplesLoc,
                   &ng->glowSamples, SHADER_UNIFORM_INT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowOriginalVisibilityLoc,
                   &ng->originalVisibility, SHADER_UNIFORM_FLOAT);
}

void SetupRadialPulse(PostEffect* pe)
{
    const RadialPulseConfig* rp = &pe->effects.radialPulse;
    SetShaderValue(pe->radialPulseShader, pe->radialPulseRadialFreqLoc,
                   &rp->radialFreq, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulseRadialAmpLoc,
                   &rp->radialAmp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulseSegmentsLoc,
                   &rp->segments, SHADER_UNIFORM_INT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulseAngularAmpLoc,
                   &rp->angularAmp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulsePetalAmpLoc,
                   &rp->petalAmp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulsePhaseLoc,
                   &pe->radialPulseTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialPulseShader, pe->radialPulseSpiralTwistLoc,
                   &rp->spiralTwist, SHADER_UNIFORM_FLOAT);
}

void SetupDuotone(PostEffect* pe)
{
    const DuotoneConfig* dt = &pe->effects.duotone;
    float shadowColor[3] = { dt->shadowR, dt->shadowG, dt->shadowB };
    SetShaderValue(pe->duotoneShader, pe->duotoneShadowColorLoc,
                   shadowColor, SHADER_UNIFORM_VEC3);
    float highlightColor[3] = { dt->highlightR, dt->highlightG, dt->highlightB };
    SetShaderValue(pe->duotoneShader, pe->duotoneHighlightColorLoc,
                   highlightColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(pe->duotoneShader, pe->duotoneIntensityLoc,
                   &dt->intensity, SHADER_UNIFORM_FLOAT);
}

void SetupHalftone(PostEffect* pe)
{
    const HalftoneConfig* ht = &pe->effects.halftone;
    float rotation = pe->currentHalftoneRotation + ht->rotationAngle;

    SetShaderValue(pe->halftoneShader, pe->halftoneDotScaleLoc,
                   &ht->dotScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->halftoneShader, pe->halftoneDotSizeLoc,
                   &ht->dotSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->halftoneShader, pe->halftoneRotationLoc,
                   &rotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->halftoneShader, pe->halftoneThresholdLoc,
                   &ht->threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->halftoneShader, pe->halftoneSoftnessLoc,
                   &ht->softness, SHADER_UNIFORM_FLOAT);
}

void SetupChladniWarp(PostEffect* pe)
{
    const ChladniWarpConfig* cw = &pe->effects.chladniWarp;

    // CPU phase accumulation for smooth animation
    pe->chladniWarpPhase += pe->currentDeltaTime * cw->animRate;

    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpNLoc,
                   &cw->n, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpMLoc,
                   &cw->m, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpPlateSizeLoc,
                   &cw->plateSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpStrengthLoc,
                   &cw->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpModeLoc,
                   &cw->warpMode, SHADER_UNIFORM_INT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpAnimPhaseLoc,
                   &pe->chladniWarpPhase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpAnimRangeLoc,
                   &cw->animRange, SHADER_UNIFORM_FLOAT);
    int preFold = cw->preFold ? 1 : 0;
    SetShaderValue(pe->chladniWarpShader, pe->chladniWarpPreFoldLoc,
                   &preFold, SHADER_UNIFORM_INT);
}
