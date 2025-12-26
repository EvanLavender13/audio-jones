#ifndef UI_THEME_H
#define UI_THEME_H

#include "imgui.h"
#include "raylib.h"

// Neon Eclipse Theme - Synthwave-inspired palette for audio visualization
// Deep cosmic backgrounds with electric cyan/magenta accents and warm orange highlights

namespace Theme
{
    // Background layers (deep cosmic blue-black)
    constexpr ImVec4 BG_VOID    = ImVec4(0.04f, 0.03f, 0.06f, 1.00f);    // #0A0810 - deepest
    constexpr ImVec4 BG_DEEP    = ImVec4(0.06f, 0.05f, 0.09f, 0.95f);    // #0F0D17 - window bg
    constexpr ImVec4 BG_MID     = ImVec4(0.09f, 0.08f, 0.13f, 1.00f);    // #171421 - frame bg
    constexpr ImVec4 BG_SURFACE = ImVec4(0.12f, 0.10f, 0.16f, 1.00f);    // #1F1A29 - elevated

    // Primary accent - Electric Cyan
    constexpr ImVec4 ACCENT_CYAN       = ImVec4(0.00f, 0.90f, 0.95f, 1.00f);    // #00E6F2
    constexpr ImVec4 ACCENT_CYAN_DIM   = ImVec4(0.00f, 0.55f, 0.60f, 1.00f);    // #008C99
    constexpr ImVec4 ACCENT_CYAN_HOVER = ImVec4(0.20f, 1.00f, 1.00f, 1.00f);    // #33FFFF

    // Secondary accent - Hot Magenta
    constexpr ImVec4 ACCENT_MAGENTA       = ImVec4(1.00f, 0.08f, 0.58f, 1.00f); // #FF1493
    constexpr ImVec4 ACCENT_MAGENTA_DIM   = ImVec4(0.65f, 0.05f, 0.38f, 1.00f); // #A60D61
    constexpr ImVec4 ACCENT_MAGENTA_HOVER = ImVec4(1.00f, 0.30f, 0.70f, 1.00f); // #FF4DB3

    // Tertiary accent - Sunset Orange
    constexpr ImVec4 ACCENT_ORANGE       = ImVec4(1.00f, 0.45f, 0.05f, 1.00f);  // #FF730D
    constexpr ImVec4 ACCENT_ORANGE_DIM   = ImVec4(0.70f, 0.30f, 0.04f, 1.00f);  // #B34D0A
    constexpr ImVec4 ACCENT_ORANGE_HOVER = ImVec4(1.00f, 0.55f, 0.20f, 1.00f);  // #FF8C33

    // Text hierarchy
    constexpr ImVec4 TEXT_PRIMARY   = ImVec4(0.92f, 0.90f, 0.95f, 1.00f); // #EBE6F2 - bright
    constexpr ImVec4 TEXT_SECONDARY = ImVec4(0.60f, 0.58f, 0.68f, 1.00f); // #9994AD - muted
    constexpr ImVec4 TEXT_DISABLED  = ImVec4(0.40f, 0.38f, 0.45f, 1.00f); // #666173 - dim

    // Borders
    constexpr ImVec4 BORDER       = ImVec4(0.22f, 0.18f, 0.30f, 1.00f);  // #382E4D
    constexpr ImVec4 BORDER_GLOW  = ImVec4(0.00f, 0.70f, 0.75f, 0.40f);  // Cyan glow

    // Glow effects (use with alpha blending)
    constexpr ImU32 GLOW_CYAN    = IM_COL32(0, 230, 242, 80);
    constexpr ImU32 GLOW_MAGENTA = IM_COL32(255, 20, 147, 80);
    constexpr ImU32 GLOW_ORANGE  = IM_COL32(255, 115, 13, 80);

    // Interactive widget colors as ImU32 for direct draw calls
    constexpr ImU32 WIDGET_BG_TOP    = IM_COL32(23, 20, 33, 255);
    constexpr ImU32 WIDGET_BG_BOTTOM = IM_COL32(15, 13, 23, 255);
    constexpr ImU32 WIDGET_BORDER    = IM_COL32(56, 46, 77, 255);

    // Text colors as ImU32 for draw list operations
    constexpr ImU32 TEXT_PRIMARY_U32   = IM_COL32(235, 230, 242, 255);
    constexpr ImU32 TEXT_SECONDARY_U32 = IM_COL32(153, 148, 173, 255);

    // Band meter colors - ImU32 versions
    constexpr ImU32 BAND_CYAN_U32         = IM_COL32(0, 230, 242, 255);
    constexpr ImU32 BAND_WHITE_U32        = IM_COL32(235, 230, 242, 255);
    constexpr ImU32 BAND_MAGENTA_U32      = IM_COL32(255, 20, 147, 255);
    constexpr ImU32 BAND_CYAN_GLOW_U32    = IM_COL32(0, 230, 242, 100);
    constexpr ImU32 BAND_WHITE_GLOW_U32   = IM_COL32(235, 230, 242, 80);
    constexpr ImU32 BAND_MAGENTA_GLOW_U32 = IM_COL32(255, 20, 147, 100);
    constexpr ImU32 BAND_CYAN_ACTIVE_U32    = IM_COL32(51, 255, 255, 255);
    constexpr ImU32 BAND_WHITE_ACTIVE_U32   = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 BAND_MAGENTA_ACTIVE_U32 = IM_COL32(255, 77, 179, 255);

    // Accent colors as ImU32 for handle borders
    constexpr ImU32 ACCENT_CYAN_U32    = IM_COL32(0, 230, 242, 255);
    constexpr ImU32 ACCENT_MAGENTA_U32 = IM_COL32(255, 20, 147, 255);
    constexpr ImU32 ACCENT_ORANGE_U32  = IM_COL32(255, 115, 13, 255);

    // Shared slider handle dimensions
    constexpr float HANDLE_WIDTH   = 8.0f;
    constexpr float HANDLE_HEIGHT  = 18.0f;
    constexpr float HANDLE_OVERLAP = 4.0f;
    constexpr float HANDLE_RADIUS  = 2.0f;

} // namespace Theme

// Draw an interactive handle with glow effects for hover/active states
inline void DrawInteractiveHandle(ImDrawList* draw, ImVec2 handleMin, ImVec2 handleMax,
                                   ImU32 fillColor, bool isActive, bool isHovered, float cornerRadius)
{
    ImU32 borderColor = Theme::WIDGET_BORDER;
    if (isActive) {
        const ImVec2 glowMin = ImVec2(handleMin.x - 3, handleMin.y - 3);
        const ImVec2 glowMax = ImVec2(handleMax.x + 3, handleMax.y + 3);
        draw->AddRectFilled(glowMin, glowMax, Theme::GLOW_MAGENTA, cornerRadius + 2);
        borderColor = Theme::ACCENT_MAGENTA_U32;
    } else if (isHovered) {
        const ImVec2 glowMin = ImVec2(handleMin.x - 2, handleMin.y - 2);
        const ImVec2 glowMax = ImVec2(handleMax.x + 2, handleMax.y + 2);
        draw->AddRectFilled(glowMin, glowMax, Theme::GLOW_CYAN, cornerRadius + 1);
        borderColor = Theme::ACCENT_CYAN_U32;
    }
    draw->AddRectFilled(handleMin, handleMax, fillColor, cornerRadius);
    draw->AddRect(handleMin, handleMax, borderColor, cornerRadius, 0, 1.5f);
}

// raylib Color versions for waveform presets
// Use NEON_ prefix to avoid conflicts with raylib color macros
namespace ThemeColor
{
    inline const Color NEON_CYAN           = {0, 230, 242, 255};
    inline const Color NEON_MAGENTA        = {255, 20, 147, 255};
    inline const Color NEON_ORANGE         = {255, 115, 13, 255};
    inline const Color NEON_WHITE          = {235, 230, 242, 255};
    inline const Color NEON_CYAN_BRIGHT    = {51, 255, 255, 255};
    inline const Color NEON_MAGENTA_BRIGHT = {255, 77, 179, 255};
    inline const Color NEON_ORANGE_BRIGHT  = {255, 140, 51, 255};
    inline const Color NEON_CYAN_DIM       = {0, 140, 153, 255};
}

#endif // UI_THEME_H
