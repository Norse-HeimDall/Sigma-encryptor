/**
 * @file chacha20.cpp
 * @brief Реализация ChaCha20-Poly1305 шифрования
 * 
 * @details
 * ChaCha20-Poly1305 - это AEAD (Authenticated Encryption with Associated Data)
 * алгоритм, стандартизированный в RFC 8439.
 * 
 * Использует EVP API OpenSSL для работы с ChaCha20-Poly1305.
 * 
 * @author Sigma Team
 * @version 1.0
 */

#include "chacha20.hpp"

#include <openssl/evp.h>
#include <openssl/err.h>

#include <iostream>
#include <stdexcept>
#include <cstring>  // для memcpy

#pragma warning(disable: 4100)

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

ChaCha20Encryptor::ChaCha20Encryptor()
{
    setlocale(LC_ALL,"RU");
    m_context = EVP_CIPHER_CTX_new();
    
    if (m_context == nullptr)
    {
        std::cerr << "[ОШИБКА] Не удалось создать контекст ChaCha20" << std::endl;
        handleError("EVP_CIPHER_CTX_new");
        throw std::runtime_error("Failed to create ChaCha20 context");
    }
    
    std::cout << "[ИНФО] ChaCha20-Poly1305 контекст создан" << std::endl;
}

ChaCha20Encryptor::~ChaCha20Encryptor()
{
    if (m_context != nullptr)
    {
        EVP_CIPHER_CTX_free(m_context);
        m_context = nullptr;
        std::cout << "[ИНФО] ChaCha20 контекст освобождён" << std::endl;
    }
}

// =============================================================================
// ШИФРОВАНИЕ
// =============================================================================

EncryptionResult ChaCha20Encryptor::encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    std::vector<unsigned char>& ciphertext)
{
    // Проверяем входные данные
    if (key.size() != 32)
    {
        std::cerr << "[ОШИБКА] Неверная длина ключа (ожидалось 32 байта)" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }

    if (nonce.size() != 12)
    {
        std::cerr << "[ОШИБКА] Неверная длина nonce (ожидалось 12 байт)" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }

    // Сброс контекста
    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::EncryptionFailed;
    }

    // Инициализируем ChaCha20-Poly1305
    // EVP_chacha20_poly1305() - это AEAD алгоритм
    if (EVP_EncryptInit_ex(
        m_context,
        EVP_chacha20_poly1305(),  // Алгоритм ChaCha20-Poly1305
        nullptr,
        nullptr,
        nullptr) == 0)
    {
        handleError("EVP_EncryptInit_ex (algorithm)");
        return EncryptionResult::EncryptionFailed;
    }

    // Устанавливаем длину nonce для Poly1305 (если требуется)
    // EVP_CTRL_CHACHA20_POLY1305_SET_IVLEN - устанавливает длину nonce
    
    // Устанавливаем ключ и nonce
    if (EVP_EncryptInit_ex(
        m_context,
        nullptr,
        nullptr,
        key.data(),
        nonce.data()) == 0)
    {
        handleError("EVP_EncryptInit_ex (key/nonce)");
        return EncryptionResult::EncryptionFailed;
    }

    // Выделяем буфер для зашифрованных данных (без тега, тег добавим вручную)
    // ChaCha20-Poly1305 не добавляет тег автоматически в буфер, нужно получить его отдельно
    ciphertext.resize(plaintext.size());
    
    int outLength1 = 0;
    int outLength2 = 0;

    // Шифруем данные
    if (EVP_EncryptUpdate(
        m_context,
        ciphertext.data(),
        &outLength1,
        plaintext.data(),
        static_cast<int>(plaintext.size())) == 0)
    {
        handleError("EVP_EncryptUpdate");
        return EncryptionResult::EncryptionFailed;
    }

    // Финализация (для ChaCha20-Poly1305 тег получаем через ctrl после финализации)
    if (EVP_EncryptFinal_ex(
        m_context,
        ciphertext.data() + outLength1,
        &outLength2) == 0)
    {
        handleError("EVP_EncryptFinal_ex");
        return EncryptionResult::EncryptionFailed;
    }

    // Обрезаем до реального размера зашифрованных данных
    size_t encryptedSize = static_cast<size_t>(outLength1) + outLength2;
    ciphertext.resize(encryptedSize);

    // Получаем тег аутентификации (Poly1305)
    // Для ChaCha20-Poly1305 тег получаем через EVP_CTRL_AEAD_GET_TAG
    std::vector<unsigned char> tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_AEAD_GET_TAG,
        TAG_SIZE,
        tag.data()) == 0)
    {
        handleError("EVP_CTRL_AEAD_GET_TAG");
        return EncryptionResult::EncryptionFailed;
    }

    // Добавляем тег в конец зашифрованных данных
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

    std::cout << "[ИНФО] ChaCha20-Poly1305 шифрование завершено" << std::endl;
    std::cout << "[ИНФО] Размер данных: " << plaintext.size() 
              << " -> " << ciphertext.size() << " байт" << std::endl;

    return EncryptionResult::Success;
}

// =============================================================================
// ДЕШИФРОВАНИЕ
// =============================================================================

EncryptionResult ChaCha20Encryptor::decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& nonce,
    std::vector<unsigned char>& plaintext)
{
    // Проверяем входные данные
    if (key.size() != 32)
    {
        std::cerr << "[ОШИБКА] Неверная длина ключа" << std::endl;
        return EncryptionResult::DecryptionFailed;
    }

    if (nonce.size() != 12)
    {
        std::cerr << "[ОШИБКА] Неверная длина nonce" << std::endl;
        return EncryptionResult::DecryptionFailed;
    }

    if (ciphertext.size() < TAG_SIZE)
    {
        std::cerr << "[ОШИБКА] Данные слишком короткие" << std::endl;
        return EncryptionResult::InvalidFileFormat;
    }

    // Сброс контекста
    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::DecryptionFailed;
    }

    // Инициализируем для дешифрования
    if (EVP_DecryptInit_ex(
        m_context,
        EVP_chacha20_poly1305(),
        nullptr,
        nullptr,
        nullptr) == 0)
    {
        handleError("EVP_DecryptInit_ex (algorithm)");
        return EncryptionResult::DecryptionFailed;
    }

    // Устанавливаем ключ и nonce
    if (EVP_DecryptInit_ex(
        m_context,
        nullptr,
        nullptr,
        key.data(),
        nonce.data()) == 0)
    {
        handleError("EVP_DecryptInit_ex (key/nonce)");
        return EncryptionResult::DecryptionFailed;
    }

    // Устанавливаем тег для проверки (аналогично AES-GCM)
    const unsigned char* tag = ciphertext.data() + ciphertext.size() - TAG_SIZE;
    
    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_AEAD_SET_TAG,
        TAG_SIZE,
        const_cast<unsigned char*>(tag)) == 0)
    {
        handleError("EVP_CTRL_AEAD_SET_TAG");
        return EncryptionResult::DecryptionFailed;
    }

    // Дешифруем данные
    size_t encryptedDataSize = ciphertext.size() - TAG_SIZE;
    
    plaintext.resize(encryptedDataSize);
    
    int outLength1 = 0;
    int outLength2 = 0;

    if (EVP_DecryptUpdate(
        m_context,
        plaintext.data(),
        &outLength1,
        ciphertext.data(),
        static_cast<int>(encryptedDataSize)) == 0)
    {
        handleError("EVP_DecryptUpdate");
        return EncryptionResult::DecryptionFailed;
    }

    // Финализация и проверка тега
    int result = EVP_DecryptFinal_ex(
        m_context,
        plaintext.data() + outLength1,
        &outLength2);

    if (result == 0)
    {
        std::cerr << "[ОШИБКА] ChaCha20-Poly1305: Неверный тег аутентификации" << std::endl;
        std::cerr << "[ОШИБКА] Возможно, файл был изменён или введён неверный пароль" << std::endl;
        plaintext.clear();
        return EncryptionResult::InvalidPassword;
    }

    plaintext.resize(static_cast<size_t>(outLength1) + outLength2);

    std::cout << "[ИНФО] ChaCha20-Poly1305 дешифрование завершено" << std::endl;
    std::cout << "[ИНФО] Размер данных: " << ciphertext.size() 
              << " -> " << plaintext.size() << " байт" << std::endl;

    return EncryptionResult::Success;
}

// =============================================================================
// ОБРАБОТКА ОШИБОК
// =============================================================================

void ChaCha20Encryptor::handleError(const std::string& operation) const
{
    unsigned long error = ERR_get_error();
    
    if (error == 0)
    {
        std::cerr << "[ОШИБКА] ChaCha20: " << operation << " - неизвестная ошибка" << std::endl;
        return;
    }

    char errorBuf[256];
    ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
    
    std::cerr << "[ОШИБКА] ChaCha20 (" << operation << "): " << errorBuf << std::endl;
    
    while ((error = ERR_get_error()) != 0)
    {
        ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
        std::cerr << "       -> " << errorBuf << std::endl;
    }
}

} // namespace sigma
