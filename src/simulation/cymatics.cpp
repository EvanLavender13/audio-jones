#include "cymatics.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "render/color_lut.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/cymatics.glsl";

static GLuint LoadComputeProgram(Cymatics* cym)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "CYMATICS: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "CYMATICS: Failed to load compute shader program");
        return 0;
    }

    cym->resolutionLoc = rlGetLocationUniform(program, "resolution");
    cym->waveScaleLoc = rlGetLocationUniform(program, "waveScale");
    cym->falloffLoc = rlGetLocationUniform(program, "falloff");
    cym->visualGainLoc = rlGetLocationUniform(program, "visualGain");
    cym->contourCountLoc = rlGetLocationUniform(program, "contourCount");
    cym->bufferSizeLoc = rlGetLocationUniform(program, "bufferSize");
    cym->writeIndexLoc = rlGetLocationUniform(program, "writeIndex");
    cym->valueLoc = rlGetLocationUniform(program, "value");
    cym->sourcesLoc = rlGetLocationUniform(program, "sources");
    cym->sourceCountLoc = rlGetLocationUniform(program, "sourceCount");

    return program;
}

bool CymaticsSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

Cymatics* CymaticsInit(int width, int height, const CymaticsConfig* config)
{
    if (!CymaticsSupported()) {
        TraceLog(LOG_WARNING, "CYMATICS: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    Cymatics* cym = (Cymatics*)calloc(1, sizeof(Cymatics));
    if (cym == NULL) {
        return NULL;
    }

    cym->width = width;
    cym->height = height;
    cym->config = (config != NULL) ? *config : CymaticsConfig{};
    cym->supported = true;
    cym->sourcePhase = 0.0f;

    cym->computeProgram = LoadComputeProgram(cym);
    if (cym->computeProgram == 0) {
        goto cleanup;
    }

    cym->trailMap = TrailMapInit(width, height);
    if (cym->trailMap == NULL) {
        TraceLog(LOG_ERROR, "CYMATICS: Failed to create trail map");
        goto cleanup;
    }

    cym->colorLUT = ColorLUTInit(&cym->config.color);
    if (cym->colorLUT == NULL) {
        TraceLog(LOG_ERROR, "CYMATICS: Failed to create color LUT");
        goto cleanup;
    }

    cym->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (cym->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "CYMATICS: Failed to load debug shader, using default");
    }

    TraceLog(LOG_INFO, "CYMATICS: Initialized at %dx%d", width, height);
    return cym;

cleanup:
    CymaticsUninit(cym);
    return NULL;
}

void CymaticsUninit(Cymatics* cym)
{
    if (cym == NULL) {
        return;
    }

    TrailMapUninit(cym->trailMap);
    ColorLUTUninit(cym->colorLUT);
    if (cym->debugShader.id != 0) {
        UnloadShader(cym->debugShader);
    }
    rlUnloadShaderProgram(cym->computeProgram);
    free(cym);
}

void CymaticsUpdate(Cymatics* cym, Texture2D waveformTexture, int writeIndex, float deltaTime)
{
    if (cym == NULL || !cym->supported || !cym->config.enabled) {
        return;
    }

    // CPU phase accumulation (Hz to radians/sec)
    const float TWO_PI = 6.28318530718f;
    cym->sourcePhase += deltaTime * TWO_PI;

    // Compute animated source positions with circular distribution
    float sources[16];  // 8 sources * 2 components
    const float amp = cym->config.sourceAmplitude;
    const float phaseX = cym->sourcePhase * cym->config.sourceFreqX;
    const float phaseY = cym->sourcePhase * cym->config.sourceFreqY;
    const int count = cym->config.sourceCount > 8 ? 8 : cym->config.sourceCount;
    for (int i = 0; i < count; i++) {
        const float angle = TWO_PI * (float)i / (float)count + cym->config.patternAngle;
        const float baseX = cym->config.baseRadius * cosf(angle);
        const float baseY = cym->config.baseRadius * sinf(angle);
        const float offset = (float)i / (float)count * TWO_PI;
        sources[i * 2 + 0] = baseX + amp * sinf(phaseX + offset);
        sources[i * 2 + 1] = baseY + amp * cosf(phaseY + offset);
    }

    const float resolution[2] = { (float)cym->width, (float)cym->height };
    const int bufferSize = waveformTexture.width;

    rlEnableShader(cym->computeProgram);

    rlSetUniform(cym->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(cym->waveScaleLoc, &cym->config.waveScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cym->falloffLoc, &cym->config.falloff, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cym->visualGainLoc, &cym->config.visualGain, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(cym->contourCountLoc, &cym->config.contourCount, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cym->bufferSizeLoc, &bufferSize, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cym->writeIndexLoc, &writeIndex, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(cym->sourcesLoc, sources, RL_SHADER_UNIFORM_VEC2, count);
    rlSetUniform(cym->sourceCountLoc, &count, RL_SHADER_UNIFORM_INT, 1);

    float value;
    if (cym->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        float s;
        ColorConfigRGBToHSV(cym->config.color.solid, &h, &s, &value);
    } else if (cym->config.color.mode == COLOR_MODE_GRADIENT) {
        value = 1.0f;
    } else {
        value = cym->config.color.rainbowVal;
    }
    rlSetUniform(cym->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    // Bind waveform texture (unit 0)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, waveformTexture.id);

    // Bind trail map for writing (image unit 1)
    rlBindImageTexture(TrailMapGetTexture(cym->trailMap).id, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    // Bind color LUT (texture unit 3)
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ColorLUTGetTexture(cym->colorLUT).id);

    const int workGroupX = (cym->width + 15) / 16;
    const int workGroupY = (cym->height + 15) / 16;
    rlComputeShaderDispatch((unsigned int)workGroupX, (unsigned int)workGroupY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();
}

void CymaticsProcessTrails(Cymatics* cym, float deltaTime)
{
    if (cym == NULL || !cym->supported || !cym->config.enabled) {
        return;
    }

    TrailMapProcess(cym->trailMap, deltaTime, cym->config.decayHalfLife, cym->config.diffusionScale);
}

void CymaticsResize(Cymatics* cym, int width, int height)
{
    if (cym == NULL || (width == cym->width && height == cym->height)) {
        return;
    }

    cym->width = width;
    cym->height = height;

    TrailMapResize(cym->trailMap, width, height);
}

void CymaticsReset(Cymatics* cym)
{
    if (cym == NULL) {
        return;
    }

    TrailMapClear(cym->trailMap);
}

void CymaticsApplyConfig(Cymatics* cym, const CymaticsConfig* newConfig)
{
    if (cym == NULL || newConfig == NULL) {
        return;
    }

    ColorLUTUpdate(cym->colorLUT, &newConfig->color);
    cym->config = *newConfig;
}

void CymaticsDrawDebug(Cymatics* cym)
{
    if (cym == NULL || !cym->supported || !cym->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(cym->trailMap);
    if (cym->debugShader.id != 0) {
        BeginShaderMode(cym->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)cym->width, (float)-cym->height},
        {0, 0}, WHITE);
    if (cym->debugShader.id != 0) {
        EndShaderMode();
    }
}
