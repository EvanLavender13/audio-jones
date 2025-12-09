# Audio Module

> Part of [AudioJones](../architecture.md)

## Purpose

Captures system audio via WASAPI loopback into a lock-free ring buffer.

## Files

- `src/audio/audio.h` - Public API and constants
- `src/audio/audio.cpp` - miniaudio loopback implementation
- `src/audio/audio_config.h` - ChannelMode enum and AudioConfig struct

## Function Reference

| Function | Purpose |
|----------|---------|
| `AudioCaptureInit` | Creates loopback device at 48kHz stereo, allocates 4096-frame ring buffer |
| `AudioCaptureUninit` | Frees device and ring buffer |
| `AudioCaptureStart` | Starts the loopback device |
| `AudioCaptureStop` | Stops the loopback device |
| `AudioCaptureRead` | Reads up to N frames from ring buffer (lock-free) |
| `AudioCaptureAvailable` | Returns frames available in ring buffer |

## Types

### ChannelMode

| Value | Description |
|-------|-------------|
| `CHANNEL_LEFT` | Left channel only |
| `CHANNEL_RIGHT` | Right channel only |
| `CHANNEL_MAX` | Max magnitude of L/R with sign from larger |
| `CHANNEL_MIX` | (L+R)/2 mono downmix |
| `CHANNEL_SIDE` | L-R stereo difference |
| `CHANNEL_INTERLEAVED` | Alternating L/R samples |

### AudioConfig

| Field | Default | Description |
|-------|---------|-------------|
| `channelMode` | `CHANNEL_LEFT` | Stereo-to-mono mixing mode |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `AUDIO_SAMPLE_RATE` | 48000 | Capture sample rate (Hz) |
| `AUDIO_CHANNELS` | 2 | Stereo capture |
| `AUDIO_BUFFER_FRAMES` | 1024 | Frames per waveform read |
| `AUDIO_RING_BUFFER_FRAMES` | 4096 | Ring buffer capacity |
| `AUDIO_MAX_FRAMES_PER_UPDATE` | 3072 | Max frames per update (~64ms) |

## Data Flow

1. **Entry:** Audio thread writes stereo f32 samples to `ma_pcm_rb`
2. **Exit:** Main thread reads via `AudioCaptureRead` (lock-free)
