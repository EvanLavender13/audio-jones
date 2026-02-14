#include "effect_serialization.h"
#include "config/dual_lissajous_config.h"
#include "config/effect_descriptor.h"
#include "render/gradient.h"
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GradientStop, position, color)

void to_json(json &j, const ColorConfig &c) {
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

void from_json(const json &j, ColorConfig &c) {
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
    starDensity, starSharpness, glowWidth, glowIntensity, noiseType,
    fbmFrontOct, fbmMidOct, fbmBackOct, dustScale, dustStrength, dustEdge,
    spikeIntensity, spikeSharpness, brightness, gradient, blendMode,
    blendIntensity)
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

// clang-format off
#define EFFECT_CONFIG_FIELDS(X) \
  X(sineWarp) X(kaleidoscope) X(voronoi) X(physarum) X(curlFlow) \
  X(curlAdvection) X(attractorFlow) X(boids) X(cymatics) X(infiniteZoom) \
  X(interferenceWarp) X(radialStreak) X(relativisticDoppler) X(textureWarp) \
  X(waveRipple) X(mobius) X(pixelation) X(glitch) X(poincareDisk) X(toon) \
  X(heightfieldRelief) X(gradientFlow) X(drosteZoom) X(kifs) X(latticeFold) \
  X(multiScaleGrid) X(colorGrade) X(asciiArt) X(oilPaint) X(watercolor) \
  X(neonGlow) X(radialPulse) X(falseColor) X(halftone) X(dotMatrix) \
  X(chladniWarp) X(corridorWarp) X(crossHatching) X(crt) \
  X(paletteQuantization) X(bokeh) X(bloom) X(anamorphicStreak) X(mandelbox) \
  X(triangleFold) X(radialIfs) X(domainWarp) X(phyllotaxis) \
  X(densityWaveSpiral) X(moireInterference) X(pencilSketch) X(matrixRain) \
  X(impressionist) X(kuwahara) X(legoBricks) X(inkWash) X(discoBall) \
  X(particleLife) X(surfaceWarp) X(shake) X(circuitBoard) X(synthwave) \
  X(constellation) X(plasma) X(interference) X(solidColor) X(toneWarp) \
  X(scanBars) X(pitchSpiral) X(spectralArcs) X(moireGenerator) X(muons) \
  X(filaments) X(slashes) X(glyphField) X(arcStrobe) X(signalFrames) \
  X(nebula) X(motherboard) X(attractorLines) X(phiBlur)
// clang-format on

void to_json(json &j, const EffectConfig &e) {
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
#define SERIALIZE_EFFECT(name)                                                 \
  if (e.name.enabled)                                                          \
    j[#name] = e.name;
  EFFECT_CONFIG_FIELDS(SERIALIZE_EFFECT)
#undef SERIALIZE_EFFECT
}

void from_json(const json &j, EffectConfig &e) {
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
#define DESERIALIZE_EFFECT(name) e.name = j.value(#name, e.name);
  EFFECT_CONFIG_FIELDS(DESERIALIZE_EFFECT)
#undef DESERIALIZE_EFFECT
}
