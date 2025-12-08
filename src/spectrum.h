#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "spectrum_config.h"
#include "waveform.h"  // for RenderContext
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
void SpectrumBarsDrawCircular(const SpectrumBars* sb,
                              const RenderContext* ctx,
                              const SpectrumConfig* config,
                              uint64_t globalTick);

void SpectrumBarsDrawLinear(const SpectrumBars* sb,
                            const RenderContext* ctx,
                            const SpectrumConfig* config,
                            uint64_t globalTick);

#endif // SPECTRUM_H
