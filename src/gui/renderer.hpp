/**
 * @file renderer.hpp
 * @brief Заголовочный файл рендерера OpenGL для ImGui
 * 
 * @details
 * Этот файл содержит объявление класса Renderer, который управляет:
 * - Инициализацией контекста OpenGL
 * - Настройкой ImGui
 * - Рендерингом кадров
 * 
 * @author Sigma Team
 * @version 1.0
 */

#ifndef SIGMA_RENDERER_HPP
#define SIGMA_RENDERER_HPP

// Стандартные библиотеки
#include <string>
#include <memory>

// GLFW - библиотека для создания окон
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// ImGui - библиотека GUI (включает IM_ASSERT)
#include "imgui.h"

namespace sigma
{

/**
 * @class Renderer
 * @brief Класс для управления рендерингом
 */
class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool initialize(GLFWwindow* window);
    void beginFrame();
    void endFrame();
    void shutdown();

private:
    GLFWwindow* m_window = nullptr;
    double m_time = 0.0;
    float m_mouseWheel = 0.0f;
    bool m_initialized = false;
};

} // namespace sigma

#endif // SIGMA_RENDERER_HPP
