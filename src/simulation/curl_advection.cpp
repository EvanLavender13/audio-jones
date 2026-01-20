#include "curl_advection.h"
#include "trail_map.h"
#include "shader_utils.h"
#include "render/color_config.h"
#include "render/color_lut.h"
#include "rlgl.h"
#include "external/glad.h"
#include <stdlib.h>
#include <math.h>

static const char* COMPUTE_SHADER_PATH = "shaders/curl_advection.glsl";

static GLuint CreateStateTexture(int width, int height)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

static void InitializeStateWithNoise(GLuint texture, int width, int height)
{
    const int size = width * height * 4;
    float* data = (float*)malloc(size * sizeof(float));
    if (data == NULL) {
        return;
    }

    for (int i = 0; i < width * height; i++) {
        // Random velocity in [-0.1, 0.1]
        data[i * 4 + 0] = ((float)GetRandomValue(-100, 100) / 1000.0f);
        data[i * 4 + 1] = ((float)GetRandomValue(-100, 100) / 1000.0f);
        // Divergence starts at zero
        data[i * 4 + 2] = 0.0f;
        data[i * 4 + 3] = 0.0f;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data);
}

static GLuint LoadComputeProgram(CurlAdvection* ca)
{
    char* shaderSource = SimLoadShaderSource(COMPUTE_SHADER_PATH);
    if (shaderSource == NULL) {
        return 0;
    }

    const unsigned int shaderId = rlCompileShader(shaderSource, RL_COMPUTE_SHADER);
    UnloadFileText(shaderSource);

    if (shaderId == 0) {
        TraceLog(LOG_ERROR, "CURL_ADVECTION: Failed to compile compute shader");
        return 0;
    }

    const GLuint program = rlLoadComputeShaderProgram(shaderId);
    if (program == 0) {
        TraceLog(LOG_ERROR, "CURL_ADVECTION: Failed to load compute shader program");
        return 0;
    }

    ca->resolutionLoc = rlGetLocationUniform(program, "resolution");
    ca->stepsLoc = rlGetLocationUniform(program, "steps");
    ca->advectionCurlLoc = rlGetLocationUniform(program, "advectionCurl");
    ca->curlScaleLoc = rlGetLocationUniform(program, "curlScale");
    ca->laplacianScaleLoc = rlGetLocationUniform(program, "laplacianScale");
    ca->pressureScaleLoc = rlGetLocationUniform(program, "pressureScale");
    ca->divergenceScaleLoc = rlGetLocationUniform(program, "divergenceScale");
    ca->divergenceUpdateLoc = rlGetLocationUniform(program, "divergenceUpdate");
    ca->divergenceSmoothingLoc = rlGetLocationUniform(program, "divergenceSmoothing");
    ca->selfAmpLoc = rlGetLocationUniform(program, "selfAmp");
    ca->updateSmoothingLoc = rlGetLocationUniform(program, "updateSmoothing");
    ca->injectionIntensityLoc = rlGetLocationUniform(program, "injectionIntensity");
    ca->injectionThresholdLoc = rlGetLocationUniform(program, "injectionThreshold");
    ca->valueLoc = rlGetLocationUniform(program, "value");

    return program;
}

bool CurlAdvectionSupported(void)
{
    const int version = rlGetVersion();
    return version == RL_OPENGL_43;
}

CurlAdvection* CurlAdvectionInit(int width, int height, const CurlAdvectionConfig* config)
{
    if (!CurlAdvectionSupported()) {
        TraceLog(LOG_WARNING, "CURL_ADVECTION: Compute shaders not supported (requires OpenGL 4.3)");
        return NULL;
    }

    CurlAdvection* ca = (CurlAdvection*)calloc(1, sizeof(CurlAdvection));
    if (ca == NULL) {
        return NULL;
    }

    ca->width = width;
    ca->height = height;
    ca->config = (config != NULL) ? *config : CurlAdvectionConfig{};
    ca->currentBuffer = 0;
    ca->supported = true;

    ca->computeProgram = LoadComputeProgram(ca);
    if (ca->computeProgram == 0) {
        goto cleanup;
    }

    ca->stateTextures[0] = CreateStateTexture(width, height);
    ca->stateTextures[1] = CreateStateTexture(width, height);
    if (ca->stateTextures[0] == 0 || ca->stateTextures[1] == 0) {
        TraceLog(LOG_ERROR, "CURL_ADVECTION: Failed to create state textures");
        goto cleanup;
    }

    InitializeStateWithNoise(ca->stateTextures[0], width, height);
    InitializeStateWithNoise(ca->stateTextures[1], width, height);

    ca->trailMap = TrailMapInit(width, height);
    if (ca->trailMap == NULL) {
        TraceLog(LOG_ERROR, "CURL_ADVECTION: Failed to create trail map");
        goto cleanup;
    }

    ca->colorLUT = ColorLUTInit(&ca->config.color);
    if (ca->colorLUT == NULL) {
        TraceLog(LOG_ERROR, "CURL_ADVECTION: Failed to create color LUT");
        goto cleanup;
    }

    ca->debugShader = LoadShader(NULL, "shaders/trail_debug.fs");
    if (ca->debugShader.id == 0) {
        TraceLog(LOG_WARNING, "CURL_ADVECTION: Failed to load debug shader, using default");
    }

    TraceLog(LOG_INFO, "CURL_ADVECTION: Initialized at %dx%d", width, height);
    return ca;

cleanup:
    CurlAdvectionUninit(ca);
    return NULL;
}

void CurlAdvectionUninit(CurlAdvection* ca)
{
    if (ca == NULL) {
        return;
    }

    if (ca->stateTextures[0] != 0) {
        glDeleteTextures(1, &ca->stateTextures[0]);
    }
    if (ca->stateTextures[1] != 0) {
        glDeleteTextures(1, &ca->stateTextures[1]);
    }
    TrailMapUninit(ca->trailMap);
    ColorLUTUninit(ca->colorLUT);
    if (ca->debugShader.id != 0) {
        UnloadShader(ca->debugShader);
    }
    rlUnloadShaderProgram(ca->computeProgram);
    free(ca);
}

void CurlAdvectionUpdate(CurlAdvection* ca, float deltaTime, Texture2D accumTexture)
{
    if (ca == NULL || !ca->supported || !ca->config.enabled) {
        return;
    }

    const float resolution[2] = { (float)ca->width, (float)ca->height };
    const int readBuffer = ca->currentBuffer;
    const int writeBuffer = 1 - ca->currentBuffer;

    rlEnableShader(ca->computeProgram);

    rlSetUniform(ca->resolutionLoc, resolution, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(ca->stepsLoc, &ca->config.steps, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(ca->advectionCurlLoc, &ca->config.advectionCurl, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->curlScaleLoc, &ca->config.curlScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->laplacianScaleLoc, &ca->config.laplacianScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->pressureScaleLoc, &ca->config.pressureScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->divergenceScaleLoc, &ca->config.divergenceScale, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->divergenceUpdateLoc, &ca->config.divergenceUpdate, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->divergenceSmoothingLoc, &ca->config.divergenceSmoothing, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->selfAmpLoc, &ca->config.selfAmp, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->updateSmoothingLoc, &ca->config.updateSmoothing, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->injectionIntensityLoc, &ca->config.injectionIntensity, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(ca->injectionThresholdLoc, &ca->config.injectionThreshold, RL_SHADER_UNIFORM_FLOAT, 1);

    float value;
    if (ca->config.color.mode == COLOR_MODE_SOLID) {
        float h;
        float s;
        ColorConfigRGBToHSV(ca->config.color.solid, &h, &s, &value);
    } else if (ca->config.color.mode == COLOR_MODE_GRADIENT) {
        value = 1.0f;
    } else {
        value = ca->config.color.rainbowVal;
    }
    rlSetUniform(ca->valueLoc, &value, RL_SHADER_UNIFORM_FLOAT, 1);

    // Bind state texture for reading (texture unit 0)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ca->stateTextures[readBuffer]);

    // Bind state texture for writing (image unit 1)
    glBindImageTexture(1, ca->stateTextures[writeBuffer], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    // Bind trail map for writing (image unit 2)
    rlBindImageTexture(TrailMapGetTexture(ca->trailMap).id, 2, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    // Bind color LUT (texture unit 3)
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ColorLUTGetTexture(ca->colorLUT).id);

    // Bind accumulation texture (texture unit 4)
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, accumTexture.id);

    const int workGroupX = (ca->width + 15) / 16;
    const int workGroupY = (ca->height + 15) / 16;
    rlComputeShaderDispatch((unsigned int)workGroupX, (unsigned int)workGroupY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    rlDisableShader();

    ca->currentBuffer = writeBuffer;
}

void CurlAdvectionProcessTrails(CurlAdvection* ca, float deltaTime)
{
    if (ca == NULL || !ca->supported || !ca->config.enabled) {
        return;
    }

    // Use fixed decay since the simulation generates continuous output
    TrailMapProcess(ca->trailMap, deltaTime, 0.5f, 0);
}

void CurlAdvectionResize(CurlAdvection* ca, int width, int height)
{
    if (ca == NULL || (width == ca->width && height == ca->height)) {
        return;
    }

    ca->width = width;
    ca->height = height;

    // Recreate state textures at new size
    if (ca->stateTextures[0] != 0) {
        glDeleteTextures(1, &ca->stateTextures[0]);
    }
    if (ca->stateTextures[1] != 0) {
        glDeleteTextures(1, &ca->stateTextures[1]);
    }

    ca->stateTextures[0] = CreateStateTexture(width, height);
    ca->stateTextures[1] = CreateStateTexture(width, height);
    InitializeStateWithNoise(ca->stateTextures[0], width, height);
    InitializeStateWithNoise(ca->stateTextures[1], width, height);

    TrailMapResize(ca->trailMap, width, height);
    ca->currentBuffer = 0;
}

void CurlAdvectionReset(CurlAdvection* ca)
{
    if (ca == NULL) {
        return;
    }

    InitializeStateWithNoise(ca->stateTextures[0], ca->width, ca->height);
    InitializeStateWithNoise(ca->stateTextures[1], ca->width, ca->height);
    TrailMapClear(ca->trailMap);
    ca->currentBuffer = 0;
}

void CurlAdvectionApplyConfig(CurlAdvection* ca, const CurlAdvectionConfig* newConfig)
{
    if (ca == NULL || newConfig == NULL) {
        return;
    }

    ColorLUTUpdate(ca->colorLUT, &newConfig->color);
    ca->config = *newConfig;
}

void CurlAdvectionDrawDebug(CurlAdvection* ca)
{
    if (ca == NULL || !ca->supported || !ca->config.enabled) {
        return;
    }

    const Texture2D trailTex = TrailMapGetTexture(ca->trailMap);
    if (ca->debugShader.id != 0) {
        BeginShaderMode(ca->debugShader);
    }
    DrawTextureRec(trailTex,
        {0, 0, (float)ca->width, (float)-ca->height},
        {0, 0}, WHITE);
    if (ca->debugShader.id != 0) {
        EndShaderMode();
    }
}
