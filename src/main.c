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
    float amplitude = 400.0f;

    // Waveform updates at 30fps, rendering at 60fps
    const float waveformUpdateInterval = 1.0f / 20.0f;
    float waveformAccumulator = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        waveformAccumulator += deltaTime;

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
        VisualizerBeginAccum(vis, deltaTime);
            // Draw new waveform on top
            if (mode == WAVEFORM_LINEAR) {
                DrawWaveformLinear(waveform, WAVEFORM_SAMPLES, 1920, 540, (int)amplitude, GREEN);
            } else {
                // Use extended (mirrored) waveform for seamless circular display
                // Smaller base radius, bigger amplitude for fat waves like AudioThing
                // baseRadius=250 is center of oscillation, amplitude is total range (±amplitude/2)
                DrawWaveformCircularRainbow(waveformExtended, WAVEFORM_EXTENDED, 960, 540,
                                            250.0f, amplitude, rotation, hueOffset);
            }
        VisualizerEndAccum(vis);

        // Draw to screen
        BeginDrawing();
            ClearBackground(BLACK);
            VisualizerToScreen(vis);
            DrawText(mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 10, 16, GRAY);

            // UI controls
            DrawText("Height", 10, 40, 16, GRAY);
            GuiSliderBar((Rectangle){70, 38, 150, 20}, NULL, NULL, &amplitude, 50.0f, 500.0f);
        EndDrawing();
    }

    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    VisualizerUninit(vis);
    CloseWindow();
    return 0;
}
