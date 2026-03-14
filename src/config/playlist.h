#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "preset.h"
#include <stdbool.h>

#define MAX_PLAYLIST_ENTRIES 64
#define PLAYLIST_DIR "playlists"

struct Playlist {
  char name[PRESET_NAME_MAX];
  char entries[MAX_PLAYLIST_ENTRIES][PRESET_PATH_MAX]; // Relative preset paths
  int entryCount;
  int activeIndex; // Currently-playing index (-1 = none)
};

// Initialize playlist with defaults
Playlist PlaylistDefault(void);

// Save playlist to file. Returns true on success.
bool PlaylistSave(const Playlist *playlist, const char *filepath);

// Load playlist from file. Returns true on success.
bool PlaylistLoad(Playlist *playlist, const char *filepath);

// List playlist files in the playlists directory. Returns number found.
int PlaylistListFiles(char files[][PRESET_PATH_MAX], int maxFiles);

// Add a preset path to the playlist. Returns true on success.
bool PlaylistAdd(Playlist *playlist, const char *presetPath);

// Remove entry at index from the playlist. Returns true on success.
bool PlaylistRemove(Playlist *playlist, int index);

// Swap two entries in the playlist. Returns true on success.
bool PlaylistSwap(Playlist *playlist, int indexA, int indexB);

// Advance playlist by direction (-1 prev, +1 next). Returns true on success.
bool PlaylistAdvance(Playlist *playlist, int direction);

#endif // PLAYLIST_H
