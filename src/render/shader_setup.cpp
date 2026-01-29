#include "shader_setup.h"
#include "post_effect.h"
#include "render_utils.h"
#include "color_lut.h"
#include <math.h>
#include "blend_compositor.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include "simulation/curl_flow.h"
#include "simulation/curl_advection.h"
#include "simulation/attractor_flow.h"
#include "simulation/particle_life.h"
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
        case TRANSFORM_FALSE_COLOR:
            return { &pe->falseColorShader, SetupFalseColor, &pe->effects.falseColor.enabled };
        case TRANSFORM_HALFTONE:
            return { &pe->halftoneShader, SetupHalftone, &pe->effects.halftone.enabled };
        case TRANSFORM_CHLADNI_WARP:
            return { &pe->chladniWarpShader, SetupChladniWarp, &pe->effects.chladniWarp.enabled };
        case TRANSFORM_CROSS_HATCHING:
            return { &pe->crossHatchingShader, SetupCrossHatching, &pe->effects.crossHatching.enabled };
        case TRANSFORM_PALETTE_QUANTIZATION:
            return { &pe->paletteQuantizationShader, SetupPaletteQuantization, &pe->effects.paletteQuantization.enabled };
        case TRANSFORM_BOKEH:
            return { &pe->bokehShader, SetupBokeh, &pe->effects.bokeh.enabled };
        case TRANSFORM_BLOOM:
            return { &pe->bloomCompositeShader, SetupBloom, &pe->effects.bloom.enabled };
        case TRANSFORM_MANDELBOX:
            return { &pe->mandelboxShader, SetupMandelbox, &pe->effects.mandelbox.enabled };
        case TRANSFORM_TRIANGLE_FOLD:
            return { &pe->triangleFoldShader, SetupTriangleFold, &pe->effects.triangleFold.enabled };
        case TRANSFORM_DOMAIN_WARP:
            return { &pe->domainWarpShader, SetupDomainWarp, &pe->effects.domainWarp.enabled };
        case TRANSFORM_PHYLLOTAXIS:
            return { &pe->phyllotaxisShader, SetupPhyllotaxis, &pe->effects.phyllotaxis.enabled };
        case TRANSFORM_DENSITY_WAVE_SPIRAL:
            return { &pe->densityWaveSpiralShader, SetupDensityWaveSpiral, &pe->effects.densityWaveSpiral.enabled };
        case TRANSFORM_MOIRE_INTERFERENCE:
            return { &pe->moireInterferenceShader, SetupMoireInterference, &pe->effects.moireInterference.enabled };
        case TRANSFORM_PENCIL_SKETCH:
            return { &pe->pencilSketchShader, SetupPencilSketch, &pe->effects.pencilSketch.enabled };
        case TRANSFORM_MATRIX_RAIN:
            return { &pe->matrixRainShader, SetupMatrixRain, &pe->effects.matrixRain.enabled };
        case TRANSFORM_IMPRESSIONIST:
            return { &pe->impressionistShader, SetupImpressionist, &pe->effects.impressionist.enabled };
        case TRANSFORM_KUWAHARA:
            return { &pe->kuwaharaShader, SetupKuwahara, &pe->effects.kuwahara.enabled };
        case TRANSFORM_INK_WASH:
            return { &pe->inkWashShader, SetupInkWash, &pe->effects.inkWash.enabled };
        case TRANSFORM_DISCO_BALL:
            return { &pe->discoBallShader, SetupDiscoBall, &pe->effects.discoBall.enabled };
        case TRANSFORM_PHYSARUM_BOOST:
            return { &pe->blendCompositor->shader, SetupTrailBoost, &pe->physarumBoostActive };
        case TRANSFORM_CURL_FLOW_BOOST:
            return { &pe->blendCompositor->shader, SetupCurlFlowTrailBoost, &pe->curlFlowBoostActive };
        case TRANSFORM_CURL_ADVECTION_BOOST:
            return { &pe->blendCompositor->shader, SetupCurlAdvectionTrailBoost, &pe->curlAdvectionBoostActive };
        case TRANSFORM_ATTRACTOR_FLOW_BOOST:
            return { &pe->blendCompositor->shader, SetupAttractorFlowTrailBoost, &pe->attractorFlowBoostActive };
        case TRANSFORM_BOIDS_BOOST:
            return { &pe->blendCompositor->shader, SetupBoidsTrailBoost, &pe->boidsBoostActive };
        case TRANSFORM_CYMATICS_BOOST:
            return { &pe->blendCompositor->shader, SetupCymaticsTrailBoost, &pe->cymaticsBoostActive };
        case TRANSFORM_PARTICLE_LIFE_BOOST:
            return { &pe->blendCompositor->shader, SetupParticleLifeTrailBoost, &pe->particleLifeBoostActive };
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
    const float ms = pe->effects.motionScale;
    const FlowFieldConfig* ff = &pe->effects.flowField;

    SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                   &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);

    // Identity-centered values: scale deviation from 1.0
    float zoomEff = 1.0f + (ff->zoomBase - 1.0f) * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomBaseLoc,
                   &zoomEff, SHADER_UNIFORM_FLOAT);

    // Radial/angular zoom offsets: direct multiplication (additive modifiers)
    float zoomRadialEff = ff->zoomRadial * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomRadialLoc,
                   &zoomRadialEff, SHADER_UNIFORM_FLOAT);

    // Speed values: direct multiplication
    float rotBase = ff->rotationSpeed * pe->currentDeltaTime * ms;
    float rotRadial = ff->rotationSpeedRadial * pe->currentDeltaTime * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc,
                   &rotBase, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc,
                   &rotRadial, SHADER_UNIFORM_FLOAT);

    // Translation: direct multiplication
    float dxBaseEff = ff->dxBase * ms;
    float dxRadialEff = ff->dxRadial * ms;
    float dyBaseEff = ff->dyBase * ms;
    float dyRadialEff = ff->dyRadial * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackDxBaseLoc,
                   &dxBaseEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxRadialLoc,
                   &dxRadialEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyBaseLoc,
                   &dyBaseEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyRadialLoc,
                   &dyRadialEff, SHADER_UNIFORM_FLOAT);

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
    SetShaderValue(pe->feedbackShader, pe->feedbackCxLoc,
                   &ff->cx, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackCyLoc,
                   &ff->cy, SHADER_UNIFORM_FLOAT);

    // Directional stretch: identity-centered
    float sxEff = 1.0f + (ff->sx - 1.0f) * ms;
    float syEff = 1.0f + (ff->sy - 1.0f) * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackSxLoc,
                   &sxEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackSyLoc,
                   &syEff, SHADER_UNIFORM_FLOAT);

    // Angular modulation: treat as speeds (need deltaTime for frame-rate independence)
    float zoomAngularEff = ff->zoomAngular * pe->currentDeltaTime * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularLoc,
                   &zoomAngularEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularFreqLoc,
                   &ff->zoomAngularFreq, SHADER_UNIFORM_INT);
    float rotAngularEff = ff->rotAngular * pe->currentDeltaTime * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularLoc,
                   &rotAngularEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularFreqLoc,
                   &ff->rotAngularFreq, SHADER_UNIFORM_INT);
    float dxAngularEff = ff->dxAngular * pe->currentDeltaTime * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularLoc,
                   &dxAngularEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularFreqLoc,
                   &ff->dxAngularFreq, SHADER_UNIFORM_INT);
    float dyAngularEff = ff->dyAngular * pe->currentDeltaTime * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularLoc,
                   &dyAngularEff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularFreqLoc,
                   &ff->dyAngularFreq, SHADER_UNIFORM_INT);

    // Procedural warp: scale displacement intensity
    float warpEff = pe->effects.proceduralWarp.warp * ms;
    SetShaderValue(pe->feedbackShader, pe->feedbackWarpLoc,
                   &warpEff, SHADER_UNIFORM_FLOAT);
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
    // Decay compensation: increase halfLife proportionally to motion slowdown
    float safeMotionScale = fmaxf(pe->effects.motionScale, 0.01f);
    float effectiveHalfLife = pe->effects.halfLife / safeMotionScale;
    SetShaderValue(pe->blurVShader, pe->halfLifeLoc,
                   &effectiveHalfLife, SHADER_UNIFORM_FLOAT);
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

void SetupParticleLifeTrailBoost(PostEffect* pe)
{
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->particleLife->trailMap),
                         pe->effects.particleLife.boostIntensity,
                         pe->effects.particleLife.blendMode);
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
    SetShaderValue(pe->kaleidoShader, pe->kaleidoTwistLoc,
                   &k->twistAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kaleidoShader, pe->kaleidoSmoothingLoc,
                   &k->smoothing, SHADER_UNIFORM_FLOAT);
}

void SetupKifs(PostEffect* pe)
{
    const KifsConfig* k = &pe->effects.kifs;

    SetShaderValue(pe->kifsShader, pe->kifsRotationLoc,
                   &pe->currentKifsRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->kifsShader, pe->kifsTwistLoc,
                   &pe->currentKifsTwist, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->kifsShader, pe->kifsPolarFoldSmoothingLoc,
                   &k->polarFoldSmoothing, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->latticeFoldShader, pe->latticeFoldSmoothingLoc,
                   &l->smoothing, SHADER_UNIFORM_FLOAT);
}

void SetupSineWarp(PostEffect* pe)
{
    const SineWarpConfig* sw = &pe->effects.sineWarp;
    SetShaderValue(pe->sineWarpShader, pe->sineWarpTimeLoc, &pe->sineWarpTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpRotationLoc, &pe->sineWarpTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpOctavesLoc, &sw->octaves, SHADER_UNIFORM_INT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpStrengthLoc, &sw->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->sineWarpShader, pe->sineWarpOctaveRotationLoc, &sw->octaveRotation, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->textureWarpShader, pe->textureWarpRidgeAngleLoc,
                   &tw->ridgeAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->textureWarpShader, pe->textureWarpAnisotropyLoc,
                   &tw->anisotropy, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->textureWarpShader, pe->textureWarpNoiseAmountLoc,
                   &tw->noiseAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->textureWarpShader, pe->textureWarpNoiseScaleLoc,
                   &tw->noiseScale, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->waveRippleShader, pe->waveRippleDecayLoc,
                   &wr->decay, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->waveRippleShader, pe->waveRippleCenterHoleLoc,
                   &wr->centerHole, SHADER_UNIFORM_FLOAT);
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

    // Datamosh
    int datamoshEnabled = g->datamoshEnabled ? 1 : 0;
    SetShaderValue(pe->glitchShader, pe->glitchDatamoshEnabledLoc,
                   &datamoshEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(pe->glitchShader, pe->glitchDatamoshIntensityLoc,
                   &g->datamoshIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchDatamoshMinLoc,
                   &g->datamoshMin, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchDatamoshMaxLoc,
                   &g->datamoshMax, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->glitchShader, pe->glitchColSliceRowsLoc,
                   &g->colSliceRows, SHADER_UNIFORM_FLOAT);

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
    float blockMaskTint[3] = { g->blockMaskTintR, g->blockMaskTintG, g->blockMaskTintB };
    SetShaderValue(pe->glitchShader, pe->glitchBlockMaskTintLoc,
                   blockMaskTint, SHADER_UNIFORM_VEC3);

    // Temporal Jitter
    int temporalJitterEnabled = g->temporalJitterEnabled ? 1 : 0;
    SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterEnabledLoc,
                   &temporalJitterEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterAmountLoc,
                   &g->temporalJitterAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->glitchShader, pe->glitchTemporalJitterGateLoc,
                   &g->temporalJitterGate, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowEdgeWeightLoc,
                   &gf->edgeWeight, SHADER_UNIFORM_FLOAT);
    int randomDir = gf->randomDirection ? 1 : 0;
    SetShaderValue(pe->gradientFlowShader, pe->gradientFlowRandomDirectionLoc,
                   &randomDir, SHADER_UNIFORM_INT);
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
    SetShaderValue(pe->oilPaintShader, pe->oilPaintSpecularLoc,
                   &op->specular, SHADER_UNIFORM_FLOAT);
}

void ApplyOilPaintStrokePass(PostEffect* pe, RenderTexture2D* source)
{
    const OilPaintConfig* op = &pe->effects.oilPaint;
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintBrushSizeLoc,
                   &op->brushSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeBendLoc,
                   &op->strokeBend, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintLayersLoc,
                   &op->layers, SHADER_UNIFORM_INT);
    SetShaderValueTexture(pe->oilPaintStrokeShader, pe->oilPaintNoiseTexLoc,
                          pe->oilPaintNoiseTex);

    BeginTextureMode(pe->oilPaintIntermediate);
    BeginShaderMode(pe->oilPaintStrokeShader);
    RenderUtilsDrawFullscreenQuad(source->texture, pe->screenWidth, pe->screenHeight);
    EndShaderMode();
    EndTextureMode();
}

void SetupWatercolor(PostEffect* pe)
{
    const WatercolorConfig* wc = &pe->effects.watercolor;
    SetShaderValue(pe->watercolorShader, pe->watercolorSamplesLoc,
                   &wc->samples, SHADER_UNIFORM_INT);
    SetShaderValue(pe->watercolorShader, pe->watercolorStrokeStepLoc,
                   &wc->strokeStep, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorWashStrengthLoc,
                   &wc->washStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorPaperScaleLoc,
                   &wc->paperScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->watercolorShader, pe->watercolorPaperStrengthLoc,
                   &wc->paperStrength, SHADER_UNIFORM_FLOAT);
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
    SetShaderValue(pe->neonGlowShader, pe->neonGlowColorModeLoc,
                   &ng->colorMode, SHADER_UNIFORM_INT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowSaturationBoostLoc,
                   &ng->saturationBoost, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->neonGlowShader, pe->neonGlowBrightnessBoostLoc,
                   &ng->brightnessBoost, SHADER_UNIFORM_FLOAT);
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

void SetupFalseColor(PostEffect* pe)
{
    const FalseColorConfig* fc = &pe->effects.falseColor;

    ColorLUTUpdate(pe->falseColorLUT, &fc->gradient);

    SetShaderValue(pe->falseColorShader, pe->falseColorIntensityLoc,
                   &fc->intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(pe->falseColorShader, pe->falseColorGradientLUTLoc,
                          ColorLUTGetTexture(pe->falseColorLUT));
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

void SetupCrossHatching(PostEffect* pe)
{
    const CrossHatchingConfig* ch = &pe->effects.crossHatching;

    // CPU time accumulation for temporal stutter
    pe->crossHatchingTime += pe->currentDeltaTime;

    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingTimeLoc,
                   &pe->crossHatchingTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingWidthLoc,
                   &ch->width, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingThresholdLoc,
                   &ch->threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingNoiseLoc,
                   &ch->noise, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->crossHatchingShader, pe->crossHatchingOutlineLoc,
                   &ch->outline, SHADER_UNIFORM_FLOAT);
}

void SetupPaletteQuantization(PostEffect* pe)
{
    const PaletteQuantizationConfig* pq = &pe->effects.paletteQuantization;
    SetShaderValue(pe->paletteQuantizationShader, pe->paletteQuantizationColorLevelsLoc,
                   &pq->colorLevels, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->paletteQuantizationShader, pe->paletteQuantizationDitherStrengthLoc,
                   &pq->ditherStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->paletteQuantizationShader, pe->paletteQuantizationBayerSizeLoc,
                   &pq->bayerSize, SHADER_UNIFORM_INT);
}

void SetupBokeh(PostEffect* pe)
{
    const BokehConfig* b = &pe->effects.bokeh;
    SetShaderValue(pe->bokehShader, pe->bokehRadiusLoc,
                   &b->radius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->bokehShader, pe->bokehIterationsLoc,
                   &b->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->bokehShader, pe->bokehBrightnessPowerLoc,
                   &b->brightnessPower, SHADER_UNIFORM_FLOAT);
}

void SetupBloom(PostEffect* pe)
{
    const BloomConfig* b = &pe->effects.bloom;
    SetShaderValue(pe->bloomCompositeShader, pe->bloomIntensityLoc,
                   &b->intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(pe->bloomCompositeShader, pe->bloomBloomTexLoc,
                          pe->bloomMips[0].texture);
}

void SetupMandelbox(PostEffect* pe)
{
    const MandelboxConfig* m = &pe->effects.mandelbox;
    SetShaderValue(pe->mandelboxShader, pe->mandelboxIterationsLoc,
                   &m->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxBoxLimitLoc,
                   &m->boxLimit, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereMinLoc,
                   &m->sphereMin, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereMaxLoc,
                   &m->sphereMax, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxScaleLoc,
                   &m->scale, SHADER_UNIFORM_FLOAT);
    const float offset[2] = { m->offsetX, m->offsetY };
    SetShaderValue(pe->mandelboxShader, pe->mandelboxOffsetLoc,
                   offset, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxRotationLoc,
                   &pe->currentMandelboxRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxTwistAngleLoc,
                   &pe->currentMandelboxTwist, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxBoxIntensityLoc,
                   &m->boxIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereIntensityLoc,
                   &m->sphereIntensity, SHADER_UNIFORM_FLOAT);
    const int polarFold = m->polarFold ? 1 : 0;
    SetShaderValue(pe->mandelboxShader, pe->mandelboxPolarFoldLoc,
                   &polarFold, SHADER_UNIFORM_INT);
    SetShaderValue(pe->mandelboxShader, pe->mandelboxPolarFoldSegmentsLoc,
                   &m->polarFoldSegments, SHADER_UNIFORM_INT);
}

void SetupTriangleFold(PostEffect* pe)
{
    const TriangleFoldConfig* t = &pe->effects.triangleFold;
    SetShaderValue(pe->triangleFoldShader, pe->triangleFoldIterationsLoc,
                   &t->iterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->triangleFoldShader, pe->triangleFoldScaleLoc,
                   &t->scale, SHADER_UNIFORM_FLOAT);
    const float offset[2] = { t->offsetX, t->offsetY };
    SetShaderValue(pe->triangleFoldShader, pe->triangleFoldOffsetLoc,
                   offset, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->triangleFoldShader, pe->triangleFoldRotationLoc,
                   &pe->currentTriangleFoldRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->triangleFoldShader, pe->triangleFoldTwistAngleLoc,
                   &pe->currentTriangleFoldTwist, SHADER_UNIFORM_FLOAT);
}

void SetupDomainWarp(PostEffect* pe)
{
    const DomainWarpConfig* dw = &pe->effects.domainWarp;
    SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpStrengthLoc,
                   &dw->warpStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpScaleLoc,
                   &dw->warpScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->domainWarpShader, pe->domainWarpWarpIterationsLoc,
                   &dw->warpIterations, SHADER_UNIFORM_INT);
    SetShaderValue(pe->domainWarpShader, pe->domainWarpFalloffLoc,
                   &dw->falloff, SHADER_UNIFORM_FLOAT);
    float timeOffset[2] = {
        cosf(dw->driftAngle) * pe->domainWarpDrift,
        sinf(dw->driftAngle) * pe->domainWarpDrift
    };
    SetShaderValue(pe->domainWarpShader, pe->domainWarpTimeOffsetLoc,
                   timeOffset, SHADER_UNIFORM_VEC2);
}

static const float GOLDEN_ANGLE = 2.39996322972865f;

void SetupPhyllotaxis(PostEffect* pe)
{
    const PhyllotaxisConfig* ph = &pe->effects.phyllotaxis;

    // Compute divergence angle from base golden angle + animated offset
    float divergenceAngle = GOLDEN_ANGLE + pe->phyllotaxisAngleTime;

    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisScaleLoc,
                   &ph->scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisDivergenceAngleLoc,
                   &divergenceAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisPhaseTimeLoc,
                   &pe->phyllotaxisPhaseTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisCellRadiusLoc,
                   &ph->cellRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisIsoFrequencyLoc,
                   &ph->isoFrequency, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisUvDistortIntensityLoc,
                   &ph->uvDistortIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisOrganicFlowIntensityLoc,
                   &ph->organicFlowIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisEdgeIsoIntensityLoc,
                   &ph->edgeIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisCenterIsoIntensityLoc,
                   &ph->centerIsoIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisFlatFillIntensityLoc,
                   &ph->flatFillIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisEdgeGlowIntensityLoc,
                   &ph->edgeGlowIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisRatioIntensityLoc,
                   &ph->ratioIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisDeterminantIntensityLoc,
                   &ph->determinantIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisEdgeDetectIntensityLoc,
                   &ph->edgeDetectIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->phyllotaxisShader, pe->phyllotaxisSpinOffsetLoc,
                   &pe->phyllotaxisSpinOffset, SHADER_UNIFORM_FLOAT);
}

void SetupMoireInterference(PostEffect* pe)
{
    const MoireInterferenceConfig* mi = &pe->effects.moireInterference;

    // CPU rotation accumulation for smooth animation
    pe->moireInterferenceRotationAccum += mi->animationSpeed * pe->currentDeltaTime;

    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceRotationAngleLoc,
                   &mi->rotationAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceScaleDiffLoc,
                   &mi->scaleDiff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceLayersLoc,
                   &mi->layers, SHADER_UNIFORM_INT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceBlendModeLoc,
                   &mi->blendMode, SHADER_UNIFORM_INT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceCenterXLoc,
                   &mi->centerX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceCenterYLoc,
                   &mi->centerY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->moireInterferenceShader, pe->moireInterferenceRotationAccumLoc,
                   &pe->moireInterferenceRotationAccum, SHADER_UNIFORM_FLOAT);
}

void SetupDensityWaveSpiral(PostEffect* pe)
{
    const DensityWaveSpiralConfig* dws = &pe->effects.densityWaveSpiral;
    float center[2] = { dws->centerX, dws->centerY };
    float aspect[2] = { dws->aspectX, dws->aspectY };
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralCenterLoc,
                   center, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralAspectLoc,
                   aspect, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralTightnessLoc,
                   &dws->tightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralRotationAccumLoc,
                   &pe->densityWaveSpiralRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralGlobalRotationAccumLoc,
                   &pe->densityWaveSpiralGlobalRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralThicknessLoc,
                   &dws->thickness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralRingCountLoc,
                   &dws->ringCount, SHADER_UNIFORM_INT);
    SetShaderValue(pe->densityWaveSpiralShader, pe->densityWaveSpiralFalloffLoc,
                   &dws->falloff, SHADER_UNIFORM_FLOAT);
}

void SetupPencilSketch(PostEffect* pe)
{
    const PencilSketchConfig* ps = &pe->effects.pencilSketch;

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

void SetupMatrixRain(PostEffect* pe)
{
    const MatrixRainConfig* cfg = &pe->effects.matrixRain;

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

void SetupImpressionist(PostEffect* pe)
{
    const ImpressionistConfig* cfg = &pe->effects.impressionist;
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

void SetupKuwahara(PostEffect* pe)
{
    const KuwaharaConfig* k = &pe->effects.kuwahara;
    int radius = (int)k->radius;
    SetShaderValue(pe->kuwaharaShader, pe->kuwaharaRadiusLoc,
                   &radius, SHADER_UNIFORM_INT);
}

void SetupInkWash(PostEffect* pe)
{
    const InkWashConfig* iw = &pe->effects.inkWash;
    SetShaderValue(pe->inkWashShader, pe->inkWashStrengthLoc,
                   &iw->strength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->inkWashShader, pe->inkWashGranulationLoc,
                   &iw->granulation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->inkWashShader, pe->inkWashBleedStrengthLoc,
                   &iw->bleedStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->inkWashShader, pe->inkWashBleedRadiusLoc,
                   &iw->bleedRadius, SHADER_UNIFORM_FLOAT);
    int softness = (int)iw->softness;
    SetShaderValue(pe->inkWashShader, pe->inkWashSoftnessLoc,
                   &softness, SHADER_UNIFORM_INT);
}

void SetupDiscoBall(PostEffect* pe)
{
    const DiscoBallConfig* db = &pe->effects.discoBall;

    // Accumulate rotation angle
    pe->discoBallAngle += db->rotationSpeed * pe->currentDeltaTime;

    SetShaderValue(pe->discoBallShader, pe->discoBallSphereRadiusLoc,
                   &db->sphereRadius, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallTileSizeLoc,
                   &db->tileSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallSphereAngleLoc,
                   &pe->discoBallAngle, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallBumpHeightLoc,
                   &db->bumpHeight, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallReflectIntensityLoc,
                   &db->reflectIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallSpotIntensityLoc,
                   &db->spotIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallSpotSizeLoc,
                   &db->spotSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallSpotFalloffLoc,
                   &db->spotFalloff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallBrightnessThresholdLoc,
                   &db->brightnessThreshold, SHADER_UNIFORM_FLOAT);
}

static void BloomRenderPass(RenderTexture2D* source, RenderTexture2D* dest, Shader shader)
{
    BeginTextureMode(*dest);
    BeginShaderMode(shader);
    DrawTexturePro(source->texture,
                   { 0, 0, (float)source->texture.width, (float)-source->texture.height },
                   { 0, 0, (float)dest->texture.width, (float)dest->texture.height },
                   { 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

void ApplyBloomPasses(PostEffect* pe, RenderTexture2D* source, int* writeIdx)
{
    const BloomConfig* b = &pe->effects.bloom;
    int iterations = b->iterations;
    if (iterations < 1) { iterations = 1; }
    if (iterations > BLOOM_MIP_COUNT) { iterations = BLOOM_MIP_COUNT; }

    // Prefilter: extract bright pixels from source to mip[0]
    SetShaderValue(pe->bloomPrefilterShader, pe->bloomThresholdLoc,
                   &b->threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->bloomPrefilterShader, pe->bloomKneeLoc,
                   &b->knee, SHADER_UNIFORM_FLOAT);
    BloomRenderPass(source, &pe->bloomMips[0], pe->bloomPrefilterShader);

    // Downsample: mip[0] → mip[1] → ... → mip[iterations-1]
    for (int i = 1; i < iterations; i++) {
        float halfpixel[2] = {
            0.5f / (float)pe->bloomMips[i - 1].texture.width,
            0.5f / (float)pe->bloomMips[i - 1].texture.height
        };
        SetShaderValue(pe->bloomDownsampleShader, pe->bloomDownsampleHalfpixelLoc,
                       halfpixel, SHADER_UNIFORM_VEC2);
        BloomRenderPass(&pe->bloomMips[i - 1], &pe->bloomMips[i], pe->bloomDownsampleShader);
    }

    // Upsample: mip[iterations-1] → ... → mip[0] (additive blend at each level)
    for (int i = iterations - 1; i > 0; i--) {
        float halfpixel[2] = {
            0.5f / (float)pe->bloomMips[i].texture.width,
            0.5f / (float)pe->bloomMips[i].texture.height
        };
        SetShaderValue(pe->bloomUpsampleShader, pe->bloomUpsampleHalfpixelLoc,
                       halfpixel, SHADER_UNIFORM_VEC2);

        // Upsample mip[i] and add to mip[i-1]
        BeginTextureMode(pe->bloomMips[i - 1]);
        BeginBlendMode(BLEND_ADDITIVE);
        BeginShaderMode(pe->bloomUpsampleShader);
        DrawTexturePro(pe->bloomMips[i].texture,
                       { 0, 0, (float)pe->bloomMips[i].texture.width, (float)-pe->bloomMips[i].texture.height },
                       { 0, 0, (float)pe->bloomMips[i - 1].texture.width, (float)pe->bloomMips[i - 1].texture.height },
                       { 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
        EndBlendMode();
        EndTextureMode();
    }

    // Final composite uses SetupBloom to bind uniforms, called by render_pipeline
}

void ApplyHalfResEffect(PostEffect* pe, RenderTexture2D* source, int* writeIdx, Shader shader, RenderPipelineShaderSetupFn setup)
{
    int halfW = pe->screenWidth / 2;
    int halfH = pe->screenHeight / 2;
    Rectangle srcRect = { 0, 0, (float)source->texture.width, (float)-source->texture.height };
    Rectangle halfRect = { 0, 0, (float)halfW, (float)halfH };
    Rectangle fullRect = { 0, 0, (float)pe->screenWidth, (float)pe->screenHeight };

    BeginTextureMode(pe->halfResA);
    DrawTexturePro(source->texture, srcRect, halfRect, { 0, 0 }, 0.0f, WHITE);
    EndTextureMode();

    int resLoc = GetShaderLocation(shader, "resolution");
    float halfRes[2] = { (float)halfW, (float)halfH };
    if (resLoc >= 0) {
        SetShaderValue(shader, resLoc, halfRes, SHADER_UNIFORM_VEC2);
    }

    if (setup != NULL) {
        setup(pe);
    }
    BeginTextureMode(pe->halfResB);
    BeginShaderMode(shader);
    DrawTexturePro(pe->halfResA.texture,
                   { 0, 0, (float)halfW, (float)-halfH },
                   halfRect, { 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Subsequent effects may share this shader
    if (resLoc >= 0) {
        float fullRes[2] = { (float)pe->screenWidth, (float)pe->screenHeight };
        SetShaderValue(shader, resLoc, fullRes, SHADER_UNIFORM_VEC2);
    }

    BeginTextureMode(pe->pingPong[*writeIdx]);
    DrawTexturePro(pe->halfResB.texture,
                   { 0, 0, (float)halfW, (float)-halfH },
                   fullRect, { 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
}

void ApplyHalfResOilPaint(PostEffect* pe, RenderTexture2D* source, int* writeIdx)
{
    int halfW = pe->screenWidth / 2;
    int halfH = pe->screenHeight / 2;
    Rectangle srcRect = { 0, 0, (float)source->texture.width, (float)-source->texture.height };
    Rectangle halfRect = { 0, 0, (float)halfW, (float)halfH };
    Rectangle fullRect = { 0, 0, (float)pe->screenWidth, (float)pe->screenHeight };
    float halfRes[2] = { (float)halfW, (float)halfH };
    float fullRes[2] = { (float)pe->screenWidth, (float)pe->screenHeight };

    BeginTextureMode(pe->halfResA);
    DrawTexturePro(source->texture, srcRect, halfRect, { 0, 0 }, 0.0f, WHITE);
    EndTextureMode();

    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeResolutionLoc, halfRes, SHADER_UNIFORM_VEC2);

    const OilPaintConfig* op = &pe->effects.oilPaint;
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintBrushSizeLoc, &op->brushSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeBendLoc, &op->strokeBend, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintLayersLoc, &op->layers, SHADER_UNIFORM_INT);
    SetShaderValueTexture(pe->oilPaintStrokeShader, pe->oilPaintNoiseTexLoc, pe->oilPaintNoiseTex);

    BeginTextureMode(pe->halfResB);
    BeginShaderMode(pe->oilPaintStrokeShader);
    DrawTexturePro(pe->halfResA.texture,
                   { 0, 0, (float)halfW, (float)-halfH },
                   halfRect, { 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    SetShaderValue(pe->oilPaintShader, pe->oilPaintResolutionLoc, halfRes, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->oilPaintShader, pe->oilPaintSpecularLoc, &op->specular, SHADER_UNIFORM_FLOAT);

    BeginTextureMode(pe->halfResA);
    BeginShaderMode(pe->oilPaintShader);
    DrawTexturePro(pe->halfResB.texture,
                   { 0, 0, (float)halfW, (float)-halfH },
                   halfRect, { 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Subsequent effects may share these shaders
    SetShaderValue(pe->oilPaintStrokeShader, pe->oilPaintStrokeResolutionLoc, fullRes, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->oilPaintShader, pe->oilPaintResolutionLoc, fullRes, SHADER_UNIFORM_VEC2);

    BeginTextureMode(pe->pingPong[*writeIdx]);
    DrawTexturePro(pe->halfResA.texture,
                   { 0, 0, (float)halfW, (float)-halfH },
                   fullRect, { 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
}
