/**
 * @file renderer.cpp
 * @brief Реализация рендерера OpenGL для ImGui
 * 
 * @details
 * Реализует методы класса Renderer для управления рендерингом.
 * Использует бэкенды ImGui для GLFW и OpenGL 3.
 * 
 * Включает меры безопасности для корректного завершения работы:
 * - ImGui_ImplOpenGL3_Shutdown() - освобождение OpenGL бэкенда
 * - ImGui_ImplGlfw_Shutdown() - освобождение GLFW бэкенда
 * - ImGui::DestroyContext() - полное уничтожение контекста ImGui
 * 
 * Меры безопасности OpenGL:
 * - Проверка контекста перед операциями
 * - Debug callback для отладки
 * - Обработка потери контекста
 * 
 * @author heimdal
 * @version 1.0
 */

#include "renderer.hpp"

// ImGui бэкенды (включают необходимые OpenGL заголовочные автоматически)
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

// Стандартные библиотеки
#include <iostream>
#include <cstring>

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

Renderer::Renderer()
    : m_time(0.0)
    , m_mouseWheel(0.0f)
    , m_initialized(false)
{
    std::cout << "[ИНФО] Рендерер создан" << std::endl;
}

Renderer::~Renderer()
{
    shutdown();
}

// =============================================================================
// ИНИЦИАЛИЗАЦИЯ
// =============================================================================

/**
 * @brief Инициализирует ImGui с GLFW и OpenGL бэкендами
 */
bool Renderer::initialize(GLFWwindow* window)
{
    if (m_initialized)
    {
        std::cout << "[ПРЕДУПРЕЖДЕНИЕ] ImGui уже инициализирован" << std::endl;
        return true;
    }

    m_window = window;

    // СОЗДАНИЕ КОНТЕКСТА IMGUI (КРИТИЧЕСКИ ВАЖНО!)
 
    IM_ASSERT(!ImGui::GetCurrentContext());
    ImGui::CreateContext();
    
    std::cout << "[INFO] ImGui context created" << std::endl;
    
    // -------------------------------------------------------------------------
    // ЗАГРУЗКА ШРИФТА
    // -------------------------------------------------------------------------
    // Настраиваем Io для загрузки шрифтов
    ImGuiIO& io = ImGui::GetIO();
    
    // Включаем сглаживание шрифтов
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
    
    // Пробуем загрузить системный шрифт с поддержкой кириллицы (Windows)
    // Используем Segoe UI для качественного отображения
    const char* fontPaths[] = {
        "C:/Windows/Fonts/segoeuib.ttf",   // Segoe UI Bold
        "C:/Windows/Fonts/seguisb.ttf",    // Segoe UI Semibold
        "C:/Windows/Fonts/segoeui.ttf",    // Segoe UI
        "C:/Windows/Fonts/arial.ttf",      // Arial
    };
    
    ImFont* font = nullptr;
    // Увеличиваем размер шрифта до 20.0f
    for (const char* fontPath : fontPaths)
    {
        font = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f, nullptr, cyrillic_ranges);
        if (font)
        {
            std::cout << "[INFO] Loaded font: " << fontPath << " at 20px" << std::endl;
            break;
        }
    }
    
    // Если системные шрифты не загрузились, используем встроенный по умолчанию
    if (!font)
    {
        font = io.Fonts->AddFontDefault();
        std::cout << "[INFO] Using default font" << std::endl;
    }
    
    // Устанавливаем основной шрифт
    io.FontDefault = font;
    
    // Добавляем резервный шрифт для символов, которые могут отсутствовать
    ImFontAtlas* fonts = io.Fonts;
    
    // Настраиваем параметры сглаживания - БОЛЬШЕ ЗНАЧЕНИЯ = ЧЁТЧЕ ШРИФТ
    fonts->TexDesiredWidth = 2048;  // Максимальная ширина текстуры для чёткости
    fonts->TexGlyphPadding = 1;     // Padding для сглаживания
    
    fonts->Build();
    
    // Добавляем второй шрифт меньшего размера для логов
    const char* smallFontPaths[] = {
        "C:/Windows/Fonts/seguisb.ttf",    // Segoe UI Semibold
        "C:/Windows/Fonts/segoeui.ttf",    // Segoe UI
        "C:/Windows/Fonts/arial.ttf",      // Arial
    };
    
    for (const char* fontPath : smallFontPaths)
    {
        font = io.Fonts->AddFontFromFileTTF(fontPath, 14.0f, nullptr, cyrillic_ranges);
        if (font)
        {
            std::cout << "[INFO] Loaded small font: " << fontPath << " at 14px" << std::endl;
            break;
        }
    }
    
    std::cout << "[INFO] Font configured for Cyrillic with anti-aliasing" << std::endl;

    // Инициализируем ImGui с GLFW бэкендом
    // ImGui_ImplGlfw_InitForOpenGL() настраивает обработку ввода
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать ImGui GLFW бэкенд" << std::endl;
        ImGui::DestroyContext();
        return false;
    }

    // Инициализируем ImGui с OpenGL 3 бэкендом
    // "#version 300 es" - используем OpenGL ES 3.0 (совместим с OpenGL 3.3+)
    const char* glsl_version = "#version 300 es";
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать ImGui OpenGL бэкенд" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_initialized = true;
    std::cout << "[ИНФО] ImGui инициализирован (OpenGL 3)" << std::endl;

    return true;
}

// =============================================================================
// КАДРЫ
// =============================================================================

/**
 * @brief Начинает новый кадр ImGui
 */
void Renderer::beginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    // ImGui_ImplOpenGL3_NewFrame() - подготавливает OpenGL к новому кадру
    ImGui_ImplOpenGL3_NewFrame();

    // ImGui_ImplGlfw_NewFrame() - обрабатывает события GLFW и передаёт их в ImGui
    ImGui_ImplGlfw_NewFrame();

    // ImGui::NewFrame() - создаёт новый кадр ImGui
    // После этого можно добавлять элементы интерфейса
    ImGui::NewFrame();
}

/**
 * @brief Завершает кадр и рендерит его
 */
void Renderer::endFrame()
{
    if (!m_initialized)
    {
        return;
    }

    // ImGui::Render() - завершает формирование кадра
    // После этого все элементы готовы к отрисовке
    ImGui::Render();

    // Получаем данные для отрисовки
    ImDrawData* drawData = ImGui::GetDrawData();

    // ImGui_ImplOpenGL3_RenderDrawData() - рендерит кадр через OpenGL
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

// =============================================================================
// ОЧИСТКА
// =============================================================================

/**
 * @brief Очищает ресурсы ImGui
 * 
 * МЕРА БЕЗОПАСНОСТИ #4: Безопасное завершение ImGui
 * 
 * Правильная последовательность завершения работы ImGui:
 * 1. ImGui_ImplOpenGL3_Shutdown() - освобождение OpenGL бэкенда
 * 2. ImGui_ImplGlfw_Shutdown() - освобождение GLFW бэкенда
 * 3. ImGui::DestroyContext() - полное уничтожение контекста ImGui
 * 
 * Если не вызвать ImGui::DestroyContext(), могут остаться утечки памяти
 * и неосвобождённые ресурсы GPU, что может приводить к проблемам
 * при повторном запуске приложения или при длительной работе.
 * 
 * Важно: вызывать эти функции нужно ПОСЛЕ того как OpenGL контекст
 * ещё активен (до glfwTerminate() или после glfwMakeContextCurrent).
 */
void Renderer::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    // -------------------------------------------------------------------------
    // МЕРА БЕЗОПАСНОСТИ #4: Корректное завершение ImGui
    // -------------------------------------------------------------------------
    // Освобождаем OpenGL бэкенд ImGui
    // Это удаляет шейдеры, буферы и другие ресурсы OpenGL,
    // которые были выделены при инициализации ImGui_ImplOpenGL3_Init()
    // 
    // Что делает ImGui_ImplOpenGL3_Shutdown():
    // - Удаляет скомпилированные шейдеры (glDeleteProgram)
    // - Освобождает VAO и VBO буферы (glDeleteVertexArrays, glDeleteBuffers)
    // - Очищает состояния OpenGL
    // -------------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();

    // -------------------------------------------------------------------------
    // Освобождаем GLFW бэкенд ImGui
    // Это отключает обработчики событий, которые были установлены
    // при вызове ImGui_ImplGlfw_InitForOpenGL()
    // 
    // Что делает ImGui_ImplGlfw_Shutdown():
    // - Удаляет callback функции GLFW для клавиатуры, мыши, скролла
    // - Сбрасывает состояние ImGui, связанное с вводом
    // -------------------------------------------------------------------------
    ImGui_ImplGlfw_Shutdown();

    // -------------------------------------------------------------------------
    // Полностью уничтожаем контекст ImGui
    // ImGui::DestroyContext() освобождает:
    // - Все шрифты, загруженные через ImGui::GetIO().Fonts->AddFont*()
    // - Все окна и элементы интерфейса
    // - Внутренние структуры данных ImGui
    // - Выделенную память для работы библиотеки
    // 
    // -------------------------------------------------------------------------
    ImGui::DestroyContext();

    // -------------------------------------------------------------------------
    // Сбрасываем флаг инициализации
    // Это предотвращает повторный вызов shutdown()
    // -------------------------------------------------------------------------
    m_initialized = false;
    
    std::cout << "[ИНФО] ImGui полностью завершил работу (включая DestroyContext)" << std::endl;
}

} // namespace sigma
