#include "shader_setup.h"
#include "post_effect.h"
#include "blend_compositor.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"

TransformEffectEntry GetTransformEffect(PostEffect* pe, TransformEffectType type)
{
    switch (type) {
        case TRANSFORM_MOBIUS:
            return { &pe->mobiusShader, SetupMobius, &pe->effects.mobius.enabled };
        case TRANSFORM_TURBULENCE:
            return { &pe->turbulenceShader, SetupTurbulence, &pe->effects.turbulence.enabled };
        case TRANSFORM_KALEIDOSCOPE:
            return { &pe->kaleidoShader, SetupKaleido, &pe->effects.kaleidoscope.enabled };
        case TRANSFORM_INFINITE_ZOOM:
            return { &pe->infiniteZoomShader, SetupInfiniteZoom, &pe->effects.infiniteZoom.enabled };
        case TRANSFORM_RADIAL_STREAK:
            return { &pe->radialStreakShader, SetupRadialStreak, &pe->effects.radialStreak.enabled };
        case TRANSFORM_MULTI_INVERSION:
            return { &pe->multiInversionShader, SetupMultiInversion, &pe->effects.multiInversion.enabled };
        case TRANSFORM_VORONOI:
            return { &pe->voronoiShader, SetupVoronoi, &pe->effects.voronoi.enabled };
        default:
            return { NULL, NULL, NULL };
    }
}

void SetupVoronoi(PostEffect* pe)
{
    SetShaderValue(pe->voronoiShader, pe->voronoiScaleLoc,
                   &pe->effects.voronoi.scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiStrengthLoc,
                   &pe->effects.voronoi.strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiTimeLoc,
                   &pe->voronoiTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->voronoiShader, pe->voronoiEdgeFalloffLoc,
                   &pe->effects.voronoi.edgeFalloff, SHADER_UNIFORM_FLOAT);
    int voronoiMode = (int)pe->effects.voronoi.mode;
    SetShaderValue(pe->voronoiShader, pe->voronoiModeLoc,
                   &voronoiMode, SHADER_UNIFORM_INT);
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
    const int mode = (int)k->mode;
    SetShaderValue(pe->kaleidoShader, pe->kaleidoModeLoc,
                   &mode, SHADER_UNIFORM_INT);
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
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsIterationsLoc,
                   &k->kifsIterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsScaleLoc,
                   &k->kifsScale, SHADER_UNIFORM_FLOAT);
    const float kifsOffset[2] = { k->kifsOffsetX, k->kifsOffsetY };
    SetShaderValue(pe->kaleidoShader, pe->kaleidoKifsOffsetLoc,
                   kifsOffset, SHADER_UNIFORM_VEC2);
}

void SetupMobius(PostEffect* pe)
{
    const MobiusConfig* m = &pe->effects.mobius;
    SetShaderValue(pe->mobiusShader, pe->mobiusTimeLoc, &pe->mobiusTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mobiusShader, pe->mobiusIterationsLoc, &m->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->mobiusShader, pe->mobiusPoleMagLoc, &m->poleMagnitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mobiusShader, pe->mobiusRotationLoc, &pe->mobiusRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mobiusShader, pe->mobiusUvScaleLoc, &m->uvScale, SHADER_UNIFORM_FLOAT);
}

void SetupTurbulence(PostEffect* pe)
{
    const TurbulenceConfig* t = &pe->effects.turbulence;
    SetShaderValue(pe->turbulenceShader, pe->turbulenceTimeLoc, &pe->turbulenceTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->turbulenceShader, pe->turbulenceOctavesLoc, &t->octaves, SHADER_UNIFORM_INT);
    SetShaderValue(pe->turbulenceShader, pe->turbulenceStrengthLoc, &t->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->turbulenceShader, pe->turbulenceOctaveTwistLoc, &t->octaveTwist, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->turbulenceShader, pe->turbulenceUvScaleLoc, &t->uvScale, SHADER_UNIFORM_FLOAT);
}

void SetupInfiniteZoom(PostEffect* pe)
{
    const InfiniteZoomConfig* iz = &pe->effects.infiniteZoom;
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomTimeLoc,
                   &pe->infiniteZoomTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomZoomDepthLoc,
                   &iz->zoomDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->infiniteZoomShader, pe->infiniteZoomFocalLoc,
                   pe->infiniteZoomFocal, SHADER_UNIFORM_VEC2);
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
    SetShaderValue(pe->radialStreakShader, pe->radialStreakTimeLoc,
                   &pe->radialStreakTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialStreakShader, pe->radialStreakSamplesLoc,
                   &rs->samples, SHADER_UNIFORM_INT);
    SetShaderValue(pe->radialStreakShader, pe->radialStreakStreakLengthLoc,
                   &rs->streakLength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialStreakShader, pe->radialStreakSpiralTwistLoc,
                   &rs->spiralTwist, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->radialStreakShader, pe->radialStreakFocalLoc,
                   pe->radialStreakFocal, SHADER_UNIFORM_VEC2);
}

void SetupMultiInversion(PostEffect* pe)
{
    const MultiInversionConfig* mi = &pe->effects.multiInversion;
    SetShaderValue(pe->multiInversionShader, pe->multiInversionTimeLoc,
                   &pe->multiInversionTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionIterationsLoc,
                   &mi->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionRadiusLoc,
                   &mi->radius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionRadiusScaleLoc,
                   &mi->radiusScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionFocalAmplitudeLoc,
                   &mi->focalAmplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionFocalFreqXLoc,
                   &mi->focalFreqX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionFocalFreqYLoc,
                   &mi->focalFreqY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->multiInversionShader, pe->multiInversionPhaseOffsetLoc,
                   &mi->phaseOffset, SHADER_UNIFORM_FLOAT);
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
