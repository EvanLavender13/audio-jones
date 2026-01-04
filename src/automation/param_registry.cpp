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
    {"flowField.rotBase",       {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"flowField.rotRadial",     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"flowField.dxBase",        {-0.02f, 0.02f}},
    {"flowField.dxRadial",      {-0.02f, 0.02f}},
    {"flowField.dyBase",        {-0.02f, 0.02f}},
    {"flowField.dyRadial",      {-0.02f, 0.02f}},
    {"voronoi.intensity",       {0.1f, 1.0f}},
    {"voronoi.scale",           {5.0f, 50.0f}},
    {"voronoi.speed",           {0.1f, 2.0f}},
    {"voronoi.edgeWidth",       {0.01f, 0.1f}},
    {"kaleidoscope.rotationSpeed",    {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedX",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedY",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationSpeedZ",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"attractorFlow.rotationX",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationY",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"attractorFlow.rotationZ",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"turbulence.strength",           {0.0f, 2.0f}},
    {"turbulence.rotationPerOctave",  {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralTurns",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"infiniteZoom.spiralTwist",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"radialStreak.spiralTwist",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
    {"radialStreak.spiralTurns",      {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
};

static const int PARAM_COUNT = sizeof(PARAM_TABLE) / sizeof(PARAM_TABLE[0]);

// Drawable field bounds: matched by field name in "drawable.<id>.<field>"
static const ParamEntry DRAWABLE_FIELD_TABLE[] = {
    {"x",              {0.0f, 1.0f}},
    {"y",              {0.0f, 1.0f}},
    {"rotationSpeed",  {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
    {"rotationOffset", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
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
        &effects->flowField.rotBase,
        &effects->flowField.rotRadial,
        &effects->flowField.dxBase,
        &effects->flowField.dxRadial,
        &effects->flowField.dyBase,
        &effects->flowField.dyRadial,
        &effects->voronoi.intensity,
        &effects->voronoi.scale,
        &effects->voronoi.speed,
        &effects->voronoi.edgeWidth,
        &effects->kaleidoscope.rotationSpeed,
        &effects->attractorFlow.rotationSpeedX,
        &effects->attractorFlow.rotationSpeedY,
        &effects->attractorFlow.rotationSpeedZ,
        &effects->attractorFlow.rotationX,
        &effects->attractorFlow.rotationY,
        &effects->attractorFlow.rotationZ,
        &effects->turbulence.strength,
        &effects->turbulence.rotationPerOctave,
        &effects->infiniteZoom.spiralTurns,
        &effects->infiniteZoom.spiralTwist,
        &effects->radialStreak.spiralTwist,
        &effects->radialStreak.spiralTurns,
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

    return false;
}
