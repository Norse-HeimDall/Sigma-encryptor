/**
 * @file theme.cpp
 * @brief Реализация темы ImGui Obsidian для проекта Sigma
 */

#include "theme.hpp"
#include <iostream>

namespace sigma
{

// =============================================================================
// ВНУТРЕННИЕ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// =============================================================================

/**
 * @brief Преобразует IM_COL32 (U32) в ImVec4 для стилей ImGui
 */
static ImVec4 ColorU32ToVec4(ImU32 col)
{
    float a = ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
    float b = ((col >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
    float g = ((col >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
    float r = ((col >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
    return ImVec4(r, g, b, a);
}

// =============================================================================
// ЦВЕТОВАЯ ПАЛИТРА
// =============================================================================

namespace Colors
{
    // Глубокий обсидиан
    constexpr ImU32 Background       = IM_COL32(33, 33, 38, 255);   // #212126
    constexpr ImU32 BackgroundLight  = IM_COL32(40, 40, 48, 255);   // #282830
    constexpr ImU32 Header           = IM_COL32(20, 20, 24, 255);   // #141418
    
    // Текст
    constexpr ImU32 Text             = IM_COL32(220, 220, 220, 255); // #DCDCDC
    constexpr ImU32 TextMuted        = IM_COL32(150, 150, 150, 255); // #969696
    
    // Акценты (Steel Blue)
    constexpr ImU32 Accent           = IM_COL32(70, 130, 180, 255);  // #4682B4
    constexpr ImU32 AccentHover      = IM_COL32(90, 150, 200, 255);  // #5A96C8
    
    // Границы
    constexpr ImU32 Border           = IM_COL32(60, 60, 65, 255);    // #3C3C41
}

// =============================================================================
// ПРИМЕНЕНИЕ ТЕМЫ
// =============================================================================

void Theme::applyObsidianTheme()
{
    std::cout << "[ИНФО] Применение темы Obsidian..." << std::endl;

    ImGuiStyle& style = ImGui::GetStyle();

    // 1. Настройка геометрии (Padding, Rounding)
    // ========================================
    style.WindowPadding      = ImVec2(16, 16);
    style.FramePadding       = ImVec2(12, 8);
    style.ItemSpacing        = ImVec2(8, 8);
    style.ItemInnerSpacing   = ImVec2(8, 6);
    style.IndentSpacing      = 20.0f;
    
    style.ScrollbarSize      = 12.0f;
    style.GrabMinSize        = 10.0f;

    style.WindowRounding     = 8.0f;
    style.FrameRounding      = 6.0f;
    style.PopupRounding      = 6.0f;
    style.ScrollbarRounding  = 12.0f;
    style.GrabRounding       = 4.0f;
    style.TabRounding        = 4.0f;

    style.WindowBorderSize   = 1.0f;
    style.ChildBorderSize    = 1.0f;
    style.PopupBorderSize    = 1.0f;
    style.FrameBorderSize    = 1.0f;
    style.TabBorderSize      = 0.0f;

    style.WindowTitleAlign   = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign    = ImVec2(0.5f, 0.5f);

    // 2. Цветовая схема
    // ========================================
    ImVec4* colors = style.Colors;

    // Фоновые элементы
    colors[ImGuiCol_WindowBg]             = ColorU32ToVec4(Colors::Background);
    colors[ImGuiCol_ChildBg]              = ColorU32ToVec4(Colors::BackgroundLight);
    colors[ImGuiCol_PopupBg]              = ColorU32ToVec4(Colors::BackgroundLight);
    colors[ImGuiCol_MenuBarBg]            = ColorU32ToVec4(Colors::Header);

    // Текст
    colors[ImGuiCol_Text]                 = ColorU32ToVec4(Colors::Text);
    colors[ImGuiCol_TextDisabled]         = ColorU32ToVec4(Colors::TextMuted);
    colors[ImGuiCol_TextSelectedBg]       = ColorU32ToVec4(Colors::Accent);

    // Заголовки
    colors[ImGuiCol_TitleBg]              = ColorU32ToVec4(Colors::Header);
    colors[ImGuiCol_TitleBgActive]        = ColorU32ToVec4(Colors::Header);
    colors[ImGuiCol_TitleBgCollapsed]     = ColorU32ToVec4(Colors::Header);

    // Границы
    colors[ImGuiCol_Border]               = ColorU32ToVec4(Colors::Border);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);

    // Интерактивные элементы (Кнопки, Поля ввода)
    colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.24f, 0.26f, 1.0f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);

    colors[ImGuiCol_Button]               = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered]        = ColorU32ToVec4(Colors::AccentHover);
    colors[ImGuiCol_ButtonActive]         = ColorU32ToVec4(Colors::Accent);

    // Специфические элементы
    colors[ImGuiCol_CheckMark]            = ColorU32ToVec4(Colors::Accent);
    colors[ImGuiCol_SliderGrab]           = ImVec4(0.40f, 0.40f, 0.44f, 1.0f);
    colors[ImGuiCol_SliderGrabActive]     = ColorU32ToVec4(Colors::Accent);
    
    colors[ImGuiCol_Header]               = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.32f, 0.32f, 0.35f, 1.0f);
    colors[ImGuiCol_HeaderActive]         = ColorU32ToVec4(Colors::Accent);

    colors[ImGuiCol_Separator]            = ColorU32ToVec4(Colors::Border);
    colors[ImGuiCol_SeparatorHovered]     = ColorU32ToVec4(Colors::Accent);
    colors[ImGuiCol_SeparatorActive]      = ColorU32ToVec4(Colors::Accent);

    colors[ImGuiCol_ResizeGrip]           = ImVec4(0, 0, 0, 0); // Скрываем для чистоты
    colors[ImGuiCol_ResizeGripHovered]    = ColorU32ToVec4(Colors::AccentHover);
    colors[ImGuiCol_ResizeGripActive]     = ColorU32ToVec4(Colors::Accent);

    std::cout << "[ИНФО] Тема Obsidian успешно применена" << std::endl;
}

void Theme::resetTheme()
{
    ImGui::StyleColorsDark();
    std::cout << "[ИНФО] Тема сброшена к стандартной" << std::endl;
}

} // namespace sigma