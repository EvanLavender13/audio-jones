#include "raylib.h"

#include <stdbool.h>
#include "audio.h"
#include "waveform.h"
#include "visualizer.h"
#include "ui.h"

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

static void UpdateWaveformAudio(AudioCapture* capture, float* audioBuffer,
                                float* waveform, float waveformExtended[][WAVEFORM_EXTENDED],
                                WaveformConfig* waveforms, int waveformCount)
{
    uint32_t framesRead = AudioCaptureRead(capture, audioBuffer, AUDIO_BUFFER_FRAMES);
    if (framesRead > 0) {
        ProcessWaveformBase(audioBuffer, framesRead, waveform);
        for (int i = 0; i < waveformCount; i++) {
            ProcessWaveformSmooth(waveform, waveformExtended[i], waveforms[i].smoothness);
        }
    }
    for (int i = 0; i < waveformCount; i++) {
        waveforms[i].rotation += waveforms[i].rotationSpeed;
    }
}

static void RenderWaveforms(WaveformMode mode, float waveformExtended[][WAVEFORM_EXTENDED],
                            WaveformConfig* waveforms, int waveformCount, RenderContext* ctx)
{
    if (mode == WAVEFORM_LINEAR) {
        DrawWaveformLinear(waveformExtended[0], WAVEFORM_SAMPLES, ctx, &waveforms[0]);
    } else {
        for (int i = 0; i < waveformCount; i++) {
            DrawWaveformCircular(waveformExtended[i], WAVEFORM_EXTENDED, ctx, &waveforms[i]);
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "AudioJones");
    SetTargetFPS(60);

    Visualizer* vis = VisualizerInit(1920, 1080);
    if (vis == NULL) {
        CloseWindow();
        return -1;
    }

    AudioCapture* capture = AudioCaptureInit();
    if (capture == NULL) {
        VisualizerUninit(vis);
        CloseWindow();
        return -1;
    }

    if (!AudioCaptureStart(capture)) {
        AudioCaptureUninit(capture);
        VisualizerUninit(vis);
        CloseWindow();
        return -1;
    }

    UIState* ui = UIStateInit();
    if (ui == NULL) {
        AudioCaptureStop(capture);
        AudioCaptureUninit(capture);
        VisualizerUninit(vis);
        CloseWindow();
        return -1;
    }

    float audioBuffer[AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS];
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
        waveform[i] = 0.0f;
    }
    for (int w = 0; w < MAX_WAVEFORMS; w++) {
        for (int i = 0; i < WAVEFORM_EXTENDED; i++) {
            waveformExtended[w][i] = 0.0f;
        }
    }

    WaveformMode mode = WAVEFORM_LINEAR;

    // Multiple waveform support
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount = 1;
    int selectedWaveform = 0;
    waveforms[0] = WaveformConfigDefault();

    // Waveform updates at 30fps, rendering at 60fps
    const float waveformUpdateInterval = 1.0f / 20.0f;
    float waveformAccumulator = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        waveformAccumulator += deltaTime;

        // Handle window resize
        if (IsWindowResized()) {
            VisualizerResize(vis, GetScreenWidth(), GetScreenHeight());
        }

        // Toggle mode with Space
        if (IsKeyPressed(KEY_SPACE)) {
            mode = (mode == WAVEFORM_LINEAR) ? WAVEFORM_CIRCULAR : WAVEFORM_LINEAR;
        }

        // Update waveform at fixed rate
        if (waveformAccumulator >= waveformUpdateInterval) {
            UpdateWaveformAudio(capture, audioBuffer, waveform, waveformExtended,
                                waveforms, waveformCount);
            waveformAccumulator = 0.0f;
        }

        // Render to accumulation texture
        RenderContext ctx = {
            .screenW = vis->screenWidth,
            .screenH = vis->screenHeight,
            .centerX = vis->screenWidth / 2,
            .centerY = vis->screenHeight / 2,
            .minDim = (float)(vis->screenWidth < vis->screenHeight ? vis->screenWidth : vis->screenHeight)
        };

        VisualizerBeginAccum(vis, deltaTime);
            RenderWaveforms(mode, waveformExtended, waveforms, waveformCount, &ctx);
        VisualizerEndAccum(vis);

        // Draw to screen
        BeginDrawing();
            ClearBackground(BLACK);
            VisualizerToScreen(vis);
            // FPS and frame time
            DrawText(TextFormat("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f), 10, 10, 16, GRAY);
            DrawText(mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 30, 16, GRAY);

            UIBeginPanels(ui, 55);
            UIDrawWaveformPanel(ui, waveforms, &waveformCount, &selectedWaveform, &vis->halfLife);
            UIDrawPresetPanel(ui, waveforms, &waveformCount, &vis->halfLife);
        EndDrawing();
    }

    UIStateUninit(ui);
    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    VisualizerUninit(vis);
    CloseWindow();
    return 0;
}
