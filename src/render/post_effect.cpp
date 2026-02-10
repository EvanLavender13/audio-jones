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
#include "effects/glyph_field.h"
#include "effects/gradient_flow.h"
#include "effects/infinite_zoom.h"
#include "effects/interference_warp.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/lattice_fold.h"
#include "effects/mandelbox.h"
#include "effects/mobius.h"
#include "effects/moire_interference.h"
#include "effects/phyllotaxis.h"
#include "effects/poincare_disk.h"
#include "effects/radial_ifs.h"
#include "effects/radial_pulse.h"
#include "effects/radial_streak.h"
#include "effects/relativistic_doppler.h"
#include "effects/scan_bars.h"
#include "effects/shake.h"
#include "effects/sine_warp.h"
#include "effects/solid_color.h"
#include "effects/surface_warp.h"
#include "effects/texture_warp.h"
#include "effects/triangle_fold.h"
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

static bool LoadPostEffectShaders(PostEffect *pe) {
  pe->feedbackShader = LoadShader(0, "shaders/feedback.fs");
  pe->blurHShader = LoadShader(0, "shaders/blur_h.fs");
  pe->blurVShader = LoadShader(0, "shaders/blur_v.fs");
  pe->chromaticShader = LoadShader(0, "shaders/chromatic.fs");
  pe->fxaaShader = LoadShader(0, "shaders/fxaa.fs");
  pe->clarityShader = LoadShader(0, "shaders/clarity.fs");
  pe->gammaShader = LoadShader(0, "shaders/gamma.fs");
  pe->shapeTextureShader = LoadShader(0, "shaders/shape_texture.fs");
  ToonEffectInit(&pe->toon);
  NeonGlowEffectInit(&pe->neonGlow);
  HalftoneEffectInit(&pe->halftone);
  KuwaharaEffectInit(&pe->kuwahara);
  DiscoBallEffectInit(&pe->discoBall);
  LegoBricksEffectInit(&pe->legoBricks);

  return pe->feedbackShader.id != 0 && pe->blurHShader.id != 0 &&
         pe->blurVShader.id != 0 && pe->chromaticShader.id != 0 &&
         pe->fxaaShader.id != 0 && pe->clarityShader.id != 0 &&
         pe->gammaShader.id != 0 && pe->shapeTextureShader.id != 0 &&
         pe->toon.shader.id != 0 && pe->neonGlow.shader.id != 0 &&
         pe->halftone.shader.id != 0 && pe->kuwahara.shader.id != 0 &&
         pe->discoBall.shader.id != 0 && pe->legoBricks.shader.id != 0;
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
}

static void SetResolutionUniforms(PostEffect *pe, int width, int height) {
  float resolution[2] = {(float)width, (float)height};
  SetShaderValue(pe->blurHShader, pe->blurHResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->blurVShader, pe->blurVResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->chromaticShader, pe->chromaticResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->feedbackShader, pe->feedbackResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->fxaaShader, pe->fxaaResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->clarityShader, pe->clarityResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
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
    goto cleanup;
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
    goto cleanup;
  }
  if (!LatticeFoldEffectInit(&pe->latticeFold)) {
    goto cleanup;
  }
  if (!MultiScaleGridEffectInit(&pe->multiScaleGrid)) {
    goto cleanup;
  }
  if (!PhyllotaxisEffectInit(&pe->phyllotaxis)) {
    goto cleanup;
  }
  if (!SineWarpEffectInit(&pe->sineWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize sine warp effect");
    goto cleanup;
  }
  if (!TextureWarpEffectInit(&pe->textureWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init TextureWarp");
    goto cleanup;
  }
  if (!WaveRippleEffectInit(&pe->waveRipple)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init WaveRipple");
    goto cleanup;
  }
  if (!MobiusEffectInit(&pe->mobius)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init Mobius");
    goto cleanup;
  }
  if (!GradientFlowEffectInit(&pe->gradientFlow)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init GradientFlow");
    goto cleanup;
  }
  if (!ChladniWarpEffectInit(&pe->chladniWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init ChladniWarp");
    goto cleanup;
  }
  if (!DomainWarpEffectInit(&pe->domainWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init DomainWarp");
    goto cleanup;
  }
  if (!SurfaceWarpEffectInit(&pe->surfaceWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init SurfaceWarp");
    goto cleanup;
  }
  if (!InterferenceWarpEffectInit(&pe->interferenceWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init InterferenceWarp");
    goto cleanup;
  }
  if (!CorridorWarpEffectInit(&pe->corridorWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init CorridorWarp");
    goto cleanup;
  }
  if (!FftRadialWarpEffectInit(&pe->fftRadialWarp)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init FftRadialWarp");
    goto cleanup;
  }
  if (!CircuitBoardEffectInit(&pe->circuitBoard)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init CircuitBoard");
    goto cleanup;
  }
  if (!RadialPulseEffectInit(&pe->radialPulse)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to init RadialPulse");
    goto cleanup;
  }
  if (!KaleidoscopeEffectInit(&pe->kaleidoscope)) {
    goto cleanup;
  }
  if (!KifsEffectInit(&pe->kifs)) {
    goto cleanup;
  }
  if (!PoincareDiskEffectInit(&pe->poincareDisk)) {
    goto cleanup;
  }
  if (!MandelboxEffectInit(&pe->mandelbox)) {
    goto cleanup;
  }
  if (!TriangleFoldEffectInit(&pe->triangleFold)) {
    goto cleanup;
  }
  if (!MoireInterferenceEffectInit(&pe->moireInterference)) {
    goto cleanup;
  }
  if (!RadialIfsEffectInit(&pe->radialIfs)) {
    goto cleanup;
  }
  if (!InfiniteZoomEffectInit(&pe->infiniteZoom)) {
    goto cleanup;
  }
  if (!RadialStreakEffectInit(&pe->radialStreak)) {
    goto cleanup;
  }
  if (!DrosteZoomEffectInit(&pe->drosteZoom)) {
    goto cleanup;
  }
  if (!DensityWaveSpiralEffectInit(&pe->densityWaveSpiral)) {
    goto cleanup;
  }
  if (!ShakeEffectInit(&pe->shake)) {
    goto cleanup;
  }
  if (!RelativisticDopplerEffectInit(&pe->relativisticDoppler)) {
    goto cleanup;
  }
  if (!OilPaintEffectInit(&pe->oilPaint, screenWidth, screenHeight)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init oil paint");
    goto cleanup;
  }
  if (!WatercolorEffectInit(&pe->watercolor)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init watercolor");
    goto cleanup;
  }
  if (!ImpressionistEffectInit(&pe->impressionist)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init impressionist");
    goto cleanup;
  }
  if (!InkWashEffectInit(&pe->inkWash)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init ink wash");
    goto cleanup;
  }
  if (!PencilSketchEffectInit(&pe->pencilSketch)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init pencil sketch");
    goto cleanup;
  }
  if (!CrossHatchingEffectInit(&pe->crossHatching)) {
    TraceLog(LOG_ERROR, "EFFECT: Failed to init cross hatching");
    goto cleanup;
  }
  if (!PixelationEffectInit(&pe->pixelation)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize pixelation");
    goto cleanup;
  }
  if (!GlitchEffectInit(&pe->glitch)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize glitch");
    goto cleanup;
  }
  if (!CrtEffectInit(&pe->crt)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize CRT");
    goto cleanup;
  }
  if (!AsciiArtEffectInit(&pe->asciiArt)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize ascii art");
    goto cleanup;
  }
  if (!MatrixRainEffectInit(&pe->matrixRain)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize matrix rain");
    goto cleanup;
  }
  if (!SynthwaveEffectInit(&pe->synthwave)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize synthwave");
    goto cleanup;
  }
  if (!BloomEffectInit(&pe->bloom, screenWidth, screenHeight)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize bloom");
    goto cleanup;
  }
  if (!BokehEffectInit(&pe->bokeh)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize bokeh");
    goto cleanup;
  }
  if (!HeightfieldReliefEffectInit(&pe->heightfieldRelief)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize heightfield relief");
    goto cleanup;
  }
  if (!AnamorphicStreakEffectInit(&pe->anamorphicStreak, screenWidth,
                                  screenHeight)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize anamorphic streak");
    goto cleanup;
  }
  if (!ColorGradeEffectInit(&pe->colorGrade)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize color grade");
    goto cleanup;
  }
  if (!FalseColorEffectInit(&pe->falseColor, &pe->effects.falseColor)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize false color");
    goto cleanup;
  }
  if (!PaletteQuantizationEffectInit(&pe->paletteQuantization)) {
    TraceLog(LOG_ERROR,
             "POST_EFFECT: Failed to initialize palette quantization");
    goto cleanup;
  }
  if (!ConstellationEffectInit(&pe->constellation,
                               &pe->effects.constellation)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize constellation");
    goto cleanup;
  }
  if (!PlasmaEffectInit(&pe->plasma, &pe->effects.plasma)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize plasma");
    goto cleanup;
  }
  if (!InterferenceEffectInit(&pe->interference, &pe->effects.interference)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize interference");
    goto cleanup;
  }
  if (!SolidColorEffectInit(&pe->solidColor, &pe->effects.solidColor)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize solid color");
    goto cleanup;
  }
  if (!ScanBarsEffectInit(&pe->scanBars, &pe->effects.scanBars)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize scan bars");
    goto cleanup;
  }
  if (!PitchSpiralEffectInit(&pe->pitchSpiral, &pe->effects.pitchSpiral)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize pitch spiral");
    goto cleanup;
  }
  if (!SpectralArcsEffectInit(&pe->spectralArcs, &pe->effects.spectralArcs)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize spectral arcs");
    goto cleanup;
  }
  if (!SignalFramesEffectInit(&pe->signalFrames, &pe->effects.signalFrames)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize signal frames");
    goto cleanup;
  }
  if (!MuonsEffectInit(&pe->muons, &pe->effects.muons)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize muons");
    goto cleanup;
  }
  if (!FilamentsEffectInit(&pe->filaments, &pe->effects.filaments)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize filaments");
    goto cleanup;
  }
  if (!ArcStrobeEffectInit(&pe->arcStrobe, &pe->effects.arcStrobe)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize arc strobe");
    goto cleanup;
  }
  if (!SlashesEffectInit(&pe->slashes, &pe->effects.slashes)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize slashes");
    goto cleanup;
  }
  if (!MoireGeneratorEffectInit(&pe->moireGenerator)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize moire generator");
    goto cleanup;
  }
  if (!GlyphFieldEffectInit(&pe->glyphField, &pe->effects.glyphField)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize glyph field");
    goto cleanup;
  }
  if (!DotMatrixEffectInit(&pe->dotMatrix)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize dot matrix");
    goto cleanup;
  }
  if (!NebulaEffectInit(&pe->nebula, &pe->effects.nebula)) {
    TraceLog(LOG_ERROR, "POST_EFFECT: Failed to initialize nebula");
    goto cleanup;
  }

  RenderUtilsInitTextureHDR(&pe->generatorScratch, screenWidth, screenHeight,
                            LOG_PREFIX);

  InitFFTTexture(&pe->fftTexture);
  pe->fftMaxMagnitude = 1.0f;
  TraceLog(LOG_INFO, "POST_EFFECT: FFT texture created (%dx%d)",
           pe->fftTexture.width, pe->fftTexture.height);

  InitWaveformTexture(&pe->waveformTexture);
  TraceLog(LOG_INFO, "POST_EFFECT: Waveform texture created (%dx%d)",
           pe->waveformTexture.width, pe->waveformTexture.height);

  RenderUtilsInitTextureHDR(&pe->halfResA, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, screenWidth / 2, screenHeight / 2,
                            LOG_PREFIX);
  TraceLog(LOG_INFO, "POST_EFFECT: Half-res textures allocated (%dx%d)",
           pe->halfResA.texture.width, pe->halfResA.texture.height);

  return pe;

cleanup:
  PostEffectUninit(pe);
  return NULL;
}

void PostEffectRegisterParams(PostEffect *pe) {
  if (pe == NULL) {
    return;
  }

  // Warp effects
  SineWarpRegisterParams(&pe->effects.sineWarp);
  TextureWarpRegisterParams(&pe->effects.textureWarp);
  WaveRippleRegisterParams(&pe->effects.waveRipple);
  MobiusRegisterParams(&pe->effects.mobius);
  GradientFlowRegisterParams(&pe->effects.gradientFlow);
  ChladniWarpRegisterParams(&pe->effects.chladniWarp);
  DomainWarpRegisterParams(&pe->effects.domainWarp);
  SurfaceWarpRegisterParams(&pe->effects.surfaceWarp);
  InterferenceWarpRegisterParams(&pe->effects.interferenceWarp);
  CorridorWarpRegisterParams(&pe->effects.corridorWarp);
  FftRadialWarpRegisterParams(&pe->effects.fftRadialWarp);
  CircuitBoardRegisterParams(&pe->effects.circuitBoard);

  // Symmetry effects
  KaleidoscopeRegisterParams(&pe->effects.kaleidoscope);
  KifsRegisterParams(&pe->effects.kifs);
  PoincareDiskRegisterParams(&pe->effects.poincareDisk);
  MandelboxRegisterParams(&pe->effects.mandelbox);
  TriangleFoldRegisterParams(&pe->effects.triangleFold);
  MoireInterferenceRegisterParams(&pe->effects.moireInterference);
  RadialIfsRegisterParams(&pe->effects.radialIfs);
  RadialPulseRegisterParams(&pe->effects.radialPulse);

  // Cellular effects
  VoronoiRegisterParams(&pe->effects.voronoi);
  LatticeFoldRegisterParams(&pe->effects.latticeFold);
  MultiScaleGridRegisterParams(&pe->effects.multiScaleGrid);
  PhyllotaxisRegisterParams(&pe->effects.phyllotaxis);

  // Motion effects
  InfiniteZoomRegisterParams(&pe->effects.infiniteZoom);
  DrosteZoomRegisterParams(&pe->effects.drosteZoom);
  DensityWaveSpiralRegisterParams(&pe->effects.densityWaveSpiral);
  ShakeRegisterParams(&pe->effects.shake);
  RelativisticDopplerRegisterParams(&pe->effects.relativisticDoppler);

  // Optical effects
  BloomRegisterParams(&pe->effects.bloom);
  AnamorphicStreakRegisterParams(&pe->effects.anamorphicStreak);
  BokehRegisterParams(&pe->effects.bokeh);
  HeightfieldReliefRegisterParams(&pe->effects.heightfieldRelief);
  RadialStreakRegisterParams(&pe->effects.radialStreak);

  // Artistic effects
  OilPaintRegisterParams(&pe->effects.oilPaint);
  WatercolorRegisterParams(&pe->effects.watercolor);
  ImpressionistRegisterParams(&pe->effects.impressionist);
  InkWashRegisterParams(&pe->effects.inkWash);
  PencilSketchRegisterParams(&pe->effects.pencilSketch);
  CrossHatchingRegisterParams(&pe->effects.crossHatching);

  // Graphic effects
  NeonGlowRegisterParams(&pe->effects.neonGlow);
  KuwaharaRegisterParams(&pe->effects.kuwahara);
  HalftoneRegisterParams(&pe->effects.halftone);
  DiscoBallRegisterParams(&pe->effects.discoBall);
  DotMatrixRegisterParams(&pe->effects.dotMatrix);
  LegoBricksRegisterParams(&pe->effects.legoBricks);

  // Retro effects
  PixelationRegisterParams(&pe->effects.pixelation);
  GlitchRegisterParams(&pe->effects.glitch);
  CrtRegisterParams(&pe->effects.crt);
  AsciiArtRegisterParams(&pe->effects.asciiArt);
  MatrixRainRegisterParams(&pe->effects.matrixRain);
  ToonRegisterParams(&pe->effects.toon);

  // Color effects
  ColorGradeRegisterParams(&pe->effects.colorGrade);
  FalseColorRegisterParams(&pe->effects.falseColor);
  PaletteQuantizationRegisterParams(&pe->effects.paletteQuantization);

  // Generator effects
  PlasmaRegisterParams(&pe->effects.plasma);
  InterferenceRegisterParams(&pe->effects.interference);
  ConstellationRegisterParams(&pe->effects.constellation);
  SolidColorRegisterParams(&pe->effects.solidColor);
  ScanBarsRegisterParams(&pe->effects.scanBars);
  PitchSpiralRegisterParams(&pe->effects.pitchSpiral);
  SpectralArcsRegisterParams(&pe->effects.spectralArcs);
  SignalFramesRegisterParams(&pe->effects.signalFrames);
  MoireGeneratorRegisterParams(&pe->effects.moireGenerator);
  MuonsRegisterParams(&pe->effects.muons);
  FilamentsRegisterParams(&pe->effects.filaments);
  ArcStrobeRegisterParams(&pe->effects.arcStrobe);
  SlashesRegisterParams(&pe->effects.slashes);
  GlyphFieldRegisterParams(&pe->effects.glyphField);
  NebulaRegisterParams(&pe->effects.nebula);

  // Graphic effects (continued)
  SynthwaveRegisterParams(&pe->effects.synthwave);

  // Simulation
  PhysarumRegisterParams(&pe->effects.physarum);
  AttractorFlowRegisterParams(&pe->effects.attractorFlow);
  ParticleLifeRegisterParams(&pe->effects.particleLife);
  BoidsRegisterParams(&pe->effects.boids);
  CurlFlowRegisterParams(&pe->effects.curlFlow);
  CurlAdvectionRegisterParams(&pe->effects.curlAdvection);
  CymaticsRegisterParams(&pe->effects.cymatics);
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
  PixelationEffectUninit(&pe->pixelation);
  GlitchEffectUninit(&pe->glitch);
  CrtEffectUninit(&pe->crt);
  ToonEffectUninit(&pe->toon);
  HeightfieldReliefEffectUninit(&pe->heightfieldRelief);
  DrosteZoomEffectUninit(&pe->drosteZoom);
  LatticeFoldEffectUninit(&pe->latticeFold);
  MultiScaleGridEffectUninit(&pe->multiScaleGrid);
  ColorGradeEffectUninit(&pe->colorGrade);
  AsciiArtEffectUninit(&pe->asciiArt);
  OilPaintEffectUninit(&pe->oilPaint);
  WatercolorEffectUninit(&pe->watercolor);
  NeonGlowEffectUninit(&pe->neonGlow);
  FalseColorEffectUninit(&pe->falseColor);
  HalftoneEffectUninit(&pe->halftone);
  CrossHatchingEffectUninit(&pe->crossHatching);
  PaletteQuantizationEffectUninit(&pe->paletteQuantization);
  BokehEffectUninit(&pe->bokeh);
  BloomEffectUninit(&pe->bloom);
  AnamorphicStreakEffectUninit(&pe->anamorphicStreak);
  PhyllotaxisEffectUninit(&pe->phyllotaxis);
  DensityWaveSpiralEffectUninit(&pe->densityWaveSpiral);
  PencilSketchEffectUninit(&pe->pencilSketch);
  MatrixRainEffectUninit(&pe->matrixRain);
  ImpressionistEffectUninit(&pe->impressionist);
  KuwaharaEffectUninit(&pe->kuwahara);
  InkWashEffectUninit(&pe->inkWash);
  DiscoBallEffectUninit(&pe->discoBall);
  ShakeEffectUninit(&pe->shake);
  LegoBricksEffectUninit(&pe->legoBricks);
  SynthwaveEffectUninit(&pe->synthwave);
  RelativisticDopplerEffectUninit(&pe->relativisticDoppler);
  ConstellationEffectUninit(&pe->constellation);
  PlasmaEffectUninit(&pe->plasma);
  InterferenceEffectUninit(&pe->interference);
  SolidColorEffectUninit(&pe->solidColor);
  ScanBarsEffectUninit(&pe->scanBars);
  PitchSpiralEffectUninit(&pe->pitchSpiral);
  SpectralArcsEffectUninit(&pe->spectralArcs);
  SignalFramesEffectUninit(&pe->signalFrames);
  MoireGeneratorEffectUninit(&pe->moireGenerator);
  MuonsEffectUninit(&pe->muons);
  FilamentsEffectUninit(&pe->filaments);
  ArcStrobeEffectUninit(&pe->arcStrobe);
  SlashesEffectUninit(&pe->slashes);
  GlyphFieldEffectUninit(&pe->glyphField);
  DotMatrixEffectUninit(&pe->dotMatrix);
  NebulaEffectUninit(&pe->nebula);
  UnloadRenderTexture(pe->generatorScratch);
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

  BloomEffectResize(&pe->bloom, width, height);
  AnamorphicStreakEffectResize(&pe->anamorphicStreak, width, height);

  UnloadRenderTexture(pe->halfResA);
  UnloadRenderTexture(pe->halfResB);
  RenderUtilsInitTextureHDR(&pe->halfResA, width / 2, height / 2, LOG_PREFIX);
  RenderUtilsInitTextureHDR(&pe->halfResB, width / 2, height / 2, LOG_PREFIX);

  UnloadRenderTexture(pe->generatorScratch);
  RenderUtilsInitTextureHDR(&pe->generatorScratch, width, height, LOG_PREFIX);

  OilPaintEffectResize(&pe->oilPaint, width, height);

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
