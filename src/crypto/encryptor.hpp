/**
 * @file encryptor.hpp
 * @brief Заголовочный файл главного класса шифрования Sigma Encryptor
 * 
 * Этот файл содержит объявление класса Encryptor, который является
 * основным интерфейсом для всех криптографических операций.
 * 
 * Класс поддерживает несколько алгоритмов шифрования:
 * - AES-256-GCM (Advanced Encryption Standard в режиме Galois/Counter Mode)
 * - ChaCha20-Poly1305 (современный потоковый шифр с аутентификацией)
 * 
 * Особенности:
 * - Используется пароль пользователя для генерации ключа
 * - Применяется соль для защиты от rainbow table атак
 * - Используется PBKDF2 для надежной генерации ключа из пароля
 * - Каждый файл шифруется с уникальным IV (Initial Vector)
 * 
 * @author heimdall
 * @version 1.0
 */

#ifndef SIGMA_ENCRYPTOR_HPP
#define SIGMA_ENCRYPTOR_HPP

// инклюды
#include <string>       // Для std::string
#include <vector>       // Для std::vector
#include <memory>       // Для умных указателей
#include <cstdint>      // Для uint8_t, uint16_t и т.д.
#include <functional>   // Для std::function (callback прогресса)


// =========================================================================
// ПОТОМ ПОМЕНЯТЬ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// =================================================================================

// Определение макросов экспорта/импорта DLL (для Windows)
// Для статической линковки макрос пустой
#ifdef _WIN32
    #ifdef SIGMA_EXPORTS
        // Экспорт из DLL (когда мы компилируем библиотеку)
        #define SIGMA_API __declspec(dllexport)
    #else
        // Для статической линковки используем пустой макрос
        #define SIGMA_API
    #endif
#else
    // Для других платформ макрос пустой
    #define SIGMA_API
#endif

namespace sigma
{

// Предварительные объявления классов алгоритмов
class AESEncryptor;
class ChaCha20Encryptor;

/**
 * @enum CipherType
 * @brief Перечисление поддерживаемых типов шифрования
 * 
 * Определяет доступные алгоритмы шифрования в приложении.
 * Каждый алгоритм имеет свои преимущества:
 * - AES: стандарт, проверенный временем, аппаратное ускорение
 * - ChaCha20: современный, быстрый, устойчивый к атакам
 */
enum class CipherType
{
    AES_256_GCM,        // AES 256-bit Galois/Counter Mode
    ChaCha20_Poly1305   // ChaCha20 с аутентификацией Poly1305
};

/**
 * @enum class EncryptionResult
 * @brief Результат операции шифрования/дешифрования
 * 
 * Используется для возврата статуса операции
 */
enum class EncryptionResult
{
    Success,            // Операция выполнена успешно
    InvalidPassword,    // Неверный пароль
    FileNotFound,       // Файл не найден
    InvalidFileFormat,  // Формат файла некорректен
    EncryptionFailed,   // Ошибка при шифровании
    DecryptionFailed,   // Ошибка при дешифровании
    IOError,            // Ошибка ввода/вывода
    UnknownError        // Неизвестная ошибка
};

/**
 * @struct EncryptionOptions
 * @brief Настройки для операции шифрования/дешифрования
 * 
 * Структура содержит все параметры, необходимые для
 * шифрования или дешифрования файла.
 */
struct EncryptionOptions
{
    // Тип шифрования (AES или ChaCha20)
    CipherType cipherType = CipherType::AES_256_GCM;
    
    // Пароль пользователя (будет хеширован для получения ключа)
    std::string password;
    
    // Соль для PBKDF2 (16 байт, генерируется случайно при шифровании)
    // При дешифровании соль читается из зашифрованного файла
    std::vector<unsigned char> salt;
    
    // Инициализирующий вектор (IV/Nonce) - уникален для каждого файла
    // 12 байт для GCM, 12 байт для ChaCha20
    std::vector<unsigned char> iv;
    
    // Количество итераций PBKDF2 (больше = безопаснее, но медленнее)
    // Рекомендуется 100000+ для паролей
    int pbkdf2Iterations = 100000;
};

/**
 * @class Encryptor
 * @brief Главный класс для шифрования и дешифрования файлов
 * 
 * Этот класс инкапсулирует всю логику шифрования.
 * Он управляет алгоритмами и предоставляет
 * единый интерфейс для операций шифрования.
 * 
 * Пример использования:
 * @code
 * sigma::Encryptor enc;
 * 
 * // Шифрование файла
 * auto result = enc.encryptFile("input.txt", "output.enc", "myPassword", 
 *                                sigma::CipherType::AES_256_GCM);
 * 
 * // Дешифрование файла  
 * auto result2 = enc.decryptFile("output.enc", "decrypted.txt", "myPassword");
 * @endcode
 */
class SIGMA_API Encryptor
{
public:
    /**
     * @brief Конструктор по умолчанию
     * 
     * Инициализирует внутренние компоненты.
     */
    Encryptor();

    /**
     * @brief Деструктор
     * 
     * Освобождает ресурсы.
     */
    ~Encryptor();

    /**
     * @brief Шифрует файл
     * 
     * Читает исходный файл, шифрует его содержимое и записывает
     * результат в выходной файл. Выходной файл имеет следующую структуру:
     * 
     * [4 байта - magic bytes: "SGE1"]
     * [1 байт - версия формата: 0x01]
     * [1 байт - тип шифрования]
     * [16 байт - соль для PBKDF2]
     * [12 байт - IV/Nonce]
     * [N байт - зашифрованные данные с аутентификацией]
     * 
     * @param inputPath Путь к исходному (незашифрованному) файлу
     * @param outputPath Путь для сохранения зашифрованного файла
     * @param password Пароль для шифрования
     * @param cipherType Алгоритм шифрования
     * @return EncryptionResult Результат операции
     */
    EncryptionResult encryptFile(
        const std::string& inputPath,
        const std::string& outputPath,
        const std::string& password,
        CipherType cipherType = CipherType::AES_256_GCM
    );

    /**
     * @brief Дешифрует файл
     * 
     * Читает зашифрованный файл, дешифрует его и записывает
     * результат в выходной файл. Автоматически определяет
     * тип шифрования по заголовку файла.
     * 
     * @param inputPath Путь к зашифрованному файлу
     * @param outputPath Путь для сохранения дешифрованного файла
     * @param password Пароль для дешифрования
     * @return EncryptionResult Результат операции
     */
    EncryptionResult decryptFile(
        const std::string& inputPath,
        const std::string& outputPath,
        const std::string& password
    );

    /**
     * @brief Проверяет, является ли файл зашифрованным
     * 
     * Читает первые байты файла и проверяет наличие
     * сигнатуры "SGE1" (Sigma Encryptor v1)
     * 
     * @param filePath Путь к файлу
     * @return true если файл зашифрован нашим приложением
     * @return false если файл не зашифрован или имеет неверный формат
     */
    bool isEncrypted(const std::string& filePath) const;

    /**
     * @brief Получает тип шифрования зашифрованного файла
     * 
     * @param filePath Путь к зашифрованному файлу
     * @return CipherType тип шифрования или AES_256_GCM по умолчанию
     */
    CipherType getCipherType(const std::string& filePath) const;

    /**
     * @brief Устанавливает количество итераций PBKDF2
     * 
     * Большее количество итераций = более безопасный ключ,
     * но более медленная скорость шифрования/дешифрования.
     * 
     * @param iterations Количество итераций (минимум 10000)
     */
    void setPBKDF2Iterations(int iterations);

    // =========================================================================
    // CALLBACKS ПРОГРЕССА
    // =========================================================================
    
    /**
     * @brief Тип функции callback для прогресса
     * @param current Текущее значение (0-100)
     * @param total Общее значение
     * @param userData Пользовательские данные
     */
    using ProgressCallback = std::function<void(size_t current, size_t total, void* userData)>;
    
    /**
     * @brief Устанавливает callback для отображения прогресса
     * @param callback Функция callback
     * @param userData Пользовательские данные
     */
    void setProgressCallback(ProgressCallback callback, void* userData = nullptr);

private:
    /**
     * @brief Генерирует ключ из пароля
     * 
     * Использует PBKDF2 (Password-Based Key Derivation Function 2)
     * для генерации криптографически стойкого ключа из пароля.
     * 
     * PBKDF2 - это стандарт для генерации ключей
     * из паролей. Он использует:
     * - Соль (salt) - случайные данные для защиты от rainbow tables
     * - Итерации - для увеличения времени вычисления (защита от брутфорса)
     * - Хеш-функцию (SHA-256 в нашем случае)
     * 
     * @param password Пароль пользователя
     * @param salt Соль (должна быть уникальной для каждого файла)
     * @param iterations Количество итераций
     * @param keyLength Длина ключа в байтах (32 для AES-256, 32 для ChaCha20)
     * @return Вектор с ключом
     */
    std::vector<unsigned char> deriveKey(
        const std::string& password,
        const std::vector<unsigned char>& salt,
        int iterations,
        size_t keyLength
    ) const;

    /**
     * @brief Генерирует случайные байты
     * 
     * Использует криптографически стойкий генератор случайных чисел
     * (CSPRNG - Cryptographically Secure Pseudo-Random Number Generator)
     * 
     * @param length Количество байт для генерации
     * @return Вектор со случайными байтами
     */
    std::vector<unsigned char> generateRandomBytes(size_t length) const;

    // Уникальные указатели на реализации алгоритмов
    // Используем unique_ptr для автоматического управления памятью
    std::unique_ptr<AESEncryptor> m_aes;         // AES реализация
    std::unique_ptr<ChaCha20Encryptor> m_chacha; // ChaCha20 реализация

    // Количество итераций PBKDF2
    int m_pbkdf2Iterations = 100000;

    // Callback для прогресса
    ProgressCallback m_progressCallback = nullptr;
    void* m_progressUserData = nullptr;

    // Magic bytes для идентификации зашифрованных файлов
    // "SGE1" = Sigma G Encryptor version 1
    static constexpr const char* MAGIC_BYTES = "SGE1";
    
    // Версия формата файла
    static constexpr uint8_t FORMAT_VERSION = 0x01;
};

} // namespace sigma

#endif // SIGMA_ENCRYPTOR_HPP
