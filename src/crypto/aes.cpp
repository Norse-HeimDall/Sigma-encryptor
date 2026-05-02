/**
 * @file aes.cpp
 * @brief Реализация симметричного шифрования AES-256 в режиме GCM (Galois/Counter Mode).
 *  
 * @author HeimDall
 */

#include "aes.hpp"

#include <openssl/evp.h>
#include <openssl/err.h>

#include <iostream>
#include <stdexcept>

namespace sigma
{

// =============================================================================
// КОНСТРУКТОРЫ И ДЕСТРУКТОР
// =============================================================================

/**
 * @brief Инициализация объекта AES-обертки.
 * Создает новый контекст EVP_CIPHER_CTX, который является основным объектом OpenSSL
 * необходимый для криптографических операций.
 */
AESEncryptor::AESEncryptor()
{
    m_context = EVP_CIPHER_CTX_new();
    if (!m_context)
    {
        std::cerr << "[ОШИБКА] Не удалось выделить память под контекст OpenSSL EVP\n";
        handleError("EVP_CIPHER_CTX_new");
        throw std::runtime_error("Failed to create AES context");
    }
}

/**
 * @brief Безопасная очистка ресурсов.
 */
AESEncryptor::~AESEncryptor()
{
    if (m_context)
    {
        EVP_CIPHER_CTX_free(m_context);
        m_context = nullptr;
    }
}

/**
 * @brief Перемещающий конструктор.
 * Позволяет передавать владение контекстом другому объекту без копирования.
 */
AESEncryptor::AESEncryptor(AESEncryptor&& other) noexcept : m_context(other.m_context)
{
    other.m_context = nullptr;
}

/**
 * @brief Оператор перемещающего присваивания.
 */
AESEncryptor& AESEncryptor::operator=(AESEncryptor&& other) noexcept
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
 * @brief Шифрование данных методом AES-256-GCM.
 * Процесс: инициализация, установку длины IV, передачу ключа/IV,
 * само шифрование и генерация тега аутентификации.
 */
EncryptionResult AESEncryptor::encrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& ciphertext)
{
    // Валидация входных параметров согласно стандартам AES-256
    if (key.size() != KEY_SIZE || iv.size() != IV_SIZE)
    {
        std::cerr << "[ОШИБКА] Неверные параметры: Ключ должен быть 32 байта, IV — 12 байт.\n";
        return EncryptionResult::EncryptionFailed;
    }

    // Сброс контекста для нового использования (позволяет переиспользовать объект)
    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::EncryptionFailed;
    }

    /**
     *  Инициализация операции шифрования.
     * Мы используем AES-256 в режиме GCM.
     */
    if (EVP_EncryptInit_ex(m_context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        handleError("AES Encrypt Algorithm Init");
        return EncryptionResult::EncryptionFailed;
    }

    //  Установка длины вектора инициализации (IV). Стандарт для GCM — 12 байт (96 бит).
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1)
    {
        handleError("EVP_CTRL_GCM_SET_IVLEN");
        return EncryptionResult::EncryptionFailed;
    }

    //  Установка самого ключа и вектора инициализации.
    if (EVP_EncryptInit_ex(m_context, nullptr, nullptr, key.data(), iv.data()) != 1)
    {
        handleError("AES Encrypt Key/IV Init");
        return EncryptionResult::EncryptionFailed;
    }

    // Резервируем память под результат
    ciphertext.resize(plaintext.size());
    
    int updateLen = 0;
    int finalLen = 0;

    /**
     *  Основной цикл шифрования. 
     * EVP_EncryptUpdate обрабатывает данные блоками.
     */
    if (EVP_EncryptUpdate(m_context, ciphertext.data(), &updateLen, 
                          plaintext.data(), static_cast<int>(plaintext.size())) != 1)
    {
        handleError("EVP_EncryptUpdate");
        return EncryptionResult::EncryptionFailed;
    }

    /**
     *  Финализация. 
     * В GCM режиме здесь не создается дополнительных данных, но это важный этап завершения.
     */
    if (EVP_EncryptFinal_ex(m_context, ciphertext.data() + updateLen, &finalLen) != 1)
    {
        handleError("EVP_EncryptFinal_ex");
        return EncryptionResult::EncryptionFailed;
    }

    ciphertext.resize(static_cast<size_t>(updateLen) + finalLen);

    /**
     *  Работа с тегом (Authentication Tag).
     * GCM создает 16-байтный тег, который подтверждает, что данные не были изменены.
     */
    std::vector<unsigned char> tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1)
    {
        handleError("EVP_CTRL_GCM_GET_TAG");
        return EncryptionResult::EncryptionFailed;
    }

    /**
     * Формирование финального пакета.
     * Тег хранится в конце(это не обязательно, можно свободно перемешивать, но разбиратся будете сами)
     */
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

    return EncryptionResult::Success;
}

// =============================================================================
// ДЕШИФРОВАНИЕ (DECRYPTION)
// =============================================================================

/**
 * @brief Дешифрование и проверка целостности данных.
 * Отличается от обычного режима тем, что перед завершением требует передачи тега
 * для сверки контрольной суммы (аутентификации).
 */
EncryptionResult AESEncryptor::decrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv,
    std::vector<unsigned char>& plaintext)
{
    // Базовая проверка размеров
    if (key.size() != KEY_SIZE || iv.size() != IV_SIZE)
    {
        return EncryptionResult::DecryptionFailed;
    }

    // Шифротекст в GCM всегда содержит в конце TAG_SIZE байт тега
    if (ciphertext.size() < TAG_SIZE)
    {
        std::cerr << "[ОШИБКА] Пакет данных поврежден или слишком мал для режима GCM\n";
        return EncryptionResult::InvalidFileFormat;
    }

    if (EVP_CIPHER_CTX_reset(m_context) != 1)
    {
        handleError("EVP_CIPHER_CTX_reset");
        return EncryptionResult::DecryptionFailed;
    }

    // 1. Инициализация дешифратора
    if (EVP_DecryptInit_ex(m_context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        handleError("AES Decrypt Algorithm Init");
        return EncryptionResult::DecryptionFailed;
    }

    // 2. Установка параметров IV
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1)
    {
        handleError("EVP_CTRL_GCM_SET_IVLEN");
        return EncryptionResult::DecryptionFailed;
    }

    // 3. Привязка ключа и IV
    if (EVP_DecryptInit_ex(m_context, nullptr, nullptr, key.data(), iv.data()) != 1)
    {
        handleError("AES Decrypt Key/IV Init");
        return EncryptionResult::DecryptionFailed;
    }

    // Рассчитываем размер чистых зашифрованных данных (без учета тега в конце)
    size_t encryptedDataSize = ciphertext.size() - TAG_SIZE;
    const unsigned char* tagPtr = ciphertext.data() + encryptedDataSize;
    
    /**
     * 4. Установка ожидаемого тега.
     * В GCM-дешифровании мы должны передать тег в контекст до вызова Final.
     */
    if (EVP_CIPHER_CTX_ctrl(m_context, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tagPtr)) != 1)
    {
        handleError("EVP_CTRL_GCM_SET_TAG");
        return EncryptionResult::DecryptionFailed;
    }

    plaintext.resize(encryptedDataSize);
    
    int updateLen = 0;
    int finalLen = 0;

    // 5. Процесс расшифровки
    if (EVP_DecryptUpdate(m_context, plaintext.data(), &updateLen, 
                          ciphertext.data(), static_cast<int>(encryptedDataSize)) != 1)
    {
        handleError("EVP_DecryptUpdate");
        return EncryptionResult::DecryptionFailed;
    }

    /**
     * 6. Проверка аутентичности.
     * Если данные были изменены (хотя бы один бит), EVP_DecryptFinal_ex вернет ошибку.
     * Это гарантирует, что мы не расшифруем нерабочий хлам при неверном пароле или поврежденном файле.
     */
    if (EVP_DecryptFinal_ex(m_context, plaintext.data() + updateLen, &finalLen) != 1)
    {
        std::cerr << "[БЕЗОПАСНОСТЬ] Сбой проверки тега! Файл изменен или введен неверный пароль.\n";
        plaintext.clear(); 
        return EncryptionResult::InvalidPassword;
    }

    plaintext.resize(static_cast<size_t>(updateLen) + finalLen);
    return EncryptionResult::Success;
}

// =============================================================================
// ОБРАБОТКА ОШИБОК OPENSSL
// =============================================================================

/**
 * @brief Разбор стека ошибок OpenSSL.
 * Криптобиблиотека ведет свой внутренний лог ошибок, который нужно вычитывать
 * для получения понятных сообщений.
 */
void AESEncryptor::handleError(const std::string& operation) const
{
    unsigned long error = ERR_get_error();
    if (error == 0) return;

    char errorBuf[256];
    while (error != 0)
    {
        ERR_error_string_n(error, errorBuf, sizeof(errorBuf));
        std::cerr << "[КРИТИЧЕСКАЯ ОШИБКА] AES (" << operation << "): " << errorBuf << "\n";
        error = ERR_get_error();
    }
}

} // namespace sigma