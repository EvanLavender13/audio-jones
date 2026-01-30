#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

typedef enum {
  CHANNEL_LEFT,       // Left channel only
  CHANNEL_RIGHT,      // Right channel only
  CHANNEL_MAX,        // Max magnitude of L/R with sign from larger
  CHANNEL_MIX,        // (L+R)/2 mono downmix
  CHANNEL_SIDE,       // L-R stereo difference
  CHANNEL_INTERLEAVED // Alternating L/R samples (legacy behavior)
} ChannelMode;

struct AudioConfig {
  ChannelMode channelMode = CHANNEL_LEFT;
};

#endif // AUDIO_CONFIG_H
