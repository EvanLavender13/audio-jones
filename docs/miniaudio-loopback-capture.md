---
archctl_doc_id: miniaudio-loopback-capture
title: Miniaudio Loopback Capture for System Audio
tags: [research, audio-capture, cross-platform]
date: 2025-12-02
related_modules: [audio]
---

# Miniaudio Loopback Capture for System Audio

## Overview

Miniaudio provides loopback capture via `ma_device_type_loopback`, which records audio played through speakers. This replicates AudioThing's WASAPI loopback functionality with a simpler, cross-platform API. Native loopback support exists only on WASAPI (Windows); Linux requires PulseAudio monitor device selection, and macOS lacks support entirely.

## Key Findings

### Device Configuration

Loopback mode requires `ma_device_type_loopback` device type with capture properties configured:

```c
ma_device_config deviceConfig = ma_device_config_init(ma_device_type_loopback);
deviceConfig.capture.pDeviceID = NULL;  // NULL = default playback device
deviceConfig.capture.format    = ma_format_f32;
deviceConfig.capture.channels  = 2;
deviceConfig.sampleRate        = 48000;
deviceConfig.periodSizeInFrames = 1024;  // Match AudioThing buffer size
deviceConfig.dataCallback      = data_callback;
deviceConfig.pUserData         = &userData;
```

The `capture.pDeviceID` accepts a *playback* device ID—loopback captures what that device outputs.

### Callback Signature

```c
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    // pOutput is NULL in loopback mode
    // pInput contains captured float32 samples (interleaved)
    // frameCount varies per callback, use periodSizeInFrames for consistency
}
```

### Backend Initialization

Force WASAPI backend for loopback on Windows:

```c
ma_backend backends[] = { ma_backend_wasapi };
ma_result result = ma_device_init_ex(
    backends,
    sizeof(backends)/sizeof(backends[0]),
    NULL,  // No context config
    &deviceConfig,
    &device
);
```

### Buffer Configuration

Miniaudio changed buffer semantics in recent versions:

| Config Variable | Meaning |
|-----------------|---------|
| `periodSizeInFrames` | Size of each callback buffer (latency) |
| `periodSizeInMilliseconds` | Alternative to frames (one must be 0) |
| `periods` | Number of periods in internal buffer (default: `MA_DEFAULT_PERIODS`) |

For visualization matching AudioThing's 1024-sample buffer at 48kHz:
- `periodSizeInFrames = 1024` yields ~21ms latency per callback
- Total buffer = `periodSizeInFrames * periods`

### Ring Buffer for Thread Communication

Miniaudio provides `ma_pcm_rb` for producer-consumer audio transfer between callback thread and main thread:

```c
// Initialization
ma_pcm_rb ringBuffer;
ma_pcm_rb_init(ma_format_f32, 2, 4096, NULL, NULL, &ringBuffer);

// In callback (producer)
void* pWriteBuffer;
ma_uint32 framesToWrite = frameCount;
ma_pcm_rb_acquire_write(&ringBuffer, &framesToWrite, &pWriteBuffer);
memcpy(pWriteBuffer, pInput, framesToWrite * sizeof(float) * 2);
ma_pcm_rb_commit_write(&ringBuffer, framesToWrite);

// In main thread (consumer)
void* pReadBuffer;
ma_uint32 framesToRead = 1024;
ma_pcm_rb_acquire_read(&ringBuffer, &framesToRead, &pReadBuffer);
// Process pReadBuffer
ma_pcm_rb_commit_read(&ringBuffer, framesToRead);
```

The acquire functions return pointers to internal buffers—no allocation needed. Frame counts may be reduced if buffer wraps.

### Platform Support Matrix

| Platform | Loopback Support | Method |
|----------|------------------|--------|
| Windows | Native | `ma_device_type_loopback` with WASAPI |
| Linux | Indirect | Select PulseAudio monitor device during enumeration |
| macOS | None | Requires third-party (BlackHole) or future CoreAudio tap API |

### Linux PulseAudio Workaround

PulseAudio exposes monitor sources for each sink. Enumerate devices and select the monitor:

```c
// During device enumeration callback
// Look for device names containing ".monitor"
// e.g., "alsa_output.pci-0000_00_1f.3.analog-stereo.monitor"
```

Alternatively, create a virtual loopback device:
```bash
pactl load-module module-null-sink sink_name=Loopback
# Application captures from Loopback.monitor
```

### macOS Status

Native loopback not implemented in miniaudio. Planned for v0.12 using CoreAudio tap API (macOS 14.2+). Current workarounds:

1. **BlackHole**: Virtual audio driver, requires user installation
2. **CoreAudio tap**: Process-specific capture, not system-wide

## Technical Details

### Comparison with AudioThing WASAPI Implementation

| Aspect | AudioThing (WASAPI) | Miniaudio |
|--------|---------------------|-----------|
| Device init | `IMMDeviceEnumerator`, `IAudioClient` | `ma_device_init_ex()` |
| Loopback flag | `AUDCLNT_STREAMFLAGS_LOOPBACK` | `ma_device_type_loopback` |
| Buffer config | `IAudioClient::Initialize()` | `periodSizeInFrames` |
| Capture loop | Manual `GetBuffer()`/`ReleaseBuffer()` | Callback-driven |
| Threading | Manual thread + mutex | Internal thread, ring buffer for sync |
| Format | Float32, system rate | Configurable, automatic conversion |

### Complete Loopback Capture Example

```c
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

typedef struct {
    ma_pcm_rb ringBuffer;
    ma_uint32 channels;
} UserData;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    UserData* pUserData = (UserData*)pDevice->pUserData;

    void* pWriteBuffer;
    ma_uint32 framesToWrite = frameCount;

    if (ma_pcm_rb_acquire_write(&pUserData->ringBuffer, &framesToWrite, &pWriteBuffer) == MA_SUCCESS) {
        memcpy(pWriteBuffer, pInput, framesToWrite * sizeof(float) * pUserData->channels);
        ma_pcm_rb_commit_write(&pUserData->ringBuffer, framesToWrite);
    }

    (void)pOutput;
}

int main(void)
{
    UserData userData;
    userData.channels = 2;

    // Ring buffer: 1 second at 48kHz
    ma_pcm_rb_init(ma_format_f32, 2, 48000, NULL, NULL, &userData.ringBuffer);

    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format = ma_format_f32;
    config.capture.channels = 2;
    config.sampleRate = 48000;
    config.periodSizeInFrames = 1024;
    config.dataCallback = data_callback;
    config.pUserData = &userData;

    ma_device device;
    ma_backend backends[] = { ma_backend_wasapi };

    if (ma_device_init_ex(backends, 1, NULL, &config, &device) != MA_SUCCESS) {
        return -1;
    }

    ma_device_start(&device);

    // Main loop: read from ring buffer for visualization
    float buffer[1024 * 2];
    while (running) {
        void* pReadBuffer;
        ma_uint32 framesToRead = 1024;

        if (ma_pcm_rb_acquire_read(&userData.ringBuffer, &framesToRead, &pReadBuffer) == MA_SUCCESS) {
            memcpy(buffer, pReadBuffer, framesToRead * sizeof(float) * 2);
            ma_pcm_rb_commit_read(&userData.ringBuffer, framesToRead);

            // Process buffer for visualization
        }
    }

    ma_device_uninit(&device);
    ma_pcm_rb_uninit(&userData.ringBuffer);
    return 0;
}
```

### Silence Handling

WASAPI loopback reports nothing when no audio plays. This matches AudioThing behavior. If silent audio plays through speakers, samples still arrive (zeros).

### Full-Duplex Limitation

Loopback mode incompatible with full-duplex. Capturing loopback while playing to another device requires two separate `ma_device` objects sharing one `ma_context`.

## References

- [miniaudio simple_loopback.c example](https://github.com/mackron/miniaudio/blob/master/examples/simple_loopback.c)
- [miniaudio documentation](https://miniaud.io/docs/manual/index.html)
- [ma_pcm_rb API reference](https://miniaudio.docsforge.com/master/api/ma_pcm_rb/)
- [Ring buffer usage discussion](https://github.com/mackron/miniaudio/discussions/646)
- [macOS loopback feature request](https://github.com/mackron/miniaudio/issues/875)
- [PulseAudio loopback guide](https://wiki.ubuntu.com/record_system_sound)

## Decisions

### Use miniaudio over raw WASAPI

Miniaudio wraps WASAPI loopback with identical functionality but simpler API. Reduces code from ~280 LOC to ~50 LOC. Callback-driven model eliminates manual threading.

### Ring buffer for thread synchronization

Replace AudioThing's mutex + condition_variable pattern with `ma_pcm_rb`. Lock-free, designed for audio producer-consumer scenarios.

### Accept platform limitations

Native loopback on Windows only. Linux users must configure PulseAudio monitor. macOS requires third-party software until miniaudio v0.12.

## Open Questions

1. **Should we abstract platform differences?** Could provide unified interface that falls back gracefully on unsupported platforms
2. **File-based alternative?** For cross-platform demos, allow audio file input instead of system capture
3. **Device enumeration UI?** Let users select specific playback device to capture from
