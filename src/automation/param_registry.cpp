#include "param_registry.h"
#include "modulation_engine.h"
#include "config/effect_config.h"
#include "ui/ui_units.h"
#include <string.h>

typedef struct ParamEntry {
    const char* id;
    ParamDef def;
} ParamEntry;

static const ParamEntry PARAM_TABLE[] = {
    {"physarum.sensorDistance", {1.0f, 100.0f}},
    {"physarum.sensorAngle",    {0.0f, 6.28f}},
    {"physarum.turningAngle",   {0.0f, 6.28f}},
    {"physarum.stepSize",       {0.1f, 100.0f}},
    {"effects.blurScale",       {0.0f, 10.0f}},
    {"effects.chromaticOffset", {0.0f, 50.0f}},
    {"flowField.zoomBase",      {0.98f, 1.02f}},
    {"flowField.zoomRadial",    {-0.02f, 0.02f}},
    {"flowField.rotationSpeed",       {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"flowField.rotationSpeedRadial", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"flowField.dxBase",        {-0.02f, 0.02f}},
    {"flowField.dxRadial",      {-0.02f, 0.02f}},
    {"flowField.dyBase",        {-0.02f, 0.02f}},
    {"flowField.dyRadial",      {-0.02f, 0.02f}},
    {"flowField.cx",            {0.0f, 1.0f}},
    {"flowField.cy",            {0.0f, 1.0f}},
    {"flowField.sx",            {0.9f, 1.1f}},
    {"flowField.sy",            {0.9f, 1.1f}},
    {"flowField.zoomAngular",   {-0.1f, 0.1f}},
    {"flowField.rotAngular",    {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"feedbackFlow.strength",   {0.0f, 20.0f}},
    {"feedbackFlow.flowAngle",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"feedbackFlow.scale",      {1.0f, 5.0f}},
    {"feedbackFlow.threshold",  {0.0f, 0.1f}},
    {"voronoi.scale",                {5.0f, 50.0f}},
    {"voronoi.speed",                {0.1f, 2.0f}},
    {"voronoi.edgeFalloff",          {0.1f, 1.0f}},
    {"voronoi.isoFrequency",         {1.0f, 50.0f}},
    {"voronoi.uvDistortIntensity",   {0.0f, 1.0f}},
    {"voronoi.edgeIsoIntensity",     {0.0f, 1.0f}},
    {"voronoi.centerIsoIntensity",   {0.0f, 1.0f}},
    {"voronoi.flatFillIntensity",    {0.0f, 1.0f}},
    {"voronoi.edgeDarkenIntensity",  {0.0f, 1.0f}},
    {"voronoi.angleShadeIntensity",  {0.0f, 1.0f}},
    {"voronoi.determinantIntensity", {0.0f, 1.0f}},
    {"voronoi.ratioIntensity",       {0.0f, 1.0f}},
    {"voronoi.edgeDetectIntensity",  {0.0f, 1.0f}},
    {"kaleidoscope.rotationSpeed",    {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"kaleidoscope.twistAngle",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"kaleidoscope.smoothing",        {0.0f, 0.5f}},
    {"kifs.rotationSpeed",            {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"kifs.twistAngle",               {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"latticeFold.rotationSpeed",     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"latticeFold.cellScale",         {1.0f, 20.0f}},
    {"attractorFlow.rotationSpeedX",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedY",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedZ",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationAngleX",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationAngleY",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationAngleZ",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"sineWarp.strength",             {0.0f, 2.0f}},
    {"sineWarp.octaveRotation",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralAngle",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralTwist",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"textureWarp.strength",     {0.0f, 0.3f}},
    {"waveRipple.strength",      {0.0f, 0.5f}},
    {"waveRipple.frequency",     {1.0f, 20.0f}},
    {"waveRipple.steepness",     {0.0f, 1.0f}},
    {"waveRipple.originX",       {0.0f, 1.0f}},
    {"waveRipple.originY",       {0.0f, 1.0f}},
    {"waveRipple.shadeIntensity", {0.0f, 0.5f}},
    {"mobius.spiralTightness",   {-2.0f, 2.0f}},
    {"mobius.zoomFactor",        {-2.0f, 2.0f}},
    {"mobius.point1X",           {0.0f, 1.0f}},
    {"mobius.point1Y",           {0.0f, 1.0f}},
    {"mobius.point2X",           {0.0f, 1.0f}},
    {"mobius.point2Y",           {0.0f, 1.0f}},
    {"pixelation.cellCount",     {4.0f, 256.0f}},
    {"pixelation.ditherScale",   {1.0f, 8.0f}},
    {"glitch.analogIntensity",   {0.0f, 1.0f}},
    {"glitch.blockThreshold",    {0.0f, 0.9f}},
    {"glitch.aberration",        {0.0f, 20.0f}},
    {"glitch.blockOffset",       {0.0f, 0.5f}},
    {"poincareDisk.translationX",         {-0.9f, 0.9f}},
    {"poincareDisk.translationY",         {-0.9f, 0.9f}},
    {"poincareDisk.translationSpeed",     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"poincareDisk.translationAmplitude", {0.0f, 0.9f}},
    {"poincareDisk.diskScale",            {0.5f, 2.0f}},
    {"poincareDisk.rotationSpeed",        {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"heightfieldRelief.lightAngle",      {0.0f, 6.28f}},
    {"heightfieldRelief.intensity",       {0.0f, 1.0f}},
    {"gradientFlow.strength",             {0.0f, 0.1f}},
    {"gradientFlow.flowAngle",            {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"gradientFlow.edgeWeight",           {0.0f, 1.0f}},
    {"drosteZoom.scale",                  {1.5f, 10.0f}},
    {"drosteZoom.spiralAngle",            {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"drosteZoom.shearCoeff",             {-1.0f, 1.0f}},
    {"drosteZoom.innerRadius",            {0.0f, 0.5f}},
    {"colorGrade.hueShift",               {0.0f, 1.0f}},
    {"colorGrade.saturation",             {0.0f, 2.0f}},
    {"colorGrade.brightness",             {-2.0f, 2.0f}},
    {"colorGrade.contrast",               {0.5f, 2.0f}},
    {"colorGrade.temperature",            {-1.0f, 1.0f}},
    {"colorGrade.shadowsOffset",          {-0.5f, 0.5f}},
    {"colorGrade.midtonesOffset",         {-0.5f, 0.5f}},
    {"colorGrade.highlightsOffset",       {-0.5f, 0.5f}},
    {"asciiArt.cellSize",                 {4.0f, 32.0f}},
    {"oilPaint.radius",                   {2.0f, 8.0f}},
    {"watercolor.edgeDarkening",          {0.0f, 1.0f}},
    {"watercolor.granulationStrength",    {0.0f, 1.0f}},
    {"watercolor.bleedStrength",          {0.0f, 1.0f}},
    {"neonGlow.glowIntensity",            {0.5f, 5.0f}},
    {"neonGlow.edgeThreshold",            {0.0f, 0.5f}},
    {"neonGlow.originalVisibility",       {0.0f, 1.0f}},
    {"boids.cohesionWeight",              {0.0f, 2.0f}},
    {"boids.separationWeight",            {0.0f, 2.0f}},
    {"boids.alignmentWeight",             {0.0f, 2.0f}},
    {"radialPulse.radialFreq",            {1.0f, 30.0f}},
    {"radialPulse.radialAmp",             {-0.3f, 0.3f}},
    {"radialPulse.angularAmp",            {-0.5f, 0.5f}},
    {"radialPulse.petalAmp",              {-1.0f, 1.0f}},
    {"radialPulse.spiralTwist",           {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"duotone.intensity",                 {0.0f, 1.0f}},
    {"halftone.dotScale",                 {2.0f, 20.0f}},
    {"halftone.threshold",                {0.5f, 1.0f}},
    {"halftone.rotationSpeed",            {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"halftone.rotationAngle",            {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
};

static const int PARAM_COUNT = sizeof(PARAM_TABLE) / sizeof(PARAM_TABLE[0]);

// Drawable field bounds: matched by field name in "drawable.<id>.<field>"
static const ParamEntry DRAWABLE_FIELD_TABLE[] = {
    {"x",              {0.0f, 1.0f}},
    {"y",              {0.0f, 1.0f}},
    {"rotationSpeed",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"rotationAngle",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"texAngle",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
};

static const int DRAWABLE_FIELD_COUNT = sizeof(DRAWABLE_FIELD_TABLE) / sizeof(DRAWABLE_FIELD_TABLE[0]);

void ParamRegistryInit(EffectConfig* effects)
{
    float* targets[] = {
        &effects->physarum.sensorDistance,
        &effects->physarum.sensorAngle,
        &effects->physarum.turningAngle,
        &effects->physarum.stepSize,
        &effects->blurScale,
        &effects->chromaticOffset,
        &effects->flowField.zoomBase,
        &effects->flowField.zoomRadial,
        &effects->flowField.rotationSpeed,
        &effects->flowField.rotationSpeedRadial,
        &effects->flowField.dxBase,
        &effects->flowField.dxRadial,
        &effects->flowField.dyBase,
        &effects->flowField.dyRadial,
        &effects->flowField.cx,
        &effects->flowField.cy,
        &effects->flowField.sx,
        &effects->flowField.sy,
        &effects->flowField.zoomAngular,
        &effects->flowField.rotAngular,
        &effects->feedbackFlow.strength,
        &effects->feedbackFlow.flowAngle,
        &effects->feedbackFlow.scale,
        &effects->feedbackFlow.threshold,
        &effects->voronoi.scale,
        &effects->voronoi.speed,
        &effects->voronoi.edgeFalloff,
        &effects->voronoi.isoFrequency,
        &effects->voronoi.uvDistortIntensity,
        &effects->voronoi.edgeIsoIntensity,
        &effects->voronoi.centerIsoIntensity,
        &effects->voronoi.flatFillIntensity,
        &effects->voronoi.edgeDarkenIntensity,
        &effects->voronoi.angleShadeIntensity,
        &effects->voronoi.determinantIntensity,
        &effects->voronoi.ratioIntensity,
        &effects->voronoi.edgeDetectIntensity,
        &effects->kaleidoscope.rotationSpeed,
        &effects->kaleidoscope.twistAngle,
        &effects->kaleidoscope.smoothing,
        &effects->kifs.rotationSpeed,
        &effects->kifs.twistAngle,
        &effects->latticeFold.rotationSpeed,
        &effects->latticeFold.cellScale,
        &effects->attractorFlow.rotationSpeedX,
        &effects->attractorFlow.rotationSpeedY,
        &effects->attractorFlow.rotationSpeedZ,
        &effects->attractorFlow.rotationAngleX,
        &effects->attractorFlow.rotationAngleY,
        &effects->attractorFlow.rotationAngleZ,
        &effects->sineWarp.strength,
        &effects->sineWarp.octaveRotation,
        &effects->infiniteZoom.spiralAngle,
        &effects->infiniteZoom.spiralTwist,
        &effects->textureWarp.strength,
        &effects->waveRipple.strength,
        &effects->waveRipple.frequency,
        &effects->waveRipple.steepness,
        &effects->waveRipple.originX,
        &effects->waveRipple.originY,
        &effects->waveRipple.shadeIntensity,
        &effects->mobius.spiralTightness,
        &effects->mobius.zoomFactor,
        &effects->mobius.point1X,
        &effects->mobius.point1Y,
        &effects->mobius.point2X,
        &effects->mobius.point2Y,
        &effects->pixelation.cellCount,
        &effects->pixelation.ditherScale,
        &effects->glitch.analogIntensity,
        &effects->glitch.blockThreshold,
        &effects->glitch.aberration,
        &effects->glitch.blockOffset,
        &effects->poincareDisk.translationX,
        &effects->poincareDisk.translationY,
        &effects->poincareDisk.translationSpeed,
        &effects->poincareDisk.translationAmplitude,
        &effects->poincareDisk.diskScale,
        &effects->poincareDisk.rotationSpeed,
        &effects->heightfieldRelief.lightAngle,
        &effects->heightfieldRelief.intensity,
        &effects->gradientFlow.strength,
        &effects->gradientFlow.flowAngle,
        &effects->gradientFlow.edgeWeight,
        &effects->drosteZoom.scale,
        &effects->drosteZoom.spiralAngle,
        &effects->drosteZoom.shearCoeff,
        &effects->drosteZoom.innerRadius,
        &effects->colorGrade.hueShift,
        &effects->colorGrade.saturation,
        &effects->colorGrade.brightness,
        &effects->colorGrade.contrast,
        &effects->colorGrade.temperature,
        &effects->colorGrade.shadowsOffset,
        &effects->colorGrade.midtonesOffset,
        &effects->colorGrade.highlightsOffset,
        &effects->asciiArt.cellSize,
        &effects->oilPaint.radius,
        &effects->watercolor.edgeDarkening,
        &effects->watercolor.granulationStrength,
        &effects->watercolor.bleedStrength,
        &effects->neonGlow.glowIntensity,
        &effects->neonGlow.edgeThreshold,
        &effects->neonGlow.originalVisibility,
        &effects->boids.cohesionWeight,
        &effects->boids.separationWeight,
        &effects->boids.alignmentWeight,
        &effects->radialPulse.radialFreq,
        &effects->radialPulse.radialAmp,
        &effects->radialPulse.angularAmp,
        &effects->radialPulse.petalAmp,
        &effects->radialPulse.spiralTwist,
        &effects->duotone.intensity,
        &effects->halftone.dotScale,
        &effects->halftone.threshold,
        &effects->halftone.rotationSpeed,
        &effects->halftone.rotationAngle,
    };

    for (int i = 0; i < PARAM_COUNT; i++) {
        ModEngineRegisterParam(PARAM_TABLE[i].id, targets[i],
                               PARAM_TABLE[i].def.min, PARAM_TABLE[i].def.max);
    }
}

const ParamDef* ParamRegistryGet(const char* paramId)
{
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (strcmp(PARAM_TABLE[i].id, paramId) == 0) {
            return &PARAM_TABLE[i].def;
        }
    }
    return NULL;
}

bool ParamRegistryGetDynamic(const char* paramId, ParamDef* outDef)
{
    // Check static table first
    const ParamDef* staticDef = ParamRegistryGet(paramId);
    if (staticDef != NULL) {
        *outDef = *staticDef;
        return true;
    }

    // Match drawable.<id>.<field> pattern
    if (strncmp(paramId, "drawable.", 9) == 0) {
        const char* afterPrefix = paramId + 9;
        const char* dot = strchr(afterPrefix, '.');
        if (dot != NULL) {
            const char* fieldName = dot + 1;
            for (int i = 0; i < DRAWABLE_FIELD_COUNT; i++) {
                if (strcmp(DRAWABLE_FIELD_TABLE[i].id, fieldName) == 0) {
                    *outDef = DRAWABLE_FIELD_TABLE[i].def;
                    return true;
                }
            }
        }
    }

    // Match lfo<n>.rate pattern (registered separately via ModEngineRegisterParam)
    if (strncmp(paramId, "lfo", 3) == 0 && strstr(paramId, ".rate") != NULL) {
        outDef->min = LFO_RATE_MIN;
        outDef->max = LFO_RATE_MAX;
        return true;
    }

    return false;
}
