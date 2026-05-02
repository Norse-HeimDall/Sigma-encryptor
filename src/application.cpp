/**
 * @file application.cpp
 * @brief Реализация жизненного цикла графического приложения Sigma Encryptor.
 * @author heimdall
 */

#include "application.hpp"
#include "gui/renderer.hpp"
#include "gui/ui.hpp"

#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

#include <GLFW/glfw3.h>

namespace sigma
{

// ============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// ============================================================================

/**
 * @brief Создает объект приложения, устанавливая начальное состояние.
 */
Application::Application()
    : m_state(AppState::Running)
    , m_window(nullptr)
    , m_shouldClose(false)
{
    std::cout << "[ИНФО] Объект Application создан..." << std::endl;
}

/**
 * @brief Гарантирует корректную очистку ресурсов перед уничтожением объекта.
 */
Application::~Application()
{
    cleanup();
    std::cout << "[ИНФО] Объект Application уничтожен" << std::endl;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================

/**
 * @brief Инициализация графической подсистемы.
 * 
 * Настраивает GLFW, создает контекст OpenGL 3.3 Core Profile и 
 * инициализирует компоненты рендеринга и UI.
 * 
 * @return true если инициализация прошла успешно.
 */
bool Application::initialize()
{
    std::cout << "[ИНФО] Начало инициализации приложения..." << std::endl;

    if (!glfwInit())
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать GLFW" << std::endl;
        return false;
    }

    // Настройка параметров графического контекста (OpenGL 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Создание главного окна
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

    if (m_window == nullptr)
    {
        std::cerr << "[ОШИБКА] Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // ОПТИМИЗАЦИЯ: Синхронизация с частотой обновления монитора (V-Sync)
    // Предотвращает разрывы изображения и излишнюю нагрузку на GPU.
    glfwSwapInterval(1);
    std::cout << "[ИНФО] V-Sync включен" << std::endl;

    // Сбор отладочной информации о видеодрайвере
    const GLubyte* gl_renderer = glGetString(GL_RENDERER);
    if (gl_renderer) std::cout << "[ИНФО] OpenGL Renderer: " << gl_renderer << std::endl;

    // Инициализация системы отрисовки (интеграция с ImGui)
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->initialize(m_window))
    {
        std::cerr << "[ОШИБКА] Не удалось инициализировать рендерер" << std::endl;
        return false;
    }

    // Создание слоя пользовательского интерфейса
    m_ui = std::make_unique<UI>();
    m_ui->initialize();

    // Регистрация обратного вызова для безопасного закрытия приложения
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) app->shutdown();
    });

    std::cout << "[ИНФО] Инициализация завершена успешно" << std::endl;
    return true;
}

// ============================================================================
// ГЛАВНЫЙ ЦИКЛ ПРИЛОЖЕНИЯ
// ============================================================================

/**
 * @brief Главный цикл обработки (Game Loop).
 */
void Application::run()
{
    m_state = AppState::Running;

    while (m_state == AppState::Running && !glfwWindowShouldClose(m_window))
    {
        processEvents();
        update();
        render();

        if (m_shouldClose) m_state = AppState::ShuttingDown;
    }
}

/**
 * @brief Обработка системных событий ввода.
 * 
 * ОПТИМИЗАЦИЯ: Ожидание событий с таймаутом (7мс) позволяет снизить
 * использование CPU, если нету взаимодействия с окном.
 */
void Application::processEvents()
{
    glfwWaitEventsTimeout(0.007);
}

void Application::update()
{
    // Место для обновления логики UI или фоновых задач(прогресс бар. пока отсуствует, потом доделаю)
}

/**
 * @brief Отрисовка текущего кадра.
 * 
 * ОПТИМИЗАЦИЯ: Если окно свернуто, отрисовка приостанавливается,
 * чтобы сэкономить ресурсы системы.
 */
void Application::render()
{
    if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED))
    {
        // Спим 100мс, чтобы не нагружать цикл в свернутом режиме
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    m_renderer->beginFrame(); // Подготовка буферов
    
    if (m_ui) m_ui->render(); // Рисование виджетов ImGui

    m_renderer->endFrame();   // Финализация кадра
    
    glfwSwapBuffers(m_window); // Вывод кадра на экран
}

// ============================================================================
// ЗАВЕРШЕНИЕ
// ============================================================================

/**
 * @brief Инициирует процесс выхода из приложения.
 */
void Application::shutdown()
{
    m_shouldClose = true;
    m_state = AppState::ShuttingDown;
}

/**
 * @brief Безопасное освобождение всех выделенных ресурсов.
 */
void Application::cleanup()
{
    std::cout << "[ИНФО] Очистка ресурсов..." << std::endl;

    if (m_ui) m_ui.reset();

    if (m_renderer)
    {
        if (m_window) glfwMakeContextCurrent(m_window);
        m_renderer->shutdown();
        m_renderer.reset();
    }

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

} // namespace sigma