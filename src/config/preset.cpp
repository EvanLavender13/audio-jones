#include "preset.h"
#include "app_configs.h"
#include "automation/drawable_params.h"
#include "config/dual_lissajous_config.h"
#include "config/effect_descriptor.h"
#include "render/drawable.h"
#include "render/gradient.h"
#include "ui/imgui_panels.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GradientStop, position, color)

static void to_json(json &j, const ColorConfig &c) {
  j["mode"] = c.mode;
  switch (c.mode) {
  case COLOR_MODE_SOLID: {
    j["solid"] = c.solid;
  } break;
  case COLOR_MODE_RAINBOW: {
    j["rainbowHue"] = c.rainbowHue;
    j["rainbowRange"] = c.rainbowRange;
    j["rainbowSat"] = c.rainbowSat;
    j["rainbowVal"] = c.rainbowVal;
  } break;
  case COLOR_MODE_GRADIENT: {
    j["gradientStopCount"] = c.gradientStopCount;
    j["gradientStops"] = json::array();
    for (int i = 0; i < c.gradientStopCount; i++) {
      j["gradientStops"].push_back(c.gradientStops[i]);
    }
  } break;
  case COLOR_MODE_PALETTE: {
    j["paletteAR"] = c.paletteAR;
    j["paletteAG"] = c.paletteAG;
    j["paletteAB"] = c.paletteAB;
    j["paletteBR"] = c.paletteBR;
    j["paletteBG"] = c.paletteBG;
    j["paletteBB"] = c.paletteBB;
    j["paletteCR"] = c.paletteCR;
    j["paletteCG"] = c.paletteCG;
    j["paletteCB"] = c.paletteCB;
    j["paletteDR"] = c.paletteDR;
    j["paletteDG"] = c.paletteDG;
    j["paletteDB"] = c.paletteDB;
  } break;
  }
}

static void from_json(const json &j, ColorConfig &c) {
  c = ColorConfig{};
  if (j.contains("mode")) {
    c.mode = j["mode"].get<ColorMode>();
  }
  if (j.contains("solid")) {
    c.solid = j["solid"].get<Color>();
  }
  if (j.contains("rainbowHue")) {
    c.rainbowHue = j["rainbowHue"].get<float>();
  }
  if (j.contains("rainbowRange")) {
    c.rainbowRange = j["rainbowRange"].get<float>();
  }
  if (j.contains("rainbowSat")) {
    c.rainbowSat = j["rainbowSat"].get<float>();
  }
  if (j.contains("rainbowVal")) {
    c.rainbowVal = j["rainbowVal"].get<float>();
  }
  if (j.contains("gradientStopCount")) {
    c.gradientStopCount = j["gradientStopCount"].get<int>();
  }
  if (j.contains("gradientStops")) {
    const auto &arr = j["gradientStops"];
    const int count = (int)arr.size();
    for (int i = 0; i < count && i < MAX_GRADIENT_STOPS; i++) {
      c.gradientStops[i] = arr[i].get<GradientStop>();
    }
    c.gradientStopCount =
        (count < MAX_GRADIENT_STOPS) ? count : MAX_GRADIENT_STOPS;
  }

  if (j.contains("paletteAR")) {
    c.paletteAR = j["paletteAR"].get<float>();
  }
  if (j.contains("paletteAG")) {
    c.paletteAG = j["paletteAG"].get<float>();
  }
  if (j.contains("paletteAB")) {
    c.paletteAB = j["paletteAB"].get<float>();
  }
  if (j.contains("paletteBR")) {
    c.paletteBR = j["paletteBR"].get<float>();
  }
  if (j.contains("paletteBG")) {
    c.paletteBG = j["paletteBG"].get<float>();
  }
  if (j.contains("paletteBB")) {
    c.paletteBB = j["paletteBB"].get<float>();
  }
  if (j.contains("paletteCR")) {
    c.paletteCR = j["paletteCR"].get<float>();
  }
  if (j.contains("paletteCG")) {
    c.paletteCG = j["paletteCG"].get<float>();
  }
  if (j.contains("paletteCB")) {
    c.paletteCB = j["paletteCB"].get<float>();
  }
  if (j.contains("paletteDR")) {
    c.paletteDR = j["paletteDR"].get<float>();
  }
  if (j.contains("paletteDG")) {
    c.paletteDG = j["paletteDG"].get<float>();
  }
  if (j.contains("paletteDB")) {
    c.paletteDB = j["paletteDB"].get<float>();
  }

  // Validation: ensure at least 2 stops for gradient mode
  if (c.gradientStopCount < 2) {
    GradientInitDefault(c.gradientStops, &c.gradientStopCount);
  }

  // Ensure stops are sorted by position
  std::sort(c.gradientStops, c.gradientStops + c.gradientStopCount,
            [](const GradientStop &a, const GradientStop &b) {
              return a.position < b.position;
            });
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DualLissajousConfig, amplitude,
                                                motionSpeed, freqX1, freqY1,
                                                freqX2, freqY2, offsetX2,
                                                offsetY2)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PhysarumConfig, enabled, boundsMode, agentCount, sensorDistance,
    sensorDistanceVariance, sensorAngle, turningAngle, stepSize, walkMode,
    levyAlpha, densityResponse, cauchyScale, expScale, gaussianVariance,
    sprintFactor, gradientBoost, depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, accumSenseBlend, repulsionStrength,
    samplingExponent, vectorSteering, respawnMode, gravityStrength, orbitOffset,
    attractorCount, attractorBaseRadius, lissajous, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    CurlFlowConfig, enabled, agentCount, noiseFrequency, noiseEvolution,
    momentum, trailInfluence, accumSenseBlend, gradientRadius, stepSize,
    respawnProbability, depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    AttractorFlowConfig, enabled, attractorType, agentCount, timeScale,
    attractorScale, sigma, rho, beta, rosslerC, thomasB, x, y, rotationAngleX,
    rotationAngleY, rotationAngleZ, rotationSpeedX, rotationSpeedY,
    rotationSpeedZ, depositAmount, maxSpeed, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BoidsConfig, enabled, boundsMode, agentCount, perceptionRadius,
    separationRadius, cohesionWeight, separationWeight, alignmentWeight,
    hueAffinity, accumRepulsion, maxSpeed, minSpeed, depositAmount,
    decayHalfLife, diffusionScale, boostIntensity, blendMode, debugOverlay,
    color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    CurlAdvectionConfig, enabled, steps, advectionCurl, curlScale,
    laplacianScale, pressureScale, divergenceScale, divergenceUpdate,
    divergenceSmoothing, selfAmp, updateSmoothing, injectionIntensity,
    injectionThreshold, decayHalfLife, diffusionScale, boostIntensity,
    blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    CymaticsConfig, enabled, waveScale, falloff, visualGain, contourCount,
    decayHalfLife, diffusionScale, boostIntensity, sourceCount, baseRadius,
    lissajous, boundaries, reflectionGain, blendMode, debugOverlay, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ParticleLifeConfig, enabled, agentCount, speciesCount, rMax, forceFactor,
    momentum, beta, attractionSeed, evolutionSpeed, symmetricForces,
    boundsRadius, boundaryStiffness, x, y, rotationAngleX, rotationAngleY,
    rotationAngleZ, rotationSpeedX, rotationSpeedY, rotationSpeedZ,
    projectionScale, depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    FlowFieldConfig, zoomBase, zoomRadial, rotationSpeed, rotationSpeedRadial,
    dxBase, dxRadial, dyBase, dyRadial, cx, cy, sx, sy, zoomAngular,
    zoomAngularFreq, rotAngular, rotAngularFreq, dxAngular, dxAngularFreq,
    dyAngular, dyAngularFreq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FeedbackFlowConfig, strength,
                                                flowAngle, scale, threshold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProceduralWarpConfig, warp,
                                                warpSpeed, warpScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KaleidoscopeConfig, enabled,
                                                segments, rotationSpeed,
                                                twistAngle, smoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    KifsConfig, enabled, iterations, scale, offsetX, offsetY, rotationSpeed,
    twistSpeed, octantFold, polarFold, polarFoldSegments, polarFoldSmoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LatticeFoldConfig, enabled,
                                                cellType, cellScale,
                                                rotationSpeed, smoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MultiScaleGridConfig, enabled, scale1, scale2, scale3, warpAmount,
    edgeContrast, edgePower, glowThreshold, glowAmount, cellVariation, glowMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    VoronoiConfig, enabled, smoothMode, scale, speed, edgeFalloff, isoFrequency,
    uvDistortIntensity, edgeIsoIntensity, centerIsoIntensity, flatFillIntensity,
    organicFlowIntensity, edgeGlowIntensity, determinantIntensity,
    ratioIntensity, edgeDetectIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfiniteZoomConfig, enabled,
                                                speed, zoomDepth, layers,
                                                spiralAngle, spiralTwist,
                                                layerRotate)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SineWarpConfig, enabled,
                                                octaves, strength, speed,
                                                octaveRotation, radialMode,
                                                depthBlend)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialStreakConfig, enabled,
                                                samples, streakLength,
                                                intensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RelativisticDopplerConfig,
                                                enabled, velocity, centerX,
                                                centerY, aberration, colorShift,
                                                headlight)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextureWarpConfig, enabled,
                                                strength, iterations,
                                                channelMode, ridgeAngle,
                                                anisotropy, noiseAmount,
                                                noiseScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveRippleConfig, enabled,
                                                octaves, strength, speed,
                                                frequency, steepness, decay,
                                                centerHole, originX, originY,
                                                originLissajous, shadeEnabled,
                                                shadeIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MobiusConfig, enabled, point1X,
                                                point1Y, point2X, point2Y,
                                                spiralTightness, zoomFactor,
                                                speed, point1Lissajous,
                                                point2Lissajous)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PixelationConfig, enabled,
                                                cellCount, posterizeLevels,
                                                ditherScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    GlitchConfig, enabled, analogIntensity, aberration, blockThreshold,
    blockOffset, vhsEnabled, trackingBarIntensity, scanlineNoiseIntensity,
    colorDriftIntensity, scanlineAmount, noiseAmount, datamoshEnabled,
    datamoshIntensity, datamoshMin, datamoshMax, datamoshSpeed, datamoshBands,
    rowSliceEnabled, rowSliceIntensity, rowSliceBurstFreq, rowSliceBurstPower,
    rowSliceColumns, colSliceEnabled, colSliceIntensity, colSliceBurstFreq,
    colSliceBurstPower, colSliceRows, diagonalBandsEnabled, diagonalBandCount,
    diagonalBandDisplace, diagonalBandSpeed, blockMaskEnabled,
    blockMaskIntensity, blockMaskMinSize, blockMaskMaxSize, blockMaskTintR,
    blockMaskTintG, blockMaskTintB, temporalJitterEnabled, temporalJitterAmount,
    temporalJitterGate, blockMultiplyEnabled, blockMultiplySize,
    blockMultiplyControl, blockMultiplyIterations, blockMultiplyIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PoincareDiskConfig, enabled,
                                                tileP, tileQ, tileR,
                                                translationX, translationY,
                                                translationSpeed,
                                                translationAmplitude, diskScale,
                                                rotationSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToonConfig, enabled, levels,
                                                edgeThreshold, edgeSoftness,
                                                thicknessVariation, noiseScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HeightfieldReliefConfig,
                                                enabled, intensity, reliefScale,
                                                lightAngle, lightHeight,
                                                shininess)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GradientFlowConfig, enabled,
                                                strength, iterations,
                                                edgeWeight, randomDirection)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrosteZoomConfig, enabled,
                                                speed, scale, spiralAngle,
                                                shearCoeff, innerRadius,
                                                branches)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ColorGradeConfig, enabled, hueShift, saturation, brightness, contrast,
    temperature, shadowsOffset, midtonesOffset, highlightsOffset)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    AsciiArtConfig, enabled, cellSize, colorMode, foregroundR, foregroundG,
    foregroundB, backgroundR, backgroundG, backgroundB, invert)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OilPaintConfig, enabled,
                                                brushSize, strokeBend, specular,
                                                layers)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WatercolorConfig, enabled,
                                                samples, strokeStep,
                                                washStrength, paperScale,
                                                paperStrength, edgePool,
                                                flowCenter, flowWidth)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    NeonGlowConfig, enabled, glowR, glowG, glowB, edgeThreshold, edgePower,
    glowIntensity, glowRadius, glowSamples, originalVisibility, colorMode,
    saturationBoost, brightnessBoost)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    RadialPulseConfig, enabled, radialFreq, radialAmp, segments, angularAmp,
    petalAmp, phaseSpeed, spiralTwist, octaves, octaveRotation, depthBlend)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FalseColorConfig, enabled,
                                                gradient, intensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HalftoneConfig, enabled,
                                                dotScale, dotSize,
                                                rotationSpeed, rotationAngle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DotMatrixConfig, enabled,
                                                dotScale, softness, brightness,
                                                rotationSpeed, rotationAngle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChladniWarpConfig, enabled, n,
                                                m, plateSize, strength,
                                                warpMode, speed, animRange,
                                                preFold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CorridorWarpConfig, enabled,
                                                horizon, perspectiveStrength,
                                                mode, viewRotationSpeed,
                                                planeRotationSpeed, scale,
                                                scrollSpeed, strafeSpeed,
                                                fogStrength)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrossHatchingConfig, enabled,
                                                width, threshold, noise,
                                                outline)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    CrtConfig, enabled, maskMode, maskSize, maskIntensity, maskBorder,
    scanlineIntensity, scanlineSpacing, scanlineSharpness, scanlineBrightBoost,
    curvatureEnabled, curvatureAmount, vignetteEnabled, vignetteExponent,
    pulseEnabled, pulseIntensity, pulseWidth, pulseSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PaletteQuantizationConfig,
                                                enabled, colorLevels,
                                                ditherStrength, bayerSize)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhiBlurConfig, enabled, mode,
                                                radius, angle, aspectRatio,
                                                samples, gamma)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BokehConfig, enabled, radius,
                                                iterations, brightnessPower)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig, enabled, threshold,
                                                knee, intensity, iterations)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AnamorphicStreakConfig, enabled,
                                                threshold, knee, intensity,
                                                stretch, tintR, tintG, tintB,
                                                iterations)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MandelboxConfig, enabled, iterations, boxLimit, sphereMin, sphereMax, scale,
    offsetX, offsetY, rotationSpeed, twistSpeed, boxIntensity, sphereIntensity,
    polarFold, polarFoldSegments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleFoldConfig, enabled,
                                                iterations, scale, offsetX,
                                                offsetY, rotationSpeed,
                                                twistSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialIfsConfig, enabled,
                                                segments, iterations, scale,
                                                offset, rotationSpeed,
                                                twistSpeed, smoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DomainWarpConfig, enabled,
                                                warpStrength, warpScale,
                                                warpIterations, falloff,
                                                driftSpeed, driftAngle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PhyllotaxisConfig, enabled, smoothMode, scale, divergenceAngle, angleSpeed,
    phaseSpeed, spinSpeed, cellRadius, isoFrequency, uvDistortIntensity,
    organicFlowIntensity, edgeIsoIntensity, centerIsoIntensity,
    flatFillIntensity, edgeGlowIntensity, ratioIntensity, determinantIntensity,
    edgeDetectIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DensityWaveSpiralConfig,
                                                enabled, centerX, centerY,
                                                aspectX, aspectY, tightness,
                                                rotationSpeed,
                                                globalRotationSpeed, thickness,
                                                ringCount, falloff)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireInterferenceConfig,
                                                enabled, rotationAngle,
                                                scaleDiff, layers, blendMode,
                                                centerX, centerY,
                                                animationSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PencilSketchConfig, enabled,
                                                angleCount, sampleCount,
                                                strokeFalloff, gradientEps,
                                                paperStrength, vignetteStrength,
                                                wobbleSpeed, wobbleAmount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MatrixRainConfig, enabled,
                                                cellSize, rainSpeed,
                                                trailLength, fallerCount,
                                                overlayIntensity, refreshRate,
                                                leadBrightness, sampleMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ImpressionistConfig, enabled, splatCount, splatSizeMin, splatSizeMax,
    strokeFreq, strokeOpacity, outlineStrength, edgeStrength, edgeMaxDarken,
    grainScale, grainAmount, exposure)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KuwaharaConfig, enabled, radius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LegoBricksConfig, enabled,
                                                brickScale, studHeight,
                                                edgeShadow, colorThreshold,
                                                maxBrickSize, lightAngle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InkWashConfig, enabled,
                                                strength, granulation,
                                                bleedStrength, bleedRadius,
                                                softness)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterferenceWarpConfig, enabled,
                                                amplitude, scale, axes,
                                                axisRotationSpeed, harmonics,
                                                decay, speed, drift)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    DiscoBallConfig, enabled, sphereRadius, tileSize, rotationSpeed, bumpHeight,
    reflectIntensity, spotIntensity, spotFalloff, brightnessThreshold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SurfaceWarpConfig, enabled,
                                                intensity, angle, rotationSpeed,
                                                scrollSpeed, depthShade)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShakeConfig, enabled, intensity,
                                                samples, rate, gaussian)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CircuitBoardConfig, enabled,
                                                tileScale, strength, baseSize,
                                                breathe, breatheSpeed,
                                                dualLayer, layerOffset,
                                                contourFreq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SynthwaveConfig, enabled, horizonY, colorMix, palettePhaseR, palettePhaseG,
    palettePhaseB, gridSpacing, gridThickness, gridOpacity, gridGlow, gridR,
    gridG, gridB, stripeCount, stripeSoftness, stripeIntensity, sunR, sunG,
    sunB, horizonIntensity, horizonFalloff, horizonR, horizonG, horizonB,
    gridScrollSpeed, stripeScrollSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ConstellationConfig, enabled, gridScale, animSpeed, wanderAmp, waveFreq,
    waveAmp, waveSpeed, depthLayers, pointSize, pointBrightness, pointOpacity,
    lineThickness, maxLineLen, lineOpacity, interpolateLineColor, fillEnabled,
    fillOpacity, fillThreshold, waveCenterX, waveCenterY, waveInfluence,
    pointGradient, lineGradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PlasmaConfig, enabled, boltCount, layerCount, octaves, falloffType,
    driftSpeed, driftAmount, animSpeed, displacement, glowRadius,
    coreBrightness, flickerAmount, gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    InterferenceConfig, enabled, sourceCount, baseRadius, lissajous, waveFreq,
    waveSpeed, falloffType, falloffStrength, boundaries, reflectionGain,
    visualMode, contourCount, visualGain, chromaSpread, colorMode, color,
    blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SolidColorConfig, enabled,
                                                color, blendMode,
                                                blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ScanBarsConfig, enabled, mode, angle, barDensity, convergence,
    convergenceFreq, convergenceOffset, sharpness, scrollSpeed, colorSpeed,
    chaosFreq, chaosIntensity, snapAmount, baseFreq, numOctaves, gain, curve,
    baseBright, gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToneWarpConfig, enabled,
                                                intensity, numOctaves, baseFreq,
                                                gain, curve, baseBright,
                                                maxRadius, segments,
                                                pushPullBalance,
                                                pushPullSmoothness, phaseSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PitchSpiralConfig, enabled, numOctaves, baseFreq, numTurns, spiralSpacing,
    lineWidth, blur, gain, curve, baseBright, tilt, tiltAngle, gradient,
    blendMode, blendIntensity, rotationSpeed, breathSpeed, breathDepth,
    shapeExponent)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SpectralArcsConfig, enabled, baseFreq, numOctaves, gain, curve, ringScale,
    tilt, tiltAngle, arcWidth, glowIntensity, glowFalloff, baseBright,
    rotationSpeed, gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireLayerConfig, frequency,
                                                angle, rotationSpeed,
                                                warpAmount, scale, phase)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MoireGeneratorConfig, enabled, patternMode, layerCount, sharpMode,
    colorIntensity, globalBrightness, layer0, layer1, layer2, layer3, gradient,
    blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MuonsConfig, enabled, marchSteps, turbulenceOctaves, turbulenceStrength,
    ringThickness, cameraDistance, colorFreq, colorSpeed, brightness, exposure,
    gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    FilamentsConfig, enabled, baseFreq, numOctaves, gain, curve, radius, spread,
    stepAngle, glowIntensity, falloffExponent, baseBright, noiseStrength,
    noiseSpeed, rotationSpeed, gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SlashesConfig, enabled, baseFreq, numOctaves, gain, curve, tickRate,
    envelopeSharp, maxBarLength, barThickness, thicknessVariation, scatter,
    glowSoftness, baseBright, rotationDepth, gradient, blendMode,
    blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    GlyphFieldConfig, enabled, gridSize, layerCount, layerScaleSpread,
    layerSpeedSpread, layerOpacity, scrollDirection, scrollSpeed, stutterAmount,
    stutterSpeed, stutterDiscrete, flutterAmount, flutterSpeed, waveAmplitude,
    waveFreq, waveSpeed, driftAmount, driftSpeed, bandDistortion, inversionRate,
    inversionSpeed, lcdMode, lcdFreq, baseFreq, numOctaves, gain, curve,
    baseBright, gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    ArcStrobeConfig, enabled, lissajous, orbitOffset, lineThickness,
    glowIntensity, strobeSpeed, strobeDecay, strobeBoost, strobeStride,
    baseFreq, numOctaves, segmentsPerOctave, gain, curve, baseBright, gradient,
    blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SignalFramesConfig, enabled, numOctaves, baseFreq, gain, curve, baseBright,
    rotationSpeed, orbitRadius, orbitSpeed, sizeMin, sizeMax, aspectRatio,
    outlineThickness, glowWidth, glowIntensity, sweepSpeed, sweepIntensity,
    gradient, blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    NebulaConfig, enabled, baseFreq, numOctaves, gain, curve, baseBright,
    driftSpeed, frontScale, midScale, backScale, frontIter, midIter, backIter,
    starDensity, starSharpness, glowWidth, glowIntensity, brightness, gradient,
    blendMode, blendIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MotherboardConfig, enabled, baseFreq, numOctaves, gain, curve, baseBright,
    iterations, rangeX, rangeY, size, fallOff, rotAngle, glowIntensity,
    accentIntensity, rotationSpeed, blendIntensity, gradient, blendMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    AttractorLinesConfig, enabled, attractorType, sigma, rho, beta, rosslerC,
    thomasB, dadrasA, dadrasB, dadrasC, dadrasD, dadrasE, steps, speed,
    viewScale, intensity, decayHalfLife, focus, maxSpeed, numParticles, x, y,
    rotationAngleX, rotationAngleY, rotationAngleZ, rotationSpeedX,
    rotationSpeedY, rotationSpeedZ, gradient, blendMode, blendIntensity)

// Look up effect name -> enum value, returns -1 if not found
static int TransformEffectFromName(const char *name) {
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (strcmp(TransformEffectName((TransformEffectType)i), name) == 0) {
      return i;
    }
  }
  return -1;
}

// TransformOrderConfig serialization helpers - called from EffectConfig
// to_json/from_json to_json: Save as string names (stable across enum changes)
static void TransformOrderToJson(json &j, const TransformOrderConfig &t,
                                 const EffectConfig &e) {
  j = json::array();
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (IsTransformEnabled(&e, t.order[i])) {
      j.push_back(TransformEffectName(t.order[i]));
    }
  }
}

// from_json: Merge saved order with defaults - saved effects first, then
// remaining in default order Supports both string names (new) and integer
// indices (legacy)
static void TransformOrderFromJson(const json &j, TransformOrderConfig &t) {
  if (!j.is_array()) {
    t = TransformOrderConfig{};
    return;
  }

  // Track which effects were in saved order
  bool seen[TRANSFORM_EFFECT_COUNT] = {};
  int outIdx = 0;

  // First pass: add saved effects in saved order
  for (size_t i = 0; i < j.size() && outIdx < TRANSFORM_EFFECT_COUNT; i++) {
    int val = -1;
    if (j[i].is_string()) {
      val = TransformEffectFromName(j[i].get<std::string>().c_str());
    } else if (j[i].is_number_integer()) {
      val = j[i].get<int>();
    }
    if (val >= 0 && val < TRANSFORM_EFFECT_COUNT && !seen[val]) {
      t.order[outIdx++] = (TransformEffectType)val;
      seen[val] = true;
    }
  }

  // Second pass: add remaining effects in default order
  const TransformOrderConfig defaultOrder{};
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT && outIdx < TRANSFORM_EFFECT_COUNT;
       i++) {
    const int type = (int)defaultOrder.order[i];
    if (!seen[type]) {
      t.order[outIdx++] = defaultOrder.order[i];
      seen[type] = true;
    }
  }
}

// NOLINTNEXTLINE(readability-function-size) - serializes all effect fields
static void to_json(json &j, const EffectConfig &e) {
  j["halfLife"] = e.halfLife;
  j["blurScale"] = e.blurScale;
  j["chromaticOffset"] = e.chromaticOffset;
  j["feedbackDesaturate"] = e.feedbackDesaturate;
  j["motionScale"] = e.motionScale;
  j["flowField"] = e.flowField;
  j["feedbackFlow"] = e.feedbackFlow;
  j["proceduralWarp"] = e.proceduralWarp;
  j["gamma"] = e.gamma;
  j["clarity"] = e.clarity;
  TransformOrderToJson(j["transformOrder"], e.transformOrder, e);
  // Only serialize enabled effects
  if (e.sineWarp.enabled) {
    j["sineWarp"] = e.sineWarp;
  }
  if (e.kaleidoscope.enabled) {
    j["kaleidoscope"] = e.kaleidoscope;
  }
  if (e.voronoi.enabled) {
    j["voronoi"] = e.voronoi;
  }
  if (e.physarum.enabled) {
    j["physarum"] = e.physarum;
  }
  if (e.curlFlow.enabled) {
    j["curlFlow"] = e.curlFlow;
  }
  if (e.curlAdvection.enabled) {
    j["curlAdvection"] = e.curlAdvection;
  }
  if (e.attractorFlow.enabled) {
    j["attractorFlow"] = e.attractorFlow;
  }
  if (e.boids.enabled) {
    j["boids"] = e.boids;
  }
  if (e.cymatics.enabled) {
    j["cymatics"] = e.cymatics;
  }
  if (e.infiniteZoom.enabled) {
    j["infiniteZoom"] = e.infiniteZoom;
  }
  if (e.interferenceWarp.enabled) {
    j["interferenceWarp"] = e.interferenceWarp;
  }
  if (e.radialStreak.enabled) {
    j["radialStreak"] = e.radialStreak;
  }
  if (e.relativisticDoppler.enabled) {
    j["relativisticDoppler"] = e.relativisticDoppler;
  }
  if (e.textureWarp.enabled) {
    j["textureWarp"] = e.textureWarp;
  }
  if (e.waveRipple.enabled) {
    j["waveRipple"] = e.waveRipple;
  }
  if (e.mobius.enabled) {
    j["mobius"] = e.mobius;
  }
  if (e.pixelation.enabled) {
    j["pixelation"] = e.pixelation;
  }
  if (e.glitch.enabled) {
    j["glitch"] = e.glitch;
  }
  if (e.poincareDisk.enabled) {
    j["poincareDisk"] = e.poincareDisk;
  }
  if (e.toon.enabled) {
    j["toon"] = e.toon;
  }
  if (e.heightfieldRelief.enabled) {
    j["heightfieldRelief"] = e.heightfieldRelief;
  }
  if (e.gradientFlow.enabled) {
    j["gradientFlow"] = e.gradientFlow;
  }
  if (e.drosteZoom.enabled) {
    j["drosteZoom"] = e.drosteZoom;
  }
  if (e.kifs.enabled) {
    j["kifs"] = e.kifs;
  }
  if (e.latticeFold.enabled) {
    j["latticeFold"] = e.latticeFold;
  }
  if (e.multiScaleGrid.enabled) {
    j["multiScaleGrid"] = e.multiScaleGrid;
  }
  if (e.colorGrade.enabled) {
    j["colorGrade"] = e.colorGrade;
  }
  if (e.asciiArt.enabled) {
    j["asciiArt"] = e.asciiArt;
  }
  if (e.oilPaint.enabled) {
    j["oilPaint"] = e.oilPaint;
  }
  if (e.watercolor.enabled) {
    j["watercolor"] = e.watercolor;
  }
  if (e.neonGlow.enabled) {
    j["neonGlow"] = e.neonGlow;
  }
  if (e.radialPulse.enabled) {
    j["radialPulse"] = e.radialPulse;
  }
  if (e.falseColor.enabled) {
    j["falseColor"] = e.falseColor;
  }
  if (e.halftone.enabled) {
    j["halftone"] = e.halftone;
  }
  if (e.dotMatrix.enabled) {
    j["dotMatrix"] = e.dotMatrix;
  }
  if (e.chladniWarp.enabled) {
    j["chladniWarp"] = e.chladniWarp;
  }
  if (e.corridorWarp.enabled) {
    j["corridorWarp"] = e.corridorWarp;
  }
  if (e.crossHatching.enabled) {
    j["crossHatching"] = e.crossHatching;
  }
  if (e.crt.enabled) {
    j["crt"] = e.crt;
  }
  if (e.paletteQuantization.enabled) {
    j["paletteQuantization"] = e.paletteQuantization;
  }
  if (e.bokeh.enabled) {
    j["bokeh"] = e.bokeh;
  }
  if (e.bloom.enabled) {
    j["bloom"] = e.bloom;
  }
  if (e.anamorphicStreak.enabled) {
    j["anamorphicStreak"] = e.anamorphicStreak;
  }
  if (e.mandelbox.enabled) {
    j["mandelbox"] = e.mandelbox;
  }
  if (e.triangleFold.enabled) {
    j["triangleFold"] = e.triangleFold;
  }
  if (e.radialIfs.enabled) {
    j["radialIfs"] = e.radialIfs;
  }
  if (e.domainWarp.enabled) {
    j["domainWarp"] = e.domainWarp;
  }
  if (e.phyllotaxis.enabled) {
    j["phyllotaxis"] = e.phyllotaxis;
  }
  if (e.densityWaveSpiral.enabled) {
    j["densityWaveSpiral"] = e.densityWaveSpiral;
  }
  if (e.moireInterference.enabled) {
    j["moireInterference"] = e.moireInterference;
  }
  if (e.pencilSketch.enabled) {
    j["pencilSketch"] = e.pencilSketch;
  }
  if (e.matrixRain.enabled) {
    j["matrixRain"] = e.matrixRain;
  }
  if (e.impressionist.enabled) {
    j["impressionist"] = e.impressionist;
  }
  if (e.kuwahara.enabled) {
    j["kuwahara"] = e.kuwahara;
  }
  if (e.legoBricks.enabled) {
    j["legoBricks"] = e.legoBricks;
  }
  if (e.inkWash.enabled) {
    j["inkWash"] = e.inkWash;
  }
  if (e.discoBall.enabled) {
    j["discoBall"] = e.discoBall;
  }
  if (e.particleLife.enabled) {
    j["particleLife"] = e.particleLife;
  }
  if (e.surfaceWarp.enabled) {
    j["surfaceWarp"] = e.surfaceWarp;
  }
  if (e.shake.enabled) {
    j["shake"] = e.shake;
  }
  if (e.circuitBoard.enabled) {
    j["circuitBoard"] = e.circuitBoard;
  }
  if (e.synthwave.enabled) {
    j["synthwave"] = e.synthwave;
  }
  if (e.constellation.enabled) {
    j["constellation"] = e.constellation;
  }
  if (e.plasma.enabled) {
    j["plasma"] = e.plasma;
  }
  if (e.interference.enabled) {
    j["interference"] = e.interference;
  }
  if (e.solidColor.enabled) {
    j["solidColor"] = e.solidColor;
  }
  if (e.toneWarp.enabled) {
    j["toneWarp"] = e.toneWarp;
  }
  if (e.scanBars.enabled) {
    j["scanBars"] = e.scanBars;
  }
  if (e.pitchSpiral.enabled) {
    j["pitchSpiral"] = e.pitchSpiral;
  }
  if (e.spectralArcs.enabled) {
    j["spectralArcs"] = e.spectralArcs;
  }
  if (e.moireGenerator.enabled) {
    j["moireGenerator"] = e.moireGenerator;
  }
  if (e.muons.enabled) {
    j["muons"] = e.muons;
  }
  if (e.filaments.enabled) {
    j["filaments"] = e.filaments;
  }
  if (e.slashes.enabled) {
    j["slashes"] = e.slashes;
  }
  if (e.glyphField.enabled) {
    j["glyphField"] = e.glyphField;
  }
  if (e.arcStrobe.enabled) {
    j["arcStrobe"] = e.arcStrobe;
  }
  if (e.signalFrames.enabled) {
    j["signalFrames"] = e.signalFrames;
  }
  if (e.nebula.enabled) {
    j["nebula"] = e.nebula;
  }
  if (e.motherboard.enabled) {
    j["motherboard"] = e.motherboard;
  }
  if (e.attractorLines.enabled) {
    j["attractorLines"] = e.attractorLines;
  }
  if (e.phiBlur.enabled) {
    j["phiBlur"] = e.phiBlur;
  }
}

static void from_json(const json &j, EffectConfig &e) {
  e = EffectConfig{};
  e.halfLife = j.value("halfLife", e.halfLife);
  e.blurScale = j.value("blurScale", e.blurScale);
  e.chromaticOffset = j.value("chromaticOffset", e.chromaticOffset);
  e.feedbackDesaturate = j.value("feedbackDesaturate", e.feedbackDesaturate);
  e.motionScale = j.value("motionScale", e.motionScale);
  e.flowField = j.value("flowField", e.flowField);
  e.feedbackFlow = j.value("feedbackFlow", e.feedbackFlow);
  e.proceduralWarp = j.value("proceduralWarp", e.proceduralWarp);
  e.gamma = j.value("gamma", e.gamma);
  e.clarity = j.value("clarity", e.clarity);
  if (j.contains("transformOrder")) {
    TransformOrderFromJson(j["transformOrder"], e.transformOrder);
  }
  e.sineWarp = j.value("sineWarp", e.sineWarp);
  e.kaleidoscope = j.value("kaleidoscope", e.kaleidoscope);
  e.voronoi = j.value("voronoi", e.voronoi);
  e.physarum = j.value("physarum", e.physarum);
  e.curlFlow = j.value("curlFlow", e.curlFlow);
  e.curlAdvection = j.value("curlAdvection", e.curlAdvection);
  e.attractorFlow = j.value("attractorFlow", e.attractorFlow);
  e.boids = j.value("boids", e.boids);
  e.cymatics = j.value("cymatics", e.cymatics);
  e.infiniteZoom = j.value("infiniteZoom", e.infiniteZoom);
  e.interferenceWarp = j.value("interferenceWarp", e.interferenceWarp);
  e.radialStreak = j.value("radialStreak", e.radialStreak);
  e.relativisticDoppler = j.value("relativisticDoppler", e.relativisticDoppler);
  e.textureWarp = j.value("textureWarp", e.textureWarp);
  e.waveRipple = j.value("waveRipple", e.waveRipple);
  e.mobius = j.value("mobius", e.mobius);
  e.pixelation = j.value("pixelation", e.pixelation);
  e.glitch = j.value("glitch", e.glitch);
  e.poincareDisk = j.value("poincareDisk", e.poincareDisk);
  e.toon = j.value("toon", e.toon);
  e.heightfieldRelief = j.value("heightfieldRelief", e.heightfieldRelief);
  e.gradientFlow = j.value("gradientFlow", e.gradientFlow);
  e.drosteZoom = j.value("drosteZoom", e.drosteZoom);
  e.kifs = j.value("kifs", e.kifs);
  e.latticeFold = j.value("latticeFold", e.latticeFold);
  e.multiScaleGrid = j.value("multiScaleGrid", e.multiScaleGrid);
  e.colorGrade = j.value("colorGrade", e.colorGrade);
  e.asciiArt = j.value("asciiArt", e.asciiArt);
  e.oilPaint = j.value("oilPaint", e.oilPaint);
  e.watercolor = j.value("watercolor", e.watercolor);
  e.neonGlow = j.value("neonGlow", e.neonGlow);
  e.radialPulse = j.value("radialPulse", e.radialPulse);
  e.falseColor = j.value("falseColor", e.falseColor);
  e.halftone = j.value("halftone", e.halftone);
  e.dotMatrix = j.value("dotMatrix", e.dotMatrix);
  e.chladniWarp = j.value("chladniWarp", e.chladniWarp);
  e.corridorWarp = j.value("corridorWarp", e.corridorWarp);
  e.crossHatching = j.value("crossHatching", e.crossHatching);
  e.crt = j.value("crt", e.crt);
  e.paletteQuantization = j.value("paletteQuantization", e.paletteQuantization);
  e.bokeh = j.value("bokeh", e.bokeh);
  e.bloom = j.value("bloom", e.bloom);
  e.anamorphicStreak = j.value("anamorphicStreak", e.anamorphicStreak);
  e.mandelbox = j.value("mandelbox", e.mandelbox);
  e.triangleFold = j.value("triangleFold", e.triangleFold);
  e.radialIfs = j.value("radialIfs", e.radialIfs);
  e.domainWarp = j.value("domainWarp", e.domainWarp);
  e.phyllotaxis = j.value("phyllotaxis", e.phyllotaxis);
  e.densityWaveSpiral = j.value("densityWaveSpiral", e.densityWaveSpiral);
  e.moireInterference = j.value("moireInterference", e.moireInterference);
  e.pencilSketch = j.value("pencilSketch", e.pencilSketch);
  e.matrixRain = j.value("matrixRain", e.matrixRain);
  e.impressionist = j.value("impressionist", e.impressionist);
  e.kuwahara = j.value("kuwahara", e.kuwahara);
  e.legoBricks = j.value("legoBricks", e.legoBricks);
  e.inkWash = j.value("inkWash", e.inkWash);
  e.discoBall = j.value("discoBall", e.discoBall);
  e.particleLife = j.value("particleLife", e.particleLife);
  e.surfaceWarp = j.value("surfaceWarp", e.surfaceWarp);
  e.shake = j.value("shake", e.shake);
  e.circuitBoard = j.value("circuitBoard", e.circuitBoard);
  e.synthwave = j.value("synthwave", e.synthwave);
  e.constellation = j.value("constellation", e.constellation);
  e.plasma = j.value("plasma", e.plasma);
  e.interference = j.value("interference", e.interference);
  e.solidColor = j.value("solidColor", e.solidColor);
  e.toneWarp = j.value("toneWarp", e.toneWarp);
  e.scanBars = j.value("scanBars", e.scanBars);
  e.pitchSpiral = j.value("pitchSpiral", e.pitchSpiral);
  e.spectralArcs = j.value("spectralArcs", e.spectralArcs);
  e.moireGenerator = j.value("moireGenerator", e.moireGenerator);
  e.muons = j.value("muons", e.muons);
  e.filaments = j.value("filaments", e.filaments);
  e.slashes = j.value("slashes", e.slashes);
  e.glyphField = j.value("glyphField", e.glyphField);
  e.arcStrobe = j.value("arcStrobe", e.arcStrobe);
  e.signalFrames = j.value("signalFrames", e.signalFrames);
  e.nebula = j.value("nebula", e.nebula);
  e.motherboard = j.value("motherboard", e.motherboard);
  e.attractorLines = j.value("attractorLines", e.attractorLines);
  e.phiBlur = j.value("phiBlur", e.phiBlur);
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioConfig, channelMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrawableBase, enabled, x, y,
                                                rotationSpeed, rotationAngle,
                                                opacity, drawInterval, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveformData, amplitudeScale,
                                                thickness, smoothness, radius,
                                                waveformMotionScale, colorShift,
                                                colorShiftSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectrumData, innerRadius,
                                                barHeight, barWidth, smoothing,
                                                minDb, maxDb, colorShift,
                                                colorShiftSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShapeData, sides, width, height,
                                                aspectLocked, textured, texZoom,
                                                texAngle, texBrightness,
                                                texMotionScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParametricTrailData, lissajous,
                                                shapeType, size, filled,
                                                strokeThickness, gateFreq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, enabled, rate,
                                                waveform)

static void to_json(json &j, const Drawable &d) {
  j["id"] = d.id;
  j["type"] = d.type;
  j["path"] = d.path;
  j["base"] = d.base;
  switch (d.type) {
  case DRAWABLE_WAVEFORM:
    j["waveform"] = d.waveform;
    break;
  case DRAWABLE_SPECTRUM:
    j["spectrum"] = d.spectrum;
    break;
  case DRAWABLE_SHAPE:
    j["shape"] = d.shape;
    break;
  case DRAWABLE_PARAMETRIC_TRAIL:
    j["parametricTrail"] = d.parametricTrail;
    break;
  }
}

static void from_json(const json &j, Drawable &d) {
  d = Drawable{};
  d.id = j.value("id", (uint32_t)0);
  d.type = j.value("type", DRAWABLE_WAVEFORM);
  d.path = j.value("path", PATH_CIRCULAR);
  d.base = j.value("base", DrawableBase{});
  switch (d.type) {
  case DRAWABLE_WAVEFORM:
    if (j.contains("waveform")) {
      d.waveform = j["waveform"].get<WaveformData>();
    }
    break;
  case DRAWABLE_SPECTRUM:
    if (j.contains("spectrum")) {
      d.spectrum = j["spectrum"].get<SpectrumData>();
    }
    break;
  case DRAWABLE_SHAPE:
    if (j.contains("shape")) {
      d.shape = j["shape"].get<ShapeData>();
    }
    break;
  case DRAWABLE_PARAMETRIC_TRAIL:
    if (j.contains("parametricTrail")) {
      d.parametricTrail = j["parametricTrail"].get<ParametricTrailData>();
    }
    break;
  }
}

void to_json(json &j, const Preset &p) {
  j["name"] = std::string(p.name);
  j["effects"] = p.effects;
  j["audio"] = p.audio;
  j["drawableCount"] = p.drawableCount;
  j["drawables"] = json::array();
  for (int i = 0; i < p.drawableCount; i++) {
    j["drawables"].push_back(p.drawables[i]);
  }
  j["modulation"] = p.modulation;
  j["lfos"] = json::array();
  for (int i = 0; i < NUM_LFOS; i++) {
    j["lfos"].push_back(p.lfos[i]);
  }
}

void from_json(const json &j, Preset &p) {
  const std::string name = j.at("name").get<std::string>();
  strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
  p.name[PRESET_NAME_MAX - 1] = '\0';
  p.effects = j.value("effects", EffectConfig{});
  p.audio = j.value("audio", AudioConfig{});
  p.drawableCount = j.value("drawableCount", 0);
  if (p.drawableCount > MAX_DRAWABLES) {
    p.drawableCount = MAX_DRAWABLES;
  }
  if (j.contains("drawables")) {
    const auto &arr = j["drawables"];
    for (int i = 0; i < MAX_DRAWABLES && i < (int)arr.size(); i++) {
      p.drawables[i] = arr[i].get<Drawable>();
    }
  }
  p.modulation = j.value("modulation", ModulationConfig{});
  if (j.contains("lfos")) {
    const auto &lfoArr = j["lfos"];
    for (int i = 0; i < NUM_LFOS && i < (int)lfoArr.size(); i++) {
      p.lfos[i] = lfoArr[i].get<LFOConfig>();
    }
  }
}

Preset PresetDefault(void) {
  Preset p = {};
  strncpy(p.name, "Default", PRESET_NAME_MAX);
  p.effects = EffectConfig{};
  p.audio = AudioConfig{};
  p.drawableCount = 0;
  for (int i = 0; i < NUM_LFOS; i++) {
    p.lfos[i] = LFOConfig{};
  }
  return p;
}

bool PresetSave(const Preset *preset, const char *filepath) {
  try {
    const json j = *preset;
    std::ofstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    file << j.dump(2);
    return true;
  } catch (...) {
    return false;
  }
}

bool PresetLoad(Preset *preset, const char *filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    const json j = json::parse(file);
    *preset = j.get<Preset>();
    return true;
  } catch (...) {
    return false;
  }
}

int PresetListFiles(const char *directory, char outFiles[][PRESET_PATH_MAX],
                    int maxFiles) {
  int count = 0;
  try {
    if (!fs::exists(directory)) {
      fs::create_directories(directory);
      return 0;
    }
    for (const auto &entry : fs::directory_iterator(directory)) {
      if (count >= maxFiles) {
        break;
      }
      if (entry.path().extension() == ".json") {
        const std::string filename = entry.path().filename().string();
        strncpy(outFiles[count], filename.c_str(), PRESET_PATH_MAX - 1);
        outFiles[count][PRESET_PATH_MAX - 1] = '\0';
        count++;
      }
    }
  } catch (...) {
    return count;
  }
  return count;
}

void PresetFromAppConfigs(Preset *preset, const AppConfigs *configs) {
  // Write base values before copying (next ModEngineUpdate restores modulation)
  ModEngineWriteBaseValues();

  preset->effects = *configs->effects;
  preset->audio = *configs->audio;
  preset->drawableCount = *configs->drawableCount;
  for (int i = 0; i < *configs->drawableCount; i++) {
    preset->drawables[i] = configs->drawables[i];
  }
  ModulationConfigFromEngine(&preset->modulation);
  for (int i = 0; i < NUM_LFOS; i++) {
    preset->lfos[i] = configs->lfos[i];
  }
}

void PresetToAppConfigs(const Preset *preset, AppConfigs *configs) {
  *configs->effects = preset->effects;
  *configs->audio = preset->audio;
  // Clear old drawable params before loading new preset to avoid stale pointers
  for (uint32_t i = 1; i <= MAX_DRAWABLES; i++) {
    DrawableParamsUnregister(i);
  }
  *configs->drawableCount = preset->drawableCount;
  for (int i = 0; i < preset->drawableCount; i++) {
    configs->drawables[i] = preset->drawables[i];
  }
  ImGuiDrawDrawablesSyncIdCounter(configs->drawables, *configs->drawableCount);
  DrawableParamsSyncAll(configs->drawables, *configs->drawableCount);
  // Load LFO configs before ModulationConfigToEngine so SyncBases captures
  // correct rates
  for (int i = 0; i < NUM_LFOS; i++) {
    configs->lfos[i] = preset->lfos[i];
  }
  ModulationConfigToEngine(&preset->modulation);
}
