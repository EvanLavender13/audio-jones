#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "audio/audio.h"
#include "audio/audio_config.h"
#include "analysis/analysis_pipeline.h"
#include "render/drawable.h"
#include "config/app_configs.h"
#include "render/post_effect.h"
#include "render/render_pipeline.h"
#include "render/profiler.h"
#include "ui/imgui_panels.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "automation/param_registry.h"
#include "automation/drawable_params.h"
#include "automation/lfo.h"
#include "ui/ui_units.h"

typedef struct AppContext {
    AnalysisPipeline analysis;
    DrawableState drawableState;
    PostEffect* postEffect;
    AudioCapture* capture;
    AudioConfig audio;

    Drawable drawables[MAX_DRAWABLES];
    int drawableCount;
    int selectedDrawable;

    float updateAccumulator;
    bool uiVisible;
    ModSources modSources;
    LFOState modLFOs[4];
    LFOConfig modLFOConfigs[4];
    Profiler profiler;
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
    DrawableStateUninit(&ctx->drawableState);
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

    // Initialize drawable system with one default waveform
    DrawableStateInit(&ctx->drawableState);
    ctx->drawableCount = 1;
    ctx->drawables[0] = Drawable{};
    ctx->drawables[0].id = 1;
    ctx->selectedDrawable = 0;
    ctx->uiVisible = true;

    CHECK_OR_FAIL(AnalysisPipelineInit(&ctx->analysis));

    // Initialize modulation system
    ModEngineInit();
    ParamRegistryInit(&ctx->postEffect->effects);
    ImGuiDrawDrawablesSyncIdCounter(ctx->drawables, ctx->drawableCount);
    DrawableParamsRegister(&ctx->drawables[0]);
    ModSourcesInit(&ctx->modSources);
    for (int i = 0; i < 4; i++) {
        LFOStateInit(&ctx->modLFOs[i]);
        ctx->modLFOConfigs[i] = LFOConfig{};
    }

    // Register LFO rate params (separate from ParamRegistryInit because configs live in AppContext)
    ModEngineRegisterParam("lfo1.rate", &ctx->modLFOConfigs[0].rate, LFO_RATE_MIN, LFO_RATE_MAX);
    ModEngineRegisterParam("lfo2.rate", &ctx->modLFOConfigs[1].rate, LFO_RATE_MIN, LFO_RATE_MAX);
    ModEngineRegisterParam("lfo3.rate", &ctx->modLFOConfigs[2].rate, LFO_RATE_MIN, LFO_RATE_MAX);
    ModEngineRegisterParam("lfo4.rate", &ctx->modLFOConfigs[3].rate, LFO_RATE_MIN, LFO_RATE_MAX);

    ProfilerInit(&ctx->profiler);

    return ctx;
}

#undef INIT_OR_FAIL
#undef CHECK_OR_FAIL

// Visual updates run at 20Hz (sufficient for smooth display)
static void UpdateVisuals(AppContext* ctx)
{
    DrawableProcessWaveforms(&ctx->drawableState,
                             ctx->analysis.audioBuffer,
                             ctx->analysis.lastFramesRead,
                             ctx->drawables,
                             ctx->drawableCount,
                             ctx->audio.channelMode);

    DrawableProcessSpectrum(&ctx->drawableState,
                            ctx->analysis.fft.magnitude,
                            FFT_BIN_COUNT,
                            ctx->drawables,
                            ctx->drawableCount);

    AnalysisPipelineUpdateWaveformHistory(&ctx->analysis);
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

    const float updateInterval = 1.0f / 20.0f;

    while (!WindowShouldClose())
    {
        const float deltaTime = GetFrameTime();
        ctx->updateAccumulator += deltaTime;

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

        // Accumulate rotation speeds every frame
        DrawableTickRotations(ctx->drawables, ctx->drawableCount);

        // Visual updates at 20Hz (sufficient for smooth display)
        if (ctx->updateAccumulator >= updateInterval) {
            UpdateVisuals(ctx);
            ctx->updateAccumulator = 0.0f;
        }

        const int screenW = ctx->postEffect->screenWidth;
        const int screenH = ctx->postEffect->screenHeight;
        RenderContext renderCtx = {
            .screenW = screenW,
            .screenH = screenH,
            .centerX = screenW / 2,
            .centerY = screenH / 2,
            .minDim = (float)(screenW < screenH ? screenW : screenH),
            .accumTexture = ctx->postEffect->outputTexture.texture,
            .postEffect = ctx->postEffect
        };

        RenderPipelineExecute(ctx->postEffect, &ctx->drawableState,
                              ctx->drawables, ctx->drawableCount,
                              &renderCtx, deltaTime,
                              ctx->analysis.fft.magnitude,
                              ctx->analysis.waveformHistory,
                              ctx->analysis.waveformWriteIndex, &ctx->profiler);

        if (ctx->uiVisible) {
            AppConfigs configs = {
                .drawables = ctx->drawables,
                .drawableCount = &ctx->drawableCount,
                .selectedDrawable = &ctx->selectedDrawable,
                .effects = &ctx->postEffect->effects,
                .audio = &ctx->audio,
                .beat = &ctx->analysis.beat,
                .bandEnergies = &ctx->analysis.bands,
                .lfos = ctx->modLFOConfigs,
                .postEffect = ctx->postEffect
            };
            rlImGuiBegin();
                ImGuiDrawDockspace();
                ImGuiDrawEffectsPanel(&ctx->postEffect->effects, &ctx->modSources);
                ImGuiDrawDrawablesPanel(ctx->drawables, &ctx->drawableCount, &ctx->selectedDrawable, &ctx->modSources);
                ImGuiDrawAudioPanel(&ctx->audio);
                ImGuiDrawAnalysisPanel(&ctx->analysis.beat, &ctx->analysis.bands, &ctx->profiler);
                ImGuiDrawLFOPanel(ctx->modLFOConfigs, &ctx->modSources);
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
