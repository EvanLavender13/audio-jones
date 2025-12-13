#include "ui_panel_analysis.h"
#include "ui_widgets.h"

void UIDrawAnalysisPanel(UILayout* l, BeatDetector* beat, BandEnergies* bands,
                         BandConfig* config)
{
    UILayoutGroupBegin(l, NULL);

    // Beat detection graph
    UILayoutRow(l, 40);
    GuiBeatGraph(UILayoutSlot(l, 1.0f), beat->graphHistory, BEAT_GRAPH_SIZE, beat->graphIndex);

    // Band energy meter
    UILayoutRow(l, 36);
    GuiBandMeter(UILayoutSlot(l, 1.0f), bands, config);

    UILayoutGroupEnd(l);
}
