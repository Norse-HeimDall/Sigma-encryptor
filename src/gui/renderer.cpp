/**
 * @file renderer.cpp
 * @brief Реализация рендерера OpenGL для ImGui (Windows/Master branch)
 * @author heimdall
 */

#include "renderer.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <iostream>
#include <vector>

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

Renderer::Renderer()
    : m_window(nullptr)
    , m_time(0.0)
    , m_mouseWheel(0.0f)
    , m_initialized(false)
{
    std::cout << "[ИНФО] Рендерер Sigma создан" << std::endl;
}

Renderer::~Renderer()
{
    shutdown();
}

// =============================================================================
// ИНИЦИАЛИЗАЦИЯ
// =============================================================================

bool Renderer::initialize(GLFWwindow* window)
{
    if (m_initialized)
    {
        std::cout << "[ПРЕДУПРЕЖДЕНИЕ] ImGui уже инициализирован" << std::endl;
        return true;
    }

    if (!window) return false;
    m_window = window;

    // 1. Создание контекста ImGui
    IMGUI_CHECKVERSION();
    IM_ASSERT(!ImGui::GetCurrentContext());
    ImGui::CreateContext();
    
    std::cout << "[INFO] ImGui context created" << std::endl;

    // 2. Настройка ввода и шрифтов (Кириллица)
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;               Просчитался. скачал не ту версию glfw
    io.FontGlobalScale = 1.0f;
    io.FontAllowUserScaling = true;

    // Диапазоны символов для кириллицы
    static const ImWchar cyrillic_ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x04FF, // Cyrillic
        0x0500, 0x052F, // Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };

    // Поиск системного шрифта (Windows)
    const char* fontPaths[] = {
        "C:/Windows/Fonts/segoeuib.ttf",   // Segoe UI Bold
        "C:/Windows/Fonts/seguisb.ttf",    // Segoe UI Semibold
        "C:/Windows/Fonts/segoeui.ttf",    // Segoe UI
        "C:/Windows/Fonts/arial.ttf"
    };

    ImFont* mainFont = nullptr;
    for (const char* path : fontPaths)
    {
        mainFont = io.Fonts->AddFontFromFileTTF(path, 20.0f, nullptr, cyrillic_ranges);
        if (mainFont)
        {
            std::cout << "[INFO] Loaded font: " << path << " (20px)" << std::endl;
            break;
        }
    }

    if (!mainFont)
    {
        io.Fonts->AddFontDefault();
        std::cout << "[INFO] Using default ImGui font" << std::endl;
    }

    // Настройка чёткости шрифтов
    io.Fonts->TexDesiredWidth = 2048;
    io.Fonts->TexGlyphPadding = 1;
    io.Fonts->Build();

    // 3. Инициализация бэкендов
    // Используем GLSL 130 для широкой совместимости
    const char* glsl_version = "#version 130";
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать ImGui GLFW бэкенд" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать ImGui OpenGL бэкенд" << std::endl;
        return false;
    }

    // Стилизация под проект Sigma
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;

    m_initialized = true;
    std::cout << "[ИНФО] ImGui инициализирован (OpenGL 3)" << std::endl;
    return true;
}

// =============================================================================
// ЦИКЛ КАДРА
// =============================================================================

void Renderer::beginFrame()
{
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Renderer::endFrame()
{
    if (!m_initialized) return;

    ImGui::Render();
    
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    // Фоновая очистка в стиле Kali (темно-серый)
    glClearColor(0.05f, 0.05f, 0.05f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// =============================================================================
// ОЧИСТКА (МЕРЫ БЕЗОПАСНОСТИ)
// =============================================================================

void Renderer::shutdown()
{
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    std::cout << "[ИНФО] ImGui полностью завершил работу" << std::endl;
}

} // namespace sigma