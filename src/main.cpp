#include "raylib.h"

#include <stdbool.h>
#include <stdlib.h>
#include "audio/audio.h"
#include "audio/audio_config.h"
#include "analysis/fft.h"
#include "analysis/beat.h"
#include "render/waveform.h"
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
    PostEffect* postEffect;
    AudioCapture* capture;
    UIState* ui;
    PresetPanelState* presetPanel;
    FFTProcessor* fft;
    SpectrumBars* spectrumBars;
    BeatDetector beat;
    AudioConfig audio;
    SpectrumConfig spectrum;
    float audioBuffer[AUDIO_MAX_FRAMES_PER_UPDATE * AUDIO_CHANNELS];
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    int selectedWaveform;
    WaveformMode mode;
    float waveformAccumulator;
    uint64_t globalTick;  // Shared counter for synchronized rotation
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
    if (ctx->fft != NULL) {
        FFTProcessorUninit(ctx->fft);
    }
    if (ctx->spectrumBars != NULL) {
        SpectrumBarsUninit(ctx->spectrumBars);
    }
    free(ctx);
}

static AppContext* AppContextInit(int screenW, int screenH)
{
    AppContext* ctx = (AppContext*)calloc(1, sizeof(AppContext));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->postEffect = PostEffectInit(screenW, screenH);
    if (ctx->postEffect == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->capture = AudioCaptureInit();
    if (ctx->capture == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    if (!AudioCaptureStart(ctx->capture)) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->ui = UIStateInit();
    if (ctx->ui == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->presetPanel = PresetPanelInit();
    if (ctx->presetPanel == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->waveformCount = 1;
    ctx->waveforms[0] = WaveformConfig{};
    ctx->mode = WAVEFORM_LINEAR;

    ctx->fft = FFTProcessorInit();
    if (ctx->fft == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->spectrumBars = SpectrumBarsInit();
    if (ctx->spectrumBars == NULL) {
        AppContextUninit(ctx);
        return NULL;
    }

    ctx->spectrum = SpectrumConfig{};
    BeatDetectorInit(&ctx->beat);

    return ctx;
}

static void UpdateWaveformAudio(AppContext* ctx, float deltaTime)
{
    // Drain all available audio from the ring buffer
    const uint32_t available = AudioCaptureAvailable(ctx->capture);
    if (available == 0) {
        ctx->globalTick++;
        return;
    }

    // Cap to buffer size
    uint32_t framesToRead = available;
    if (framesToRead > AUDIO_MAX_FRAMES_PER_UPDATE) {
        framesToRead = AUDIO_MAX_FRAMES_PER_UPDATE;
    }

    const uint32_t framesRead = AudioCaptureRead(ctx->capture, ctx->audioBuffer, framesToRead);
    if (framesRead == 0) {
        ctx->globalTick++;
        return;
    }

    // Feed audio to FFT processor and process beat detection when FFT updates
    // Loop until all samples consumed (each FFT uses 512 new samples with 75% overlap)
    uint32_t offset = 0;
    bool hadFFTUpdate = false;
    while (offset < framesRead) {
        int consumed = FFTProcessorFeed(ctx->fft, ctx->audioBuffer + offset * AUDIO_CHANNELS, framesRead - offset);
        offset += consumed;
        if (FFTProcessorUpdate(ctx->fft)) {
            hadFFTUpdate = true;
            const float* magnitude = FFTProcessorGetMagnitude(ctx->fft);
            int binCount = FFTProcessorGetBinCount(ctx->fft);
            BeatDetectorProcess(&ctx->beat, magnitude, binCount, deltaTime,
                                ctx->postEffect->effects.beatSensitivity);
            SpectrumBarsProcess(ctx->spectrumBars, magnitude, binCount, &ctx->spectrum);
        }
    }
    if (!hadFFTUpdate) {
        // Decay beat intensity even when no new FFT data
        BeatDetectorProcess(&ctx->beat, NULL, 0, deltaTime,
                            ctx->postEffect->effects.beatSensitivity);
    }

    // Waveform uses only the last 1024 frames (most recent audio)
    uint32_t waveformOffset = 0;
    uint32_t waveformFrames = framesRead;
    if (framesRead > AUDIO_BUFFER_FRAMES) {
        waveformOffset = framesRead - AUDIO_BUFFER_FRAMES;
        waveformFrames = AUDIO_BUFFER_FRAMES;
    }

    ProcessWaveformBase(ctx->audioBuffer + ((size_t)waveformOffset * AUDIO_CHANNELS),
                        waveformFrames, ctx->waveform, ctx->audio.channelMode);

    for (int i = 0; i < ctx->waveformCount; i++) {
        ProcessWaveformSmooth(ctx->waveform, ctx->waveformExtended[i], ctx->waveforms[i].smoothness);
    }

    ctx->globalTick++;
}

static void RenderWaveforms(AppContext* ctx, RenderContext* renderCtx)
{
    if (ctx->mode == WAVEFORM_LINEAR) {
        // Linear mode shows only the first waveform - horizontal layout doesn't suit multiple layers
        DrawWaveformLinear(ctx->waveformExtended[0], WAVEFORM_SAMPLES, renderCtx, &ctx->waveforms[0], ctx->globalTick);
        if (ctx->spectrum.enabled) {
            SpectrumBarsDrawLinear(ctx->spectrumBars, renderCtx, &ctx->spectrum, ctx->globalTick);
        }
    } else {
        for (int i = 0; i < ctx->waveformCount; i++) {
            DrawWaveformCircular(ctx->waveformExtended[i], WAVEFORM_EXTENDED, renderCtx, &ctx->waveforms[i], ctx->globalTick);
        }
        if (ctx->spectrum.enabled) {
            SpectrumBarsDrawCircular(ctx->spectrumBars, renderCtx, &ctx->spectrum, ctx->globalTick);
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

        if (ctx->waveformAccumulator >= waveformUpdateInterval) {
            UpdateWaveformAudio(ctx, waveformUpdateInterval);
            ctx->waveformAccumulator = 0.0f;
        }

        RenderContext renderCtx = {
            .screenW = ctx->postEffect->screenWidth,
            .screenH = ctx->postEffect->screenHeight,
            .centerX = ctx->postEffect->screenWidth / 2,
            .centerY = ctx->postEffect->screenHeight / 2,
            .minDim = (float)(ctx->postEffect->screenWidth < ctx->postEffect->screenHeight ? ctx->postEffect->screenWidth : ctx->postEffect->screenHeight)
        };

        const float beatIntensity = BeatDetectorGetIntensity(&ctx->beat);
        PostEffectBeginAccum(ctx->postEffect, deltaTime, beatIntensity);
            RenderWaveforms(ctx, &renderCtx);
        PostEffectEndAccum(ctx->postEffect);

        BeginDrawing();
            ClearBackground(BLACK);
            PostEffectToScreen(ctx->postEffect);
            DrawText(TextFormat("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f), 10, 10, 16, GRAY);
            DrawText(ctx->mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 30, 16, GRAY);

            AppConfigs configs = {
                .waveforms = ctx->waveforms,
                .waveformCount = &ctx->waveformCount,
                .selectedWaveform = &ctx->selectedWaveform,
                .effects = &ctx->postEffect->effects,
                .audio = &ctx->audio,
                .spectrum = &ctx->spectrum,
                .beat = &ctx->beat
            };
            int panelY = UIDrawPresetPanel(ctx->presetPanel, 55, &configs);
            UIDrawWaveformPanel(ctx->ui, panelY, &configs);
        EndDrawing();
    }

    AppContextUninit(ctx);
    CloseWindow();
    return 0;
}
