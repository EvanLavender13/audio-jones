#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include <stdlib.h>
#include <string.h>

struct AudioCapture {
    ma_device device;
    ma_pcm_rb ringBuffer;
    bool initialized;
    bool started;
};

static void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    AudioCapture* capture = (AudioCapture*)pDevice->pUserData;
    (void)pOutput;

    if (pInput == NULL) {
        return;
    }

    void* pWriteBuffer;
    ma_uint32 framesToWrite = frameCount;

    if (ma_pcm_rb_acquire_write(&capture->ringBuffer, &framesToWrite, &pWriteBuffer) == MA_SUCCESS) {
        memcpy(pWriteBuffer, pInput, framesToWrite * sizeof(float) * AUDIO_CHANNELS);
        ma_pcm_rb_commit_write(&capture->ringBuffer, framesToWrite);
    }
}

AudioCapture* AudioCaptureInit(void)
{
    AudioCapture* capture = (AudioCapture*)calloc(1, sizeof(AudioCapture));
    if (capture == NULL) {
        return NULL;
    }

    // Initialize ring buffer for thread-safe audio transfer
    ma_result result = ma_pcm_rb_init(
        ma_format_f32,
        AUDIO_CHANNELS,
        AUDIO_RING_BUFFER_FRAMES,
        NULL,
        NULL,
        &capture->ringBuffer
    );
    if (result != MA_SUCCESS) {
        free(capture);
        return NULL;
    }

    // Configure loopback capture device
    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.pDeviceID = NULL;  // Default playback device
    config.capture.format = ma_format_f32;
    config.capture.channels = AUDIO_CHANNELS;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.periodSizeInFrames = AUDIO_BUFFER_FRAMES;
    config.dataCallback = audio_data_callback;
    config.pUserData = capture;

    // Force WASAPI backend for loopback support on Windows
    ma_backend backends[] = { ma_backend_wasapi };
    result = ma_device_init_ex(
        backends,
        sizeof(backends) / sizeof(backends[0]),
        NULL,
        &config,
        &capture->device
    );
    if (result != MA_SUCCESS) {
        ma_pcm_rb_uninit(&capture->ringBuffer);
        free(capture);
        return NULL;
    }

    capture->initialized = true;
    capture->started = false;
    return capture;
}

void AudioCaptureUninit(AudioCapture* capture)
{
    if (capture == NULL) {
        return;
    }

    if (capture->started) {
        AudioCaptureStop(capture);
    }

    if (capture->initialized) {
        ma_device_uninit(&capture->device);
        ma_pcm_rb_uninit(&capture->ringBuffer);
    }

    free(capture);
}

bool AudioCaptureStart(AudioCapture* capture)
{
    if (capture == NULL || !capture->initialized || capture->started) {
        return false;
    }

    if (ma_device_start(&capture->device) != MA_SUCCESS) {
        return false;
    }

    capture->started = true;
    return true;
}

void AudioCaptureStop(AudioCapture* capture)
{
    if (capture == NULL || !capture->started) {
        return;
    }

    ma_device_stop(&capture->device);
    capture->started = false;
}

uint32_t AudioCaptureRead(AudioCapture* capture, float* buffer, uint32_t frameCount)
{
    if (capture == NULL || buffer == NULL || frameCount == 0) {
        return 0;
    }

    void* pReadBuffer;
    ma_uint32 framesToRead = frameCount;

    if (ma_pcm_rb_acquire_read(&capture->ringBuffer, &framesToRead, &pReadBuffer) != MA_SUCCESS) {
        return 0;
    }

    if (framesToRead > 0) {
        memcpy(buffer, pReadBuffer, framesToRead * sizeof(float) * AUDIO_CHANNELS);
        ma_pcm_rb_commit_read(&capture->ringBuffer, framesToRead);
    }

    return framesToRead;
}

uint32_t AudioCaptureAvailable(AudioCapture* capture)
{
    if (capture == NULL) {
        return 0;
    }

    return (uint32_t)ma_pcm_rb_available_read(&capture->ringBuffer);
}
