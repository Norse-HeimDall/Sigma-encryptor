/**
 * @file chacha20.cpp
 * @brief Реализация потокового шифра ChaCha20 с аутентификацией Poly1305 (AEAD).
 * 
 * В отличие от блочного AES, ChaCha20 — это высокоскоростной потоковый шифр.
 * В связке с Poly1305 он обеспечивает целостность и конфиденциальность, 
 * являясь стандартом (RFC 8439).
 * 
 * @author heimdall
 */

#include "chacha20.hpp"

#include <openssl/evp.h>
#include <openssl/err.h>

#include <iostream>
#include <stdexcept>
#include <cstring>

namespace sigma
{

// =============================================================================
// КОНСТРУКТОРЫ И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Инициализация контекста ChaCha20.
 * Создает EVP_CIPHER_CTX. Настройка локали вынесена в точку входа приложения (main),
 * чтобы избежать дублирования в библиотечных классах.
 */
ChaCha20Encryptor::ChaCha20Encryptor()
{
    m_context = EVP_CIPHER_CTX_new();
    if (!m_context)
    {
        handleError("EVP_CIPHER_CTX_new");
        throw std::runtime_error("ChaCha20 context creation failed");
    }
}

/**
 * @brief Очистка ресурсов.
 * Контекст должен быть освобожден через специализированную функцию OpenSSL.
 */
ChaCha20Encryptor::~ChaCha20Encryptor()
{
    if (m_context)
    {
        EVP_CIPHER_CTX_free(m_context);
        m_context = nullptr;
    }
}

/**
 * @brief Конструктор перемещения.
 * Эффективно передает владение указателем на контекст OpenSSL.
 */
ChaCha20Encryptor::ChaCha20Encryptor(ChaCha20Encryptor&& other) noexcept 
    : m_context(other.m_context)
{
    other.m_context = nullptr;
}

/**
 * @brief Оператор перемещающего присваивания.
 */
ChaCha20Encryptor& ChaCha20Encryptor::operator=(ChaCha20Encryptor&& other) noexcept
{
    if (this != &other)
    {
        if (m_context) EVP_CIPHER_CTX_free(m_context);
        m_context = other.m_context;
        other.m_context = nullptr;
    }
    return *this;
}

// =============================================================================
// ШИФРОВАНИЕ (ENCRYPTION)
// =============================================================================

/**
 * @brief Шифрование данных ChaCha20-Poly1305.
 * В режиме AEAD (Authenticated Encryption with Associated Data) мы не только
 * скрываем данные, но и вычисляем уникальный отпечаток (тег) для проверки.
 */
EncryptionResult ChaCha20Encryptor::encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& ciphertext)
{
    // Проверка соответствия длин ключа (32 байта) и IV (12 байт)
    if (key.size() != KEY_SIZE || iv.size() != IV_SIZE)
    {
        std::cerr << "[ОШИБКА] ChaCha20: Неверный размер ключа или IV\n";
        return EncryptionResult::EncryptionFailed;
    }

    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::EncryptionFailed;
    }

    /**
     * 1. Инициализация алгоритма. 
     * Передаем EVP_chacha20_poly1305() для активации режима AEAD.
     */
    if (EVP_EncryptInit_ex(m_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
        EVP_EncryptInit_ex(m_context, nullptr, nullptr, key.data(), iv.data()) != 1)
    {
        handleError("ChaCha20 Encrypt Init");
        return EncryptionResult::EncryptionFailed;
    }

    // Резервируем место: размер данных + 16 байт под тег Poly1305
    ciphertext.resize(plaintext.size() + TAG_SIZE);
    
    int updateLen = 0;
    int finalLen = 0;

    // 2. Шифрование основного блока данных
    if (EVP_EncryptUpdate(m_context, ciphertext.data(), &updateLen, 
                          plaintext.data(), static_cast<int>(plaintext.size())) != 1)
    {
        handleError("EVP_EncryptUpdate");
        return EncryptionResult::EncryptionFailed;
    }

    // 3. Завершение шифрования
    if (EVP_EncryptFinal_ex(m_context, ciphertext.data() + updateLen, &finalLen) != 1)
    {
        handleError("EVP_EncryptFinal_ex");
        return EncryptionResult::EncryptionFailed;
    }

    /**
     * 4. Извлечение тега аутентификации.
     * OpenSSL записывает тег в указанное место. Мы помещаем его сразу за данными.
     */
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, 
                            ciphertext.data() + updateLen + finalLen) != 1)
    {
        handleError("EVP_CTRL_AEAD_GET_TAG");
        return EncryptionResult::EncryptionFailed;
    }

    return EncryptionResult::Success;
}

// =============================================================================
// ДЕШИФРОВАНИЕ (DECRYPTION)
// =============================================================================

/**
 * @brief Дешифрование и проверка подлинности.
 * Если тег Poly1305 не совпадет, функция вернет ошибку,
 * что означает попытку подмены данных или неверный ключ.
 */
EncryptionResult ChaCha20Encryptor::decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& plaintext)
{
    // Валидация входных данных: должен присутствовать хотя бы тег
    if (key.size() != KEY_SIZE || iv.size() != IV_SIZE || ciphertext.size() < TAG_SIZE)
    {
        return EncryptionResult::DecryptionFailed;
    }

    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::DecryptionFailed;
    }

    // 1. Инициализация дешифратора
    if (EVP_DecryptInit_ex(m_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
        EVP_DecryptInit_ex(m_context, nullptr, nullptr, key.data(), iv.data()) != 1)
    {
        handleError("ChaCha20 Decrypt Init");
        return EncryptionResult::DecryptionFailed;
    }

    // Определяем положение тега в конце входящего массива
    size_t encryptedDataSize = ciphertext.size() - TAG_SIZE;
    const unsigned char* tagPtr = ciphertext.data() + encryptedDataSize;

    /**
     * 2. Установка тега для проверки.
     * В режиме AEAD мы должны загрузить полученный тег перед финализацией дешифрования.
     */
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_AEAD_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tagPtr)) != 1)
    {
        handleError("EVP_CTRL_AEAD_SET_TAG");
        return EncryptionResult::DecryptionFailed;
    }

    plaintext.resize(encryptedDataSize);
    int updateLen = 0;
    int finalLen = 0;

    // 3. Расшифровка данных
    if (EVP_DecryptUpdate(m_context, plaintext.data(), &updateLen, 
                          ciphertext.data(), static_cast<int>(encryptedDataSize)) != 1)
    {
        handleError("EVP_DecryptUpdate");
        return EncryptionResult::DecryptionFailed;
    }

    /**
     * 4. Финализация и сверка тега Poly1305.
     * Если данные были изменены, EVP_DecryptFinal_ex вернет 0 или меньше.
     */
    if (EVP_DecryptFinal_ex(m_context, plaintext.data() + updateLen, &finalLen) <= 0)
    {
        std::cerr << "[БЕЗОПАСНОСТЬ] Ошибка Poly1305: Целостность данных нарушена!\n";
        plaintext.clear();
        return EncryptionResult::InvalidPassword;
    }

    plaintext.resize(static_cast<size_t>(updateLen) + finalLen);
    return EncryptionResult::Success;
}

// =============================================================================
// ОБРАБОТКА ОШИБОК
// =============================================================================

/**
 * @brief Логирование ошибок через стек OpenSSL.
 */
void ChaCha20Encryptor::handleError(const std::string& operation) const
{
    unsigned long err = ERR_get_error();
    if (err == 0) return;

    char buf[256];
    while (err != 0)
    {
        ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "[ОШИБКА] ChaCha20 (" << operation << "): " << buf << "\n";
        err = ERR_get_error();
    }
}

} // namespace sigma