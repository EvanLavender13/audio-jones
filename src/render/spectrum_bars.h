#ifndef SPECTRUM_BARS_H
#define SPECTRUM_BARS_H

#include "config/spectrum_bars_config.h"
#include "render_context.h"
#include <stdint.h>

typedef struct SpectrumBars SpectrumBars;

// Lifecycle
SpectrumBars* SpectrumBarsInit(void);
void SpectrumBarsUninit(SpectrumBars* sb);

// Process magnitude spectrum into 32 display bands (call when FFT updates)
void SpectrumBarsProcess(SpectrumBars* sb,
                         const float* magnitude,
                         int binCount,
                         const SpectrumConfig* config);

// Render to current render target
// opacity: 0.0-1.0 alpha multiplier for split-pass rendering
void SpectrumBarsDrawCircular(const SpectrumBars* sb,
                              const RenderContext* ctx,
                              const SpectrumConfig* config,
                              uint64_t globalTick,
                              float opacity);

void SpectrumBarsDrawLinear(const SpectrumBars* sb,
                            const RenderContext* ctx,
                            const SpectrumConfig* config,
                            uint64_t globalTick,
                            float opacity);

#endif // SPECTRUM_BARS_H
