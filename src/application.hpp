/**
 * @file application.hpp
 * @brief Заголовочный файл управляющего ядра Sigma Encryptor.
 * 
 * Описывает класс Application, реализующий паттерны для управления 
 * жизненным циклом программы: от инициализации контекста OpenGL до 
 * корректного завершения всех потоков и очистки ресурсов.
 * 
 * @author heimdall
 */

#ifndef SIGMA_APPLICATION_HPP
#define SIGMA_APPLICATION_HPP

#include <string>       
#include <memory>       // std::unique_ptr
#include <vector>       

// GLFW используется для создания кроссплатформенных окон и обработки ввода.
#include <GLFW/glfw3.h>

namespace sigma
{

// Предварительные объявления для минимизации зависимостей в заголовочном файле.
class Renderer;
class UI;

/**
 * @enum AppState
 * @brief Состояния жизненного цикла приложения.
 */
enum class AppState
{
    Running,      ///< Активное выполнение главного цикла.
    ShuttingDown, ///< Процесс деинициализации и сохранения данных.
    Error         ///< Критическая ошибка, требующая аварийного выхода.
};

/**
 * @class Application
 * @brief Центральный контроллер приложения Sigma Encryptor.
 * 
 * Класс инкапсулирует в себе окно GLFW, графический рендерер и слой UI.
 * Реализует принцип RAII: все ресурсы, захваченные в процессе работы, 
 * гарантированно освобождаются в деструкторе или методе cleanup.
 */
class Application
{
public:
    /**
     * @brief Конструктор: подготавливает базовые параметры окна.
     */
    Application();

    /**
     * @brief Деструктор: инициирует очистку графического контекста.
     */
    ~Application();

    /**
     * @brief Инициализация графической подсистемы и библиотек.
     * 
     * Настраивает параметры окна, версию OpenGL 3.3 Core и связывает 
     * контекст рендеринга с ImGui.
     * 
     * @return true при успешном создании окна и инициализации всех систем.
     */
    bool initialize();

    /**
     * @brief Точка входа в главный цикл обработки событий и отрисовки.
     * 
     * Цикл выполняется до тех пор, пока не будет получен сигнал закрытия 
     * окна или статус не сменится на ShuttingDown.
     */
    void run();

    /**
     * @brief Команда на безопасное завершение работы приложения.
     */
    void shutdown();

private:
    /**
     * @brief Обработка системных прерываний и ввода пользователя.
     */
    void processEvents();

    /**
     * @brief Обновление внутренних таймеров и логики интерфейса.
     */
    void update();

    /**
     * @brief Отрисовка кадра через Renderer и UI.
     */
    void render();

    /**
     * @brief Финальная деинициализация библиотек GLFW и ImGui.
     */
    void cleanup();

private:
    // Компоненты графического интерфейса
    std::unique_ptr<Renderer> m_renderer;  ///< Управление OpenGL-контекстом.
    std::unique_ptr<UI> m_ui;              ///< Логика отрисовки окон и кнопок.

    // Системные объекты
    GLFWwindow* m_window = nullptr;        ///< Указатель на низкоуровневое окно.
    AppState m_state = AppState::Running;  ///< Текущий статус приложения.

    // Параметры окна
    std::string m_title = "Sigma Encryptor"; ///< Заголовок приложения.
    int m_width = 900;                       ///< Начальная ширина окна.
    int m_height = 600;                      ///< Начальная высота окна.

    bool m_shouldClose = false;              ///< Флаг выхода из цикла.
};

} // namespace sigma

#endif // SIGMA_APPLICATION_HPP