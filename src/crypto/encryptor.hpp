/**
 * @file encryptor.hpp
 * @brief Публичный интерфейс криптографического ядра Sigma.
 * 
 * Данный заголовочный файл определяет основной класс Encryptor, который
 * объединяет AES и ChaCha20, а также управляет метаданными файлов.
 * 
 * @author heimdall
 */

#define SIGMA_STATIC
#ifndef SIGMA_ENCRYPTOR_HPP
#define SIGMA_ENCRYPTOR_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Макросы управления видимостью API.
 * 
 * Позволяют собирать проект как в виде статической библиотеки, так и в виде
 * динамической (DLL/SO) с правильным экспортом символов.
 */
#if defined(SIGMA_STATIC)
    #define SIGMA_API
#elif defined(_WIN32)
    #ifdef SIGMA_EXPORTS
        #define SIGMA_API __declspec(dllexport)
    #else
        #define SIGMA_API __declspec(dllimport)
    #endif
#else
    #define SIGMA_API __attribute__((visibility("default")))
#endif

namespace sigma
{

// Упреждающие объявления для сокрытия реализации (Pimpl-like approach)
class AESEncryptor;
class ChaCha20Encryptor;

/**
 * @enum CipherType
 * @brief Поддерживаемые алгоритмы шифрования.
 */
enum class CipherType : uint8_t {
    AES_256_GCM,        ///< Стандарт AES в режиме GCM (аппаратное ускорение).
    ChaCha20_Poly1305   ///< Современный потоковый шифр (высокая скорость на CPU).
};

/**
 * @enum EncryptionResult
 * @brief Коды возврата для обработки ошибок.
 */
enum class EncryptionResult : uint8_t {
    Success = 0,        ///< Операция завершена успешно.
    InvalidPassword,    ///< Неверный пароль или данные повреждены.
    FileNotFound,       ///< Указанный файл не найден.
    InvalidFileFormat,  ///< Файл не является архивом Sigma или версия не поддерживается.
    EncryptionFailed,   ///< Внутренняя ошибка при зашифровке.
    DecryptionFailed,   ///< Внутренняя ошибка при расшифровке.
    IOError,            ///< Ошибка доступа к диску (чтение/запись).
    UnknownError        ///< Непредвиденная системная ошибка.
};

/**
 * @class Encryptor
 * @brief Главный высокоуровневый класс для управления шифрованием файлов.
 * 
 * Обеспечивает безопасное преобразование пароля в ключ через PBKDF2 и
 * управляет жизненным циклом криптографических движков.
 */
class SIGMA_API Encryptor
{
public:
    /**
     * @brief Тип обратного вызова для отслеживания прогресса (полезно для GUI).
     */
    using ProgressCallback = std::function<void(uint64_t current, uint64_t total, void* userData)>;

    Encryptor();
    ~Encryptor();

    // Запрет копирования: каждый экземпляр владеет уникальными ресурсами OpenSSL
    Encryptor(const Encryptor&) = delete;
    Encryptor& operator=(const Encryptor&) = delete;

    /**
     * @brief Зашифровывает файл и сохраняет его в формате Sigma.
     * 
     * @param inputPath Путь к исходному файлу.
     * @param outputPath Путь к выходному .sigma файлу.
     * @param password Пароль пользователя.
     * @param type Выбранный алгоритм (по умолчанию AES-GCM).
     */
    [[nodiscard]] EncryptionResult encryptFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::string& password,
        CipherType type = CipherType::AES_256_GCM
    );

    /**
     * @brief Расшифровывает файл, проверяя его целостность.
     * 
     * @param inputPath Путь к зашифрованному файлу.
     * @param outputPath Путь для сохранения результата.
     * @param password Пароль пользователя.
     */
    [[nodiscard]] EncryptionResult decryptFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::string& password
    );

    /**
     * @brief Проверяет, является ли файл зашифрованным Sigma архивом.
     */
    [[nodiscard]] bool isEncrypted(const std::filesystem::path& filePath) const;

    /**
     * @brief Извлекает тип алгоритма из заголовка файла.
     */
    [[nodiscard]] CipherType getCipherType(const std::filesystem::path& filePath) const;
    
    /**
     * @brief Настройка сложности PBKDF2.
     * @param iterations Количество итераций (рекомендуется от 100,000).
     */
    void setPBKDF2Iterations(int iterations);

    /**
     * @brief Установка функции обратного вызова для мониторинга прогресса.
     */
    void setProgressCallback(ProgressCallback callback, void* userData = nullptr);

private:
    /// Деривация ключа из пароля и соли (PBKDF2-HMAC-SHA256).
    std::vector<uint8_t> deriveKey(const std::string& password, const std::vector<uint8_t>& salt, int iterations, size_t keyLength) const;
    
    /// Генерация криптографически стойких случайных байтов (CSPRNG).
    std::vector<uint8_t> generateRandomBytes(size_t length) const;

    std::unique_ptr<AESEncryptor> m_aes;
    std::unique_ptr<ChaCha20Encryptor> m_chacha;

    int m_pbkdf2Iterations = 100'000; ///< Значение по умолчанию для защиты от брутфорса.
    ProgressCallback m_progressCallback = nullptr;
    void* m_progressUserData = nullptr;

    static const char* const MAGIC_BYTES;  ///< "SGE1" - идентификатор формата.
    static const uint8_t FORMAT_VERSION;   ///< Текущая версия структуры данных.
};

} // namespace sigma

#endif // SIGMA_ENCRYPTOR_HPP