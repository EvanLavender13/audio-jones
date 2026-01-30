#ifndef IMGUI_EFFECTS_TRANSFORMS_H
#define IMGUI_EFFECTS_TRANSFORMS_H

struct EffectConfig;
struct ModSources;

// Draw transform subcategory sections
void DrawSymmetryCategory(EffectConfig *e, const ModSources *modSources);
void DrawWarpCategory(EffectConfig *e, const ModSources *modSources);
void DrawCellularCategory(EffectConfig *e, const ModSources *modSources);
void DrawMotionCategory(EffectConfig *e, const ModSources *modSources);
void DrawStyleCategory(EffectConfig *e, const ModSources *modSources);
void DrawColorCategory(EffectConfig *e, const ModSources *modSources);

#endif // IMGUI_EFFECTS_TRANSFORMS_H
