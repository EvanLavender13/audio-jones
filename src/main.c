#include "raylib.h"
#include "audio.h"
#include <math.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 10

// Cubic interpolation between four points
float CubicInterp(float y0, float y1, float y2, float y3, float t)
{
    float a0 = y3 - y2 - y0 + y1;
    float a1 = y0 - y1 - a0;
    float a2 = y2 - y0;
    float a3 = y1;
    return a0 * t * t * t + a1 * t * t + a2 * t + a3;
}

typedef enum {
    WAVEFORM_LINEAR,
    WAVEFORM_CIRCULAR
} WaveformMode;

// Convert HSV to RGB (h: 0-1, s: 0-1, v: 0-1)
Color HsvToRgb(float h, float s, float v)
{
    float r, g, b;

    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = v; g = t; b = p; break;
    }

    return (Color){
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        255
    };
}

void DrawWaveformLinear(float* samples, int count, int centerY, int amplitude, Color color)
{
    float xStep = 800.0f / count;
    for (int i = 0; i < count - 1; i++) {
        int x1 = (int)(i * xStep);
        int y1 = centerY - (int)(samples[i] * amplitude);
        int x2 = (int)((i + 1) * xStep);
        int y2 = centerY - (int)(samples[i + 1] * amplitude);
        DrawLine(x1, y1, x2, y2, color);
    }
}

void DrawWaveformCircularRainbow(float* samples, int count, int centerX, int centerY,
                                  float baseRadius, float amplitude, float rotation, float hueOffset)
{
    int numPoints = count * INTERPOLATION_MULT;
    float angleStep = (2.0f * PI) / numPoints;

    for (int i = 0; i < numPoints; i++) {
        int next = (i + 1) % numPoints;

        float angle1 = i * angleStep + rotation - PI / 2;
        float angle2 = next * angleStep + rotation - PI / 2;

        // Cubic interpolation for smoother curves
        int idx = (i / INTERPOLATION_MULT) % count;
        float frac = (float)(i % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int i0 = (idx - 1 + count) % count;
        int i1 = idx;
        int i2 = (idx + 1) % count;
        int i3 = (idx + 2) % count;
        float sample1 = CubicInterp(samples[i0], samples[i1], samples[i2], samples[i3], frac);

        int nextIdx = (next / INTERPOLATION_MULT) % count;
        float nextFrac = (float)(next % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int n0 = (nextIdx - 1 + count) % count;
        int n1 = nextIdx;
        int n2 = (nextIdx + 1) % count;
        int n3 = (nextIdx + 2) % count;
        float sample2 = CubicInterp(samples[n0], samples[n1], samples[n2], samples[n3], nextFrac);

        float radius1 = baseRadius + sample1 * amplitude;
        float radius2 = baseRadius + sample2 * amplitude;

        // Clamp to prevent negative radii
        if (radius1 < 10.0f) radius1 = 10.0f;
        if (radius2 < 10.0f) radius2 = 10.0f;

        int x1 = centerX + (int)(cosf(angle1) * radius1);
        int y1 = centerY + (int)(sinf(angle1) * radius1);
        int x2 = centerX + (int)(cosf(angle2) * radius2);
        int y2 = centerY + (int)(sinf(angle2) * radius2);

        // Hue cycles around the circle
        float hue = (float)i / numPoints + hueOffset;
        hue = hue - floorf(hue);
        Color color = HsvToRgb(hue, 1.0f, 1.0f);

        DrawLine(x1, y1, x2, y2, color);
    }
}

int main(void)
{
    InitWindow(800, 600, "AudioJones");
    SetTargetFPS(60);

    // Load fade shader
    Shader fadeShader = LoadShader(0, "shaders/fade.fs");
    int fadeAmountLoc = GetShaderLocation(fadeShader, "fadeAmount");
    float fadeAmount = 0.95f;

    // Create render texture for accumulation
    RenderTexture2D accumTexture = LoadRenderTexture(800, 600);

    // Clear accumulation texture
    BeginTextureMode(accumTexture);
    ClearBackground(BLACK);
    EndTextureMode();

    AudioCapture* capture = AudioCaptureInit();
    if (capture == NULL) {
        CloseWindow();
        return -1;
    }

    if (!AudioCaptureStart(capture)) {
        AudioCaptureUninit(capture);
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
        BeginTextureMode(accumTexture);
            // Apply fade to existing content
            BeginShaderMode(fadeShader);
                SetShaderValue(fadeShader, fadeAmountLoc, &fadeAmount, SHADER_UNIFORM_FLOAT);
                // Draw the texture onto itself with fade
                DrawTextureRec(accumTexture.texture,
                    (Rectangle){0, 0, 800, -600},  // Flip Y
                    (Vector2){0, 0}, WHITE);
            EndShaderMode();

            // Draw new waveform on top
            if (mode == WAVEFORM_LINEAR) {
                DrawWaveformLinear(waveform, WAVEFORM_SAMPLES, 300, 200, GREEN);
            } else {
                // Use extended (mirrored) waveform for seamless circular display
                // Smaller base radius, bigger amplitude for fat waves like AudioThing
                DrawWaveformCircularRainbow(waveformExtended, WAVEFORM_EXTENDED, 400, 300,
                                            50.0f, 250.0f, rotation, hueOffset);
            }
        EndTextureMode();

        // Draw to screen
        BeginDrawing();
            ClearBackground(BLACK);

            // Draw accumulation texture
            DrawTextureRec(accumTexture.texture,
                (Rectangle){0, 0, 800, -600},  // Flip Y
                (Vector2){0, 0}, WHITE);

            DrawText(mode == WAVEFORM_LINEAR ? "[SPACE] Linear" : "[SPACE] Circular", 10, 10, 16, GRAY);
        EndDrawing();
    }

    AudioCaptureStop(capture);
    AudioCaptureUninit(capture);
    UnloadRenderTexture(accumTexture);
    UnloadShader(fadeShader);
    CloseWindow();
    return 0;
}
