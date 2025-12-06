#include "raylib.h"

#include <stdbool.h>
#include <stdlib.h>
#include "audio.h"
#include "waveform.h"
#include "visualizer.h"
#include "ui.h"

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

typedef struct AppContext {
    Visualizer* vis;
    AudioCapture* capture;
    UIState* ui;
    float audioBuffer[AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS];
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    int selectedWaveform;
    WaveformMode mode;
    float waveformAccumulator;
} AppContext;

static void AppContextUninit(AppContext* ctx)
{
    if (!ctx) {
        return;
    }
    if (ctx->ui) {
        UIStateUninit(ctx->ui);
    }
    if (ctx->capture) {
        AudioCaptureStop(ctx->capture);
        AudioCaptureUninit(ctx->capture);
    }
    if (ctx->vis) {
        VisualizerUninit(ctx->vis);
    }
    free(ctx);
}

static AppContext* AppContextInit(int screenW, int screenH)
{
    AppContext* ctx = (AppContext*)calloc(1, sizeof(AppContext));
    if (!ctx) {
        return NULL;
    }

    ctx->vis = VisualizerInit(screenW, screenH);
    if (!ctx->vis) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->capture = AudioCaptureInit();
    if (!ctx->capture) {
        AppContextUninit(ctx);
        return NULL;
    }

    if (!AudioCaptureStart(ctx->capture)) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->ui = UIStateInit();
    if (!ctx->ui) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->waveformCount = 1;
    ctx->waveforms[0] = WaveformConfig{};
    ctx->mode = WAVEFORM_LINEAR;

    return ctx;
}

static void UpdateWaveformAudio(AppContext* ctx)
{
    uint32_t framesRead = AudioCaptureRead(ctx->capture, ctx->audioBuffer, AUDIO_BUFFER_FRAMES);
    if (framesRead > 0) {
        ProcessWaveformBase(ctx->audioBuffer, framesRead, ctx->waveform);
        for (int i = 0; i < ctx->waveformCount; i++) {
            ProcessWaveformSmooth(ctx->waveform, ctx->waveformExtended[i], ctx->waveforms[i].smoothness);
        }
    }
    for (int i = 0; i < ctx->waveformCount; i++) {
        ctx->waveforms[i].rotation += ctx->waveforms[i].rotationSpeed;
    }
}

static void RenderWaveforms(AppContext* ctx, RenderContext* renderCtx)
{
    if (ctx->mode == WAVEFORM_LINEAR) {
        DrawWaveformLinear(ctx->waveformExtended[0], WAVEFORM_SAMPLES, renderCtx, &ctx->waveforms[0]);
    } else {
        for (int i = 0; i < ctx->waveformCount; i++) {
            DrawWaveformCircular(ctx->waveformExtended[i], WAVEFORM_EXTENDED, renderCtx, &ctx->waveforms[i]);
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AudioJones");
    SetTargetFPS(60);

    AppContext* ctx = AppContextInit(1920, 1080);
    if (!ctx) {
        CloseWindow();
        return -1;
    }

    const float waveformUpdateInterval = 1.0f / 20.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        ctx->waveformAccumulator += deltaTime;

        if (IsWindowResized()) {
            VisualizerResize(ctx->vis, GetScreenWidth(), GetScreenHeight());
        }

        if (IsKeyPressed(KEY_SPACE)) {
            ctx->mode = (ctx->mode == WAVEFORM_LINEAR) ? WAVEFORM_CIRCULAR : WAVEFORM_LINEAR;
        }

        if (ctx->waveformAccumulator >= waveformUpdateInterval) {
            UpdateWaveformAudio(ctx);
            ctx->waveformAccumulator = 0.0f;
        }

        RenderContext renderCtx = {
            .screenW = ctx->vis->screenWidth,
            .screenH = ctx->vis->screenHeight,
            .centerX = ctx->vis->screenWidth / 2,
            .centerY = ctx->vis->screenHeight / 2,
            .minDim = (float)(ctx->vis->screenWidth < ctx->vis->screenHeight ? ctx->vis->screenWidth : ctx->vis->screenHeight)
        };

        VisualizerBeginAccum(ctx->vis, deltaTime);
            RenderWaveforms(ctx, &renderCtx);
        VisualizerEndAccum(ctx->vis);

        BeginDrawing();
            ClearBackground(BLACK);
            VisualizerToScreen(ctx->vis);
            DrawText(TextFormat("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f), 10, 10, 16, GRAY);
            DrawText(ctx->mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 30, 16, GRAY);

            UIBeginPanels(ctx->ui, 55);
            UIDrawWaveformPanel(ctx->ui, ctx->waveforms, &ctx->waveformCount, &ctx->selectedWaveform, &ctx->vis->halfLife);
            UIDrawPresetPanel(ctx->ui, ctx->waveforms, &ctx->waveformCount, &ctx->vis->halfLife);
        EndDrawing();
    }

    AppContextUninit(ctx);
    CloseWindow();
    return 0;
}
