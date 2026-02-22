#ifndef IMGUI_EFFECTS_DISPATCH_H
#define IMGUI_EFFECTS_DISPATCH_H

#include "config/effect_config.h"

struct ModSources;

// Section toggle state indexed by TransformEffectType
extern bool g_effectSectionOpen[];

// Draw all effects belonging to the given category section index.
// Iterates EFFECT_DESCRIPTORS filtered by categorySectionIndex, calling
// drawParams/drawOutput on each effect with a matching descriptor.
void DrawEffectCategory(EffectConfig *e, const ModSources *modSources,
                        int sectionIndex);

#endif // IMGUI_EFFECTS_DISPATCH_H
