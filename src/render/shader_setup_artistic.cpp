#include "shader_setup_artistic.h"
#include "post_effect.h"

void SetupOilPaint(PostEffect *pe) {
  OilPaintEffectSetup(&pe->oilPaint, &pe->effects.oilPaint);
}

void SetupWatercolor(PostEffect *pe) {
  WatercolorEffectSetup(&pe->watercolor, &pe->effects.watercolor);
}

void SetupImpressionist(PostEffect *pe) {
  ImpressionistEffectSetup(&pe->impressionist, &pe->effects.impressionist);
}

void SetupInkWash(PostEffect *pe) {
  InkWashEffectSetup(&pe->inkWash, &pe->effects.inkWash);
}

void SetupPencilSketch(PostEffect *pe) {
  PencilSketchEffectSetup(&pe->pencilSketch, &pe->effects.pencilSketch,
                          pe->currentDeltaTime);
}

void SetupCrossHatching(PostEffect *pe) {
  CrossHatchingEffectSetup(&pe->crossHatching, &pe->effects.crossHatching,
                           pe->currentDeltaTime);
}
