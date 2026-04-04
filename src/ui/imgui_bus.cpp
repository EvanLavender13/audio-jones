#include "automation/mod_bus.h"
#include "automation/mod_sources.h"
#include "config/mod_bus_config.h"
#include "imgui.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include <stdio.h>

static const int BUS_HISTORY_SIZE = 120;
static float busHistory[NUM_MOD_BUSES][BUS_HISTORY_SIZE] = {};
static int busHistoryIndex[NUM_MOD_BUSES] = {};

static const char *operatorNames[] = {
    "Add",         "Multiply",   "Min",          "Max",
    "Gate",        "Crossfade",  "Difference",   "Env Follow",
    "Env Trigger", "Slew (Exp)", "Slew (Linear)"};

// Semantic grouping for input source combos
struct SourceGroup {
  const char *label;
  int indices[10];
  int count;
};

static const SourceGroup SOURCE_GROUPS[] = {
    {"Bands", {MOD_SOURCE_BASS, MOD_SOURCE_MID, MOD_SOURCE_TREB}, 3},
    {"Spectral", {MOD_SOURCE_BEAT, MOD_SOURCE_CENTROID}, 2},
    {"Features",
     {MOD_SOURCE_FLATNESS, MOD_SOURCE_SPREAD, MOD_SOURCE_ROLLOFF,
      MOD_SOURCE_FLUX, MOD_SOURCE_CREST},
     5},
    {"LFOs",
     {MOD_SOURCE_LFO1, MOD_SOURCE_LFO2, MOD_SOURCE_LFO3, MOD_SOURCE_LFO4,
      MOD_SOURCE_LFO5, MOD_SOURCE_LFO6, MOD_SOURCE_LFO7, MOD_SOURCE_LFO8},
     8},
    {"Buses",
     {MOD_SOURCE_BUS1, MOD_SOURCE_BUS2, MOD_SOURCE_BUS3, MOD_SOURCE_BUS4,
      MOD_SOURCE_BUS5, MOD_SOURCE_BUS6, MOD_SOURCE_BUS7, MOD_SOURCE_BUS8},
     8},
};
static const int SOURCE_GROUP_COUNT = 5;

static void DrawBusHistoryPreview(ImVec2 size, int busIndex, bool enabled,
                                  bool bipolar, ImU32 accentColor) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const ImVec2 pos = ImGui::GetCursorScreenPos();

  ImGui::Dummy(size);

  const float padX = 6.0f;
  const float padY = 4.0f;
  const ImVec2 graphMin = ImVec2(pos.x + padX, pos.y + padY);
  const ImVec2 graphMax = ImVec2(pos.x + size.x - padX, pos.y + size.y - padY);
  const float graphW = graphMax.x - graphMin.x;
  const float graphH = graphMax.y - graphMin.y;

  // Background
  draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      Theme::WIDGET_BG_BOTTOM, 4.0f);
  draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                Theme::WIDGET_BORDER, 4.0f);

  // Center line for bipolar signals
  const float centerY = graphMin.y + graphH * 0.5f;
  if (bipolar) {
    draw->AddLine(ImVec2(graphMin.x, centerY), ImVec2(graphMax.x, centerY),
                  Theme::GUIDE_LINE, 1.0f);
  }

  if (!enabled) {
    return;
  }

  // Draw history from oldest to newest (scrolling left)
  const int writeIdx = busHistoryIndex[busIndex];
  ImVec2 points[BUS_HISTORY_SIZE];
  for (int i = 0; i < BUS_HISTORY_SIZE; i++) {
    const int readIdx = (writeIdx + i) % BUS_HISTORY_SIZE;
    const float value = busHistory[busIndex][readIdx];
    float normY;
    if (bipolar) {
      normY = (value + 1.0f) * 0.5f;
    } else {
      normY = value;
    }
    const float t =
        static_cast<float>(i) / static_cast<float>(BUS_HISTORY_SIZE - 1);
    points[i] = ImVec2(graphMin.x + t * graphW, graphMax.y - normY * graphH);
  }

  // Waveform glow and line
  const ImU32 glowColor = SetColorAlpha(accentColor, 30);
  draw->AddPolyline(points, BUS_HISTORY_SIZE, glowColor, ImDrawFlags_None,
                    4.0f);
  draw->AddPolyline(points, BUS_HISTORY_SIZE, accentColor, ImDrawFlags_None,
                    1.5f);

  // Current value indicator at right edge
  const int latestIdx = (writeIdx + BUS_HISTORY_SIZE - 1) % BUS_HISTORY_SIZE;
  const float currentVal = busHistory[busIndex][latestIdx];
  float dotY;
  if (bipolar) {
    dotY = centerY - currentVal * (graphH * 0.5f);
  } else {
    dotY = graphMax.y - currentVal * graphH;
  }
  draw->AddCircleFilled(ImVec2(graphMax.x, dotY), 4.0f, accentColor);
  draw->AddCircle(ImVec2(graphMax.x, dotY), 4.0f, Theme::TEXT_PRIMARY_U32, 0,
                  1.0f);
}

static void DrawBusOutputMeter(float value, bool enabled, bool bipolar,
                               ImU32 accentColor, float height) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  const float width = 4.0f;

  ImGui::Dummy(ImVec2(width, height));

  // Background
  draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                      Theme::WIDGET_BG_BOTTOM, 2.0f);
  draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
                Theme::WIDGET_BORDER, 2.0f);

  if (!enabled) {
    return;
  }

  if (bipolar) {
    // Center-fill: fill from center based on output value
    const float centerY = pos.y + height * 0.5f;
    draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + width, centerY),
                  Theme::GUIDE_LINE, 1.0f);
    const float fillHeight = fabsf(value) * (height * 0.5f - 1.0f);
    if (value > 0.0f) {
      draw->AddRectFilled(ImVec2(pos.x + 1, centerY - fillHeight),
                          ImVec2(pos.x + width - 1, centerY), accentColor,
                          1.0f);
    } else {
      draw->AddRectFilled(ImVec2(pos.x + 1, centerY),
                          ImVec2(pos.x + width - 1, centerY + fillHeight),
                          accentColor, 1.0f);
    }
  } else {
    // Unipolar: bottom-fill proportional to value
    const float fillHeight = value * (height - 2.0f);
    draw->AddRectFilled(ImVec2(pos.x + 1, pos.y + height - 1.0f - fillHeight),
                        ImVec2(pos.x + width - 1, pos.y + height - 1.0f),
                        accentColor, 1.0f);
  }
}

static void DrawOperatorCombo(int *op, ImU32 accentColor) {
  const char *preview = operatorNames[*op];

  // Right-align the combo on the same line
  const float comboWidth = 110.0f;
  const float regionMax = ImGui::GetContentRegionMax().x;
  ImGui::SameLine(regionMax - comboWidth);
  ImGui::SetNextItemWidth(comboWidth);

  ImGui::PushStyleColor(ImGuiCol_Header, SetColorAlpha(accentColor, 60));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SetColorAlpha(accentColor, 80));

  char comboId[32];
  (void)snprintf(comboId, sizeof(comboId), "##op");
  if (ImGui::BeginCombo(comboId, preview)) {
    for (int j = 0; j < BUS_OP_COUNT; j++) {
      // Separator before envelope group and slew group
      if (j == 7) {
        ImGui::Separator();
      }
      if (j == 9) {
        ImGui::Separator();
      }
      const bool selected = (*op == j);
      if (ImGui::Selectable(operatorNames[j], selected)) {
        *op = j;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::PopStyleColor(2);
}

static void DrawSourceCombo(const char *label, int *source, float width) {
  const char *preview = ModSourceGetName(static_cast<ModSource>(*source));

  char comboId[32];
  (void)snprintf(comboId, sizeof(comboId), "##%s", label);
  ImGui::TextUnformatted(label);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(width);
  if (ImGui::BeginCombo(comboId, preview)) {
    for (int g = 0; g < SOURCE_GROUP_COUNT; g++) {
      if (g > 0) {
        ImGui::Separator();
      }
      ImGui::TextDisabled("%s", SOURCE_GROUPS[g].label);
      for (int s = 0; s < SOURCE_GROUPS[g].count; s++) {
        const int idx = SOURCE_GROUPS[g].indices[s];
        const bool selected = (*source == idx);
        if (ImGui::Selectable(ModSourceGetName(static_cast<ModSource>(idx)),
                              selected)) {
          *source = idx;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
    }
    ImGui::EndCombo();
  }
}

void ImGuiDrawBusPanel(ModBusConfig *configs, const ModBusState *states,
                       const ModSources *sources) {
  if (!ImGui::Begin("Buses")) {
    ImGui::End();
    return;
  }

  // Record current outputs to history buffers
  for (int i = 0; i < NUM_MOD_BUSES; i++) {
    busHistory[i][busHistoryIndex[i]] = states[i].output;
    busHistoryIndex[i] = (busHistoryIndex[i] + 1) % BUS_HISTORY_SIZE;
  }

  for (int i = 0; i < NUM_MOD_BUSES; i++) {
    char sectionLabel[16];
    (void)snprintf(sectionLabel, sizeof(sectionLabel), "BUS %d", i + 1);

    const ImU32 accentColor = Theme::GetSectionAccent(i);
    const bool singleInput = BusOpIsSingleInput(configs[i].op);
    const bool bipolar = !singleInput;
    const bool isEnvelope = BusOpIsEnvelope(configs[i].op);
    const bool isSlew = BusOpIsSlew(configs[i].op);

    DrawModuleStripBegin(sectionLabel, accentColor, &configs[i].enabled);

    ImGui::PushID(i);

    // Row 1: Operator combo (right-aligned)
    DrawOperatorCombo(&configs[i].op, accentColor);

    // Row 2: Input source combos
    if (singleInput) {
      const float comboW = ImGui::GetContentRegionAvail().x -
                           ImGui::CalcTextSize("Input").x -
                           ImGui::GetStyle().ItemSpacing.x;
      DrawSourceCombo("Input", &configs[i].inputA, comboW);
    } else {
      const float halfW = ImGui::GetContentRegionAvail().x * 0.5f -
                          ImGui::GetStyle().ItemSpacing.x;
      const float comboWA =
          halfW - ImGui::CalcTextSize("A").x - ImGui::GetStyle().ItemSpacing.x;
      const float comboWB =
          halfW - ImGui::CalcTextSize("B").x - ImGui::GetStyle().ItemSpacing.x;
      DrawSourceCombo("A", &configs[i].inputA, comboWA);
      ImGui::SameLine();
      DrawSourceCombo("B", &configs[i].inputB, comboWB);
    }

    // Row 3: Processor params (envelope or slew)
    if (isEnvelope) {
      ImGui::SeparatorText("Envelope");

      char attackId[32];
      char releaseId[32];
      (void)snprintf(attackId, sizeof(attackId), "bus%d.attack", i + 1);
      (void)snprintf(releaseId, sizeof(releaseId), "bus%d.release", i + 1);

      ModulatableSlider("Attack##bus", &configs[i].attack, attackId, "%.3f s",
                        sources);
      ModulatableSlider("Release##bus", &configs[i].release, releaseId,
                        "%.2f s", sources);

      if (configs[i].op == BUS_OP_ENV_TRIGGER) {
        char holdId[32];
        char threshId[32];
        (void)snprintf(holdId, sizeof(holdId), "bus%d.hold", i + 1);
        (void)snprintf(threshId, sizeof(threshId), "bus%d.threshold", i + 1);

        ModulatableSlider("Hold##bus", &configs[i].hold, holdId, "%.2f s",
                          sources);
        ModulatableSlider("Threshold##bus", &configs[i].threshold, threshId,
                          "%.2f", sources);
      }
    } else if (isSlew) {
      ImGui::SeparatorText("Slew");

      if (!configs[i].asymmetric) {
        char lagId[32];
        (void)snprintf(lagId, sizeof(lagId), "bus%d.lagTime", i + 1);
        ModulatableSlider("Lag Time##bus", &configs[i].lagTime, lagId, "%.2f s",
                          sources);
      } else {
        char riseId[32];
        char fallId[32];
        (void)snprintf(riseId, sizeof(riseId), "bus%d.riseTime", i + 1);
        (void)snprintf(fallId, sizeof(fallId), "bus%d.fallTime", i + 1);
        ModulatableSlider("Rise Time##bus", &configs[i].riseTime, riseId,
                          "%.2f s", sources);
        ModulatableSlider("Fall Time##bus", &configs[i].fallTime, fallId,
                          "%.2f s", sources);
      }

      ImGui::Checkbox("Asymmetric##bus", &configs[i].asymmetric);
    }

    // Row 4: History graph + output meter
    const float previewHeight = 32.0f;
    DrawBusHistoryPreview(ImVec2(140.0f, previewHeight), i, configs[i].enabled,
                          bipolar, accentColor);
    ImGui::SameLine(0, 4.0f);
    DrawBusOutputMeter(states[i].output, configs[i].enabled, bipolar,
                       accentColor, previewHeight);

    ImGui::PopID();

    DrawModuleStripEnd();
  }

  ImGui::End();
}
