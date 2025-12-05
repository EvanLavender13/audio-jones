#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "audio.h"
#include "waveform.h"
#include "visualizer.h"

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

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
    float waveformExtended[WAVEFORM_EXTENDED];
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) waveform[i] = 0.0f;
    for (int i = 0; i < WAVEFORM_EXTENDED; i++) waveformExtended[i] = 0.0f;

    WaveformMode mode = WAVEFORM_CIRCULAR;
    float rotation = 0.0f;
    float hueOffset = 0.0f;
    float amplitudeScale = 0.35f;  // Relative to min(width, height)
    float thickness = 2.0f;

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
            uint32_t framesRead = AudioCaptureRead(capture, audioBuffer, AUDIO_BUFFER_FRAMES);
            if (framesRead > 0) {
                ProcessWaveform(audioBuffer, framesRead, waveform, waveformExtended);
            }
            rotation += 0.01f;
            hueOffset += 0.0025f;
            waveformAccumulator = 0.0f;
        }

        // Render to accumulation texture
        int screenW = vis->screenWidth;
        int screenH = vis->screenHeight;
        int centerX = screenW / 2;
        int centerY = screenH / 2;
        float minDim = (float)(screenW < screenH ? screenW : screenH);
        float amplitude = minDim * amplitudeScale;
        float baseRadius = minDim * 0.25f;

        VisualizerBeginAccum(vis, deltaTime);
            // Draw new waveform on top
            if (mode == WAVEFORM_LINEAR) {
                DrawWaveformLinear(waveform, WAVEFORM_SAMPLES, screenW, centerY, (int)amplitude, GREEN, thickness);
            } else {
                // Use extended (mirrored) waveform for seamless circular display
                // baseRadius is center of oscillation, amplitude is total range (±amplitude/2)
                DrawWaveformCircularRainbow(waveformExtended, WAVEFORM_EXTENDED, centerX, centerY,
                                            baseRadius, amplitude, rotation, hueOffset, thickness);
            }
        VisualizerEndAccum(vis);

        // Draw to screen
        BeginDrawing();
            ClearBackground(BLACK);
            VisualizerToScreen(vis);
            // FPS and frame time
            DrawText(TextFormat("%d fps  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f), 10, 10, 16, GRAY);
            DrawText(mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 30, 16, GRAY);

            // UI controls
            const int groupX = 10;
            const int groupW = 150;
            const int labelX = 18;
            const int sliderX = 90;
            const int sliderW = 62;
            int y = 55;

            // Waveform group
            GuiGroupBox((Rectangle){groupX, y, groupW, 60}, "Waveform");
            y += 12;
            DrawText("Height", labelX, y + 2, 10, GRAY);
            GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &amplitudeScale, 0.05f, 0.5f);
            y += 22;
            DrawText("Thickness", labelX, y + 2, 10, GRAY);
            GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &thickness, 1.0f, 10.0f);
            y += 34;

            // Trails group
            GuiGroupBox((Rectangle){groupX, y, groupW, 38}, "Trails");
            y += 12;
            DrawText("Half-life", labelX, y + 2, 10, GRAY);
            GuiSliderBar((Rectangle){sliderX, y, sliderW, 16}, NULL, NULL, &vis->halfLife, 0.1f, 2.0f);
        EndDrawing();
    }

    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    VisualizerUninit(vis);
    CloseWindow();
    return 0;
}
