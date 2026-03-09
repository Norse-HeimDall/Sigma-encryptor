/**
 * @file aes.cpp
 * @brief Реализация AES-256-GCM шифрования
 * 
 * @details
 * Этот файл содержит реализацию методов класса AESEncryptor.
 * Используется высокоуровневый EVP API OpenSSL для работы с AES-256-GCM.
 * 
 * AES-256-GCM - это блочный шифр в режиме GCM (Galois/Counter Mode).
 * Основные характеристики:
 * - 256-битный ключ
 * - 128-битный блок
 * - 96-битный (12 байт) IV/Nonce
 * - 128-битный (16 байт) тег аутентификации
 * - AEAD (шифрование + аутентификация)
 * 
 * @author HeimDall
 * @version 1.0
 */

#include "aes.hpp"

// Заголовочные файлы OpenSSL
#include <openssl/evp.h>      // Основной интерфейс шифрования
#include <openssl/err.h>      // Обработка ошибок

#include <iostream>           // Отладочный вывод
#include <stdexcept>          // Исключения

#pragma warning(disable: 4100)

namespace sigma
{

// =============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Конструктор AESEncryptor
 * 
 * Создаёт контекст EVP_CIPHER_CTX, который используется для всех
 * операций шифрования/дешифрования.
 */
AESEncryptor::AESEncryptor()
{
    setlocale(LC_ALL,"RU");
    m_context = EVP_CIPHER_CTX_new();
    
    if (m_context == nullptr)
    {
        std::cerr << "[ОШИБКА] Не удалось создать контекст AES" << std::endl;
        handleError("EVP_CIPHER_CTX_new");
        throw std::runtime_error("Failed to create AES context");
    }
    
    std::cout << "[ИНФО] AES-256-GCM контекст создан" << std::endl;
}

/**
 * @brief Деструктор AESEncryptor
 */
AESEncryptor::~AESEncryptor()
{
    if (m_context != nullptr)
    {
        EVP_CIPHER_CTX_free(m_context);
        m_context = nullptr;
        std::cout << "[ИНФО] AES контекст освобождён" << std::endl;
    }
}

// =============================================================================
// ШИФРОВАНИЕ
// =============================================================================

EncryptionResult AESEncryptor::encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& ciphertext)
{
    // Проверяем входные данные
    if (key.size() != 32)
    {
        std::cerr << "[ОШИБКА] Неверная длина ключа (ожидалось 32 байта, получено " 
                  << key.size() << ")" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }

    if (iv.size() != 12)
    {
        std::cerr << "[ОШИБКА] Неверная длина IV (ожидалось 12 байт, получено " 
                  << iv.size() << ")" << std::endl;
        return EncryptionResult::EncryptionFailed;
    }

    // Сброс контекста
    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::EncryptionFailed;
    }

    // Инициализируем шифрование с алгоритмом AES-256-GCM
    if (EVP_EncryptInit_ex(
        m_context,
        EVP_aes_256_gcm(),
        nullptr,
        nullptr,
        nullptr) == 0)
    {
        handleError("EVP_EncryptInit_ex (algorithm)");
        return EncryptionResult::EncryptionFailed;
    }

    // Установка длины ключа (256 бит = 32 байта)
    if (EVP_CIPHER_CTX_set_key_length(m_context, 32) != 1)
    {
        handleError("EVP_CIPHER_CTX_set_key_length");
        return EncryptionResult::EncryptionFailed;
    }

    // Установка длины IV (96 бит = 12 байт для GCM)
    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_GCM_SET_IVLEN,
        static_cast<int>(iv.size()),
        nullptr) == 0)
    {
        handleError("EVP_CTRL_GCM_SET_IVLEN");
        return EncryptionResult::EncryptionFailed;
    }

    // Устанавливаем ключ и IV
    if (EVP_EncryptInit_ex(
        m_context,
        nullptr,
        nullptr,
        key.data(),
        iv.data()) == 0)
    {
        handleError("EVP_EncryptInit_ex (key/iv)");
        return EncryptionResult::EncryptionFailed;
    }

    // Выделяем буфер только для зашифрованных данных (без тега)
    // Тег аутентификации получим и добавим отдельно
    ciphertext.resize(plaintext.size());
    
    int outLength1 = 0;
    int outLength2 = 0;

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

    // Финализация
    int result = EVP_EncryptFinal_ex(
        m_context,
        ciphertext.data() + outLength1,
        &outLength2);

    if (result == 0)
    {
        handleError("EVP_EncryptFinal_ex");
        return EncryptionResult::EncryptionFailed;
    }

    // Обрезаем до реального размера зашифрованных данных
    size_t encryptedSize = static_cast<size_t>(outLength1) + outLength2;
    ciphertext.resize(encryptedSize);

    // Получаем тег аутентификации GCM
    std::vector<unsigned char> tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_GCM_GET_TAG,
        TAG_SIZE,
        tag.data()) == 0)
    {
        handleError("EVP_CTRL_GCM_GET_TAG");
        return EncryptionResult::EncryptionFailed;
    }

    // Добавляем тег в конец зашифрованных данных
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

    std::cout << "[ИНФО] AES-GCM шифрование завершено" << std::endl;
    std::cout << "[ИНФО] Размер данных: " << plaintext.size() 
              << " -> " << ciphertext.size() << " байт" << std::endl;

    return EncryptionResult::Success;
}

// =============================================================================
// ДЕШИФРОВАНИЕ
// =============================================================================

EncryptionResult AESEncryptor::decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& plaintext)
{
    // Проверяем входные данные
    if (key.size() != 32)
    {
        std::cerr << "[ОШИБКА] Неверная длина ключа" << std::endl;
        return EncryptionResult::DecryptionFailed;
    }

    if (iv.size() != 12)
    {
        std::cerr << "[ОШИБКА] Неверная длина IV" << std::endl;
        return EncryptionResult::DecryptionFailed;
    }

    if (ciphertext.size() < TAG_SIZE)
    {
        std::cerr << "[ОШИБКА] Данные слишком короткие" << std::endl;
        return EncryptionResult::InvalidFileFormat;
    }

    // Инициализация контекста для дешифрования
    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_DecryptInit_ex(
        m_context,
        EVP_aes_256_gcm(),
        nullptr,
        nullptr,
        nullptr) == 0)
    {
        handleError("EVP_DecryptInit_ex (algorithm)");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_CIPHER_CTX_set_key_length(m_context, 32) != 1)
    {
        handleError("EVP_CIPHER_CTX_set_key_length");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_GCM_SET_IVLEN,
        static_cast<int>(iv.size()),
        nullptr) == 0)
    {
        handleError("EVP_CTRL_GCM_SET_IVLEN");
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_DecryptInit_ex(
        m_context,
        nullptr,
        nullptr,
        key.data(),
        iv.data()) == 0)
    {
        handleError("EVP_DecryptInit_ex (key/iv)");
        return EncryptionResult::DecryptionFailed;
    }

    // Установка тега для проверки (в GCM режиме тег нужно установить ДО финализации!)
    const unsigned char* tag = ciphertext.data() + ciphertext.size() - TAG_SIZE;
    
    if (EVP_CIPHER_CTX_ctrl(
        m_context,
        EVP_CTRL_GCM_SET_TAG,
        TAG_SIZE,
        const_cast<unsigned char*>(tag)) == 0)
    {
        handleError("EVP_CTRL_GCM_SET_TAG");
        return EncryptionResult::DecryptionFailed;
    }

    // Дешифрование данных
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
        std::cerr << "[ОШИБКА] AES-GCM: Неверный тег аутентификации" << std::endl;
        std::cerr << "[ОШИБКА] Возможно, файл был изменён или введён неверный пароль" << std::endl;
        plaintext.clear();
        return EncryptionResult::InvalidPassword;
    }

    plaintext.resize(static_cast<size_t>(outLength1) + outLength2);

    std::cout << "[ИНФО] AES-GCM дешифрование завершено" << std::endl;
    std::cout << "[ИНФО] Размер данных: " << ciphertext.size() 
              << " -> " << plaintext.size() << " байт" << std::endl;

    return EncryptionResult::Success;
}

// =============================================================================
// ОБРАБОТКА ОШИБОК
// =============================================================================

void AESEncryptor::handleError(const std::string& operation) const
{
    unsigned long error = ERR_get_error();
    
    if (error == 0)
    {
        std::cerr << "[ОШИБКА] AES: " << operation << " - неизвестная ошибка" << std::endl;
        return;
    }

    char errorBuf[256];
    ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
    
    std::cerr << "[ОШИБКА] AES (" << operation << "): " << errorBuf << std::endl;
    
    while ((error = ERR_get_error()) != 0)
    {
        ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
        std::cerr << "       -> " << errorBuf << std::endl;
    }
}

} // namespace sigma
