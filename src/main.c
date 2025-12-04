#include "raylib.h"
#include "audio.h"
#include "waveform.h"
#include "visualizer.h"
#include <math.h>

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

int main(void)
{
    InitWindow(800, 600, "AudioJones");
    SetTargetFPS(60);

    Visualizer* vis = VisualizerInit(800, 600);
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

    // Waveform updates at 30fps, rendering at 60fps
    const float waveformUpdateInterval = 1.0f / 30.0f;
    float waveformAccumulator = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        waveformAccumulator += deltaTime;

        // Toggle mode with Space
        if (IsKeyPressed(KEY_SPACE)) {
            mode = (mode == WAVEFORM_LINEAR) ? WAVEFORM_CIRCULAR : WAVEFORM_LINEAR;
        }

        // Update waveform at 30fps
        if (waveformAccumulator >= waveformUpdateInterval) {
            // Read audio samples
            uint32_t framesRead = AudioCaptureRead(capture, audioBuffer, AUDIO_BUFFER_FRAMES);

            // Extract waveform - copy raw interleaved samples (L,R,L,R...) like AudioThing
            if (framesRead > 0) {
                // Copy directly, no channel separation or downsampling
                int copyCount = (framesRead > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : framesRead;
                for (int i = 0; i < copyCount; i++) {
                    waveform[i] = audioBuffer[i];
                }
                // Zero out rest if we got fewer samples
                for (int i = copyCount; i < WAVEFORM_SAMPLES; i++) {
                    waveform[i] = 0.0f;
                }

                // Normalize: scale so peak amplitude reaches 1.0
                float maxAbs = 0.0f;
                for (int i = 0; i < copyCount; i++) {
                    float absVal = fabsf(waveform[i]);
                    if (absVal > maxAbs) maxAbs = absVal;
                }
                if (maxAbs > 0.0f) {
                    for (int i = 0; i < copyCount; i++) {
                        waveform[i] /= maxAbs;
                    }
                }

                // Create extended buffer: original + mirrored (palindrome)
                for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
                    waveformExtended[i] = waveform[i];
                    waveformExtended[WAVEFORM_SAMPLES + i] = waveform[WAVEFORM_SAMPLES - 1 - i];
                }

                // Smooth the join points
                float avg1 = (waveform[WAVEFORM_SAMPLES - 1] + waveform[0]) * 0.5f;
                waveformExtended[WAVEFORM_SAMPLES - 1] = avg1;
                waveformExtended[WAVEFORM_SAMPLES] = avg1;
            }

            // Update rotation and hue
            rotation += 0.01f;
            hueOffset += 0.0025f;

            waveformAccumulator = 0.0f;
        }

        // Render to accumulation texture
        VisualizerBeginAccum(vis);
            // Draw new waveform on top
            if (mode == WAVEFORM_LINEAR) {
                DrawWaveformLinear(waveform, WAVEFORM_SAMPLES, 300, 200, GREEN);
            } else {
                // Use extended (mirrored) waveform for seamless circular display
                // Smaller base radius, bigger amplitude for fat waves like AudioThing
                DrawWaveformCircularRainbow(waveformExtended, WAVEFORM_EXTENDED, 400, 300,
                                            50.0f, 250.0f, rotation, hueOffset);
            }
        VisualizerEndAccum(vis);

        // Draw to screen
        BeginDrawing();
            ClearBackground(BLACK);
            VisualizerToScreen(vis);
            DrawText(mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 10, 16, GRAY);
        EndDrawing();
    }

    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    VisualizerUninit(vis);
    CloseWindow();
    return 0;
}
