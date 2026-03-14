/**
 * @file ui.hpp
 * @brief Заголовочный файл пользовательского интерфейса Sigma Encryptor
 * 
 * @details
 * Содержит объявление класса UI - главного класса пользовательского интерфейса.
 * UI управляет:
 * - Выбором файла через нативный диалог Windows
 * - Вводом пароля с подтверждением
 * - Выбором алгоритма шифрования (AES или ChaCha20)
 * - Выполнением операций шифрования/дешифрования
 * - Логом операций с автопрокруткой
 * 
 * Класс содержит экземпляр Encryptor для крипто-операций.
 * 
 * @author heimdall
 * @version 4.0
 */

#ifndef SIGMA_UI_HPP
#define SIGMA_UI_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstring>  // Для memset (очистка пароля)

#include "../crypto/encryptor.hpp"  // Класс шифратора
#include "file_dialog.hpp"             // Диалог файлов

namespace sigma
{

/**
 * @class UI
 * @brief Главный класс пользовательского интерфейса
 * 
 * Управляет всем ImGui интерфейсом: выбором операции (шифр/дешифр),
 * файлом, паролем, алгоритмом, выполнением операций и логом.
 * 
 * Взаимодействует с Encryptor для крипто-операций.
 * 
 * Принцип работы:
 * 1. render() - отрисовывает интерфейс каждый кадр
 * 2. Пользователь вводит данные
 * 3. Кнопка вызывает performEncryption()/performDecryption()
 * 4. Результат в лог + статус
 */
class UI
{
public:
    /**
     * @brief Конструктор
     * Создаёт Encryptor и инициализирует поля.
     */
    UI();

    /**
     * @brief Деструктор
     * Освобождает Encryptor.
     */
    ~UI();

    /**
     * @brief Инициализация UI
     * Применяет тему Obsidian.
     */
    void initialize();

    /**
     * @brief Отрисовка интерфейса (вызывается каждый кадр)
     * Создаёт главное окно ImGui с полями ввода, кнопками, логом.
     */
    void render();

    /**
     * @brief Установка пути к файлу
     * @param path Путь к файлу из file_dialog
     */
    void setFilePath(const std::string& path);

    /**
     * @brief Сброс формы
     * Очищает все поля ввода и статус.
     */
    void reset();

private:
    /**
     * @brief Проверка готовности к операции
     * @return true если файл и пароль выбраны корректно
     */
    bool canPerformOperation() const;

    /**
     * @brief Выполнение шифрования
     * Формирует outputPath (.sge), вызывает encryptor->encryptFile().
     */
    void performEncryption();

    /**
     * @brief Выполнение дешифрования
     * Формирует outputPath (убирает .sge или .dec), вызывает decryptFile().
     */
    void performDecryption();

    /**
     * @brief Добавление сообщения в лог
     * @param message Текст лога
     * Ограничивает 100 сообщений.
     */
    void addLogMessage(const std::string& message);

    // Поля состояния UI
    std::unique_ptr<Encryptor> m_encryptor;     ///< Экземпляр шифратора
    std::string m_filePath;                     ///< Путь к файлу
    char m_password[256] = {0};                 ///< Буфер пароля
    char m_passwordConfirm[256] = {0};          ///< Подтверждение пароля
    char m_filePathBuffer[512] = {0};           ///< Буфер для ImGui InputText
    CipherType m_selectedCipher = CipherType::AES_256_GCM; ///< Выбранный алгоритм
    int m_selectedOperation = 0;                ///< 0=шифр, 1=дешифр
    std::vector<std::string> m_logMessages;     ///< Лог операций
    bool m_operationComplete = false;           ///< Флаг завершения операции
    EncryptionResult m_lastResult = EncryptionResult::Success; ///< Последний результат
    std::string m_statusText = "Выберите файл для работы";    ///< Статус внизу
};

} // namespace sigma

#endif // SIGMA_UI_HPP

