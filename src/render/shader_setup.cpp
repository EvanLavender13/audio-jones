#include "shader_setup.h"
#include "blend_compositor.h"
#include "color_lut.h"
#include "post_effect.h"
#include "shader_setup_artistic.h"
#include "shader_setup_cellular.h"
#include "shader_setup_generators.h"
#include "shader_setup_graphic.h"
#include "shader_setup_motion.h"
#include "shader_setup_optical.h"
#include "shader_setup_retro.h"
#include "shader_setup_warp.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"
#include "simulation/trail_map.h"
#include <math.h>

TransformEffectEntry GetTransformEffect(PostEffect *pe,
                                        TransformEffectType type) {
  switch (type) {
  case TRANSFORM_SINE_WARP:
    return {&pe->sineWarp.shader, SetupSineWarp, &pe->effects.sineWarp.enabled};
  case TRANSFORM_KALEIDOSCOPE:
    return {&pe->kaleidoscope.shader, SetupKaleido,
            &pe->effects.kaleidoscope.enabled};
  case TRANSFORM_INFINITE_ZOOM:
    return {&pe->infiniteZoom.shader, SetupInfiniteZoom,
            &pe->effects.infiniteZoom.enabled};
  case TRANSFORM_RADIAL_STREAK:
    return {&pe->radialStreak.shader, SetupRadialStreak,
            &pe->effects.radialStreak.enabled};
  case TRANSFORM_TEXTURE_WARP:
    return {&pe->textureWarp.shader, SetupTextureWarp,
            &pe->effects.textureWarp.enabled};
  case TRANSFORM_VORONOI:
    return {&pe->voronoi.shader, SetupVoronoi, &pe->effects.voronoi.enabled};
  case TRANSFORM_WAVE_RIPPLE:
    return {&pe->waveRipple.shader, SetupWaveRipple,
            &pe->effects.waveRipple.enabled};
  case TRANSFORM_MOBIUS:
    return {&pe->mobius.shader, SetupMobius, &pe->effects.mobius.enabled};
  case TRANSFORM_PIXELATION:
    return {&pe->pixelation.shader, SetupPixelation,
            &pe->effects.pixelation.enabled};
  case TRANSFORM_GLITCH:
    return {&pe->glitch.shader, SetupGlitch, &pe->effects.glitch.enabled};
  case TRANSFORM_POINCARE_DISK:
    return {&pe->poincareDisk.shader, SetupPoincareDisk,
            &pe->effects.poincareDisk.enabled};
  case TRANSFORM_TOON:
    return {&pe->toon.shader, SetupToon, &pe->effects.toon.enabled};
  case TRANSFORM_HEIGHTFIELD_RELIEF:
    return {&pe->heightfieldRelief.shader, SetupHeightfieldRelief,
            &pe->effects.heightfieldRelief.enabled};
  case TRANSFORM_GRADIENT_FLOW:
    return {&pe->gradientFlow.shader, SetupGradientFlow,
            &pe->effects.gradientFlow.enabled};
  case TRANSFORM_DROSTE_ZOOM:
    return {&pe->drosteZoom.shader, SetupDrosteZoom,
            &pe->effects.drosteZoom.enabled};
  case TRANSFORM_KIFS:
    return {&pe->kifs.shader, SetupKifs, &pe->effects.kifs.enabled};
  case TRANSFORM_LATTICE_FOLD:
    return {&pe->latticeFold.shader, SetupLatticeFold,
            &pe->effects.latticeFold.enabled};
  case TRANSFORM_COLOR_GRADE:
    return {&pe->colorGrade.shader, SetupColorGrade,
            &pe->effects.colorGrade.enabled};
  case TRANSFORM_ASCII_ART:
    return {&pe->asciiArt.shader, SetupAsciiArt, &pe->effects.asciiArt.enabled};
  case TRANSFORM_OIL_PAINT:
    return {&pe->oilPaint.compositeShader, SetupOilPaint,
            &pe->effects.oilPaint.enabled};
  case TRANSFORM_WATERCOLOR:
    return {&pe->watercolor.shader, SetupWatercolor,
            &pe->effects.watercolor.enabled};
  case TRANSFORM_NEON_GLOW:
    return {&pe->neonGlow.shader, SetupNeonGlow, &pe->effects.neonGlow.enabled};
  case TRANSFORM_RADIAL_PULSE:
    return {&pe->radialPulse.shader, SetupRadialPulse,
            &pe->effects.radialPulse.enabled};
  case TRANSFORM_FALSE_COLOR:
    return {&pe->falseColor.shader, SetupFalseColor,
            &pe->effects.falseColor.enabled};
  case TRANSFORM_HALFTONE:
    return {&pe->halftone.shader, SetupHalftone, &pe->effects.halftone.enabled};
  case TRANSFORM_CHLADNI_WARP:
    return {&pe->chladniWarp.shader, SetupChladniWarp,
            &pe->effects.chladniWarp.enabled};
  case TRANSFORM_CROSS_HATCHING:
    return {&pe->crossHatching.shader, SetupCrossHatching,
            &pe->effects.crossHatching.enabled};
  case TRANSFORM_PALETTE_QUANTIZATION:
    return {&pe->paletteQuantization.shader, SetupPaletteQuantization,
            &pe->effects.paletteQuantization.enabled};
  case TRANSFORM_BOKEH:
    return {&pe->bokeh.shader, SetupBokeh, &pe->effects.bokeh.enabled};
  case TRANSFORM_BLOOM:
    return {&pe->bloom.compositeShader, SetupBloom, &pe->effects.bloom.enabled};
  case TRANSFORM_ANAMORPHIC_STREAK:
    return {&pe->anamorphicStreak.compositeShader, SetupAnamorphicStreak,
            &pe->effects.anamorphicStreak.enabled};
  case TRANSFORM_MANDELBOX:
    return {&pe->mandelbox.shader, SetupMandelbox,
            &pe->effects.mandelbox.enabled};
  case TRANSFORM_TRIANGLE_FOLD:
    return {&pe->triangleFold.shader, SetupTriangleFold,
            &pe->effects.triangleFold.enabled};
  case TRANSFORM_RADIAL_IFS:
    return {&pe->radialIfs.shader, SetupRadialIfs,
            &pe->effects.radialIfs.enabled};
  case TRANSFORM_DOMAIN_WARP:
    return {&pe->domainWarp.shader, SetupDomainWarp,
            &pe->effects.domainWarp.enabled};
  case TRANSFORM_PHYLLOTAXIS:
    return {&pe->phyllotaxis.shader, SetupPhyllotaxis,
            &pe->effects.phyllotaxis.enabled};
  case TRANSFORM_MULTI_SCALE_GRID:
    return {&pe->multiScaleGrid.shader, SetupMultiScaleGrid,
            &pe->effects.multiScaleGrid.enabled};
  case TRANSFORM_DENSITY_WAVE_SPIRAL:
    return {&pe->densityWaveSpiral.shader, SetupDensityWaveSpiral,
            &pe->effects.densityWaveSpiral.enabled};
  case TRANSFORM_RELATIVISTIC_DOPPLER:
    return {&pe->relativisticDoppler.shader, SetupRelativisticDoppler,
            &pe->effects.relativisticDoppler.enabled};
  case TRANSFORM_MOIRE_INTERFERENCE:
    return {&pe->moireInterference.shader, SetupMoireInterference,
            &pe->effects.moireInterference.enabled};
  case TRANSFORM_PENCIL_SKETCH:
    return {&pe->pencilSketch.shader, SetupPencilSketch,
            &pe->effects.pencilSketch.enabled};
  case TRANSFORM_MATRIX_RAIN:
    return {&pe->matrixRain.shader, SetupMatrixRain,
            &pe->effects.matrixRain.enabled};
  case TRANSFORM_IMPRESSIONIST:
    return {&pe->impressionist.shader, SetupImpressionist,
            &pe->effects.impressionist.enabled};
  case TRANSFORM_KUWAHARA:
    return {&pe->kuwahara.shader, SetupKuwahara, &pe->effects.kuwahara.enabled};
  case TRANSFORM_INK_WASH:
    return {&pe->inkWash.shader, SetupInkWash, &pe->effects.inkWash.enabled};
  case TRANSFORM_LEGO_BRICKS:
    return {&pe->legoBricks.shader, SetupLegoBricks,
            &pe->effects.legoBricks.enabled};
  case TRANSFORM_DISCO_BALL:
    return {&pe->discoBall.shader, SetupDiscoBall,
            &pe->effects.discoBall.enabled};
  case TRANSFORM_SURFACE_WARP:
    return {&pe->surfaceWarp.shader, SetupSurfaceWarp,
            &pe->effects.surfaceWarp.enabled};
  case TRANSFORM_INTERFERENCE_WARP:
    return {&pe->interferenceWarp.shader, SetupInterferenceWarp,
            &pe->effects.interferenceWarp.enabled};
  case TRANSFORM_CORRIDOR_WARP:
    return {&pe->corridorWarp.shader, SetupCorridorWarp,
            &pe->effects.corridorWarp.enabled};
  case TRANSFORM_FFT_RADIAL_WARP:
    return {&pe->fftRadialWarp.shader, SetupFftRadialWarp,
            &pe->effects.fftRadialWarp.enabled};
  case TRANSFORM_CIRCUIT_BOARD:
    return {&pe->circuitBoard.shader, SetupCircuitBoard,
            &pe->effects.circuitBoard.enabled};
  case TRANSFORM_SYNTHWAVE:
    return {&pe->synthwave.shader, SetupSynthwave,
            &pe->effects.synthwave.enabled};
  case TRANSFORM_CRT:
    return {&pe->crt.shader, SetupCrt, &pe->effects.crt.enabled};
  case TRANSFORM_PHYSARUM_BOOST:
    return {&pe->blendCompositor->shader, SetupTrailBoost,
            &pe->physarumBoostActive};
  case TRANSFORM_CURL_FLOW_BOOST:
    return {&pe->blendCompositor->shader, SetupCurlFlowTrailBoost,
            &pe->curlFlowBoostActive};
  case TRANSFORM_CURL_ADVECTION_BOOST:
    return {&pe->blendCompositor->shader, SetupCurlAdvectionTrailBoost,
            &pe->curlAdvectionBoostActive};
  case TRANSFORM_ATTRACTOR_FLOW_BOOST:
    return {&pe->blendCompositor->shader, SetupAttractorFlowTrailBoost,
            &pe->attractorFlowBoostActive};
  case TRANSFORM_BOIDS_BOOST:
    return {&pe->blendCompositor->shader, SetupBoidsTrailBoost,
            &pe->boidsBoostActive};
  case TRANSFORM_CYMATICS_BOOST:
    return {&pe->blendCompositor->shader, SetupCymaticsTrailBoost,
            &pe->cymaticsBoostActive};
  case TRANSFORM_PARTICLE_LIFE_BOOST:
    return {&pe->blendCompositor->shader, SetupParticleLifeTrailBoost,
            &pe->particleLifeBoostActive};
  case TRANSFORM_CONSTELLATION_BLEND:
    return {&pe->blendCompositor->shader, SetupConstellationBlend,
            &pe->constellationBlendActive};
  case TRANSFORM_PLASMA_BLEND:
    return {&pe->blendCompositor->shader, SetupPlasmaBlend,
            &pe->plasmaBlendActive};
  case TRANSFORM_INTERFERENCE_BLEND:
    return {&pe->blendCompositor->shader, SetupInterferenceBlend,
            &pe->interferenceBlendActive};
  case TRANSFORM_SOLID_COLOR:
    return {&pe->blendCompositor->shader, SetupSolidColorBlend,
            &pe->solidColorBlendActive};
  case TRANSFORM_SCAN_BARS_BLEND:
    return {&pe->blendCompositor->shader, SetupScanBarsBlend,
            &pe->scanBarsBlendActive};
  case TRANSFORM_PITCH_SPIRAL_BLEND:
    return {&pe->blendCompositor->shader, SetupPitchSpiralBlend,
            &pe->pitchSpiralBlendActive};
  case TRANSFORM_MOIRE_GENERATOR_BLEND:
    return {&pe->blendCompositor->shader, SetupMoireGeneratorBlend,
            &pe->moireGeneratorBlendActive};
  case TRANSFORM_SPECTRAL_ARCS_BLEND:
    return {&pe->blendCompositor->shader, SetupSpectralArcsBlend,
            &pe->spectralArcsBlendActive};
  case TRANSFORM_MUONS_BLEND:
    return {&pe->blendCompositor->shader, SetupMuonsBlend,
            &pe->muonsBlendActive};
  case TRANSFORM_FILAMENTS_BLEND:
    return {&pe->blendCompositor->shader, SetupFilamentsBlend,
            &pe->filamentsBlendActive};
  case TRANSFORM_SLASHES_BLEND:
    return {&pe->blendCompositor->shader, SetupSlashesBlend,
            &pe->slashesBlendActive};
  case TRANSFORM_DOT_MATRIX:
    return {&pe->dotMatrix.shader, SetupDotMatrix,
            &pe->effects.dotMatrix.enabled};
  case TRANSFORM_GLYPH_FIELD_BLEND:
    return {&pe->blendCompositor->shader, SetupGlyphFieldBlend,
            &pe->glyphFieldBlendActive};
  case TRANSFORM_ARC_STROBE_BLEND:
    return {&pe->blendCompositor->shader, SetupArcStrobeBlend,
            &pe->arcStrobeBlendActive};
  case TRANSFORM_SIGNAL_FRAMES_BLEND:
    return {&pe->blendCompositor->shader, SetupSignalFramesBlend,
            &pe->signalFramesBlendActive};
  case TRANSFORM_NEBULA_BLEND:
    return {&pe->blendCompositor->shader, SetupNebulaBlend,
            &pe->nebulaBlendActive};
  case TRANSFORM_SHAKE:
    return {&pe->shake.shader, SetupShake, &pe->effects.shake.enabled};
  default:
    return {NULL, NULL, NULL};
  }
}

void SetupFeedback(PostEffect *pe) {
  const float ms = pe->effects.motionScale;
  const FlowFieldConfig *ff = &pe->effects.flowField;

  SetShaderValue(pe->feedbackShader, pe->feedbackDesaturateLoc,
                 &pe->effects.feedbackDesaturate, SHADER_UNIFORM_FLOAT);

  // Identity-centered values: scale deviation from 1.0
  float zoomEff = 1.0f + (ff->zoomBase - 1.0f) * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomBaseLoc, &zoomEff,
                 SHADER_UNIFORM_FLOAT);

  // Radial/angular zoom offsets: direct multiplication (additive modifiers)
  float zoomRadialEff = ff->zoomRadial * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomRadialLoc, &zoomRadialEff,
                 SHADER_UNIFORM_FLOAT);

  // Speed values: direct multiplication
  float rotBase = ff->rotationSpeed * pe->currentDeltaTime * ms;
  float rotRadial = ff->rotationSpeedRadial * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackRotBaseLoc, &rotBase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackRotRadialLoc, &rotRadial,
                 SHADER_UNIFORM_FLOAT);

  // Translation: direct multiplication
  float dxBaseEff = ff->dxBase * ms;
  float dxRadialEff = ff->dxRadial * ms;
  float dyBaseEff = ff->dyBase * ms;
  float dyRadialEff = ff->dyRadial * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDxBaseLoc, &dxBaseEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDxRadialLoc, &dxRadialEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyBaseLoc, &dyBaseEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyRadialLoc, &dyRadialEff,
                 SHADER_UNIFORM_FLOAT);

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
  SetShaderValue(pe->feedbackShader, pe->feedbackCxLoc, &ff->cx,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackCyLoc, &ff->cy,
                 SHADER_UNIFORM_FLOAT);

  // Directional stretch: identity-centered
  float sxEff = 1.0f + (ff->sx - 1.0f) * ms;
  float syEff = 1.0f + (ff->sy - 1.0f) * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackSxLoc, &sxEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackSyLoc, &syEff,
                 SHADER_UNIFORM_FLOAT);

  // Angular modulation: treat as speeds (need deltaTime for frame-rate
  // independence)
  float zoomAngularEff = ff->zoomAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularLoc,
                 &zoomAngularEff, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackZoomAngularFreqLoc,
                 &ff->zoomAngularFreq, SHADER_UNIFORM_INT);
  float rotAngularEff = ff->rotAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularLoc, &rotAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackRotAngularFreqLoc,
                 &ff->rotAngularFreq, SHADER_UNIFORM_INT);
  float dxAngularEff = ff->dxAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularLoc, &dxAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDxAngularFreqLoc,
                 &ff->dxAngularFreq, SHADER_UNIFORM_INT);
  float dyAngularEff = ff->dyAngular * pe->currentDeltaTime * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularLoc, &dyAngularEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackDyAngularFreqLoc,
                 &ff->dyAngularFreq, SHADER_UNIFORM_INT);

  // Procedural warp: scale displacement intensity
  float warpEff = pe->effects.proceduralWarp.warp * ms;
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpLoc, &warpEff,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpTimeLoc, &pe->warpTime,
                 SHADER_UNIFORM_FLOAT);
  float warpScaleInverse = 1.0f / pe->effects.proceduralWarp.warpScale;
  SetShaderValue(pe->feedbackShader, pe->feedbackWarpScaleInverseLoc,
                 &warpScaleInverse, SHADER_UNIFORM_FLOAT);
}

void SetupBlurH(PostEffect *pe) {
  SetShaderValue(pe->blurHShader, pe->blurHScaleLoc, &pe->currentBlurScale,
                 SHADER_UNIFORM_FLOAT);
}

void SetupBlurV(PostEffect *pe) {
  SetShaderValue(pe->blurVShader, pe->blurVScaleLoc, &pe->currentBlurScale,
                 SHADER_UNIFORM_FLOAT);
  // Decay compensation: increase halfLife proportionally to motion slowdown
  const float safeMotionScale = fmaxf(pe->effects.motionScale, 0.01f);
  float effectiveHalfLife = pe->effects.halfLife / safeMotionScale;
  SetShaderValue(pe->blurVShader, pe->halfLifeLoc, &effectiveHalfLife,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->blurVShader, pe->deltaTimeLoc, &pe->currentDeltaTime,
                 SHADER_UNIFORM_FLOAT);
}

void SetupTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->physarum->trailMap),
      pe->effects.physarum.boostIntensity, pe->effects.physarum.blendMode);
}

void SetupCurlFlowTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->curlFlow->trailMap),
      pe->effects.curlFlow.boostIntensity, pe->effects.curlFlow.blendMode);
}

void SetupCurlAdvectionTrailBoost(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor,
                       TrailMapGetTexture(pe->curlAdvection->trailMap),
                       pe->effects.curlAdvection.boostIntensity,
                       pe->effects.curlAdvection.blendMode);
}

void SetupAttractorFlowTrailBoost(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor,
                       TrailMapGetTexture(pe->attractorFlow->trailMap),
                       pe->effects.attractorFlow.boostIntensity,
                       pe->effects.attractorFlow.blendMode);
}

void SetupBoidsTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->boids->trailMap),
      pe->effects.boids.boostIntensity, pe->effects.boids.blendMode);
}

void SetupParticleLifeTrailBoost(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor,
                       TrailMapGetTexture(pe->particleLife->trailMap),
                       pe->effects.particleLife.boostIntensity,
                       pe->effects.particleLife.blendMode);
}

void SetupCymaticsTrailBoost(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor, TrailMapGetTexture(pe->cymatics->trailMap),
      pe->effects.cymatics.boostIntensity, pe->effects.cymatics.blendMode);
}

void SetupChromatic(PostEffect *pe) {
  SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                 &pe->effects.chromaticOffset, SHADER_UNIFORM_FLOAT);
}

void SetupGamma(PostEffect *pe) {
  SetShaderValue(pe->gammaShader, pe->gammaGammaLoc, &pe->effects.gamma,
                 SHADER_UNIFORM_FLOAT);
}

void SetupClarity(PostEffect *pe) {
  SetShaderValue(pe->clarityShader, pe->clarityAmountLoc, &pe->effects.clarity,
                 SHADER_UNIFORM_FLOAT);
}

static void BloomRenderPass(RenderTexture2D *source, RenderTexture2D *dest,
                            Shader shader) {
  BeginTextureMode(*dest);
  BeginShaderMode(shader);
  DrawTexturePro(
      source->texture,
      {0, 0, (float)source->texture.width, (float)-source->texture.height},
      {0, 0, (float)dest->texture.width, (float)dest->texture.height}, {0, 0},
      0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();
}

void ApplyBloomPasses(PostEffect *pe, RenderTexture2D *source,
                      int * /* writeIdx */) {
  const BloomConfig *b = &pe->effects.bloom;
  int iterations = b->iterations;
  if (iterations < 1) {
    iterations = 1;
  }
  if (iterations > BLOOM_MIP_COUNT) {
    iterations = BLOOM_MIP_COUNT;
  }

  // Prefilter: extract bright pixels from source to mip[0]
  SetShaderValue(pe->bloom.prefilterShader, pe->bloom.thresholdLoc,
                 &b->threshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->bloom.prefilterShader, pe->bloom.kneeLoc, &b->knee,
                 SHADER_UNIFORM_FLOAT);
  BloomRenderPass(source, &pe->bloom.mips[0], pe->bloom.prefilterShader);

  // Downsample: mip[0] → mip[1] → ... → mip[iterations-1]
  for (int i = 1; i < iterations; i++) {
    float halfpixel[2] = {0.5f / (float)pe->bloom.mips[i - 1].texture.width,
                          0.5f / (float)pe->bloom.mips[i - 1].texture.height};
    SetShaderValue(pe->bloom.downsampleShader, pe->bloom.downsampleHalfpixelLoc,
                   halfpixel, SHADER_UNIFORM_VEC2);
    BloomRenderPass(&pe->bloom.mips[i - 1], &pe->bloom.mips[i],
                    pe->bloom.downsampleShader);
  }

  // Upsample: mip[iterations-1] → ... → mip[0] (additive blend at each level)
  for (int i = iterations - 1; i > 0; i--) {
    float halfpixel[2] = {0.5f / (float)pe->bloom.mips[i].texture.width,
                          0.5f / (float)pe->bloom.mips[i].texture.height};
    SetShaderValue(pe->bloom.upsampleShader, pe->bloom.upsampleHalfpixelLoc,
                   halfpixel, SHADER_UNIFORM_VEC2);

    // Upsample mip[i] and add to mip[i-1]
    BeginTextureMode(pe->bloom.mips[i - 1]);
    BeginBlendMode(BLEND_ADDITIVE);
    BeginShaderMode(pe->bloom.upsampleShader);
    DrawTexturePro(pe->bloom.mips[i].texture,
                   {0, 0, (float)pe->bloom.mips[i].texture.width,
                    (float)-pe->bloom.mips[i].texture.height},
                   {0, 0, (float)pe->bloom.mips[i - 1].texture.width,
                    (float)pe->bloom.mips[i - 1].texture.height},
                   {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();
  }

  // Final composite uses SetupBloom to bind uniforms, called by render_pipeline
}

void ApplyAnamorphicStreakPasses(PostEffect *pe, RenderTexture2D *source) {
  const AnamorphicStreakConfig *a = &pe->effects.anamorphicStreak;
  AnamorphicStreakEffect *e = &pe->anamorphicStreak;

  int iterations = a->iterations;
  if (iterations < 3) {
    iterations = 3;
  }
  if (iterations > STREAK_MIP_COUNT) {
    iterations = STREAK_MIP_COUNT;
  }

  // Prefilter: extract bright pixels from source into mips[0]
  SetShaderValue(e->prefilterShader, e->thresholdLoc, &a->threshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->prefilterShader, e->kneeLoc, &a->knee,
                 SHADER_UNIFORM_FLOAT);
  BeginTextureMode(e->mips[0]);
  BeginShaderMode(e->prefilterShader);
  DrawTexturePro(
      source->texture,
      {0, 0, (float)source->texture.width, (float)-source->texture.height},
      {0, 0, (float)e->mips[0].texture.width, (float)e->mips[0].texture.height},
      {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Downsample: mips[0] -> mips[1] -> ... -> mips[iterations-1]
  for (int i = 1; i < iterations; i++) {
    float texelSize = 1.0f / (float)e->mips[i - 1].texture.width;
    SetShaderValue(e->downsampleShader, e->downsampleTexelLoc, &texelSize,
                   SHADER_UNIFORM_FLOAT);

    BeginTextureMode(e->mips[i]);
    ClearBackground(BLACK);
    BeginShaderMode(e->downsampleShader);
    DrawTexturePro(e->mips[i - 1].texture,
                   {0, 0, (float)e->mips[i - 1].texture.width,
                    (float)-e->mips[i - 1].texture.height},
                   {0, 0, (float)e->mips[i].texture.width,
                    (float)e->mips[i].texture.height},
                   {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
  }

  // Upsample: walk back up the mip chain using separate down/up arrays.
  // Reads from mips[] (unmodified down chain), writes to mipsUp[].
  // Kino pattern: lastRT starts at the smallest mip, each level lerps
  // mips[i] (high-res) with upsampled lastRT, controlled by stretch.
  RenderTexture2D *lastRT = &e->mips[iterations - 1];
  for (int i = iterations - 2; i >= 0; i--) {
    float texelSize = 1.0f / (float)lastRT->texture.width;
    SetShaderValue(e->upsampleShader, e->upsampleTexelLoc, &texelSize,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->upsampleShader, e->stretchLoc, &a->stretch,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(e->upsampleShader, e->highResTexLoc,
                          e->mips[i].texture);

    BeginTextureMode(e->mipsUp[i]);
    ClearBackground(BLACK);
    BeginShaderMode(e->upsampleShader);
    DrawTexturePro(
        lastRT->texture,
        {0, 0, (float)lastRT->texture.width, (float)-lastRT->texture.height},
        {0, 0, (float)e->mipsUp[i].texture.width,
         (float)e->mipsUp[i].texture.height},
        {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    lastRT = &e->mipsUp[i];
  }

  // Final composite uses SetupAnamorphicStreak to bind uniforms, called by
  // render_pipeline
}

void ApplyHalfResEffect(PostEffect *pe, RenderTexture2D *source,
                        const int *writeIdx, Shader shader,
                        RenderPipelineShaderSetupFn setup) {
  const int halfW = pe->screenWidth / 2;
  const int halfH = pe->screenHeight / 2;
  const Rectangle srcRect = {0, 0, (float)source->texture.width,
                             (float)-source->texture.height};
  const Rectangle halfRect = {0, 0, (float)halfW, (float)halfH};
  const Rectangle fullRect = {0, 0, (float)pe->screenWidth,
                              (float)pe->screenHeight};

  BeginTextureMode(pe->halfResA);
  DrawTexturePro(source->texture, srcRect, halfRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();

  const int resLoc = GetShaderLocation(shader, "resolution");
  float halfRes[2] = {(float)halfW, (float)halfH};
  if (resLoc >= 0) {
    SetShaderValue(shader, resLoc, halfRes, SHADER_UNIFORM_VEC2);
  }

  if (setup != NULL) {
    setup(pe);
  }
  BeginTextureMode(pe->halfResB);
  BeginShaderMode(shader);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Subsequent effects may share this shader
  if (resLoc >= 0) {
    float fullRes[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
    SetShaderValue(shader, resLoc, fullRes, SHADER_UNIFORM_VEC2);
  }

  BeginTextureMode(pe->pingPong[*writeIdx]);
  DrawTexturePro(pe->halfResB.texture, {0, 0, (float)halfW, (float)-halfH},
                 fullRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();
}

void ApplyHalfResOilPaint(PostEffect *pe, RenderTexture2D *source,
                          const int *writeIdx) {
  const int halfW = pe->screenWidth / 2;
  const int halfH = pe->screenHeight / 2;
  const Rectangle srcRect = {0, 0, (float)source->texture.width,
                             (float)-source->texture.height};
  const Rectangle halfRect = {0, 0, (float)halfW, (float)halfH};
  const Rectangle fullRect = {0, 0, (float)pe->screenWidth,
                              (float)pe->screenHeight};
  float halfRes[2] = {(float)halfW, (float)halfH};
  float fullRes[2] = {(float)pe->screenWidth, (float)pe->screenHeight};

  BeginTextureMode(pe->halfResA);
  DrawTexturePro(source->texture, srcRect, halfRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();

  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.strokeResolutionLoc,
                 halfRes, SHADER_UNIFORM_VEC2);

  const OilPaintConfig *op = &pe->effects.oilPaint;
  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.brushSizeLoc,
                 &op->brushSize, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.strokeBendLoc,
                 &op->strokeBend, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.layersLoc, &op->layers,
                 SHADER_UNIFORM_INT);
  SetShaderValueTexture(pe->oilPaint.strokeShader, pe->oilPaint.noiseTexLoc,
                        pe->oilPaint.noiseTex);

  BeginTextureMode(pe->halfResB);
  BeginShaderMode(pe->oilPaint.strokeShader);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  SetShaderValue(pe->oilPaint.compositeShader,
                 pe->oilPaint.compositeResolutionLoc, halfRes,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->oilPaint.compositeShader, pe->oilPaint.specularLoc,
                 &op->specular, SHADER_UNIFORM_FLOAT);

  BeginTextureMode(pe->halfResA);
  BeginShaderMode(pe->oilPaint.compositeShader);
  DrawTexturePro(pe->halfResB.texture, {0, 0, (float)halfW, (float)-halfH},
                 halfRect, {0, 0}, 0.0f, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Subsequent effects may share these shaders
  SetShaderValue(pe->oilPaint.strokeShader, pe->oilPaint.strokeResolutionLoc,
                 fullRes, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->oilPaint.compositeShader,
                 pe->oilPaint.compositeResolutionLoc, fullRes,
                 SHADER_UNIFORM_VEC2);

  BeginTextureMode(pe->pingPong[*writeIdx]);
  DrawTexturePro(pe->halfResA.texture, {0, 0, (float)halfW, (float)-halfH},
                 fullRect, {0, 0}, 0.0f, WHITE);
  EndTextureMode();
}
