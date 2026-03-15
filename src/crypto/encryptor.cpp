/**
 * @file encryptor.cpp
 * @brief Реализация главного класса шифрования
 * 
 * @details
 * Этот файл содержит реализацию методов класса Encryptor.
 * Основная задача - управление процессом шифрования/дешифрования
 * и координация работы различных алгоритмов.
 * 
 * Формат зашифрованного файла:
 * +------------------+------------------+------------------------+
 * | Заголовок (34 б) | IV (12 байт)     | Зашифрованные данные   |
 * +------------------+------------------+------------------------+
 * | Magic: "SGE1"    | Nonce для        | AEAD (Authenticated   |
 * | Version: 0x01   | шифра            | Encryption with       |
 * | Cipher: 1 байт  | (уникален для    | Associated Data)      |
 * | Salt: 16 байт   | каждого файла)   |                       |
 * +------------------+------------------+------------------------+
 * 
 * @author heimdall
 * @version 1.0
 */

#include "encryptor.hpp"

#include <openssl/evp.h>      // High-level EVP интерфейс (шифрование)
#include <openssl/kdf.h>      // Key Derivation Functions (PBKDF2)
#include <openssl/rand.h>     // Криптографический генератор случайных чисел
#include <openssl/err.h>      // Обработка ошибок OpenSSL

#include <iostream>           // Для отладочного вывода
#include <fstream>            // Работа с файлами
#include <sstream>            // Строковые потоки
#include <iomanip>            // std::setprecision
#include <chrono>             // Для измерения времени
#include <ctime>              // Для локального времени

#ifdef _WIN32
    #include <windows.h>
#endif

#include "aes.hpp"
#include "chacha20.hpp"

// Убираем предупреждение о неиспользуемых параметрах
#pragma warning(disable: 4100)

namespace sigma
{

// =============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ЛОГИРОВАНИЯ
// =============================================================================

/**
 * @brief Получает текущее время в виде строки
 */
static std::string getCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

/**
 * @brief Конвертирует размер файла в читабельный вид
 */
static std::string formatFileSize(size_t bytes)
{
    const char* units[] = {"Б", "КБ", "МБ", "ГБ", "ТБ"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4)
    {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

/**
 * @brief Выводит подробный заголовок лога операции
 */
static void logOperationHeader(const char* operation, const std::string& filePath, size_t fileSize)
{
    std::cout << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "  [НАЧАЛО] " << operation << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "  [ВРЕМЯ]      " << getCurrentTimeString() << std::endl;
    std::cout << "  [ФАЙЛ]       " << filePath << std::endl;
    std::cout << "  [РАЗМЕР]     " << formatFileSize(fileSize) << " (" << fileSize << " байт)" << std::endl;
    std::cout << "───────────────────────────────────────────────────────────────" << std::endl;
}

/**
 * @brief Выводит подробный лог завершения операции
 */
static void logOperationEnd(const char* operation, size_t originalSize, size_t resultSize, 
                            double durationMs, bool success, const std::string& outputPath = "")
{
    // Вычисляем скорость в МБ/с
    double speedMBps = 0.0;
    if (durationMs > 0)
    {
        speedMBps = (originalSize / (1024.0 * 1024.0)) / (durationMs / 1000.0);
    }
    
    std::cout << "───────────────────────────────────────────────────────────────" << std::endl;
    std::cout << "  [ЗАВЕРШЕНО] " << operation << std::endl;
    std::cout << "  [ВРЕМЯ]     " << getCurrentTimeString() << std::endl;
    std::cout << "  [ДЛИТЕЛЬНОСТЬ] " << std::fixed << std::setprecision(2) << durationMs << " мс" << std::endl;
    
    if (speedMBps > 0)
    {
        std::cout << "  [СКОРОСТЬ]  " << std::setprecision(2) << speedMBps << " МБ/с" << std::endl;
    }
    
    if (originalSize > 0)
    {
        std::cout << "  [РАЗМЕР]    " << formatFileSize(originalSize);
        if (resultSize != originalSize)
        {
            std::cout << " -> " << formatFileSize(resultSize);
        }
        std::cout << std::endl;
    }
    
    if (!outputPath.empty())
    {
        std::cout << "  [ВЫВОД]     " << outputPath << std::endl;
    }
    
    std::cout << "  [СТАТУС]    " << (success ? "УСПЕХ ✓" : "ОШИБКА ✗") << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
}

// =============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ UNICODE НА WINDOWS
// =============================================================================

#ifdef _WIN32
/**
 * @brief Конвертирует UTF-8 строку в wide string (UTF-16)
 */
static std::wstring utf8ToWstring(const std::string& str)
{
    if (str.empty())
        return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), 
                                          nullptr, 0);
    if (size_needed == 0)
        return std::wstring();
    
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), 
                        &result[0], size_needed);
    return result;
}

/**
 * @brief Читает файл с Unicode путём (Windows)
 */
static bool readFileUnicode(const std::string& path, std::vector<unsigned char>& data)
{
    std::wstring wPath = utf8ToWstring(path);
    
    FILE* file = _wfopen(wPath.c_str(), L"rb");
    if (!file)
        return false;
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size < 0)
    {
        fclose(file);
        return false;
    }
    
    // Поддерживаем пустые файлы (размер 0)
    if (size == 0)
    {
        data.resize(0);
        fclose(file);
        return true;
    }
    
    data.resize(size);
    size_t read = fread(data.data(), 1, size, file);
    fclose(file);
    
    return read == static_cast<size_t>(size);
}

/**
 * @brief Записывает файл с Unicode путём (Windows)
 */
static bool writeFileUnicode(const std::string& path, const std::vector<unsigned char>& data)
{
    std::wstring wPath = utf8ToWstring(path);
    
    FILE* file = _wfopen(wPath.c_str(), L"wb");
    if (!file)
        return false;
    
    size_t written = fwrite(data.data(), 1, data.size(), file);
    fclose(file);
    
    return written == data.size();
}
#else
// Для Linux используем стандартные потоки
static bool readFileUnicode(const std::string& path, std::vector<unsigned char>& data)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    
    return true;
}

static bool writeFileUnicode(const std::string& path, const std::vector<unsigned char>& data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    return true;
}
#endif

// =============================================================================
// ОПРЕДЕЛЕНИЕ СТАТИЧЕСКИХ ЧЛЕНОВ КЛАССА
// =============================================================================
// Необходимо для корректной работы с DLL экспортом/импортом
// Определяем статические константы вне класса
constexpr const char* Encryptor::MAGIC_BYTES;
constexpr uint8_t Encryptor::FORMAT_VERSION;

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Конструктор по умолчанию
 * 
 * И внутренние компонентынициализирует:
 * - Создаёт экземпляры AES и ChaCha20 шифраторов
 * - Устанавливает параметры по умолчанию
 * - Инициализирует callback прогресса
 */
Encryptor::Encryptor()
    : m_aes(std::make_unique<AESEncryptor>())
    , m_chacha(std::make_unique<ChaCha20Encryptor>())
    , m_progressCallback(nullptr)
    , m_progressUserData(nullptr)
{
    std::cout << "[ИНФО] Инициализация шифратора Sigma Encryptor" << std::endl;
    std::cout << "[ИНФО] Доступные алгоритмы: AES-256-GCM, ChaCha20-Poly1305" << std::endl;
}

/**
 * @brief Деструктор
 * 
 * Освобождает ресурсы. Умные указатели автоматически
 * удалят объекты AES и ChaCha20.
 */
Encryptor::~Encryptor()
{
    std::cout << "[ИНФО] Шифратор завершил работу" << std::endl;
}

// =============================================================================
// CALLBACK ПРОГРЕССА
// =============================================================================

void Encryptor::setProgressCallback(ProgressCallback callback, void* userData)
{
    m_progressCallback = callback;
    m_progressUserData = userData;
}

// =============================================================================
// ОСНОВНЫЕ МЕТОДЫ ШИФРОВАНИЯ
// =============================================================================

/**
 * @brief Шифрует файл
 * 
 * @details
 * Процесс шифрования:
 * 1. Проверяем существование входного файла
 * 2. Читаем содержимое файла в память
 * 3. Генерируем случайную соль (16 байт)
 * 4. Генерируем случайный IV (12 байт)
 * 5. Выводим ключ из пароля с помощью PBKDF2
 * 6. Шифруем данные с выбранным алгоритмом
 * 7. Записываем заголовок + IV + зашифрованные данные
 * 
 * @param inputPath Путь к исходному файлу
 * @param outputPath Путь для сохранения зашифрованного файла  
 * @param password Пароль пользователя
 * @param cipherType Алгоритм шифрования
 * @return EncryptionResult Результат операции
 */
EncryptionResult Encryptor::encryptFile(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::string& password,
    CipherType cipherType)
{
    setlocale(LC_ALL, ".UTF8");
    
    // Замеряем общее время операции
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    
    // Логируем начало операции
    logOperationHeader("ШИФРОВАНИЕ", inputPath, 0);
    
    const char* cipherName = (cipherType == CipherType::AES_256_GCM) ? "AES-256-GCM" : "ChaCha20-Poly1305";
    std::cout << "  [АЛГОРИТМ] " << cipherName << std::endl;
    std::cout << "  [ВЫХОДНОЙ ФАЙЛ] " << outputPath << std::endl;
    std::cout << "  [PBKDF2] " << m_pbkdf2Iterations << " итераций" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 1: Читаем исходный файл (с поддержкой Unicode)
    // -------------------------------------------------------------------------
    auto readStartTime = std::chrono::high_resolution_clock::now();
    
    std::vector<unsigned char> plaintext;
    
    if (!readFileUnicode(inputPath, plaintext))
    {
        std::cerr << "[ОШИБКА] Не удалось открыть файл: " << inputPath << std::endl;
        logOperationEnd("ШИФРОВАНИЕ", 0, 0, 0, false);
        return EncryptionResult::FileNotFound;
    }
    
    auto readEndTime = std::chrono::high_resolution_clock::now();
    double readDurationMs = std::chrono::duration<double, std::milli>(readEndTime - readStartTime).count();
    
    size_t fileSize = plaintext.size();

    std::cout << "  [ЧТЕНИЕ]   Файл прочитан за " << std::fixed << std::setprecision(2) << readDurationMs << " мс" << std::endl;
    std::cout << "  [РАЗМЕР]   " << formatFileSize(fileSize) << " (" << fileSize << " байт)" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 2: Генерируем криптографические параметры
    // -------------------------------------------------------------------------
    
    // Генерируем соль (16 байт = 128 бит)
    // Соль нужна для PBKDF2, чтобы одинаковые пароли давали разные ключи
    auto keyGenStartTime = std::chrono::high_resolution_clock::now();
    std::vector<unsigned char> salt = generateRandomBytes(16);
    
    // Генерируем IV/Nonce (12 байт = 96 бит)
    // IV должен быть уникальным для каждого шифрования
    std::vector<unsigned char> iv = generateRandomBytes(12);

    // -------------------------------------------------------------------------
    // Шаг 3: Выводим ключ из пароля
    // -------------------------------------------------------------------------
    
    // Для AES-256 нужен 32-байтный ключ
    // Для ChaCha20 тоже нужен 32-байтный ключ
    size_t keyLength = 32;
    
    std::vector<unsigned char> key = deriveKey(
        password,
        salt,
        m_pbkdf2Iterations,
        keyLength
    );

    auto keyGenEndTime = std::chrono::high_resolution_clock::now();
    double keyGenDurationMs = std::chrono::duration<double, std::milli>(keyGenEndTime - keyGenStartTime).count();
    
    std::cout << "  [КЛЮЧ]    Сгенерирован за " << std::fixed << std::setprecision(2) << keyGenDurationMs << " мс (PBKDF2, " << m_pbkdf2Iterations << " итераций)" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 4: Шифруем данные
    // -------------------------------------------------------------------------
    auto encryptStartTime = std::chrono::high_resolution_clock::now();
    std::vector<unsigned char> ciphertext;

    EncryptionResult result;
    
    if (cipherType == CipherType::AES_256_GCM)
    {
        result = m_aes->encrypt(plaintext, key, iv, ciphertext);
    }
    else
    {
        result = m_chacha->encrypt(plaintext, key, iv, ciphertext);
    }

    auto encryptEndTime = std::chrono::high_resolution_clock::now();
    double encryptDurationMs = std::chrono::duration<double, std::milli>(encryptEndTime - encryptStartTime).count();

    if (result != EncryptionResult::Success)
    {
        std::cerr << "[ОШИБКА] Ошибка шифрования (код: " << static_cast<int>(result) << ")" << std::endl;
        logOperationEnd("ШИФРОВАНИЕ", fileSize, 0, 0, false);
        return result;
    }

    std::cout << "  [ШИФРОВАНИЕ] Выполнено за " << std::fixed << std::setprecision(2) << encryptDurationMs << " мс" << std::endl;
    std::cout << "  [РАЗМЕР]   Зашифровано: " << formatFileSize(ciphertext.size()) << " (" << ciphertext.size() << " байт)" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 5: Записываем зашифрованный файл (с поддержкой Unicode)
    // -------------------------------------------------------------------------
    
    // Проверяем размер файла (ограничение для простоты - 2 ГБ)
    if (ciphertext.size() > 2ULL * 1024 * 1024 * 1024)
    {
        std::cerr << "[ОШИБКА] Файл слишком большой (максимум 2 ГБ)" << std::endl;
        logOperationEnd("ШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::IOError;
    }
    
    // Формируем данные для записи (заголовок + шифротекст)
    std::vector<unsigned char> fileData;
    fileData.reserve(34 + ciphertext.size());
    
    // Magic bytes (4 байта)
    fileData.insert(fileData.end(), MAGIC_BYTES, MAGIC_BYTES + 4);
    
    // Версия формата (1 байт)
    fileData.push_back(FORMAT_VERSION);
    
    // Тип шифрования (1 байт)
    fileData.push_back(static_cast<uint8_t>(cipherType));
    
    // Соль (16 байт)
    fileData.insert(fileData.end(), salt.begin(), salt.end());
    
    // IV (12 байт)
    fileData.insert(fileData.end(), iv.begin(), iv.end());
    
    // Зашифрованные данные
    fileData.insert(fileData.end(), ciphertext.begin(), ciphertext.end());
    
    auto writeStartTime = std::chrono::high_resolution_clock::now();
    
    if (!writeFileUnicode(outputPath, fileData))
    {
        std::cerr << "[ОШИБКА] Не удалось создать файл: " << outputPath << std::endl;
        logOperationEnd("ШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::IOError;
    }
    
    auto writeEndTime = std::chrono::high_resolution_clock::now();
    double writeDurationMs = std::chrono::duration<double, std::milli>(writeEndTime - writeStartTime).count();
    
    auto totalEndTime = std::chrono::high_resolution_clock::now();
    double totalDurationMs = std::chrono::duration<double, std::milli>(totalEndTime - totalStartTime).count();

    std::cout << "  [ЗАПИСЬ]  Файл записан за " << std::fixed << std::setprecision(2) << writeDurationMs << " мс" << std::endl;
    std::cout << "───────────────────────────────────────────────────────────────" << std::endl;
    std::cout << "  [ИТОГО]   Время операции: " << std::fixed << std::setprecision(2) << totalDurationMs << " мс" << std::endl;
    
    // Вычисляем скорость
    double speedMBps = (fileSize / (1024.0 * 1024.0)) / (totalDurationMs / 1000.0);
    std::cout << "  [СКОРОСТЬ] " << std::fixed << std::setprecision(2) << speedMBps << " МБ/с" << std::endl;
    
    logOperationEnd("ШИФРОВАНИЕ", fileSize, fileData.size(), totalDurationMs, true, outputPath);

    return EncryptionResult::Success;
}

/**
 * @brief Дешифрует файл
 * 
 * @details
 * Процесс дешифрования:
 * 1. Читаем заголовок файла (проверяем magic bytes)
 * 2. Определяем тип шифрования
 * 3. Читаем соль и IV
 * 4. Выводим ключ из пароля
 * 5. Дешифруем данные
 * 6. Записываем результат
 * 
 * @param inputPath Путь к зашифрованному файлу
 * @param outputPath Путь для сохранения дешифрованного файла
 * @param password Пароль пользователя
 * @return EncryptionResult Результат операции
 */
EncryptionResult Encryptor::decryptFile(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::string& password)
{
    setlocale(LC_ALL, ".UTF8");
    
    // Замеряем общее время операции
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    
    // Логируем начало операции
    logOperationHeader("ДЕШИФРОВАНИЕ", inputPath, 0);

    // -------------------------------------------------------------------------
    // Шаг 1: Читаем зашифрованный файл (с поддержкой Unicode)
    // -------------------------------------------------------------------------
    auto readStartTime = std::chrono::high_resolution_clock::now();
    
    std::vector<unsigned char> fileData;
    
    if (!readFileUnicode(inputPath, fileData))
    {
        std::cerr << "[ОШИБКА] Не удалось открыть файл: " << inputPath << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", 0, 0, 0, false);
        return EncryptionResult::FileNotFound;
    }
    
    auto readEndTime = std::chrono::high_resolution_clock::now();
    double readDurationMs = std::chrono::duration<double, std::milli>(readEndTime - readStartTime).count();
    
    size_t fileSize = fileData.size();

    std::cout << "  [ЧТЕНИЕ]   Файл прочитан за " << std::fixed << std::setprecision(2) << readDurationMs << " мс" << std::endl;
    std::cout << "  [РАЗМЕР]   " << formatFileSize(fileSize) << " (" << fileSize << " байт)" << std::endl;

    // Проверяем минимальный размер (34 байта = заголовок)
    if (fileSize < 34)
    {
        std::cerr << "[ОШИБКА] Файл слишком маленький" << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::InvalidFileFormat;
    }

    // Читаем и проверяем magic bytes
    if (std::memcmp(fileData.data(), MAGIC_BYTES, 4) != 0)
    {
        std::cerr << "[ОШИБКА] Неверный формат файла (не зашифрован Sigma Encryptor?)" << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::InvalidFileFormat;
    }

    // Читаем версию
    uint8_t version = fileData[4];
    if (version != FORMAT_VERSION)
    {
        std::cerr << "[ОШИБКА] Неподдерживаемая версия формата: " << static_cast<int>(version) << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::InvalidFileFormat;
    }

    // Читаем тип шифрования
    uint8_t cipherByte = fileData[5];
    CipherType cipherType = static_cast<CipherType>(cipherByte);

    const char* cipherName = (cipherType == CipherType::AES_256_GCM) ? "AES-256-GCM" : "ChaCha20-Poly1305";
    std::cout << "  [АЛГОРИТМ] " << cipherName << std::endl;

    // Читаем соль (16 байт) - offset 6
    std::vector<unsigned char> salt(fileData.begin() + 6, fileData.begin() + 22);
    
    // Читаем IV (12 байт) - offset 22
    std::vector<unsigned char> iv(fileData.begin() + 22, fileData.begin() + 34);
    
    // Читаем зашифрованные данные - offset 34
    size_t headerSize = 34;
    std::vector<unsigned char> ciphertext(fileData.begin() + headerSize, fileData.end());

    std::cout << "  [РАЗМЕР]   Зашифрованные данные: " << formatFileSize(ciphertext.size()) << " (" << ciphertext.size() << " байт)" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 2: Выводим ключ из пароля
    // -------------------------------------------------------------------------
    auto keyGenStartTime = std::chrono::high_resolution_clock::now();
    
    std::vector<unsigned char> key = deriveKey(
        password,
        salt,
        m_pbkdf2Iterations,
        32
    );

    auto keyGenEndTime = std::chrono::high_resolution_clock::now();
    double keyGenDurationMs = std::chrono::duration<double, std::milli>(keyGenEndTime - keyGenStartTime).count();

    std::cout << "  [КЛЮЧ]    Сгенерирован за " << std::fixed << std::setprecision(2) << keyGenDurationMs << " мс" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 3: Дешифруем данные
    // -------------------------------------------------------------------------
    auto decryptStartTime = std::chrono::high_resolution_clock::now();
    std::vector<unsigned char> plaintext;

    EncryptionResult result;
    
    if (cipherType == CipherType::AES_256_GCM)
    {
        result = m_aes->decrypt(ciphertext, key, iv, plaintext);
    }
    else
    {
        result = m_chacha->decrypt(ciphertext, key, iv, plaintext);
    }

    auto decryptEndTime = std::chrono::high_resolution_clock::now();
    double decryptDurationMs = std::chrono::duration<double, std::milli>(decryptEndTime - decryptStartTime).count();

    if (result != EncryptionResult::Success)
    {
        std::cerr << "[ОШИБКА] Ошибка дешифрования (неверный пароль?)" << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", fileSize, 0, 0, false);
        return EncryptionResult::InvalidPassword;
    }

    std::cout << "  [ДЕШИФРОВАНИЕ] Выполнено за " << std::fixed << std::setprecision(2) << decryptDurationMs << " мс" << std::endl;
    std::cout << "  [РАЗМЕР]   Дешифровано: " << formatFileSize(plaintext.size()) << " (" << plaintext.size() << " байт)" << std::endl;

    // -------------------------------------------------------------------------
    // Шаг 4: Записываем дешифрованный файл (с поддержкой Unicode)
    // -------------------------------------------------------------------------
    auto writeStartTime = std::chrono::high_resolution_clock::now();
    
    if (!writeFileUnicode(outputPath, plaintext))
    {
        std::cerr << "[ОШИБКА] Не удалось создать файл: " << outputPath << std::endl;
        logOperationEnd("ДЕШИФРОВАНИЕ", plaintext.size(), 0, 0, false);
        return EncryptionResult::IOError;
    }
    
    auto writeEndTime = std::chrono::high_resolution_clock::now();
    double writeDurationMs = std::chrono::duration<double, std::milli>(writeEndTime - writeStartTime).count();
    
    auto totalEndTime = std::chrono::high_resolution_clock::now();
    double totalDurationMs = std::chrono::duration<double, std::milli>(totalEndTime - totalStartTime).count();

    std::cout << "  [ЗАПИСЬ]  Файл записан за " << std::fixed << std::setprecision(2) << writeDurationMs << " мс" << std::endl;
    std::cout << "───────────────────────────────────────────────────────────────" << std::endl;
    std::cout << "  [ИТОГО]   Время операции: " << std::fixed << std::setprecision(2) << totalDurationMs << " мс" << std::endl;
    
    // Вычисляем скорость
    double speedMBps = (ciphertext.size() / (1024.0 * 1024.0)) / (totalDurationMs / 1000.0);
    std::cout << "  [СКОРОСТЬ] " << std::fixed << std::setprecision(2) << speedMBps << " МБ/с" << std::endl;
    std::cout << "  [ВЫВОД]   " << outputPath << std::endl;
    
    logOperationEnd("ДЕШИФРОВАНИЕ", plaintext.size(), plaintext.size(), totalDurationMs, true, outputPath);

    return EncryptionResult::Success;
}

// =============================================================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// =============================================================================

/**
 * @brief Проверяет, является ли файл зашифрованным
 */
bool Encryptor::isEncrypted(const std::string& filePath) const
{
    std::vector<unsigned char> data;
    if (!readFileUnicode(filePath, data))
    {
        return false;
    }

    if (data.size() < 4)
    {
        return false;
    }

    return std::memcmp(data.data(), MAGIC_BYTES, 4) == 0;
}

/**
 * @brief Получает тип шифрования файла
 */
CipherType Encryptor::getCipherType(const std::string& filePath) const
{
    std::vector<unsigned char> data;
    if (!readFileUnicode(filePath, data))
    {
        return CipherType::AES_256_GCM; // По умолчанию
    }

    if (data.size() < 6)
    {
        return CipherType::AES_256_GCM;
    }

    // Читаем тип шифрования (offset 5)
    uint8_t cipherByte = data[5];
    return static_cast<CipherType>(cipherByte);
}

/**
 * @brief Устанавливает количество итераций PBKDF2
 */
void Encryptor::setPBKDF2Iterations(int iterations)
{
    if (iterations < 10000)
    {
        std::cerr << "[ПРЕДУПРЕЖДЕНИЕ] Минимальное рекомендуемое значение - 10000" << std::endl;
        iterations = 10000;
    }
    m_pbkdf2Iterations = iterations;
    std::cout << "[ИНФО] PBKDF2 итерации: " << m_pbkdf2Iterations << std::endl;
}

// =============================================================================
// КРИПТОГРАФИЧЕСКИЕ ФУНКЦИИ
// =============================================================================

/**
 * @brief Генерирует ключ из пароля с помощью PBKDF2
 * 
 * @details
 * PBKDF2 (Password-Based Key Derivation Function 2) - это стандарт
 * для генерации криптографических ключей из паролей.
 * 
 * Формула:
 * DK = PBKDF2(PRF, Password, Salt, c, dkLen)
 * 
 * Где:
 * - PRF - псевдослучайная функция (HMAC-SHA256 в нашем случае)
 * - Password - пароль пользователя
 * - Salt - случайная соль (защита от rainbow tables)
 * - c - количество итераций (увеличивает сложность атаки)
 * - dkLen - длина желаемого ключа
 * 
 * @param password Пароль пользователя
 * @param salt Соль (должна быть уникальной)
 * @param iterations Количество итераций
 * @param keyLength Длина ключа в байтах
 * @return Вектор с ключом
 */
std::vector<unsigned char> Encryptor::deriveKey(
    const std::string& password,
    const std::vector<unsigned char>& salt,
    int iterations,
    size_t keyLength) const
{
    std::vector<unsigned char> key(keyLength);

    // Используем PKCS5_PBKDF2_HMAC из OpenSSL
    // Это стандартная реализация PBKDF2
    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(),              // Пароль
        static_cast<int>(password.length()), // Длина пароля
        salt.data(),                   // Соль
        static_cast<int>(salt.size()), // Размер соли
        iterations,                   // Количество итераций
        EVP_sha256(),                 // Хеш-функция (SHA-256)
        static_cast<int>(keyLength),  // Длина ключа
        key.data()                    // Куда записать ключ
    );

    if (result != 1)
    {
        std::cerr << "[ОШИБКА] Не удалось сгенерировать ключ" << std::endl;
        throw std::runtime_error("Key derivation failed");
    }

    return key;
}

/**
 * @brief Генерирует криптографически стойкие случайные байты
 * 
 * @details
 * Использует RAND_bytes из OpenSSL, который internally использует
 * операционную систему для получения энтропии.
 * 
 * На Windows используется CryptGenRandom или аналогичный API.
 * На Linux - /dev/urandom
 * 
 * Это CSPRNG (Cryptographically Secure Pseudo-Random Number Generator),
 * который подходит для криптографических целей.
 * 
 * @param length Количество байт
 * @return Вектор со случайными байтами (использует move semantics)
 */
std::vector<unsigned char> Encryptor::generateRandomBytes(size_t length) const
{
    std::vector<unsigned char> buffer(length);

    // RAND_bytes - основная функция для генерации случайных чисел
    // Возвращает 1 при успехе, 0 при ошибке
    int result = RAND_bytes(buffer.data(), static_cast<int>(length));
    
    if (result != 1)
    {
        std::cerr << "[ОШИБКА] Не удалось сгенерировать случайные байты" << std::endl;
        
        // Получаем код ошибки OpenSSL
        unsigned long error = ERR_get_error();
        char errorBuf[256];
        ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
        std::cerr << "[ОШИБКА] OpenSSL: " << errorBuf << std::endl;
        
        throw std::runtime_error("Random number generation failed");
    }

    return buffer; // NRVO - именованный return value optimization
}

} // namespace sigma
