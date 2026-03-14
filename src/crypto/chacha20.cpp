/**
 * @file chacha20.cpp
 * @brief Реализация ChaCha20-Poly1305 AEAD шифрования
 * 
 * @details
 * Полная реализация класса ChaCha20Encryptor с использованием EVP API OpenSSL.
 * ChaCha20-Poly1305 - stream cipher в AEAD режиме (RFC 8439).
 * 
 * Ключевые характеристики:
 * - Ключ: 256 бит (32 байта)
 * - Nonce: 96 бит (12 байт) - уникален для каждого файла
 * - Тег: 128 бит (16 байт) - аутентификация
 * - Нет padding (потоковый шифр)
 * 
 * Формат: [зашифрованные данные 1:1] + [тег 16 байт]
 * 
 * Алгоритм EVP_chacha20_poly1305() - высокоуровневый API с автоматической
 * обработкой тега аутентификации.
 * 
 * @author heimdall
 * @version 4.0
 */

#include "chacha20.hpp"

// OpenSSL API
#include <openssl/evp.h>      // EVP_EncryptInit_ex, EVP_chacha20_poly1305()
#include <openssl/err.h>      // ERR_get_error, handleError
#include <openssl/ossl_typ.h> // EVP_CIPHER_CTX

// Стандарт C++
#include <iostream>           // Логирование
#include <stdexcept>          // throw runtime_error
#include <cstring>            // memcpy 

#pragma warning(disable: 4100)  // Неиспользуемые параметры

namespace sigma
{

constexpr size_t ChaCha20Encryptor::TAG_SIZE;

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Конструктор
 * Создаёт EVP_CIPHER_CTX - внутренний контекст OpenSSL для операций.
 * Контекст используется многократно (reset между encrypt/decrypt).
 */
ChaCha20Encryptor::ChaCha20Encryptor()
{
    setlocale(LC_ALL, "RU"); 
    
    m_context = EVP_CIPHER_CTX_new();
    if (!m_context)
    {
        std::cerr << "[ОШИБКА] Не удалось создать EVP контекст ChaCha20" << std::endl;
        handleError("EVP_CIPHER_CTX_new");
        throw std::runtime_error("ChaCha20 context creation failed");
    }
    
    std::cout << "[ИНФО] ChaCha20-Poly1305 контекст готов (EVP API)" << std::endl;
}

/**
 * @brief Деструктор
 * Освобождает EVP контекст. Умные указатели в Encryptor не нужны здесь.
 */
ChaCha20Encryptor::~ChaCha20Encryptor()
{
    if (m_context)
    {
        EVP_CIPHER_CTX_free(m_context);
        std::cout << "[ИНФО] ChaCha20 контекст освобождён" << std::endl;
    }
}

// =============================================================================
// ШИФРОВАНИЕ (encrypt)
// =============================================================================

/**
 * @brief Шифрование данных
 * 
 * Шаги EVP API:
 * 1. EVP_CIPHER_CTX_reset() - сброс состояния
 * 2. EVP_EncryptInit_ex(EVP_chacha20_poly1305()) - выбор алгоритма
 * 3. EVP_EncryptInit_ex(key, nonce) - установка ключа/nonce
 * 4. EVP_EncryptUpdate() - шифрование основного блока
 * 5. EVP_EncryptFinal_ex() - финализация (padding если есть, но ChaCha20 stream - нет)
 * 6. EVP_CTRL_AEAD_GET_TAG - получение тега Poly1305
 * 7. ciphertext = encrypted_data + tag
 * 
 * @param plaintext Исходные данные
 * @param key 32 байта
 * @param nonce 12 байт (уникален!)
 * @param[out] ciphertext Результат (размер = plaintext + 16)
 * @return Success или ошибка
 */
EncryptionResult ChaCha20Encryptor::encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    std::vector<unsigned char>& ciphertext)
{
    // Валидация входа
    if (key.size() != 32) {
        std::cerr << "[ОШИБКА] Ключ должен быть 32 байта (256 бит)" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }
    if (nonce.size() != 12) {
        std::cerr << "[ОШИБКА] Nonce должен быть 12 байт (96 бит)" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }

    // 1. Сброс контекста (важно для повторного использования)
    if (EVP_CIPHER_CTX_reset(m_context) != 1) {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::EncryptionFailed;
    }

    // 2. Инициализация алгоритмом ChaCha20-Poly1305 (AEAD)
    if (EVP_EncryptInit_ex(m_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) == 0) {
        handleError("EVP_EncryptInit_ex (chacha20_poly1305)");
        return EncryptionResult::EncryptionFailed;
    }

    // 3. Установка ключа и nonce
    if (EVP_EncryptInit_ex(m_context, nullptr, nullptr, key.data(), nonce.data()) == 0) {
        handleError("EVP_EncryptInit_ex (key/nonce)");
        return EncryptionResult::EncryptionFailed;
    }

    // Резерв под данные + тег
    ciphertext.resize(plaintext.size() + TAG_SIZE);
    int len1 = 0, len2 = 0;

    // 4. Шифрование основного объёма
    if (EVP_EncryptUpdate(m_context, ciphertext.data(), &len1, plaintext.data(), 
                          static_cast<int>(plaintext.size())) == 0) {
        handleError("EVP_EncryptUpdate");
        return EncryptionResult::EncryptionFailed;
    }

    // 5. Финализация (ChaCha20 stream - без padding)
    if (EVP_EncryptFinal_ex(m_context, ciphertext.data() + len1, &len2) == 0) {
        handleError("EVP_EncryptFinal_ex");
        return EncryptionResult::EncryptionFailed;
    }

    // 6. Получение тега Poly1305 (аутентификация)
    unsigned char tag[TAG_SIZE];
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag) == 0) {
        handleError("EVP_CTRL_AEAD_GET_TAG");
        return EncryptionResult::EncryptionFailed;
    }

    // 7. Копирование тега в конец
    std::memcpy(ciphertext.data() + len1 + len2, tag, TAG_SIZE);
    ciphertext.resize(len1 + len2 + TAG_SIZE);

    std::cout << "[ИНФО] ChaCha20-Poly1305: " << plaintext.size() 
              << " -> " << ciphertext.size() << " байт (+тег)" << std::endl;
    return EncryptionResult::Success;
}

// =============================================================================
// ДЕШИФРОВАНИЕ (decrypt)
// =============================================================================

/**
 * @brief Дешифрование с проверкой тега
 * Обратная последовательность encrypt.
 * Тег устанавливается ДО EVP_DecryptFinal_ex для проверки целостности.
 * 
 * Если тег неверный → InvalidPassword (неполный принципAEAD).
 */
EncryptionResult ChaCha20Encryptor::decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    std::vector<unsigned char>& plaintext)
{
    if (key.size() != 32 || nonce.size() != 12 || ciphertext.size() < TAG_SIZE) {
        std::cerr << "[ОШИБКА] Неверные размеры данных" << std::endl;
        return EncryptionResult::DecryptionFailed;
    }

    size_t dataSize = ciphertext.size() - TAG_SIZE;
    const unsigned char* tag = ciphertext.data() + dataSize;

    if (EVP_CIPHER_CTX_reset(m_context) != 1) {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_DecryptInit_ex(m_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) == 0) {
        handleError("EVP_DecryptInit_ex (chacha20_poly1305)");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_DecryptInit_ex(m_context, nullptr, nullptr, key.data(), nonce.data()) == 0) {
        handleError("EVP_DecryptInit_ex (key/nonce)");
        return EncryptionResult::DecryptionFailed;
    }

    // Установка тега ДО финализации (критично для AEAD!)
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_AEAD_SET_TAG, TAG_SIZE, 
                            const_cast<unsigned char*>(tag)) == 0) {
        handleError("EVP_CTRL_AEAD_SET_TAG");
        return EncryptionResult::DecryptionFailed;
    }

    plaintext.resize(dataSize);
    int len1 = 0, len2 = 0;

    if (EVP_DecryptUpdate(m_context, plaintext.data(), &len1, ciphertext.data(), 
                          static_cast<int>(dataSize)) == 0) {
        handleError("EVP_DecryptUpdate");
        return EncryptionResult::DecryptionFailed;
    }

    // Финализация с проверкой тега (0 = fail)
    if (EVP_DecryptFinal_ex(m_context, plaintext.data() + len1, &len2) <= 0) {
        std::cerr << "[ОШИБКА] ChaCha20: неверный тег (файл повреждён/неверный пароль)" << std::endl;
        plaintext.clear();
        return EncryptionResult::InvalidPassword;
    }

    plaintext.resize(len1 + len2);

    std::cout << "[ИНФО] ChaCha20-Poly1305 дешифровано: OK" << std::endl;
    return EncryptionResult::Success;
}

// =============================================================================
// ОБРАБОТКА ОШИБОК OPENSSL
// =============================================================================

/**
 * @brief Детальная обработка ошибок OpenSSL
 * Выводит код + описание из ERR_error_string_n.
 */
void ChaCha20Encryptor::handleError(const std::string& operation) const
{
    unsigned long err = ERR_get_error();
    if (!err) {
        std::cerr << "[ОШИБКА] ChaCha20 (" << operation << "): неизвестно" << std::endl;
        return;
    }

    char buf[256];
    ERR_error_string_n(err, buf, sizeof(buf));
    std::cerr << "[ОШИБКА] ChaCha20 (" << operation << "): " << buf << std::endl;

    // Все ошибки стека
    while ((err = ERR_get_error()) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "  └> " << buf << std::endl;
    }
}

} // namespace sigma

