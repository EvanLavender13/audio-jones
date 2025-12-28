#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

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
#include "render/render_pipeline.h"
#include "render/physarum.h"
#include "ui/imgui_panels.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "automation/param_registry.h"
#include "automation/lfo.h"

typedef struct AppContext {
    AnalysisPipeline analysis;
    WaveformPipeline waveformPipeline;
    PostEffect* postEffect;
    AudioCapture* capture;
    SpectrumBars* spectrumBars;
    AudioConfig audio;
    SpectrumConfig spectrum;
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    int selectedWaveform;
    float waveformAccumulator;
    bool uiVisible;
    ModSources modSources;
    LFOState modLFOs[4];
    LFOConfig modLFOConfigs[4];
} AppContext;

static void AppContextUninit(AppContext* ctx)
{
    if (ctx == NULL) {
        return;
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
    ModEngineUninit();
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

    ctx->waveformCount = 1;
    ctx->waveforms[0] = WaveformConfig{};
    ctx->uiVisible = true;

    CHECK_OR_FAIL(AnalysisPipelineInit(&ctx->analysis));
    WaveformPipelineInit(&ctx->waveformPipeline);
    INIT_OR_FAIL(ctx->spectrumBars, SpectrumBarsInit());
    ctx->spectrum = SpectrumConfig{};

    // Initialize modulation system
    ModEngineInit();
    ParamRegistryInit(&ctx->postEffect->effects);
    ModSourcesInit(&ctx->modSources);
    for (int i = 0; i < 4; i++) {
        LFOStateInit(&ctx->modLFOs[i]);
        ctx->modLFOConfigs[i] = LFOConfig{};
    }

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

// Draws waveforms and spectrum at full opacity (for physarum trail map)
static void RenderWaveformsFull(AppContext* ctx, RenderContext* renderCtx)
{
    const bool circular = ctx->postEffect->effects.circular;
    const uint64_t tick = ctx->waveformPipeline.globalTick;

    WaveformPipelineDraw(&ctx->waveformPipeline, renderCtx, ctx->waveforms, ctx->waveformCount, circular, 1.0f);

    if (ctx->spectrum.enabled) {
        if (circular) {
            SpectrumBarsDrawCircular(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick, 1.0f);
        } else {
            SpectrumBarsDrawLinear(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick, 1.0f);
        }
    }
}

// Draws waveforms and spectrum with per-drawable feedbackPhase opacity
// isPreFeedback: true = draw at (1-feedbackPhase), false = draw at feedbackPhase
static void RenderWaveformsWithPhase(AppContext* ctx, RenderContext* renderCtx, bool isPreFeedback)
{
    const bool circular = ctx->postEffect->effects.circular;
    const uint64_t tick = ctx->waveformPipeline.globalTick;
    const float opacityThreshold = 0.001f;

    // Draw each waveform at its own feedbackPhase-based opacity
    for (int i = 0; i < ctx->waveformCount && i < MAX_WAVEFORMS; i++) {
        const float phase = ctx->waveforms[i].feedbackPhase;
        const float opacity = isPreFeedback ? (1.0f - phase) : phase;
        if (opacity < opacityThreshold) {
            continue;
        }
        if (circular) {
            DrawWaveformCircular(ctx->waveformPipeline.waveformExtended[i], WAVEFORM_EXTENDED,
                                 renderCtx, &ctx->waveforms[i], tick, opacity);
        } else {
            DrawWaveformLinear(ctx->waveformPipeline.waveformExtended[i], WAVEFORM_SAMPLES,
                               renderCtx, &ctx->waveforms[i], tick, opacity);
        }
    }

    // Draw spectrum at its feedbackPhase-based opacity
    if (ctx->spectrum.enabled) {
        const float phase = ctx->spectrum.feedbackPhase;
        const float opacity = isPreFeedback ? (1.0f - phase) : phase;
        if (opacity >= opacityThreshold) {
            if (circular) {
                SpectrumBarsDrawCircular(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick, opacity);
            } else {
                SpectrumBarsDrawLinear(ctx->spectrumBars, renderCtx, &ctx->spectrum, tick, opacity);
            }
        }
    }
}

// Renders waveforms to physarum trail map for agent input (always full opacity)
static void RenderWaveformsToPhysarum(AppContext* ctx, RenderContext* renderCtx)
{
    if (ctx->postEffect->physarum == NULL) {
        return;
    }
    if (!PhysarumBeginTrailMapDraw(ctx->postEffect->physarum)) {
        return;
    }
    RenderWaveformsFull(ctx, renderCtx);
    PhysarumEndTrailMapDraw(ctx->postEffect->physarum);
}

// Renders waveforms BEFORE feedback shader at opacity (1 - feedbackPhase)
// These get integrated into the feedback warp/blur effects
static void RenderWaveformsPreFeedback(AppContext* ctx, RenderContext* renderCtx)
{
    PostEffectBeginDrawStage(ctx->postEffect);
    RenderWaveformsWithPhase(ctx, renderCtx, true);
    PostEffectEndDrawStage();
}

// Renders waveforms AFTER feedback shader at opacity feedbackPhase
// These appear crisp on top of the feedback effects
static void RenderWaveformsPostFeedback(AppContext* ctx, RenderContext* renderCtx)
{
    PostEffectBeginDrawStage(ctx->postEffect);
    RenderWaveformsWithPhase(ctx, renderCtx, false);
    PostEffectEndDrawStage();
}

// Standard pipeline: pre-feedback → feedback → physarum → post-feedback → composite
static void RenderStandardPipeline(AppContext* ctx, RenderContext* renderCtx, float deltaTime)
{
    // 1. Draw waveforms that will be integrated into feedback
    RenderWaveformsPreFeedback(ctx, renderCtx);

    // 2. Apply feedback effects (warp, blur, decay)
    RenderPipelineApplyFeedback(ctx->postEffect, deltaTime,
                                 ctx->analysis.fft.magnitude);

    // 3. Draw to physarum trail map (always full opacity for agent sensing)
    RenderWaveformsToPhysarum(ctx, renderCtx);

    // 4. Draw crisp waveforms on top of feedback
    RenderWaveformsPostFeedback(ctx, renderCtx);

    BeginDrawing();
    ClearBackground(BLACK);
    RenderPipelineApplyOutput(ctx->postEffect, ctx->waveformPipeline.globalTick);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AudioJones");
    SetTargetFPS(60);

    // Two-stage rlImGui init for custom font loading
    rlImGuiBeginInitImGui();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "audiojones_layout.ini";

    // Load Roboto font for modern, clean typography
    io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 15.0f);
    rlImGuiEndInitImGui();

    ImGuiApplyNeonTheme();

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
            const int newWidth = GetScreenWidth();
            const int newHeight = GetScreenHeight();
            PostEffectResize(ctx->postEffect, newWidth, newHeight);
        }

        if (IsKeyPressed(KEY_TAB) && !io.WantCaptureKeyboard) {
            ctx->uiVisible = !ctx->uiVisible;
        }

        // Audio analysis every frame for accurate beat detection
        AnalysisPipelineProcess(&ctx->analysis, ctx->capture, deltaTime);

        // Update modulation sources and apply routes
        float lfoOutputs[4];
        for (int i = 0; i < 4; i++) {
            lfoOutputs[i] = LFOProcess(&ctx->modLFOs[i], &ctx->modLFOConfigs[i], deltaTime);
        }
        ModSourcesUpdate(&ctx->modSources, &ctx->analysis.bands,
                         &ctx->analysis.beat, lfoOutputs);
        ModEngineUpdate(deltaTime, &ctx->modSources);

        // Visual updates at 20Hz (sufficient for smooth display)
        if (ctx->waveformAccumulator >= waveformUpdateInterval) {
            UpdateVisuals(ctx);
            ctx->waveformAccumulator = 0.0f;
        }

        const int screenW = ctx->postEffect->screenWidth;
        const int screenH = ctx->postEffect->screenHeight;
        RenderContext renderCtx = {
            .screenW = screenW,
            .screenH = screenH,
            .centerX = screenW / 2,
            .centerY = screenH / 2,
            .minDim = (float)(screenW < screenH ? screenW : screenH)
        };

        RenderStandardPipeline(ctx, &renderCtx, deltaTime);

        if (ctx->uiVisible) {
            AppConfigs configs = {
                .waveforms = ctx->waveforms,
                .waveformCount = &ctx->waveformCount,
                .selectedWaveform = &ctx->selectedWaveform,
                .effects = &ctx->postEffect->effects,
                .audio = &ctx->audio,
                .spectrum = &ctx->spectrum,
                .beat = &ctx->analysis.beat,
                .bandEnergies = &ctx->analysis.bands,
                .lfos = ctx->modLFOConfigs
            };
            rlImGuiBegin();
                ImGuiDrawDockspace();
                ImGuiDrawEffectsPanel(&ctx->postEffect->effects, &ctx->modSources);
                ImGuiDrawWaveformsPanel(ctx->waveforms, &ctx->waveformCount, &ctx->selectedWaveform);
                ImGuiDrawSpectrumPanel(&ctx->spectrum);
                ImGuiDrawAudioPanel(&ctx->audio);
                ImGuiDrawAnalysisPanel(&ctx->analysis.beat, &ctx->analysis.bands);
                ImGuiDrawLFOPanel(ctx->modLFOConfigs);
                ImGuiDrawPresetPanel(&configs);
            rlImGuiEnd();
        } else {
            DrawText("[Tab] Show UI", 10, 10, 16, GRAY);
        }
        EndDrawing();
    }

    rlImGuiShutdown();
    AppContextUninit(ctx);
    CloseWindow();
    return 0;
}
