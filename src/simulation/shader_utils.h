#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

// Load shader source file with error logging. Returns NULL on failure.
// Caller must call UnloadFileText() on the returned pointer.
char *SimLoadShaderSource(const char *path);

#endif // SHADER_UTILS_H
