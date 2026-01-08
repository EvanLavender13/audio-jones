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
    {"kaleidoscope.polarIntensity",       {0.0f, 1.0f}},
    {"kaleidoscope.kifsIntensity",        {0.0f, 1.0f}},
    {"kaleidoscope.iterMirrorIntensity",  {0.0f, 1.0f}},
    {"kaleidoscope.hexFoldIntensity",     {0.0f, 1.0f}},
    {"kaleidoscope.hexScale",             {1.0f, 20.0f}},
    {"mobius.animRotation",           {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedX",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedY",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedZ",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationAngleX",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationAngleY",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationAngleZ",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"turbulence.strength",           {0.0f, 2.0f}},
    {"turbulence.octaveTwist",        {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralAngle",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralTwist",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"radialStreak.spiralTwist",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
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
        &effects->kaleidoscope.polarIntensity,
        &effects->kaleidoscope.kifsIntensity,
        &effects->kaleidoscope.iterMirrorIntensity,
        &effects->kaleidoscope.hexFoldIntensity,
        &effects->kaleidoscope.hexScale,
        &effects->mobius.animRotation,
        &effects->attractorFlow.rotationSpeedX,
        &effects->attractorFlow.rotationSpeedY,
        &effects->attractorFlow.rotationSpeedZ,
        &effects->attractorFlow.rotationAngleX,
        &effects->attractorFlow.rotationAngleY,
        &effects->attractorFlow.rotationAngleZ,
        &effects->turbulence.strength,
        &effects->turbulence.octaveTwist,
        &effects->infiniteZoom.spiralAngle,
        &effects->infiniteZoom.spiralTwist,
        &effects->radialStreak.spiralTwist,
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
