#include "shader_setup_symmetry.h"
#include "config/radial_ifs_config.h"
#include "post_effect.h"

void SetupKaleido(PostEffect *pe) {
  const KaleidoscopeConfig *k = &pe->effects.kaleidoscope;

  SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc, &k->segments,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                 &pe->currentKaleidoRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->kaleidoShader, pe->kaleidoTwistLoc, &k->twistAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->kaleidoShader, pe->kaleidoSmoothingLoc, &k->smoothing,
                 SHADER_UNIFORM_FLOAT);
}

void SetupKifs(PostEffect *pe) {
  const KifsConfig *k = &pe->effects.kifs;

  SetShaderValue(pe->kifsShader, pe->kifsRotationLoc, &pe->currentKifsRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->kifsShader, pe->kifsTwistLoc, &pe->currentKifsTwist,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->kifsShader, pe->kifsIterationsLoc, &k->iterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->kifsShader, pe->kifsScaleLoc, &k->scale,
                 SHADER_UNIFORM_FLOAT);
  const float kifsOffset[2] = {k->offsetX, k->offsetY};
  SetShaderValue(pe->kifsShader, pe->kifsOffsetLoc, kifsOffset,
                 SHADER_UNIFORM_VEC2);
  const int octantFold = k->octantFold ? 1 : 0;
  SetShaderValue(pe->kifsShader, pe->kifsOctantFoldLoc, &octantFold,
                 SHADER_UNIFORM_INT);
  const int polarFold = k->polarFold ? 1 : 0;
  SetShaderValue(pe->kifsShader, pe->kifsPolarFoldLoc, &polarFold,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->kifsShader, pe->kifsPolarFoldSegmentsLoc,
                 &k->polarFoldSegments, SHADER_UNIFORM_INT);
  SetShaderValue(pe->kifsShader, pe->kifsPolarFoldSmoothingLoc,
                 &k->polarFoldSmoothing, SHADER_UNIFORM_FLOAT);
}

void SetupPoincareDisk(PostEffect *pe) {
  const PoincareDiskConfig *pd = &pe->effects.poincareDisk;
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTilePLoc, &pd->tileP,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTileQLoc, &pd->tileQ,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTileRLoc, &pd->tileR,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskTranslationLoc,
                 pe->currentPoincareTranslation, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskRotationLoc,
                 &pe->currentPoincareRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->poincareDiskShader, pe->poincareDiskDiskScaleLoc,
                 &pd->diskScale, SHADER_UNIFORM_FLOAT);
}

void SetupMandelbox(PostEffect *pe) {
  const MandelboxConfig *m = &pe->effects.mandelbox;
  SetShaderValue(pe->mandelboxShader, pe->mandelboxIterationsLoc,
                 &m->iterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxBoxLimitLoc, &m->boxLimit,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereMinLoc, &m->sphereMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereMaxLoc, &m->sphereMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxScaleLoc, &m->scale,
                 SHADER_UNIFORM_FLOAT);
  const float offset[2] = {m->offsetX, m->offsetY};
  SetShaderValue(pe->mandelboxShader, pe->mandelboxOffsetLoc, offset,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxRotationLoc,
                 &pe->currentMandelboxRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxTwistAngleLoc,
                 &pe->currentMandelboxTwist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxBoxIntensityLoc,
                 &m->boxIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxSphereIntensityLoc,
                 &m->sphereIntensity, SHADER_UNIFORM_FLOAT);
  const int polarFold = m->polarFold ? 1 : 0;
  SetShaderValue(pe->mandelboxShader, pe->mandelboxPolarFoldLoc, &polarFold,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->mandelboxShader, pe->mandelboxPolarFoldSegmentsLoc,
                 &m->polarFoldSegments, SHADER_UNIFORM_INT);
}

void SetupTriangleFold(PostEffect *pe) {
  const TriangleFoldConfig *t = &pe->effects.triangleFold;
  SetShaderValue(pe->triangleFoldShader, pe->triangleFoldIterationsLoc,
                 &t->iterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->triangleFoldShader, pe->triangleFoldScaleLoc, &t->scale,
                 SHADER_UNIFORM_FLOAT);
  const float offset[2] = {t->offsetX, t->offsetY};
  SetShaderValue(pe->triangleFoldShader, pe->triangleFoldOffsetLoc, offset,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->triangleFoldShader, pe->triangleFoldRotationLoc,
                 &pe->currentTriangleFoldRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->triangleFoldShader, pe->triangleFoldTwistAngleLoc,
                 &pe->currentTriangleFoldTwist, SHADER_UNIFORM_FLOAT);
}

void SetupMoireInterference(PostEffect *pe) {
  const MoireInterferenceConfig *mi = &pe->effects.moireInterference;

  // CPU rotation accumulation for smooth animation
  pe->moireInterferenceRotationAccum +=
      mi->animationSpeed * pe->currentDeltaTime;

  SetShaderValue(pe->moireInterferenceShader,
                 pe->moireInterferenceRotationAngleLoc, &mi->rotationAngle,
                 SHADER_UNIFORM_FLOAT);
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
  SetShaderValue(pe->moireInterferenceShader,
                 pe->moireInterferenceRotationAccumLoc,
                 &pe->moireInterferenceRotationAccum, SHADER_UNIFORM_FLOAT);
}

void SetupRadialIfs(PostEffect *pe) {
  const RadialIfsConfig *r = &pe->effects.radialIfs;

  // CPU rotation accumulation
  pe->currentRadialIfsRotation += r->rotationSpeed * pe->currentDeltaTime;
  pe->currentRadialIfsTwist += r->twistSpeed * pe->currentDeltaTime;

  SetShaderValue(pe->radialIfsShader, pe->radialIfsSegmentsLoc, &r->segments,
                 SHADER_UNIFORM_INT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsIterationsLoc,
                 &r->iterations, SHADER_UNIFORM_INT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsScaleLoc, &r->scale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsOffsetLoc, &r->offset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsRotationLoc,
                 &pe->currentRadialIfsRotation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsTwistAngleLoc,
                 &pe->currentRadialIfsTwist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->radialIfsShader, pe->radialIfsSmoothingLoc, &r->smoothing,
                 SHADER_UNIFORM_FLOAT);
}
