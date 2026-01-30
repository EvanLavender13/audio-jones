#include "shader_setup_cellular.h"
#include "post_effect.h"
#include <math.h>

static const float GOLDEN_ANGLE = 2.39996322972865f;

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
    SetShaderValue(pe->discoBallShader, pe->discoBallSpotFalloffLoc,
                   &db->spotFalloff, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->discoBallShader, pe->discoBallBrightnessThresholdLoc,
                   &db->brightnessThreshold, SHADER_UNIFORM_FLOAT);
}
