/**
 * @file main.cpp
 * @brief Главная точка входа приложения Sigma Encryptor.
 * 
 * Файл отвечает за начальную настройку окружения, инициализацию
 * глобальных параметров локали и безопасный запуск ядра приложения.
 * 
 * @author heimdall
 */

#include <iostream>     
#include <memory>       
#include <stdexcept>    
#include <clocale>      

// Кроссплатформенная настройка кодировки консоли
#ifdef _WIN32
    #include <windows.h>
#endif

#include "application.hpp"

/**
 * @brief Настраивает UTF-8 кодировку для корректного вывода логов.
 * 
 * На Kali Linux устанавливает локаль UTF-8, на Windows переключает
 * кодовую страницу консоли на 65001.
 */
void setupConsoleEncoding()


{
    // Установка универсальной локали для корректной обработки символов
    std::setlocale(LC_ALL, "en_US.UTF-8");
    
#ifdef _WIN32
    // Настройка активной кодовой страницы Windows (UTF-8)
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    
    // Ускорение ввода-вывода (отключение синхронизации с C-style stdio)
    std::ios_base::sync_with_stdio(false);
    std::cout << "[ИНФО] Кодировка консоли установлена: UTF-8" << std::endl;
}

/**
 * @brief Главная функция.
 * 
 * Реализует паттерн безопасного запуска:
 * 1. Инициализация системных параметров.
 * 2. Создание объекта Application через RAII (умные указатели).
 * 3. Глобальный перехват исключений для предотвращения тихого падения.
 */
int main()
{
    setupConsoleEncoding();
    
    try
    {
        // Создание экземпляра приложения в защищенном блоке
        // Использование unique_ptr гарантирует вызов деструктора и очистку ресурсов
        auto app = std::make_unique<sigma::Application>();
        
        if (!app->initialize())
        {
            std::cerr << "[ОШИБКА] Критическая ошибка при инициализации подсистем" << std::endl;
            return 1;
        }
        
        // Запуск рабочего цикла (обработка GUI и криптографических задач)
        app->run();
        
        std::cout << "[ИНФО] Приложение успешно завершило работу" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        // Перехват стандартных исключений (например, ошибки OpenSSL или аллокации памяти)
        std::cerr << "[КРИТИЧЕСКАЯ ОШИБКА] " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        // Резервный перехват для неопознанных типов ошибок
        std::cerr << "[КРИТИЧЕСКАЯ ОШИБКА] Произошло неопознанное исключение в runtime" << std::endl;
        return 1;
    }
}