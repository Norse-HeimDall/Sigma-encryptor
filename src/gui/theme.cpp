/**
 * @file theme.cpp
 * @brief Реализация темы ImGui
 */

#include "theme.hpp"

#include <iostream>

namespace sigma
{

// =============================================================================
// ЦВЕТОВАЯ ПАЛИТРА 
// =============================================================================

namespace Colors
{
    // Основные цвета (в стиле Obsidian)
    constexpr ImU32 Background = IM_COL32(33, 33, 38, 255);        // Тёмно-серый фон
    constexpr ImU32 BackgroundLight = IM_COL32(40, 40, 45, 255);  // Светлее для элементов
    constexpr ImU32 BackgroundHover = IM_COL32(50, 50, 55, 255);  // При наведении
    constexpr ImU32 BackgroundActive = IM_COL32(45, 45, 50, 255);  // При нажатии
    
    constexpr ImU32 Text = IM_COL32(220, 220, 220, 255);           // Основной текст
    constexpr ImU32 TextMuted = IM_COL32(150, 150, 150, 255);     // Приглушённый текст
    constexpr ImU32 TextAccent = IM_COL32(180, 180, 200, 255);     // Акцентный текст
    
    constexpr ImU32 Border = IM_COL32(60, 60, 65, 255);            // Границы
    constexpr ImU32 BorderLight = IM_COL32(70, 70, 75, 255);       // Светлые границы
    
    constexpr ImU32 Accent = IM_COL32(70, 130, 180, 255);          // Синий акцент
    constexpr ImU32 AccentHover = IM_COL32(90, 150, 200, 255);    // Акцент при наведении
    constexpr ImU32 AccentLight = IM_COL32(100, 140, 180, 255);    // Светлый акцент
    
    constexpr ImU32 Success = IM_COL32(80, 180, 120, 255);         // Зелёный (успех)
    constexpr ImU32 Warning = IM_COL32(220, 180, 60, 255);        // Жёлтый (предупреждение)
    constexpr ImU32 Error = IM_COL32(200, 80, 80, 255);            // Красный (ошибка)
}

// =============================================================================
// ПРИМЕНЕНИЕ ТЕМЫ
// =============================================================================

void Theme::applyObsidianTheme()
{
    std::cout << "[ИНФО] Применение темы Obsidian..." << std::endl;

    // Получаем стиль ImGui
    ImGuiStyle& style = ImGui::GetStyle();

    // Настройка отступов и размеров
    // ========================================
    
    // Отступы вокруг окон
    style.WindowPadding = ImVec2(16, 16);
    
    // Отступы внутри элементов
    style.ItemSpacing = ImVec2(8, 8);
    
    // Отступ между элементами (без Internal)
    style.ItemInnerSpacing = ImVec2(8, 6);
    
    // Отступ для кнопок
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.FramePadding = ImVec2(12, 8);
    
    // Скругление углов
    style.FrameRounding = 6.0f;
    style.WindowRounding = 8.0f;
    style.PopupRounding = 6.0f;
    style.GrabRounding = 4.0f;
    
    // Минимальная ширина полос прокрутки
    style.ScrollbarSize = 10.0f;
    
    // Настройка границ
    // ========================================
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    // Цветовая схема (Color Scheme)
    // ========================================
    
    // Цвета (Colors) - используем IM_COL32 для ARGB
    
// Фон и контейнеры
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.129f, 0.129f, 0.149f, 1.0f);       // #212125
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.157f, 0.157f, 0.176f, 1.0f);       // #282830
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.157f, 0.157f, 0.176f, 1.0f);       // #282830
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);        // #141418 - тёмный как заголовок
    
    // Границы
    style.Colors[ImGuiCol_Border] = ImVec4(0.235f, 0.235f, 0.255f, 1.0f);        // #3C3C41
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);        // Без тени
    
    // Текст
    style.Colors[ImGuiCol_Text] = ImVec4(0.863f, 0.863f, 0.863f, 1.0f);          // #DCDCDC
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.588f, 0.588f, 0.588f, 1.0f); // #959595
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.275f, 0.510f, 0.706f, 1.0f); // #4682B4
    
    // Заголовки и отступы (тёмные в стиле Obsidian) - делаем темнее основного фона
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.07f, 1.0f);         // Очень тёмный
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);   // Чуть светлее
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.07f, 1.0f); // Очень тёмный
    
    // Дополнительно для рамки окна
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);   // Затемнение фона
    
    // Полосы прокрутки
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.157f, 0.157f, 0.176f, 1.0f);  // #282830
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.314f, 0.314f, 0.343f, 1.0f); // #505058
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.392f, 0.392f, 0.431f, 1.0f); // #64646E
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.471f, 0.471f, 0.510f, 1.0f); // #787882
    
    // Кнопки
    style.Colors[ImGuiCol_Button] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f);        // #46464B
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.353f, 0.353f, 0.392f, 1.0f); // #5A5A64
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.314f, 0.314f, 0.343f, 1.0f); // #505058
    
    // Поля ввода
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.196f, 0.196f, 0.216f, 1.0f);       // #323237
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.235f, 0.235f, 0.255f, 1.0f); // #3C3C41
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f); // #46464B
    
    // Чекбоксы
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.431f, 0.706f, 0.882f, 1.0f);     // #6EB4
    
    // Слайдеры
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.392f, 0.392f, 0.431f, 1.0f);    // #64646E
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.431f, 0.706f, 0.882f, 1.0f); // #6EB4
    
    // Заголовки таблиц
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.196f, 0.196f, 0.216f, 1.0f); // #323237
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f); // #46464B
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.235f, 0.235f, 0.255f, 1.0f); // #3C3C41
    
    // Разделители
    style.Colors[ImGuiCol_Separator] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f);    // #46464B
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.392f, 0.392f, 0.431f, 1.0f); // #64646E
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.431f, 0.706f, 0.882f, 1.0f); // #6EB4
    
    // Список (ListBox)
    style.Colors[ImGuiCol_Header] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f);       // #46464B
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.314f, 0.314f, 0.343f, 1.0f); // #505058
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.353f, 0.353f, 0.392f, 1.0f); // #5A5A64
    
    // Вкладки
    style.Colors[ImGuiCol_Tab] = ImVec4(0.157f, 0.157f, 0.176f, 1.0f);          // #282830
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.196f, 0.196f, 0.216f, 1.0f);    // #323237
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.275f, 0.275f, 0.294f, 1.0f);  // #46464B
    style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.157f, 0.157f, 0.176f, 1.0f); // #282830
    style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.235f, 0.235f, 0.255f, 1.0f); // #3C3C41

    std::cout << "[ИНФО] Тема Obsidian применена" << std::endl;
}

void Theme::resetTheme()
{
    ImGui::StyleColorsDark();
    std::cout << "[ИНФО] Тема сброшена" << std::endl;
}

} // namespace sigma
