#include "shader_utils.h"
#include "raylib.h"
#include <stddef.h>

char* SimLoadShaderSource(const char* path)
{
    char* source = LoadFileText(path);
    if (source == NULL) {
        TraceLog(LOG_ERROR, "SIMULATION: Failed to load shader: %s", path);
    }
    return source;
}
