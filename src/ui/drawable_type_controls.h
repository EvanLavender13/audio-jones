#ifndef DRAWABLE_TYPE_CONTROLS_H
#define DRAWABLE_TYPE_CONTROLS_H

struct Drawable;
struct ModSources;

void DrawWaveformControls(Drawable *d, const ModSources *sources);
void DrawSpectrumControls(Drawable *d, const ModSources *sources);
void DrawShapeControls(Drawable *d, const ModSources *sources);
void DrawParametricTrailControls(Drawable *d, const ModSources *sources);

#endif // DRAWABLE_TYPE_CONTROLS_H
