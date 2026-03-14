#include "playlist.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

Playlist PlaylistDefault(void) {
  Playlist pl = {};
  pl.activeIndex = -1;
  return pl;
}

bool PlaylistSave(const Playlist *playlist, const char *filepath) {
  try {
    json j;
    j["name"] = std::string(playlist->name);
    j["entries"] = json::array();
    for (int i = 0; i < playlist->entryCount; i++) {
      j["entries"].push_back(std::string(playlist->entries[i]));
    }

    fs::path p(filepath);
    if (p.has_parent_path()) {
      fs::create_directories(p.parent_path());
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    file << j.dump(2);
    return true;
  } catch (...) {
    return false;
  }
}

bool PlaylistLoad(Playlist *playlist, const char *filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    const json j = json::parse(file);

    *playlist = PlaylistDefault();

    const std::string name = j.at("name").get<std::string>();
    strncpy(playlist->name, name.c_str(), PRESET_NAME_MAX - 1);
    playlist->name[PRESET_NAME_MAX - 1] = '\0';

    if (j.contains("entries")) {
      const auto &arr = j["entries"];
      int count = (int)arr.size();
      if (count > MAX_PLAYLIST_ENTRIES) {
        count = MAX_PLAYLIST_ENTRIES;
      }
      for (int i = 0; i < count; i++) {
        const std::string path = arr[i].get<std::string>();
        strncpy(playlist->entries[i], path.c_str(), PRESET_PATH_MAX - 1);
        playlist->entries[i][PRESET_PATH_MAX - 1] = '\0';
      }
      playlist->entryCount = count;
    }

    playlist->activeIndex = -1;
    return true;
  } catch (...) {
    return false;
  }
}

int PlaylistListFiles(char files[][PRESET_PATH_MAX], int maxFiles) {
  try {
    if (!fs::exists(PLAYLIST_DIR)) {
      fs::create_directories(PLAYLIST_DIR);
      return 0;
    }

    int count = 0;
    for (const auto &entry : fs::directory_iterator(PLAYLIST_DIR)) {
      if (count >= maxFiles)
        break;
      if (!entry.is_regular_file())
        continue;
      if (entry.path().extension() != ".json")
        continue;
      const std::string name = entry.path().filename().string();
      strncpy(files[count], name.c_str(), PRESET_PATH_MAX - 1);
      files[count][PRESET_PATH_MAX - 1] = '\0';
      count++;
    }
    return count;
  } catch (...) {
    return 0;
  }
}

bool PlaylistAdd(Playlist *playlist, const char *presetPath) {
  if (playlist->entryCount >= MAX_PLAYLIST_ENTRIES) {
    return false;
  }
  strncpy(playlist->entries[playlist->entryCount], presetPath,
          PRESET_PATH_MAX - 1);
  playlist->entries[playlist->entryCount][PRESET_PATH_MAX - 1] = '\0';
  playlist->entryCount++;
  return true;
}

bool PlaylistRemove(Playlist *playlist, int index) {
  if (index < 0 || index >= playlist->entryCount) {
    return false;
  }

  for (int i = index; i < playlist->entryCount - 1; i++) {
    memcpy(playlist->entries[i], playlist->entries[i + 1], PRESET_PATH_MAX);
  }
  playlist->entryCount--;

  if (playlist->activeIndex == index) {
    playlist->activeIndex = -1;
  } else if (index < playlist->activeIndex) {
    playlist->activeIndex--;
  }

  return true;
}

bool PlaylistSwap(Playlist *playlist, int indexA, int indexB) {
  if (indexA < 0 || indexA >= playlist->entryCount || indexB < 0 ||
      indexB >= playlist->entryCount) {
    return false;
  }

  char temp[PRESET_PATH_MAX];
  memcpy(temp, playlist->entries[indexA], PRESET_PATH_MAX);
  memcpy(playlist->entries[indexA], playlist->entries[indexB], PRESET_PATH_MAX);
  memcpy(playlist->entries[indexB], temp, PRESET_PATH_MAX);

  if (playlist->activeIndex == indexA) {
    playlist->activeIndex = indexB;
  } else if (playlist->activeIndex == indexB) {
    playlist->activeIndex = indexA;
  }

  return true;
}

bool PlaylistAdvance(Playlist *playlist, int direction) {
  if (playlist->entryCount == 0) {
    return false;
  }
  int newIndex = playlist->activeIndex + direction;
  if (newIndex < 0 || newIndex >= playlist->entryCount) {
    return false;
  }
  playlist->activeIndex = newIndex;
  return true;
}
