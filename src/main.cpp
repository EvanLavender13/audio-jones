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
#include "render/physarum.h"
#include "ui/imgui_panels.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "automation/param_registry.h"
#include "automation/lfo.h"

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
    ctx->selectedDrawable = 0;
    ctx->uiVisible = true;

    CHECK_OR_FAIL(AnalysisPipelineInit(&ctx->analysis));

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
}

// Draws all drawables at full opacity (for physarum trail map)
static void RenderDrawablesFull(AppContext* ctx, RenderContext* renderCtx)
{
    const uint64_t tick = DrawableGetTick(&ctx->drawableState);
    DrawableRenderFull(&ctx->drawableState, renderCtx, ctx->drawables, ctx->drawableCount, tick);
}

// Draws all drawables with per-drawable feedbackPhase opacity
// isPreFeedback: true = draw at (1-feedbackPhase), false = draw at feedbackPhase
static void RenderDrawablesWithPhase(AppContext* ctx, RenderContext* renderCtx, bool isPreFeedback)
{
    const uint64_t tick = DrawableGetTick(&ctx->drawableState);
    DrawableRenderAll(&ctx->drawableState, renderCtx, ctx->drawables, ctx->drawableCount, tick, isPreFeedback);
}

// Renders drawables to physarum trail map for agent input (always full opacity)
static void RenderDrawablesToPhysarum(AppContext* ctx, RenderContext* renderCtx)
{
    if (ctx->postEffect->physarum == NULL) {
        return;
    }
    if (!PhysarumBeginTrailMapDraw(ctx->postEffect->physarum)) {
        return;
    }
    RenderDrawablesFull(ctx, renderCtx);
    PhysarumEndTrailMapDraw(ctx->postEffect->physarum);
}

// Renders drawables BEFORE feedback shader at opacity (1 - feedbackPhase)
// These get integrated into the feedback warp/blur effects
static void RenderDrawablesPreFeedback(AppContext* ctx, RenderContext* renderCtx)
{
    PostEffectBeginDrawStage(ctx->postEffect);
    RenderDrawablesWithPhase(ctx, renderCtx, true);
    PostEffectEndDrawStage();
}

// Renders drawables AFTER feedback shader at opacity feedbackPhase
// These appear crisp on top of the feedback effects
static void RenderDrawablesPostFeedback(AppContext* ctx, RenderContext* renderCtx)
{
    PostEffectBeginDrawStage(ctx->postEffect);
    RenderDrawablesWithPhase(ctx, renderCtx, false);
    PostEffectEndDrawStage();
}

// Standard pipeline: pre-feedback → feedback → physarum → post-feedback → composite
static void RenderStandardPipeline(AppContext* ctx, RenderContext* renderCtx, float deltaTime)
{
    // 1. Draw drawables that will be integrated into feedback
    RenderDrawablesPreFeedback(ctx, renderCtx);

    // 2. Apply feedback effects (warp, blur, decay)
    RenderPipelineApplyFeedback(ctx->postEffect, deltaTime,
                                 ctx->analysis.fft.magnitude);

    // 3. Copy accumTexture for textured shapes to sample
    RenderPipelineUpdateShapeSample(ctx->postEffect);

    // 4. Draw to physarum trail map (always full opacity for agent sensing)
    RenderDrawablesToPhysarum(ctx, renderCtx);

    // 5. Draw crisp drawables on top of feedback
    RenderDrawablesPostFeedback(ctx, renderCtx);

    BeginDrawing();
    ClearBackground(BLACK);
    RenderPipelineApplyOutput(ctx->postEffect, DrawableGetTick(&ctx->drawableState));
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
            .accumTexture = ctx->postEffect->shapeSampleTex.texture,
            .postEffect = ctx->postEffect
        };

        RenderStandardPipeline(ctx, &renderCtx, deltaTime);

        if (ctx->uiVisible) {
            AppConfigs configs = {
                .drawables = ctx->drawables,
                .drawableCount = &ctx->drawableCount,
                .selectedDrawable = &ctx->selectedDrawable,
                .effects = &ctx->postEffect->effects,
                .audio = &ctx->audio,
                .beat = &ctx->analysis.beat,
                .bandEnergies = &ctx->analysis.bands,
                .lfos = ctx->modLFOConfigs
            };
            rlImGuiBegin();
                ImGuiDrawDockspace();
                ImGuiDrawEffectsPanel(&ctx->postEffect->effects, &ctx->modSources);
                ImGuiDrawDrawablesPanel(ctx->drawables, &ctx->drawableCount, &ctx->selectedDrawable);
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
