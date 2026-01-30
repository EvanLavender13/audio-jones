#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define AUDIO_BUFFER_FRAMES 1024
#define AUDIO_RING_BUFFER_FRAMES 4096
#define AUDIO_MAX_FRAMES_PER_UPDATE                                            \
  3072 // ~64ms at 48kHz, covers 50ms update + margin

typedef struct AudioCapture AudioCapture;

// Initialize audio loopback capture (captures system audio output)
// Returns NULL on failure
AudioCapture *AudioCaptureInit(void);

// Clean up audio capture resources
void AudioCaptureUninit(AudioCapture *capture);

// Start capturing audio
bool AudioCaptureStart(AudioCapture *capture);

// Stop capturing audio
void AudioCaptureStop(AudioCapture *capture);

// Read samples from the capture buffer
// Returns number of frames actually read (may be less than requested)
// buffer must hold at least frameCount * AUDIO_CHANNELS floats
uint32_t AudioCaptureRead(AudioCapture *capture, float *buffer,
                          uint32_t frameCount);

// Get number of frames available in the ring buffer
uint32_t AudioCaptureAvailable(AudioCapture *capture);

#endif // AUDIO_H
