#include "raylib.h"

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "audio/audio.h"
#include "audio/audio_config.h"
#include "analysis/analysis_pipeline.h"
#include "render/waveform_pipeline.h"
#include "render/spectrum_bars.h"
#include "config/spectrum_bars_config.h"
#include "config/app_configs.h"
#include "render/post_effect.h"
#include "ui/ui_main.h"
#include "ui/ui_panel_preset.h"

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

typedef struct AppContext {
    AnalysisPipeline analysis;
    WaveformPipeline waveformPipeline;
    PostEffect* postEffect;
    AudioCapture* capture;
    UIState* ui;
    PresetPanelState* presetPanel;
    SpectrumBars* spectrumBars;
    AudioConfig audio;
    SpectrumConfig spectrum;
    BandConfig bandConfig;
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    int selectedWaveform;
    WaveformMode mode;
    float waveformAccumulator;
} AppContext;

static void AppContextUninit(AppContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->presetPanel != NULL) {
        PresetPanelUninit(ctx->presetPanel);
    }
    if (ctx->ui != NULL) {
        UIStateUninit(ctx->ui);
    }
    if (ctx->capture != NULL) {
        AudioCaptureStop(ctx->capture);
        AudioCaptureUninit(ctx->capture);
    }
    if (ctx->postEffect != NULL) {
        PostEffectUninit(ctx->postEffect);
    }
    AnalysisPipelineUninit(&ctx->analysis);
    WaveformPipelineUninit(&ctx->waveformPipeline);
    if (ctx->spectrumBars != NULL) {
        SpectrumBarsUninit(ctx->spectrumBars);
    }
    free(ctx);
}

// Assign result to ptr; on NULL, cleanup and return NULL
#define INIT_OR_FAIL(ptr, expr) \
    do { (ptr) = (expr); if ((ptr) == NULL) { AppContextUninit(ctx); return NULL; } } while (0)

// Evaluate bool expr; on false, cleanup and return NULL
#define CHECK_OR_FAIL(expr) \
    do { if (!(expr)) { AppContextUninit(ctx); return NULL; } } while (0)

static AppContext* AppContextInit(int screenW, int screenH)
{
    AppContext* ctx = (AppContext*)calloc(1, sizeof(AppContext));
    if (ctx == NULL) return NULL;

    INIT_OR_FAIL(ctx->postEffect, PostEffectInit(screenW, screenH));
    INIT_OR_FAIL(ctx->capture, AudioCaptureInit());
    CHECK_OR_FAIL(AudioCaptureStart(ctx->capture));
    INIT_OR_FAIL(ctx->ui, UIStateInit());
    INIT_OR_FAIL(ctx->presetPanel, PresetPanelInit());

    ctx->waveformCount = 1;
    ctx->waveforms[0] = WaveformConfig{};
    ctx->mode = WAVEFORM_LINEAR;

    CHECK_OR_FAIL(AnalysisPipelineInit(&ctx->analysis));
    WaveformPipelineInit(&ctx->waveformPipeline);
    INIT_OR_FAIL(ctx->spectrumBars, SpectrumBarsInit());
    ctx->spectrum = SpectrumConfig{};

    return ctx;
}

#undef INIT_OR_FAIL
#undef CHECK_OR_FAIL

// Visual updates run at 20Hz (sufficient for smooth display)
static void UpdateVisuals(AppContext* ctx)
{
    // Spectrum bars use FFT magnitude
    SpectrumBarsProcess(ctx->spectrumBars, ctx->analysis.fft.magnitude, FFT_BIN_COUNT, &ctx->spectrum);

    // Waveform processing
    WaveformPipelineProcess(&ctx->waveformPipeline,
                            ctx->analysis.audioBuffer,
                            ctx->analysis.lastFramesRead,
                            ctx->waveforms,
                            ctx->waveformCount,
                            ctx->audio.channelMode);
}

static void RenderWaveforms(AppContext* ctx, RenderContext* renderCtx)
{
    const bool circular = (ctx->mode == WAVEFORM_CIRCULAR);
    const uint64_t tick = ctx->waveformPipeline.globalTick;

    WaveformPipelineDraw(&ctx->waveformPipeline, renderCtx, ctx->waveforms, ctx->waveformCount, circular);

    if (ctx->spectrum.enabled) {
        if (circular) {
            SpectrumBarsDrawCircular(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick);
        } else {
            SpectrumBarsDrawLinear(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick);
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AudioJones");
    SetTargetFPS(60);

    AppContext* ctx = AppContextInit(1920, 1080);
    if (ctx == NULL) {
        CloseWindow();
        return -1;
    }

    const float waveformUpdateInterval = 1.0f / 20.0f;

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();
        ctx->waveformAccumulator += deltaTime;

        if (IsWindowResized()) {
            PostEffectResize(ctx->postEffect, GetScreenWidth(), GetScreenHeight());
        }

        if (IsKeyPressed(KEY_SPACE)) {
            ctx->mode = (ctx->mode == WAVEFORM_LINEAR) ? WAVEFORM_CIRCULAR : WAVEFORM_LINEAR;
        }

        // Audio analysis every frame for accurate beat detection
        AnalysisPipelineProcess(&ctx->analysis, ctx->capture, deltaTime);

        // Visual updates at 20Hz (sufficient for smooth display)
        if (ctx->waveformAccumulator >= waveformUpdateInterval) {
            UpdateVisuals(ctx);
            ctx->waveformAccumulator = 0.0f;
        }

        RenderContext renderCtx = {
            .screenW = ctx->postEffect->screenWidth,
            .screenH = ctx->postEffect->screenHeight,
            .centerX = ctx->postEffect->screenWidth / 2,
            .centerY = ctx->postEffect->screenHeight / 2,
            .minDim = (float)(ctx->postEffect->screenWidth < ctx->postEffect->screenHeight ? ctx->postEffect->screenWidth : ctx->postEffect->screenHeight)
        };

        const float beatIntensity = BeatDetectorGetIntensity(&ctx->analysis.beat);
        PostEffectBeginAccum(ctx->postEffect, deltaTime, beatIntensity);
            RenderWaveforms(ctx, &renderCtx);
        PostEffectEndAccum();

        BeginDrawing();
            ClearBackground(BLACK);
            PostEffectToScreen(ctx->postEffect, ctx->waveformPipeline.globalTick);
            DrawText(TextFormat("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f), 10, 10, 16, GRAY);
            DrawText(ctx->mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 30, 16, GRAY);

            AppConfigs configs = {
                .waveforms = ctx->waveforms,
                .waveformCount = &ctx->waveformCount,
                .selectedWaveform = &ctx->selectedWaveform,
                .effects = &ctx->postEffect->effects,
                .audio = &ctx->audio,
                .spectrum = &ctx->spectrum,
                .beat = &ctx->analysis.beat,
                .bands = &ctx->bandConfig,
                .bandEnergies = &ctx->analysis.bands
            };
            UIUpdateWindowHoverState(ctx->ui);
            int panelY = UIDrawPresetPanel(ctx->presetPanel, 55, &configs);
            UIDrawWaveformPanel(ctx->ui, panelY, &configs);
        EndDrawing();
    }

    AppContextUninit(ctx);
    CloseWindow();
    return 0;
}
