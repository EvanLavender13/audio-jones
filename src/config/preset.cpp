#include "preset.h"
#include "app_configs.h"
#include "render/drawable.h"
#include "render/gradient.h"
#include "ui/imgui_panels.h"
#include "automation/drawable_params.h"
#include "config/infinite_zoom_config.h"
#include "config/kifs_config.h"
#include "config/lattice_fold_config.h"
#include "config/mandelbox_config.h"
#include "config/moire_interference_config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GradientStop, position, color)

static void to_json(json& j, const ColorConfig& c) {
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
    }
}

static void from_json(const json& j, ColorConfig& c) {
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
        const auto& arr = j["gradientStops"];
        const int count = (int)arr.size();
        for (int i = 0; i < count && i < MAX_GRADIENT_STOPS; i++) {
            c.gradientStops[i] = arr[i].get<GradientStop>();
        }
        c.gradientStopCount = (count < MAX_GRADIENT_STOPS) ? count : MAX_GRADIENT_STOPS;
    }

    // Validation: ensure at least 2 stops for gradient mode
    if (c.gradientStopCount < 2) {
        GradientInitDefault(c.gradientStops, &c.gradientStopCount);
    }

    // Ensure stops are sorted by position
    std::sort(c.gradientStops, c.gradientStops + c.gradientStopCount,
        [](const GradientStop& a, const GradientStop& b) {
            return a.position < b.position;
        });
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysarumConfig,
    enabled, boundsMode, agentCount, sensorDistance, sensorDistanceVariance, sensorAngle, turningAngle,
    stepSize, levyAlpha, depositAmount, decayHalfLife, diffusionScale, boostIntensity,
    blendMode, accumSenseBlend, repulsionStrength, samplingExponent, vectorSteering, respawnMode,
    gravityStrength, orbitOffset, attractorCount, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CurlFlowConfig,
    enabled, agentCount, noiseFrequency, noiseEvolution, momentum, trailInfluence,
    accumSenseBlend, gradientRadius, stepSize, depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AttractorFlowConfig,
    enabled, attractorType, agentCount, timeScale, attractorScale,
    sigma, rho, beta, rosslerC, thomasB, x, y, rotationAngleX, rotationAngleY, rotationAngleZ,
    rotationSpeedX, rotationSpeedY, rotationSpeedZ,
    depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BoidsConfig,
    enabled, boundsMode, agentCount, perceptionRadius, separationRadius,
    cohesionWeight, separationWeight, alignmentWeight, hueAffinity, accumRepulsion,
    maxSpeed, minSpeed, depositAmount, decayHalfLife, diffusionScale,
    boostIntensity, blendMode, debugOverlay, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CurlAdvectionConfig,
    enabled, steps, advectionCurl, curlScale, laplacianScale, pressureScale,
    divergenceScale, divergenceUpdate, divergenceSmoothing, selfAmp,
    updateSmoothing, injectionIntensity, injectionThreshold,
    boostIntensity, blendMode, color, debugOverlay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CymaticsConfig,
    enabled, waveScale, falloff, visualGain, contourCount, decayHalfLife,
    diffusionScale, boostIntensity, sourceCount, sourceAmplitude, sourceFreqX, sourceFreqY,
    baseRadius, patternAngle, blendMode, debugOverlay, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FlowFieldConfig,
    zoomBase, zoomRadial, rotationSpeed, rotationSpeedRadial, dxBase, dxRadial, dyBase, dyRadial,
    cx, cy, sx, sy, zoomAngular, zoomAngularFreq, rotAngular, rotAngularFreq,
    dxAngular, dxAngularFreq, dyAngular, dyAngularFreq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FeedbackFlowConfig,
    strength, flowAngle, scale, threshold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProceduralWarpConfig,
    warp, warpSpeed, warpScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KaleidoscopeConfig,
    enabled, segments, rotationSpeed, twistAngle, smoothing,
    polarIntensity, kifsIntensity, hexFoldIntensity,
    kifsIterations, kifsScale, kifsOffsetX, kifsOffsetY, hexScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KifsConfig,
    enabled, iterations, scale, offsetX, offsetY, rotationSpeed, twistSpeed, octantFold,
    polarFold, polarFoldSegments, polarFoldSmoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LatticeFoldConfig,
    enabled, cellType, cellScale, rotationSpeed, smoothing)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VoronoiConfig,
    enabled, smoothMode, scale, speed, edgeFalloff, isoFrequency,
    uvDistortIntensity, edgeIsoIntensity, centerIsoIntensity, flatFillIntensity,
    organicFlowIntensity, edgeGlowIntensity, determinantIntensity, ratioIntensity, edgeDetectIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfiniteZoomConfig,
    enabled, speed, zoomDepth, layers, spiralAngle, spiralTwist)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SineWarpConfig,
    enabled, octaves, strength, animRate, octaveRotation)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialStreakConfig,
    enabled, samples, streakLength)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextureWarpConfig,
    enabled, strength, iterations, channelMode, ridgeAngle, anisotropy, noiseAmount, noiseScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveRippleConfig,
    enabled, octaves, strength, animRate, frequency, steepness, decay, centerHole,
    originX, originY, originAmplitude, originFreqX, originFreqY,
    shadeEnabled, shadeIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MobiusConfig,
    enabled, point1X, point1Y, point2X, point2Y, spiralTightness, zoomFactor,
    animRate, pointAmplitude, pointFreq1, pointFreq2)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PixelationConfig,
    enabled, cellCount, posterizeLevels, ditherScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlitchConfig,
    enabled, crtEnabled, curvature, vignetteEnabled,
    analogIntensity, aberration,
    blockThreshold, blockOffset,
    vhsEnabled, trackingBarIntensity, scanlineNoiseIntensity, colorDriftIntensity,
    scanlineAmount, noiseAmount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PoincareDiskConfig,
    enabled, tileP, tileQ, tileR, translationX, translationY, translationSpeed,
    translationAmplitude, diskScale, rotationSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToonConfig,
    enabled, levels, edgeThreshold, edgeSoftness, thicknessVariation, noiseScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HeightfieldReliefConfig,
    enabled, intensity, reliefScale, lightAngle, lightHeight, shininess)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GradientFlowConfig,
    enabled, strength, iterations, edgeWeight, randomDirection)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrosteZoomConfig,
    enabled, speed, scale, spiralAngle, shearCoeff, innerRadius, branches)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorGradeConfig,
    enabled, hueShift, saturation, brightness, contrast, temperature,
    shadowsOffset, midtonesOffset, highlightsOffset)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AsciiArtConfig,
    enabled, cellSize, colorMode, foregroundR, foregroundG, foregroundB,
    backgroundR, backgroundG, backgroundB, invert)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OilPaintConfig,
    enabled, radius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WatercolorConfig,
    enabled, edgeDarkening, granulationStrength, paperScale, softness,
    bleedStrength, bleedRadius, colorLevels)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NeonGlowConfig,
    enabled, glowR, glowG, glowB, edgeThreshold, edgePower,
    glowIntensity, glowRadius, glowSamples, originalVisibility)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialPulseConfig,
    enabled, radialFreq, radialAmp, segments, angularAmp, petalAmp, phaseSpeed, spiralTwist)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FalseColorConfig,
    enabled, gradient, intensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HalftoneConfig,
    enabled, dotScale, dotSize, rotationSpeed, rotationAngle, threshold, softness)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChladniWarpConfig,
    enabled, n, m, plateSize, strength, warpMode, animRate, animRange, preFold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrossHatchingConfig,
    enabled, width, threshold, noise, outline)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PaletteQuantizationConfig,
    enabled, colorLevels, ditherStrength, bayerSize)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BokehConfig,
    enabled, radius, iterations, brightnessPower)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig,
    enabled, threshold, knee, intensity, iterations)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MandelboxConfig,
    enabled, iterations, boxLimit, sphereMin, sphereMax, scale, offsetX, offsetY,
    rotationSpeed, twistSpeed, boxIntensity, sphereIntensity, polarFold, polarFoldSegments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleFoldConfig,
    enabled, iterations, scale, offsetX, offsetY, rotationSpeed, twistSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DomainWarpConfig,
    enabled, warpStrength, warpScale, warpIterations, falloff, driftSpeed, driftAngle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhyllotaxisConfig,
    enabled, scale, angleSpeed, phaseSpeed, cellRadius, isoFrequency,
    uvDistortIntensity, organicFlowIntensity, edgeIsoIntensity, centerIsoIntensity,
    flatFillIntensity, edgeGlowIntensity, ratioIntensity, determinantIntensity, edgeDetectIntensity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhyllotaxisWarpConfig,
    enabled, scale, divergenceAngle, warpStrength, warpFalloff,
    tangentIntensity, radialIntensity, spinSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DensityWaveSpiralConfig,
    enabled, centerX, centerY, aspectX, aspectY, tightness, rotationSpeed,
    globalRotationSpeed, thickness, ringCount, falloff)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireInterferenceConfig,
    enabled, rotationAngle, scaleDiff, layers, blendMode, centerX, centerY, animationSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PencilSketchConfig,
    enabled, angleCount, sampleCount, strokeFalloff, gradientEps, paperStrength,
    vignetteStrength, wobbleSpeed, wobbleAmount)

// TransformOrderConfig serialization helpers - called from EffectConfig to_json/from_json
// to_json: Only save enabled effects (reduces JSON size, handles new effects gracefully)
static void TransformOrderToJson(json& j, const TransformOrderConfig& t, const EffectConfig& e) {
    j = json::array();
    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
        if (IsTransformEnabled(&e, t.order[i])) {
            j.push_back((int)t.order[i]);
        }
    }
}

// from_json: Merge saved order with defaults - saved effects first, then remaining in default order
static void TransformOrderFromJson(const json& j, TransformOrderConfig& t) {
    if (!j.is_array()) {
        t = TransformOrderConfig{};
        return;
    }

    // Track which effects were in saved order
    bool seen[TRANSFORM_EFFECT_COUNT] = {};
    int outIdx = 0;

    // First pass: add saved effects in saved order
    for (size_t i = 0; i < j.size() && outIdx < TRANSFORM_EFFECT_COUNT; i++) {
        int val = j[i].get<int>();
        if (val >= 0 && val < TRANSFORM_EFFECT_COUNT && !seen[val]) {
            t.order[outIdx++] = (TransformEffectType)val;
            seen[val] = true;
        }
    }

    // Second pass: add remaining effects in default order
    TransformOrderConfig defaultOrder{};
    for (int i = 0; i < TRANSFORM_EFFECT_COUNT && outIdx < TRANSFORM_EFFECT_COUNT; i++) {
        int type = (int)defaultOrder.order[i];
        if (!seen[type]) {
            t.order[outIdx++] = defaultOrder.order[i];
            seen[type] = true;
        }
    }
}

// NOLINTNEXTLINE(readability-function-size) - serializes all effect fields
static void to_json(json& j, const EffectConfig& e) {
    j["halfLife"] = e.halfLife;
    j["blurScale"] = e.blurScale;
    j["chromaticOffset"] = e.chromaticOffset;
    j["feedbackDesaturate"] = e.feedbackDesaturate;
    j["flowField"] = e.flowField;
    j["feedbackFlow"] = e.feedbackFlow;
    j["proceduralWarp"] = e.proceduralWarp;
    j["gamma"] = e.gamma;
    j["clarity"] = e.clarity;
    TransformOrderToJson(j["transformOrder"], e.transformOrder, e);
    // Only serialize enabled effects
    if (e.sineWarp.enabled) { j["sineWarp"] = e.sineWarp; }
    if (e.kaleidoscope.enabled) { j["kaleidoscope"] = e.kaleidoscope; }
    if (e.voronoi.enabled) { j["voronoi"] = e.voronoi; }
    if (e.physarum.enabled) { j["physarum"] = e.physarum; }
    if (e.curlFlow.enabled) { j["curlFlow"] = e.curlFlow; }
    if (e.curlAdvection.enabled) { j["curlAdvection"] = e.curlAdvection; }
    if (e.attractorFlow.enabled) { j["attractorFlow"] = e.attractorFlow; }
    if (e.boids.enabled) { j["boids"] = e.boids; }
    if (e.cymatics.enabled) { j["cymatics"] = e.cymatics; }
    if (e.infiniteZoom.enabled) { j["infiniteZoom"] = e.infiniteZoom; }
    if (e.radialStreak.enabled) { j["radialStreak"] = e.radialStreak; }
    if (e.textureWarp.enabled) { j["textureWarp"] = e.textureWarp; }
    if (e.waveRipple.enabled) { j["waveRipple"] = e.waveRipple; }
    if (e.mobius.enabled) { j["mobius"] = e.mobius; }
    if (e.pixelation.enabled) { j["pixelation"] = e.pixelation; }
    if (e.glitch.enabled) { j["glitch"] = e.glitch; }
    if (e.poincareDisk.enabled) { j["poincareDisk"] = e.poincareDisk; }
    if (e.toon.enabled) { j["toon"] = e.toon; }
    if (e.heightfieldRelief.enabled) { j["heightfieldRelief"] = e.heightfieldRelief; }
    if (e.gradientFlow.enabled) { j["gradientFlow"] = e.gradientFlow; }
    if (e.drosteZoom.enabled) { j["drosteZoom"] = e.drosteZoom; }
    if (e.kifs.enabled) { j["kifs"] = e.kifs; }
    if (e.latticeFold.enabled) { j["latticeFold"] = e.latticeFold; }
    if (e.colorGrade.enabled) { j["colorGrade"] = e.colorGrade; }
    if (e.asciiArt.enabled) { j["asciiArt"] = e.asciiArt; }
    if (e.oilPaint.enabled) { j["oilPaint"] = e.oilPaint; }
    if (e.watercolor.enabled) { j["watercolor"] = e.watercolor; }
    if (e.neonGlow.enabled) { j["neonGlow"] = e.neonGlow; }
    if (e.radialPulse.enabled) { j["radialPulse"] = e.radialPulse; }
    if (e.falseColor.enabled) { j["falseColor"] = e.falseColor; }
    if (e.halftone.enabled) { j["halftone"] = e.halftone; }
    if (e.chladniWarp.enabled) { j["chladniWarp"] = e.chladniWarp; }
    if (e.crossHatching.enabled) { j["crossHatching"] = e.crossHatching; }
    if (e.paletteQuantization.enabled) { j["paletteQuantization"] = e.paletteQuantization; }
    if (e.bokeh.enabled) { j["bokeh"] = e.bokeh; }
    if (e.bloom.enabled) { j["bloom"] = e.bloom; }
    if (e.mandelbox.enabled) { j["mandelbox"] = e.mandelbox; }
    if (e.triangleFold.enabled) { j["triangleFold"] = e.triangleFold; }
    if (e.domainWarp.enabled) { j["domainWarp"] = e.domainWarp; }
    if (e.phyllotaxis.enabled) { j["phyllotaxis"] = e.phyllotaxis; }
    if (e.phyllotaxisWarp.enabled) { j["phyllotaxisWarp"] = e.phyllotaxisWarp; }
    if (e.densityWaveSpiral.enabled) { j["densityWaveSpiral"] = e.densityWaveSpiral; }
    if (e.moireInterference.enabled) { j["moireInterference"] = e.moireInterference; }
    if (e.pencilSketch.enabled) { j["pencilSketch"] = e.pencilSketch; }
}

static void from_json(const json& j, EffectConfig& e) {
    e = EffectConfig{};
    e.halfLife = j.value("halfLife", e.halfLife);
    e.blurScale = j.value("blurScale", e.blurScale);
    e.chromaticOffset = j.value("chromaticOffset", e.chromaticOffset);
    e.feedbackDesaturate = j.value("feedbackDesaturate", e.feedbackDesaturate);
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
    e.radialStreak = j.value("radialStreak", e.radialStreak);
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
    e.colorGrade = j.value("colorGrade", e.colorGrade);
    e.asciiArt = j.value("asciiArt", e.asciiArt);
    e.oilPaint = j.value("oilPaint", e.oilPaint);
    e.watercolor = j.value("watercolor", e.watercolor);
    e.neonGlow = j.value("neonGlow", e.neonGlow);
    e.radialPulse = j.value("radialPulse", e.radialPulse);
    e.falseColor = j.value("falseColor", e.falseColor);
    e.halftone = j.value("halftone", e.halftone);
    e.chladniWarp = j.value("chladniWarp", e.chladniWarp);
    e.crossHatching = j.value("crossHatching", e.crossHatching);
    e.paletteQuantization = j.value("paletteQuantization", e.paletteQuantization);
    e.bokeh = j.value("bokeh", e.bokeh);
    e.bloom = j.value("bloom", e.bloom);
    e.mandelbox = j.value("mandelbox", e.mandelbox);
    e.triangleFold = j.value("triangleFold", e.triangleFold);
    e.domainWarp = j.value("domainWarp", e.domainWarp);
    e.phyllotaxis = j.value("phyllotaxis", e.phyllotaxis);
    e.phyllotaxisWarp = j.value("phyllotaxisWarp", e.phyllotaxisWarp);
    e.densityWaveSpiral = j.value("densityWaveSpiral", e.densityWaveSpiral);
    e.moireInterference = j.value("moireInterference", e.moireInterference);
    e.pencilSketch = j.value("pencilSketch", e.pencilSketch);
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioConfig, channelMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrawableBase,
    enabled, x, y, rotationSpeed, rotationAngle, opacity, drawInterval, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveformData,
    amplitudeScale, thickness, smoothness, radius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectrumData,
    innerRadius, barHeight, barWidth, smoothing, minDb, maxDb)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShapeData,
    sides, width, height, aspectLocked, textured, texZoom, texAngle, texBrightness)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, enabled, rate, waveform)

static void to_json(json& j, const Drawable& d) {
    j["id"] = d.id;
    j["type"] = d.type;
    j["path"] = d.path;
    j["base"] = d.base;
    switch (d.type) {
    case DRAWABLE_WAVEFORM: j["waveform"] = d.waveform; break;
    case DRAWABLE_SPECTRUM: j["spectrum"] = d.spectrum; break;
    case DRAWABLE_SHAPE: j["shape"] = d.shape; break;
    }
}

static void from_json(const json& j, Drawable& d) {
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
    }
}

void to_json(json& j, const Preset& p) {
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

void from_json(const json& j, Preset& p) {
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
        const auto& arr = j["drawables"];
        for (int i = 0; i < MAX_DRAWABLES && i < (int)arr.size(); i++) {
            p.drawables[i] = arr[i].get<Drawable>();
        }
    }
    p.modulation = j.value("modulation", ModulationConfig{});
    if (j.contains("lfos")) {
        const auto& lfoArr = j["lfos"];
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

bool PresetSave(const Preset* preset, const char* filepath) {
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

bool PresetLoad(Preset* preset, const char* filepath) {
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

int PresetListFiles(const char* directory, char outFiles[][PRESET_PATH_MAX], int maxFiles) {
    int count = 0;
    try {
        if (!fs::exists(directory)) {
            fs::create_directories(directory);
            return 0;
        }
        for (const auto& entry : fs::directory_iterator(directory)) {
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

void PresetFromAppConfigs(Preset* preset, const AppConfigs* configs) {
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

void PresetToAppConfigs(const Preset* preset, AppConfigs* configs) {
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
    // Load LFO configs before ModulationConfigToEngine so SyncBases captures correct rates
    for (int i = 0; i < NUM_LFOS; i++) {
        configs->lfos[i] = preset->lfos[i];
    }
    ModulationConfigToEngine(&preset->modulation);
}
