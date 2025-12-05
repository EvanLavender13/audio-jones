#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdbool.h>
#include "audio.h"
#include "waveform.h"
#include "visualizer.h"

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

static void UpdateWaveformAudio(AudioCapture* capture, float* audioBuffer,
                                float* waveform, float waveformExtended[][WAVEFORM_EXTENDED],
                                float* rotation, WaveformConfig* waveforms, int waveformCount)
{
    uint32_t framesRead = AudioCaptureRead(capture, audioBuffer, AUDIO_BUFFER_FRAMES);
    if (framesRead > 0) {
        ProcessWaveformBase(audioBuffer, framesRead, waveform);
        for (int i = 0; i < waveformCount; i++) {
            ProcessWaveformSmooth(waveform, waveformExtended[i], waveforms[i].smoothness);
        }
    }
    *rotation += 0.01f;
    for (int i = 0; i < waveformCount; i++) {
        waveforms[i].hueOffset += 0.0025f;
        if (waveforms[i].hueOffset > 1.0f) waveforms[i].hueOffset -= 1.0f;
    }
}

static void RenderWaveforms(WaveformMode mode, float waveformExtended[][WAVEFORM_EXTENDED],
                            WaveformConfig* waveforms, int waveformCount, RenderContext* ctx)
{
    if (mode == WAVEFORM_LINEAR) {
        DrawWaveformLinear(waveformExtended[0], WAVEFORM_SAMPLES, ctx, &waveforms[0]);
    } else {
        for (int i = 0; i < waveformCount; i++) {
            DrawWaveformCircularRainbow(waveformExtended[i], WAVEFORM_EXTENDED, ctx, &waveforms[i]);
        }
    }
}

static void DrawWaveformUI(WaveformConfig* waveforms, int* waveformCount,
                           int* selectedWaveform, float* halfLife)
{
    const int groupX = 10;
    const int groupW = 150;
    const int labelX = 18;
    const int sliderX = 90;
    const int sliderW = 62;
    int y = 55;

    // Waveforms group
    const int listHeight = 80;
    GuiGroupBox((Rectangle){groupX, y, groupW, 12 + 24 + listHeight + 8}, "Waveforms");
    y += 12;

    // New button
    GuiSetState((*waveformCount >= MAX_WAVEFORMS) ? STATE_DISABLED : STATE_NORMAL);
    if (GuiButton((Rectangle){labelX, y, groupW - 16, 20}, "New")) {
        waveforms[*waveformCount] = WaveformConfigDefault();
        waveforms[*waveformCount].hueOffset = (float)(*waveformCount) * 0.15f;
        *selectedWaveform = *waveformCount;
        (*waveformCount)++;
    }
    GuiSetState(STATE_NORMAL);
    y += 24;

    // Waveform list
    static int scrollIndex = 0;
    static char itemNames[MAX_WAVEFORMS][16];
    const char* listItems[MAX_WAVEFORMS];
    for (int i = 0; i < *waveformCount; i++) {
        snprintf(itemNames[i], sizeof(itemNames[i]), "Waveform %d", i + 1);
        listItems[i] = itemNames[i];
    }
    int focus = -1;
    GuiListViewEx((Rectangle){labelX, y, groupW - 16, listHeight},
                  listItems, *waveformCount, &scrollIndex, selectedWaveform, &focus);
    y += listHeight + 16;

    // Selected waveform settings
    WaveformConfig* sel = &waveforms[*selectedWaveform];
    GuiGroupBox((Rectangle){groupX, y, groupW, 104}, TextFormat("Waveform %d", *selectedWaveform + 1));
    y += 12;
    DrawText("Radius", labelX, y + 2, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &sel->radius, 0.05f, 0.45f);
    y += 22;
    DrawText("Height", labelX, y + 2, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &sel->amplitudeScale, 0.05f, 0.5f);
    y += 22;
    DrawText("Thickness", labelX, y + 2, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &sel->thickness, 1.0f, 10.0f);
    y += 22;
    DrawText("Smooth", labelX, y + 2, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &sel->smoothness, 0.0f, 50.0f);
    y += 34;

    // Trails group
    GuiGroupBox((Rectangle){groupX, y, groupW, 38}, "Trails");
    y += 12;
    DrawText("Half-life", labelX, y + 2, 10, GRAY);
    GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, halfLife, 0.1f, 2.0f);
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

    float audioBuffer[AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS];
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) waveform[i] = 0.0f;
    for (int w = 0; w < MAX_WAVEFORMS; w++) {
        for (int i = 0; i < WAVEFORM_EXTENDED; i++) waveformExtended[w][i] = 0.0f;
    }

    WaveformMode mode = WAVEFORM_CIRCULAR;
    float rotation = 0.0f;

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
                                &rotation, waveforms, waveformCount);
            waveformAccumulator = 0.0f;
        }

        // Render to accumulation texture
        RenderContext ctx = {
            .screenW = vis->screenWidth,
            .screenH = vis->screenHeight,
            .centerX = vis->screenWidth / 2,
            .centerY = vis->screenHeight / 2,
            .minDim = (float)(vis->screenWidth < vis->screenHeight ? vis->screenWidth : vis->screenHeight),
            .rotation = rotation
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

            DrawWaveformUI(waveforms, &waveformCount, &selectedWaveform, &vis->halfLife);
        EndDrawing();
    }

    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    VisualizerUninit(vis);
    CloseWindow();
    return 0;
}
