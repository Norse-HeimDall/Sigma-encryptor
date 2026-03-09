/**
 * @file theme.hpp
 * @brief Заголовочный файл темы Obsidian для ImGui
 * 
 * @details
 * Содержит настройки цветов и стилей для создания
 * минималистичного интерфейса в стиле Obsidian.
 * 
 * @author Sigma Team
 * @version 1.0
 */

#ifndef SIGMA_THEME_HPP
#define SIGMA_THEME_HPP

#include "imgui.h"

namespace sigma
{

/**
 * @class Theme
 * @brief Управление темой оформления в стиле Obsidian
 */
class Theme
{
public:
    /**
     * @brief Применяет тему Obsidian
     * 
     * Настраивает цвета ImGui:
     * - Тёмный фон
     * - Мягкие акцентные цвета
     * - Современные шрифты
     */
    static void applyObsidianTheme();

    /**
     * @brief Сбрасывает тему к стандартной
     */
    static void resetTheme();
};

} // namespace sigma

#endif // SIGMA_THEME_HPP
