/**
 * @file encryptor.cpp
 * @brief Оркестратор процессов шифрования и управления файлами.
 * 
 * Реализует логику работы с заголовками файлов Sigma, генерацию соли/IV
 * и преобразование пароля в криптографический ключ через PBKDF2.
 * 
 * @author heimdall
 */

#include "encryptor.hpp"
#include "aes.hpp"
#include "chacha20.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <cstring>

namespace sigma {

/**
 * @namespace detail
 * @brief Вспомогательные утилиты для форматирования вывода в консоль.
 */
namespace detail {
    static std::string formatSize(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int i = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024 && i < 3) {
            size /= 1024;
            i++;
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[i];
        return oss.str();
    }

    static void printHeader(const std::string& op, const std::filesystem::path& path) {
        std::cout << "\n[ " << op << " ]\n";
        std::cout << "Target: " << path.filename().string() << "\n";
        std::cout << "------------------------------------------\n";
    }
}

// Константы формата файла
const char* const Encryptor::MAGIC_BYTES = "SGE1"; // Sigma Guard Encrypted
const uint8_t Encryptor::FORMAT_VERSION = 0x01;

/**
 * @brief Конструктор инициализирует доступные криптографические движки.
 */
Encryptor::Encryptor()
    : m_aes(std::make_unique<AESEncryptor>())
    , m_chacha(std::make_unique<ChaCha20Encryptor>())
{
}

Encryptor::~Encryptor() = default;

/**
 * @brief Основной метод шифрования файла.
 * 
 * Процесс:
 * 1. Генерация случайной соли (16 байт) и IV (12 байт) через CSPRNG.
 * 2. Деривация ключа из пароля (PBKDF2-HMAC-SHA256).
 * 3. Вызов выбранного алгоритма.
 * 4. Запись бинарного заголовка и зашифрованных данных.
 */
EncryptionResult Encryptor::encryptFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::string& password,
    CipherType type)
{
    detail::printHeader("ENCRYPTION", inputPath);

    if (!std::filesystem::exists(inputPath)) return EncryptionResult::FileNotFound;
    
    // Чтение исходного файла в память
    std::ifstream is(inputPath, std::ios::binary);
    std::vector<uint8_t> plaintext((std::istreambuf_iterator<char>(is)), 
                                    std::istreambuf_iterator<char>());
    is.close();

    // Генерация криптографического мусора (Salt/IV)
    auto salt = generateRandomBytes(16);
    auto iv = generateRandomBytes(12);
    auto key = deriveKey(password, salt, m_pbkdf2Iterations, 32);

    std::vector<uint8_t> ciphertext;
    EncryptionResult res;

    auto start = std::chrono::high_resolution_clock::now();
    
    // Выбор стратегии шифрования
    if (type == CipherType::AES_256_GCM)
        res = m_aes->encrypt(plaintext, key, iv, ciphertext);
    else
        res = m_chacha->encrypt(plaintext, key, iv, ciphertext);

    auto end = std::chrono::high_resolution_clock::now();

    if (res != EncryptionResult::Success) return res;

    // Запись зашифрованного файла в формате Sigma
    std::ofstream os(outputPath, std::ios::binary);
    if (!os) return EncryptionResult::IOError;

    os.write(MAGIC_BYTES, 4);                                        // Магическое число
    os.write(reinterpret_cast<const char*>(&FORMAT_VERSION), 1);      // Версия формата
    uint8_t t = static_cast<uint8_t>(type);
    os.write(reinterpret_cast<const char*>(&t), 1);                  // ID алгоритма
    os.write(reinterpret_cast<const char*>(salt.data()), 16);         // Соль для PBKDF2
    os.write(reinterpret_cast<const char*>(iv.data()), 12);           // IV/Nonce
    os.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size()); // Данные + Тег
    os.close();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Status: Success\n";
    std::cout << "Algorithm: " << (type == CipherType::AES_256_GCM ? "AES-GCM" : "ChaCha20") << "\n";
    std::cout << "Time: " << ms << " ms\n";
    
    return EncryptionResult::Success;
}

/**
 * @brief Метод дешифрования. 
 * Проверяет заголовок и выполняет валидацию тега аутентификации.
 */
EncryptionResult Encryptor::decryptFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::string& password)
{
    detail::printHeader("DECRYPTION", inputPath);

    if (!std::filesystem::exists(inputPath)) return EncryptionResult::FileNotFound;

    std::ifstream is(inputPath, std::ios::binary);
    
    // Проверка Magic Bytes
    char magic[4];
    is.read(magic, 4);
    if (std::memcmp(magic, MAGIC_BYTES, 4) != 0) return EncryptionResult::InvalidFileFormat;

    // Чтение метаданных
    uint8_t version, typeByte;
    is.read(reinterpret_cast<char*>(&version), 1);
    is.read(reinterpret_cast<char*>(&typeByte), 1);
    
    if (version != FORMAT_VERSION) return EncryptionResult::InvalidFileFormat;

    std::vector<uint8_t> salt(16), iv(12);
    is.read(reinterpret_cast<char*>(salt.data()), 16);
    is.read(reinterpret_cast<char*>(iv.data()), 12);

    // Считывание остатка файла (шифротекст + тег)
    std::vector<uint8_t> ciphertext((std::istreambuf_iterator<char>(is)), 
                                     std::istreambuf_iterator<char>());
    is.close();

    // Восстановление ключа из пароля и считанной соли
    auto key = deriveKey(password, salt, m_pbkdf2Iterations, 32);

    std::vector<uint8_t> plaintext;
    EncryptionResult res;
    CipherType type = static_cast<CipherType>(typeByte);

    if (type == CipherType::AES_256_GCM)
        res = m_aes->decrypt(ciphertext, key, iv, plaintext);
    else
        res = m_chacha->decrypt(ciphertext, key, iv, plaintext);

    // Если тег не прошел проверку, OpenSSL вернет ошибку. В поле пароля будет "неверный пароль"(что думаю, очевидно)
    if (res != EncryptionResult::Success) return EncryptionResult::InvalidPassword;

    std::ofstream os(outputPath, std::ios::binary);
    os.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
    os.close();

    std::cout << "Status: Success\n";
    return EncryptionResult::Success;
}

/**
 * @brief Простая проверка заголовка файла на принадлежность к Sigma.
 */
bool Encryptor::isEncrypted(const std::filesystem::path& filePath) const {
    if (!std::filesystem::exists(filePath)) return false;
    std::ifstream is(filePath, std::ios::binary);
    char magic[4];
    is.read(magic, 4);
    return (is.gcount() == 4 && std::memcmp(magic, MAGIC_BYTES, 4) == 0);
}

void Encryptor::setPBKDF2Iterations(int iterations) {
    m_pbkdf2Iterations = iterations;
}

/**
 * @brief Реализация PBKDF2 (Password-Based Key Derivation Function 2).
 * 
 * Используется для защиты от brute-force атак на пароль. 
 * HMAC-SHA256 выполняется многократно, чтобы замедлить подбор пароля злоумышленником.
 */
std::vector<uint8_t> Encryptor::deriveKey(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    int iterations,
    size_t keyLength) const
{
    std::vector<uint8_t> key(keyLength);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                          salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          static_cast<int>(keyLength), key.data()) != 1) 
    {
        throw std::runtime_error("PBKDF2 failed");
    }
    return key;
}

/**
 * @brief Генерация данных через криптографически стойкий генератор псевдослучайных чисел (CSPRNG).
 */
std::vector<uint8_t> Encryptor::generateRandomBytes(size_t length) const
{
    std::vector<uint8_t> buf(length);
    if (RAND_bytes(buf.data(), static_cast<int>(length)) != 1)
        throw std::runtime_error("CSPRNG failure");
    return buf;
}

} // namespace sigma