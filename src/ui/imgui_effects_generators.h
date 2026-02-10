#ifndef IMGUI_EFFECTS_GENERATORS_H
#define IMGUI_EFFECTS_GENERATORS_H

struct EffectConfig;
struct ModSources;

// Draw generator subcategory sections
void DrawGeneratorsCategory(EffectConfig *e, const ModSources *modSources);
void DrawGeneratorsGeometric(EffectConfig *e, const ModSources *modSources);
void DrawGeneratorsFilament(EffectConfig *e, const ModSources *modSources);
void DrawGeneratorsTexture(EffectConfig *e, const ModSources *modSources);
void DrawGeneratorsAtmosphere(EffectConfig *e, const ModSources *modSources);

#endif // IMGUI_EFFECTS_GENERATORS_H
